#include "absim.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace absim;

namespace {

uint32_t next_random(uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void initialize_cpu(atmega32u4_t& cpu, uint32_t seed)
{
    for(auto& byte : cpu.data) byte = uint8_t(next_random(seed));
    for(auto& byte : cpu.prog) byte = uint8_t(next_random(seed));
    for(auto& handler : cpu.ld_handlers) handler = nullptr;
    for(auto& handler : cpu.st_handlers) handler = nullptr;
    cpu.pc = uint16_t(32u + next_random(seed) % 0x3f00u);
    cpu.just_read = next_random(seed);
    cpu.just_written = next_random(seed);
    cpu.io_reg_accessed = false;
    cpu.min_stack = 0xffffu;
    cpu.stack_check = 0x0100u;
    cpu.pushed_at_least_once = false;
    cpu.spl() = 0xf0u;
    cpu.sph() = 0x0au;
    cpu.spm_en_cycles = 0;
    cpu.fuse_lo = uint8_t(next_random(seed));
    cpu.fuse_hi = uint8_t(next_random(seed));
    cpu.fuse_ext = uint8_t(next_random(seed));
    cpu.lock = uint8_t(next_random(seed));

    // Keep every randomized SRAM operation in ordinary RAM, away from I/O
    // handlers and the stack. Pointer-register overlap is intentionally left
    // possible because real AVR instructions permit it.
    for(uint8_t base : {uint8_t(26), uint8_t(28), uint8_t(30)}) {
        uint16_t ptr = uint16_t(0x0100u + next_random(seed) % 0x0800u);
        cpu.gpr(base) = uint8_t(ptr);
        cpu.gpr(base + 1u) = uint8_t(ptr >> 8);
    }
}

avr_instr_t make_instruction(uint8_t func, uint32_t& random)
{
    avr_instr_t i{};
    i.func = func;
    i.word = uint16_t(next_random(random));
    i.src = uint8_t(next_random(random) & 31u);
    i.dst = uint8_t(next_random(random) & 31u);
    i.m0 = uint8_t(next_random(random) & 31u);
    i.m1 = uint8_t(next_random(random));
    i.m2 = uint8_t(next_random(random));

    switch(func) {
    case INSTR_MOVW:
        // Decoded MOVW operands are register-pair indices, not register numbers.
        i.src = uint8_t(next_random(random) & 15u);
        i.dst = uint8_t(next_random(random) & 15u);
        break;
    case INSTR_LDI:
    case INSTR_SUBI:
    case INSTR_SBCI:
    case INSTR_CPI:
    case INSTR_ORI:
    case INSTR_ANDI:
        i.dst = uint8_t(16u + (next_random(random) & 15u));
        i.src = uint8_t(next_random(random));
        break;
    case INSTR_ADIW:
    case INSTR_SBIW:
        i.dst = uint8_t(24u + 2u * (next_random(random) & 3u));
        i.src = uint8_t(next_random(random) & 63u);
        break;
    case INSTR_BSET:
        i.dst = uint8_t(1u << (next_random(random) & 7u));
        break;
    case INSTR_BCLR:
        i.dst = uint8_t(~(1u << (next_random(random) & 7u)));
        break;
    case INSTR_BRBS:
    case INSTR_BRBC:
        i.src = uint8_t(next_random(random) & 7u);
        i.word = uint16_t(int16_t(int(next_random(random) % 127u) - 63));
        break;
    case INSTR_RJMP:
        i.word = uint16_t(int16_t(int(next_random(random) % 2047u) - 1023));
        break;
    case INSTR_JMP:
        i.word = uint16_t(next_random(random) & 0x3fffu);
        break;
    case INSTR_LPM:
        i.word = uint16_t(next_random(random) & 1u);
        break;
    case INSTR_MERGED_LDI2:
        i.dst = uint8_t(16u + (next_random(random) & 15u));
        i.src = uint8_t(next_random(random));
        i.m0 = uint8_t(16u + (next_random(random) & 15u));
        i.m1 = uint8_t(next_random(random));
        break;
    case INSTR_MERGED_LDS:
    case INSTR_MERGED_STS:
        i.word = uint16_t(0x0100u + next_random(random) % 0x0800u);
        break;
    case INSTR_MERGED_LDD_Y:
    case INSTR_MERGED_LDD_Z:
    case INSTR_MERGED_STD_Y:
    case INSTR_MERGED_STD_Z:
        i.dst = uint8_t(next_random(random) & 63u);
        break;
    case INSTR_MERGED_LD_ST: {
        static constexpr uint8_t modes[] = {0, 1, 2, 4, 5, 6, 12, 13, 14};
        i.dst = modes[next_random(random) % std::size(modes)];
        i.word = uint16_t(next_random(random) & 1u);
        break;
    }
    default:
        break;
    }
    return i;
}

bool equivalent(atmega32u4_t const& exact, atmega32u4_t const& fast)
{
    return exact.pc == fast.pc &&
        exact.data == fast.data &&
        exact.just_read == fast.just_read &&
        exact.just_written == fast.just_written &&
        exact.io_reg_accessed == fast.io_reg_accessed &&
        exact.min_stack == fast.min_stack &&
        exact.pushed_at_least_once == fast.pushed_at_least_once;
}

} // namespace

int main()
{
    static constexpr std::array<uint8_t, 55> FAST_OPS = {
        INSTR_NOP, INSTR_MOV, INSTR_MOVW, INSTR_LDI, INSTR_MERGED_LDI2,
        INSTR_AND, INSTR_OR, INSTR_EOR, INSTR_ANDI, INSTR_ORI, INSTR_CLR,
        INSTR_ADD, INSTR_ADC, INSTR_SUB, INSTR_SUBI, INSTR_CP, INSTR_CPI,
        INSTR_SBC, INSTR_SBCI, INSTR_CPC, INSTR_INC, INSTR_DEC, INSTR_SWAP,
        INSTR_COM, INSTR_ADIW, INSTR_SBIW, INSTR_BSET, INSTR_BCLR,
        INSTR_BRBS, INSTR_BRBC, INSTR_RJMP, INSTR_JMP, INSTR_IJMP,
        INSTR_PUSH, INSTR_POP, INSTR_LPM, INSTR_MERGED_LDS, INSTR_MERGED_STS,
        INSTR_MERGED_LDD_Y, INSTR_MERGED_LDD_Z, INSTR_MERGED_STD_Y,
        INSTR_MERGED_STD_Z, INSTR_MERGED_LD_ST,
        INSTR_MERGED_LD_X, INSTR_MERGED_LD_Y, INSTR_MERGED_LD_Z,
        INSTR_MERGED_LD_X_INC, INSTR_MERGED_LD_Y_INC, INSTR_MERGED_LD_Z_INC,
        INSTR_MERGED_LD_X_DEC, INSTR_MERGED_LD_Y_DEC, INSTR_MERGED_LD_Z_DEC,
        INSTR_MERGED_ST_X, INSTR_MERGED_ST_Y, INSTR_MERGED_ST_Z,
    };

    // The contiguous merged load/store family also contains the increment and
    // decrement store forms after the entries above.
    uint32_t cases = 0;
    for(uint32_t iteration = 0; iteration < 4096; ++iteration) {
        for(uint8_t listed_func : FAST_OPS) {
            uint8_t last = listed_func == INSTR_MERGED_ST_X ? INSTR_MERGED_ST_Z_DEC : listed_func;
            for(uint8_t func = listed_func; func <= last; ++func) {
                uint32_t state_seed = 0x9e3779b9u ^ iteration * 0x85ebca6bu ^ uint32_t(func) * 0xc2b2ae35u;
                uint32_t instr_seed = state_seed ^ 0xa5a55a5au;
                auto exact = std::make_unique<atmega32u4_t>();
                auto fast = std::make_unique<atmega32u4_t>();
                initialize_cpu(*exact, state_seed);
                initialize_cpu(*fast, state_seed);
                avr_instr_t i = make_instruction(func, instr_seed);
                uint32_t exact_cycles = INSTR_MAP[func](*exact, i);
                uint32_t fast_cycles = fast->embedded_execute_fast(i);
                if(exact_cycles != fast_cycles || !equivalent(*exact, *fast)) {
                    std::fprintf(stderr,
                        "fast dispatch mismatch: func=%u iteration=%lu cycles=%lu/%lu pc=%04x/%04x sreg=%02x/%02x\n",
                        unsigned(func), static_cast<unsigned long>(iteration),
                        static_cast<unsigned long>(exact_cycles), static_cast<unsigned long>(fast_cycles),
                        exact->pc, fast->pc, exact->sreg(), fast->sreg());
                    return EXIT_FAILURE;
                }
                ++cases;
            }
        }
    }
    std::printf("Arduboy fast dispatcher differential passed: %lu cases\n",
        static_cast<unsigned long>(cases));
    return EXIT_SUCCESS;
}
