/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RCP_H
#define _PICO_RCP_H

#include "pico.h"

// ----------------------------------------------------------------------------
// RCP instructions (this header is Arm-only)
#if defined(PICO_RP2350) && !defined(__riscv)

#define RCP_MASK_TRUE   0xa500a500u
#define RCP_MASK_FALSE  0x00c300c3u
#define RCP_MASK_INTXOR 0x96009600u

// ----------------------------------------------------------------------------
// Remainder of header is C and C++ only.
#ifndef __ASSEMBLER__

#define __rcpinline __force_inline

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __riscv // should never do an rcp_asm in risc-v code
#ifdef __ARM_ARCH_6M__
#define rcp_asm(...) ({pico_default_asm_volatile(".cpu cortex-m33\n" __VA_ARGS__); pico_default_asm(".cpu cortex-m0plus"); })
#else
#define rcp_asm pico_default_asm_volatile
#endif
#endif
// ----------------------------------------------------------------------------
// RCP Canary instructions

// Initialise 64-bit salt value for core 0
static __rcpinline void rcp_salt_core0(uint64_t salt) {
    rcp_asm (
            "mcrr p7, #8, %0, %1, c0\n"
            : : "r" (salt & 0xffffffffu), "r" (salt >> 32)
            );
}

// Initialise 64-bit salt value for core 1
static __rcpinline void rcp_salt_core1(uint64_t salt) {
    rcp_asm (
            "mcrr p7, #8, %0, %1, c1\n"
            : : "r" (salt & 0xffffffffu), "r" (salt >> 32)
            );
}

// Get a 32-bit canary value
#define _rcp_canary_get(tag0, tag1) ({ \
    uint32_t __canary_u32; \
    rcp_asm ( \
        "mrc p7, #0, %0, " # tag0 ", " # tag1 ", #1\n" \
        : "=r" (__canary_u32) \
    ); \
    __canary_u32; \
})
#define rcp_canary_get(tag0, tag1) _rcp_canary_get(tag0, tag1)

// Assert that canary matches result of rcp_canaryget with the same tags:
#define _rcp_canary_check(tag0, tag1, canary) ({ \
    rcp_asm ( \
        "mcr p7, #0, %0, " # tag0 ", " # tag1 ", #1\n" \
        : : "r" (canary) \
    ); \
})
#define rcp_canary_check(tag0, tag1, canary) _rcp_canary_check(tag0, tag1, canary);

// Return true/false bit pattern for whether the salt for this core has been
// initialised. (Invoking with Rt=0xf will set the Arm N flag if initialised.)
// If the salt has not been initialised then any operation other than
// initialising the salt or checking the canary status is a hard error.
static __rcpinline uint32_t rcp_canary_status(void) {
    uint32_t ret;
    rcp_asm ("mrc p7, #1, %0, c0, c0, #0\n" : "=r" (ret));
    return ret;
}

// ----------------------------------------------------------------------------
// RCP Boolean instructions

// Assert b is a valid boolean (0xa500a500u or 0x00c300c3u)
static __rcpinline void rcp_bvalid(uint32_t b) {
    rcp_asm ("mcr p7, #1, %0, c0, c0, #0\n" : : "r" (b));
}

// Assert b is true (0xa500a500u)
static __rcpinline void rcp_btrue(uint32_t b) {
    rcp_asm ("mcr p7, #2, %0, c0, c0, #0\n" : : "r" (b));
}

// Assert b is false (0x00c300c3u)
static __rcpinline void rcp_bfalse(uint32_t b) {
    rcp_asm ("mcr p7, #3, %0, c0, c0, #1\n" : : "r" (b));
}

// Assert b0 and b1 are both valid booleans
static __rcpinline void rcp_b2valid(uint32_t b0, uint32_t b1) {
    rcp_asm ("mcrr p7, #0, %0, %1, c8\n" : : "r" (b0), "r" (b1));
}

// Assert b0 and b1 are both true
static __rcpinline void rcp_b2and(uint32_t b0, uint32_t b1) {
    rcp_asm ("mcrr p7, #1, %0, %1, c0\n" : : "r" (b0), "r" (b1));
}

// Assert b0 and b1 are valid, and at least one is true
static __rcpinline void rcp_b2or(uint32_t b0, uint32_t b1) {
    rcp_asm ("mcrr p7, #2, %0, %1, c0\n" : : "r" (b0), "r" (b1));
}

// Assert (b ^ mask) is a valid boolean
static __rcpinline void rcp_bxorvalid(uint32_t b, uint32_t mask) {
    rcp_asm ("mcrr p7, #3, %0, %1, c8\n" : : "r" (b), "r" (mask));
}

// Assert (b ^ mask) is true
static __rcpinline void rcp_bxortrue(uint32_t b, uint32_t mask) {
    rcp_asm ("mcrr p7, #4, %0, %1, c0\n" : : "r" (b), "r" (mask));
}

// Assert (b ^ mask) is false
static __rcpinline void rcp_bxorfalse(uint32_t b, uint32_t mask) {
    rcp_asm ("mcrr p7, #5, %0, %1, c8\n" : : "r" (b), "r" (mask));
}

// ----------------------------------------------------------------------------
// RCP Integer instructions

// Assert (x ^ parity) == 0x96009600u
static __rcpinline void rcp_ivalid(uint32_t x, uint32_t parity) {
    rcp_asm ("mcrr p7, #6, %0, %1, c8\n" : : "r" (x), "r" (parity));
}

// Assert x == y
static __rcpinline void rcp_iequal(uint32_t x, uint32_t y) {
    rcp_asm ("mcrr p7, #7, %0, %1, c0\n" : : "r" (x), "r" (y));
}

// ----------------------------------------------------------------------------
// RCP Random instructions

// Return a random 8-bit value generated from the upper 24 bits of the 64-bit
// salt value. This is the same PRNG used for random delay values.
static __rcpinline uint8_t rcp_random_byte(void) {
    uint8_t ret;
    rcp_asm ("mrc2 p7, #2, %0, c0, c0, #0\n" : "=r" (ret));
    return ret;
}

// ----------------------------------------------------------------------------
// RCP Sequence count instructions

// Directly write value cnt to the sequence counter.
// cnt must be a constant expression.
#define rcp_count_set(cnt) rcp_asm ( \
    "mcr p7, #4, r0, c%c0, c%c1, #0\n" \
    : : "i"(((cnt) >> 4) & 0xf), "i"((cnt) & 0xf) \
);

// Check value cnt against the sequence counter, then increment the counter.
// cnt must be a constant expression.
#define rcp_count_check(cnt) rcp_asm ( \
    "mcr p7, #5, r0, c%c0, c%c1, #1\n" \
    : : "i"(((cnt) >> 4) & 0xf), "i"((cnt) & 0xf) \
);

// ----------------------------------------------------------------------------
// RCP Panic instructions

// Stall the coprocessor port. If the coprocessor access goes away, assert NMI

static __rcpinline __attribute__((noreturn)) void rcp_panic(void) {
    rcp_asm("cdp p7, #0, c0, c0, c0, #1");
    __builtin_unreachable();
}

#endif // __riscv
// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif
#endif
