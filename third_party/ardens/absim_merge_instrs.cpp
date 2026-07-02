#include "absim.hpp"

// possibilities:

//     mul[s][u] + movw rN, r0 + eor r1, r1
//     ldi rC, N + add rA, rA + adc rB, rB + dec RC + brne
//     ldi rC, N + lsr rA, rA + ror rB, rB + dec RC + brne
//     ldi rC, N + asr rA, rA + ror rB, rB + dec RC + brne
//     lsr rN + lsr rN + ... + lsr rN
//     add + adc
//     subi + sbci
//     eor rN, rN + dec rN
//     and rN, rN + brbc
//     and rN, rN + brbs
//     and + or (used in sprite drawing)
//     sbiw rN, 1 + brne

// ymask:
//     ldi  r19, 0x01
//     sbrc r22, 1
//     ldi  r19, 0x04
//     sbrc r22, 0
//     add  r19, r19
//     sbrc r22, 2
//     swap r19

namespace absim
{

static uint32_t instr_is_delay(atmega32u4_t const& cpu, size_t n)
{
    auto i = cpu.decoded_prog[n];
    switch(i.func)
    {
    case INSTR_NOP:
        return 1;
    case INSTR_RJMP:
    {
        if(i.word == 0)
            return 2;
        size_t nr = n + i.word + 1;
        if(nr < cpu.decoded_prog.size() && cpu.decoded_prog[nr].func == INSTR_RET)
            return 7;
        break;
    }
    default:
        return 0;
    }
    return 0;
}

void atmega32u4_t::merge_instrs()
{
#ifndef ARDENS_EMBEDDED
    memcpy(merged_prog.data(), &decoded_prog, array_bytes(merged_prog));
#endif

#ifdef ARDENS_EMBEDDED
    auto& prog = decoded_prog;
#else
    auto& prog = merged_prog;
#endif

    for(auto& i : prog)
    {
        switch(i.func)
        {
        case INSTR_OUT     : i.func = INSTR_MERGED_OUT     ; break;
        case INSTR_IN      : i.func = INSTR_MERGED_IN      ; break;
        case INSTR_LDS     : i.func = INSTR_MERGED_LDS     ; break;
        case INSTR_STS     : i.func = INSTR_MERGED_STS     ; break;
        case INSTR_LDD_Y   : i.func = INSTR_MERGED_LDD_Y   ; break;
        case INSTR_LDD_Z   : i.func = INSTR_MERGED_LDD_Z   ; break;
        case INSTR_STD_Y   : i.func = INSTR_MERGED_STD_Y   ; break;
        case INSTR_STD_Z   : i.func = INSTR_MERGED_STD_Z   ; break;
        case INSTR_LD_ST   : i.func = INSTR_MERGED_LD_ST   ; break;
        case INSTR_LD_X    : i.func = INSTR_MERGED_LD_X    ; break;
        case INSTR_LD_Y    : i.func = INSTR_MERGED_LD_Y    ; break;
        case INSTR_LD_Z    : i.func = INSTR_MERGED_LD_Z    ; break;
        case INSTR_LD_X_INC: i.func = INSTR_MERGED_LD_X_INC; break;
        case INSTR_LD_Y_INC: i.func = INSTR_MERGED_LD_Y_INC; break;
        case INSTR_LD_Z_INC: i.func = INSTR_MERGED_LD_Z_INC; break;
        case INSTR_LD_X_DEC: i.func = INSTR_MERGED_LD_X_DEC; break;
        case INSTR_LD_Y_DEC: i.func = INSTR_MERGED_LD_Y_DEC; break;
        case INSTR_LD_Z_DEC: i.func = INSTR_MERGED_LD_Z_DEC; break;
        case INSTR_ST_X    : i.func = INSTR_MERGED_ST_X    ; break;
        case INSTR_ST_Y    : i.func = INSTR_MERGED_ST_Y    ; break;
        case INSTR_ST_Z    : i.func = INSTR_MERGED_ST_Z    ; break;
        case INSTR_ST_X_INC: i.func = INSTR_MERGED_ST_X_INC; break;
        case INSTR_ST_Y_INC: i.func = INSTR_MERGED_ST_Y_INC; break;
        case INSTR_ST_Z_INC: i.func = INSTR_MERGED_ST_Z_INC; break;
        case INSTR_ST_X_DEC: i.func = INSTR_MERGED_ST_X_DEC; break;
        case INSTR_ST_Y_DEC: i.func = INSTR_MERGED_ST_Y_DEC; break;
        case INSTR_ST_Z_DEC: i.func = INSTR_MERGED_ST_Z_DEC; break;
        case INSTR_SBI     : i.func = INSTR_MERGED_SBI     ; break;
        case INSTR_CBI     : i.func = INSTR_MERGED_CBI     ; break;
        case INSTR_SBIS    : i.func = INSTR_MERGED_SBIS    ; break;
        case INSTR_SBIC    : i.func = INSTR_MERGED_SBIC    ; break;
        default: break;
        }
    }

    for(size_t n = 0; n + 1 < prog.size(); ++n)
    {
        auto& i0 = prog[n + 0];
        auto  i1 = decoded_prog[n + 1];

#ifdef ARDENS_EMBEDDED
        if(n + 5 < decoded_prog.size())
        {
            auto i2 = decoded_prog[n + 2];
            auto i3 = decoded_prog[n + 3];
            auto i4 = decoded_prog[n + 4];
            auto i5 = decoded_prog[n + 5];
            if(decoded_prog[n + 0].func == INSTR_MERGED_ST_Z_INC &&
                i1.func == INSTR_MERGED_ST_Z_INC && i2.func == INSTR_MERGED_ST_Z_INC &&
                i3.func == INSTR_MERGED_ST_Z_INC &&
                decoded_prog[n + 0].src == i1.src && i1.src == i2.src && i2.src == i3.src &&
                i4.func == INSTR_INC && i5.func == INSTR_BRBC && i5.src == 1 &&
                (int16_t)i5.word == -6)
            {
                i0.func = INSTR_MERGED_FILL4_INC_BRNE;
                i0.src = decoded_prog[n + 0].src;
                i0.dst = i4.dst;
                continue;
            }
        }
        if(n + 8 < decoded_prog.size())
        {
            auto const& d0 = decoded_prog[n + 0];
            auto const& d1 = decoded_prog[n + 1];
            auto const& d2 = decoded_prog[n + 2];
            auto const& d3 = decoded_prog[n + 3];
            auto const& d4 = decoded_prog[n + 4];
            auto const& d5 = decoded_prog[n + 5];
            auto const& d6 = decoded_prog[n + 6];
            auto const& d7 = decoded_prog[n + 7];
            auto const& d8 = decoded_prog[n + 8];
            if(d0.func == INSTR_MERGED_LD_Z && d0.src == 0 &&
                d1.func == INSTR_MERGED_OUT && d1.dst == 0x2e && d1.src == 0 &&
                d2.func == INSTR_CPSE && d2.src == 1 &&
                d3.func == INSTR_MOV && d3.dst == 0 && d3.src == 1 &&
                d4.func == INSTR_SBIW && d4.dst == 26 && d4.src == 1 &&
                d5.func == INSTR_SBRC && d5.dst == 26 && d5.src == 1 &&
                d6.func == INSTR_RJMP && (int16_t)d6.word == -3 &&
                d7.func == INSTR_MERGED_ST_Z_INC && d7.src == 0 &&
                d8.func == INSTR_BRBC && d8.src == 1 && (int16_t)d8.word == -9)
            {
                i0.func = INSTR_MERGED_ARDUBOY_DISPLAY;
                i0.src = d2.dst;
                continue;
            }
        }
#endif

        if(i0.func == INSTR_LDI && i1.func == INSTR_LDI)
        {
            i0.func = INSTR_MERGED_LDI2;
            i0.m0 = i1.dst;
            i0.m1 = i1.src;
            continue;
        }

        if(i0.func == INSTR_DEC && i1.func == INSTR_BRBC && i1.src == 1)
        {
            i0.func = INSTR_MERGED_DEC_BRNE;
            i0.word = i1.word;
            continue;
        }

        if(i0.func == INSTR_SBIW && i1.func == INSTR_BRBC && i1.src == 1)
        {
            i0.func = INSTR_MERGED_SBIW_BRNE;
            i0.word = i1.word;
            continue;
        }

        if(i0.func == INSTR_ADD &&
            i1.func == INSTR_ADC &&
            i0.dst + 1 == i1.dst &&
            i0.src + 1 == i1.src)
        {
            i0.func = INSTR_MERGED_ADD_ADC;
            continue;
        }

        if(i0.func == INSTR_SUB &&
            i1.func == INSTR_SBC &&
            i0.dst + 1 == i1.dst &&
            i0.src + 1 == i1.src)
        {
            i0.func = INSTR_MERGED_SUB_SBC;
            continue;
        }

        if(i0.func == INSTR_CP &&
            i1.func == INSTR_CPC &&
            i0.dst + 1 == i1.dst &&
            i0.src + 1 == i1.src)
        {
            i0.func = INSTR_MERGED_CP_CPC;
            continue;
        }

        if(i0.func == INSTR_SUBI &&
            i1.func == INSTR_SBCI)
        {
            i0.func = INSTR_MERGED_SUBI_SBCI;
            i0.word = i0.src + i1.src * 256;
            i0.src = i1.dst;
            continue;
        }

        {
            uint32_t d = 0;
            uint32_t words = 0;
            for(size_t m = n; words < 254 && m < decoded_prog.size();)
            {
                uint32_t t = instr_is_delay(*this, m);
                if(t == 0) break;
                if(d + t > MAX_INSTR_CYCLES)
                    break;
                d += t;
                uint32_t instr_words = instr_is_two_words(decoded_prog[m]) ? 2u : 1u;
                m += instr_words;
                words += instr_words;
            }
            if(d > 1)
            {
                i0.func = INSTR_MERGED_DELAY;
                i0.src = (uint8_t)words;
                i0.word = (uint16_t)d;
            }
        }
    }
}

}
