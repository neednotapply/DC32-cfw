#include "absim.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

using namespace absim;

namespace {

uint64_t hash_bytes(uint64_t hash, uint8_t const* bytes, size_t size)
{
    for(size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

} // namespace

int main(int argc, char** argv)
{
    if(argc != 3) {
        std::fprintf(stderr, "usage: %s ROM.bin guest-cycles\n", argv[0]);
        return EXIT_FAILURE;
    }
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<uint8_t> image{
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    uint64_t deadline = std::strtoull(argv[2], nullptr, 0);
    if(image.empty() || image.size() > atmega32u4_t::PROG_SIZE_BYTES || deadline == 0) {
        std::fprintf(stderr, "invalid execution-window input\n");
        return EXIT_FAILURE;
    }

    auto cpu = std::make_unique<atmega32u4_t>();
    cpu->lock = 0xff;
    cpu->fuse_lo = 0xff;
    cpu->fuse_hi = 0xd3;
    cpu->fuse_ext = 0xcb;
    cpu->reset();
    std::fill(cpu->prog.begin(), cpu->prog.end(), 0xffu);
    std::copy(image.begin(), image.end(), cpu->prog.begin());
    cpu->program_loaded = true;
    cpu->last_addr = uint16_t((image.size() + 1u) & ~1u);
    cpu->decode();
    cpu->pc = 0;
    cpu->executing_instr_pc = 0;
    cpu->spi_datain_byte = 0xffu;
    cpu->PINB() = 0x10u;
    cpu->PINE() = 0x40u;
    cpu->PINF() = 0xf0u;

    while(cpu->cycle_count < deadline) {
        if(cpu->pc >= cpu->decoded_prog.size()) {
            std::fprintf(stderr, "invalid PC during execution window: %04x\n", cpu->pc);
            return EXIT_FAILURE;
        }
        cpu->advance_cycle();
        if(cpu->should_autobreak()) {
            std::fprintf(stderr, "autobreak during execution window: %04x\n", cpu->pc);
            return EXIT_FAILURE;
        }
    }

    uint64_t hash = UINT64_C(1469598103934665603);
    hash = hash_bytes(hash, cpu->data.data(), cpu->data.size());
    hash = hash_bytes(hash, cpu->eeprom.data(), cpu->eeprom.size());
    hash = hash_bytes(hash, reinterpret_cast<uint8_t const*>(&cpu->pc), sizeof(cpu->pc));
    hash = hash_bytes(hash, reinterpret_cast<uint8_t const*>(&cpu->cycle_count), sizeof(cpu->cycle_count));
    std::printf("cycles=%llu pc=%04x sp=%04x sreg=%02x state=%016llx\n",
        static_cast<unsigned long long>(cpu->cycle_count), cpu->pc, cpu->sp(), cpu->sreg(),
        static_cast<unsigned long long>(hash));
    return EXIT_SUCCESS;
}
