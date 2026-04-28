/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
// =============================================================================
// Register block : OTP_DATA
// Version        : 1
// Bus type       : apb
// Description    : Predefined OTP data layout for RP2350
// =============================================================================
#ifndef HARDWARE_REGS_OTP_DATA_DEFINED
#define HARDWARE_REGS_OTP_DATA_DEFINED
// =============================================================================
// Register    : OTP_DATA_CHIPID_DEVICE0
// Description : Lower 16 bits of test lot/device number (ECC)
#define OTP_DATA_CHIPID_DEVICE0_ROW _u(0x0)
#define OTP_DATA_CHIPID_DEVICE0_BITS   _u(0x0000ffff)
#define OTP_DATA_CHIPID_DEVICE0_RESET  "-"
#define OTP_DATA_CHIPID_DEVICE0_MSB    _u(15)
#define OTP_DATA_CHIPID_DEVICE0_LSB    _u(0)
#define OTP_DATA_CHIPID_DEVICE0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CHIPID_DEVICE1
// Description : Upper 16 bits of test lot/device number (ECC)
#define OTP_DATA_CHIPID_DEVICE1_ROW _u(0x1)
#define OTP_DATA_CHIPID_DEVICE1_BITS   _u(0x0000ffff)
#define OTP_DATA_CHIPID_DEVICE1_RESET  "-"
#define OTP_DATA_CHIPID_DEVICE1_MSB    _u(15)
#define OTP_DATA_CHIPID_DEVICE1_LSB    _u(0)
#define OTP_DATA_CHIPID_DEVICE1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CHIPID_WAFER0
// Description : Lower 16 bits of wafer lot (ECC)
#define OTP_DATA_CHIPID_WAFER0_ROW _u(0x2)
#define OTP_DATA_CHIPID_WAFER0_BITS   _u(0x0000ffff)
#define OTP_DATA_CHIPID_WAFER0_RESET  "-"
#define OTP_DATA_CHIPID_WAFER0_MSB    _u(15)
#define OTP_DATA_CHIPID_WAFER0_LSB    _u(0)
#define OTP_DATA_CHIPID_WAFER0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CHIPID_WAFER1
// Description : Upper 16 bits of wafer lot (ECC)
#define OTP_DATA_CHIPID_WAFER1_ROW _u(0x3)
#define OTP_DATA_CHIPID_WAFER1_BITS   _u(0x0000ffff)
#define OTP_DATA_CHIPID_WAFER1_RESET  "-"
#define OTP_DATA_CHIPID_WAFER1_MSB    _u(15)
#define OTP_DATA_CHIPID_WAFER1_LSB    _u(0)
#define OTP_DATA_CHIPID_WAFER1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID0
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID0_ROW _u(0x4)
#define OTP_DATA_RANDID0_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID0_RESET  "-"
#define OTP_DATA_RANDID0_MSB    _u(15)
#define OTP_DATA_RANDID0_LSB    _u(0)
#define OTP_DATA_RANDID0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID1
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID1_ROW _u(0x5)
#define OTP_DATA_RANDID1_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID1_RESET  "-"
#define OTP_DATA_RANDID1_MSB    _u(15)
#define OTP_DATA_RANDID1_LSB    _u(0)
#define OTP_DATA_RANDID1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID2
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID2_ROW _u(0x6)
#define OTP_DATA_RANDID2_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID2_RESET  "-"
#define OTP_DATA_RANDID2_MSB    _u(15)
#define OTP_DATA_RANDID2_LSB    _u(0)
#define OTP_DATA_RANDID2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID3
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID3_ROW _u(0x7)
#define OTP_DATA_RANDID3_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID3_RESET  "-"
#define OTP_DATA_RANDID3_MSB    _u(15)
#define OTP_DATA_RANDID3_LSB    _u(0)
#define OTP_DATA_RANDID3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID4
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID4_ROW _u(0x8)
#define OTP_DATA_RANDID4_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID4_RESET  "-"
#define OTP_DATA_RANDID4_MSB    _u(15)
#define OTP_DATA_RANDID4_LSB    _u(0)
#define OTP_DATA_RANDID4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID5
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID5_ROW _u(0x9)
#define OTP_DATA_RANDID5_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID5_RESET  "-"
#define OTP_DATA_RANDID5_MSB    _u(15)
#define OTP_DATA_RANDID5_LSB    _u(0)
#define OTP_DATA_RANDID5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID6
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID6_ROW _u(0xa)
#define OTP_DATA_RANDID6_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID6_RESET  "-"
#define OTP_DATA_RANDID6_MSB    _u(15)
#define OTP_DATA_RANDID6_LSB    _u(0)
#define OTP_DATA_RANDID6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_RANDID7
// Description : 16 bits of random data sampled from the TRNG during
//               manufacturing (ECC)
#define OTP_DATA_RANDID7_ROW _u(0xb)
#define OTP_DATA_RANDID7_BITS   _u(0x0000ffff)
#define OTP_DATA_RANDID7_RESET  "-"
#define OTP_DATA_RANDID7_MSB    _u(15)
#define OTP_DATA_RANDID7_LSB    _u(0)
#define OTP_DATA_RANDID7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_ROSC_CALIB
// Description : Ring oscillator calibration constant measured during
//               manufacturing (ECC)
#define OTP_DATA_ROSC_CALIB_ROW _u(0x10)
#define OTP_DATA_ROSC_CALIB_BITS   _u(0x0000ffff)
#define OTP_DATA_ROSC_CALIB_RESET  "-"
#define OTP_DATA_ROSC_CALIB_MSB    _u(15)
#define OTP_DATA_ROSC_CALIB_LSB    _u(0)
#define OTP_DATA_ROSC_CALIB_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_LPOSC_CALIB
// Description : Low-power oscillator calibration constant measured during
//               manufacturing (ECC)
#define OTP_DATA_LPOSC_CALIB_ROW _u(0x11)
#define OTP_DATA_LPOSC_CALIB_BITS   _u(0x0000ffff)
#define OTP_DATA_LPOSC_CALIB_RESET  "-"
#define OTP_DATA_LPOSC_CALIB_MSB    _u(15)
#define OTP_DATA_LPOSC_CALIB_LSB    _u(0)
#define OTP_DATA_LPOSC_CALIB_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_INFO_CRC0
// Description : Lower 16 bits of CRC32 of OTP addresses 0x00 through 0x6b
//               (polynomial 0x4c11db7, input reflected, output reflected, seed
//               all-ones, final XOR all-ones) (ECC)
#define OTP_DATA_INFO_CRC0_ROW _u(0x36)
#define OTP_DATA_INFO_CRC0_BITS   _u(0x0000ffff)
#define OTP_DATA_INFO_CRC0_RESET  "-"
#define OTP_DATA_INFO_CRC0_MSB    _u(15)
#define OTP_DATA_INFO_CRC0_LSB    _u(0)
#define OTP_DATA_INFO_CRC0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_INFO_CRC1
// Description : Upper 16 bits of CRC32 of OTP addresses 0x00 through 0x6b (ECC)
#define OTP_DATA_INFO_CRC1_ROW _u(0x37)
#define OTP_DATA_INFO_CRC1_BITS   _u(0x0000ffff)
#define OTP_DATA_INFO_CRC1_RESET  "-"
#define OTP_DATA_INFO_CRC1_MSB    _u(15)
#define OTP_DATA_INFO_CRC1_LSB    _u(0)
#define OTP_DATA_INFO_CRC1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0
// Description : Page 0 critical boot flags (RBIT-8)
#define OTP_DATA_CRIT0_ROW _u(0x38)
#define OTP_DATA_CRIT0_BITS   _u(0x00000003)
#define OTP_DATA_CRIT0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT0_RISCV_DISABLE
// Description : Permanently disable RISC-V processors (Hazard3)
#define OTP_DATA_CRIT0_RISCV_DISABLE_RESET  "-"
#define OTP_DATA_CRIT0_RISCV_DISABLE_BITS   _u(0x00000002)
#define OTP_DATA_CRIT0_RISCV_DISABLE_MSB    _u(1)
#define OTP_DATA_CRIT0_RISCV_DISABLE_LSB    _u(1)
#define OTP_DATA_CRIT0_RISCV_DISABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT0_ARM_DISABLE
// Description : Permanently disable ARM processors (Cortex-M33)
#define OTP_DATA_CRIT0_ARM_DISABLE_RESET  "-"
#define OTP_DATA_CRIT0_ARM_DISABLE_BITS   _u(0x00000001)
#define OTP_DATA_CRIT0_ARM_DISABLE_MSB    _u(0)
#define OTP_DATA_CRIT0_ARM_DISABLE_LSB    _u(0)
#define OTP_DATA_CRIT0_ARM_DISABLE_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R1
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R1_ROW _u(0x39)
#define OTP_DATA_CRIT0_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R1_RESET  "-"
#define OTP_DATA_CRIT0_R1_MSB    _u(23)
#define OTP_DATA_CRIT0_R1_LSB    _u(0)
#define OTP_DATA_CRIT0_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R2
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R2_ROW _u(0x3a)
#define OTP_DATA_CRIT0_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R2_RESET  "-"
#define OTP_DATA_CRIT0_R2_MSB    _u(23)
#define OTP_DATA_CRIT0_R2_LSB    _u(0)
#define OTP_DATA_CRIT0_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R3
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R3_ROW _u(0x3b)
#define OTP_DATA_CRIT0_R3_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R3_RESET  "-"
#define OTP_DATA_CRIT0_R3_MSB    _u(23)
#define OTP_DATA_CRIT0_R3_LSB    _u(0)
#define OTP_DATA_CRIT0_R3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R4
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R4_ROW _u(0x3c)
#define OTP_DATA_CRIT0_R4_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R4_RESET  "-"
#define OTP_DATA_CRIT0_R4_MSB    _u(23)
#define OTP_DATA_CRIT0_R4_LSB    _u(0)
#define OTP_DATA_CRIT0_R4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R5
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R5_ROW _u(0x3d)
#define OTP_DATA_CRIT0_R5_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R5_RESET  "-"
#define OTP_DATA_CRIT0_R5_MSB    _u(23)
#define OTP_DATA_CRIT0_R5_LSB    _u(0)
#define OTP_DATA_CRIT0_R5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R6
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R6_ROW _u(0x3e)
#define OTP_DATA_CRIT0_R6_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R6_RESET  "-"
#define OTP_DATA_CRIT0_R6_MSB    _u(23)
#define OTP_DATA_CRIT0_R6_LSB    _u(0)
#define OTP_DATA_CRIT0_R6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT0_R7
// Description : Redundant copy of CRIT0
#define OTP_DATA_CRIT0_R7_ROW _u(0x3f)
#define OTP_DATA_CRIT0_R7_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT0_R7_RESET  "-"
#define OTP_DATA_CRIT0_R7_MSB    _u(23)
#define OTP_DATA_CRIT0_R7_LSB    _u(0)
#define OTP_DATA_CRIT0_R7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1
// Description : Page 1 critical boot flags (RBIT-8)
#define OTP_DATA_CRIT1_ROW _u(0x40)
#define OTP_DATA_CRIT1_BITS   _u(0x0000007f)
#define OTP_DATA_CRIT1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS
// Description : Increase the sensitivity of the glitch detectors from their
//               default.
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS_RESET  "-"
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS_BITS   _u(0x00000060)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS_MSB    _u(6)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS_LSB    _u(5)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_SENS_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE
// Description : Arm the glitch detectors to reset the system if an abnormal
//               clock/power event is observed.
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE_RESET  "-"
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE_BITS   _u(0x00000010)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE_MSB    _u(4)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE_LSB    _u(4)
#define OTP_DATA_CRIT1_GLITCH_DETECTOR_ENABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_BOOT_ARCH
// Description : Set the default boot architecture, 0=ARM 1=RISC-V. Ignored if
//               ARM_DISABLE, RISCV_DISABLE or SECURE_BOOT_ENABLE is set.
#define OTP_DATA_CRIT1_BOOT_ARCH_RESET  "-"
#define OTP_DATA_CRIT1_BOOT_ARCH_BITS   _u(0x00000008)
#define OTP_DATA_CRIT1_BOOT_ARCH_MSB    _u(3)
#define OTP_DATA_CRIT1_BOOT_ARCH_LSB    _u(3)
#define OTP_DATA_CRIT1_BOOT_ARCH_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_DEBUG_DISABLE
// Description : Disable all debug access
#define OTP_DATA_CRIT1_DEBUG_DISABLE_RESET  "-"
#define OTP_DATA_CRIT1_DEBUG_DISABLE_BITS   _u(0x00000004)
#define OTP_DATA_CRIT1_DEBUG_DISABLE_MSB    _u(2)
#define OTP_DATA_CRIT1_DEBUG_DISABLE_LSB    _u(2)
#define OTP_DATA_CRIT1_DEBUG_DISABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE
// Description : Disable Secure debug access
#define OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE_RESET  "-"
#define OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE_BITS   _u(0x00000002)
#define OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE_MSB    _u(1)
#define OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE_LSB    _u(1)
#define OTP_DATA_CRIT1_SECURE_DEBUG_DISABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_CRIT1_SECURE_BOOT_ENABLE
// Description : Enable boot signature enforcement, and permanently disable the
//               RISC-V cores.
#define OTP_DATA_CRIT1_SECURE_BOOT_ENABLE_RESET  "-"
#define OTP_DATA_CRIT1_SECURE_BOOT_ENABLE_BITS   _u(0x00000001)
#define OTP_DATA_CRIT1_SECURE_BOOT_ENABLE_MSB    _u(0)
#define OTP_DATA_CRIT1_SECURE_BOOT_ENABLE_LSB    _u(0)
#define OTP_DATA_CRIT1_SECURE_BOOT_ENABLE_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R1
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R1_ROW _u(0x41)
#define OTP_DATA_CRIT1_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R1_RESET  "-"
#define OTP_DATA_CRIT1_R1_MSB    _u(23)
#define OTP_DATA_CRIT1_R1_LSB    _u(0)
#define OTP_DATA_CRIT1_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R2
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R2_ROW _u(0x42)
#define OTP_DATA_CRIT1_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R2_RESET  "-"
#define OTP_DATA_CRIT1_R2_MSB    _u(23)
#define OTP_DATA_CRIT1_R2_LSB    _u(0)
#define OTP_DATA_CRIT1_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R3
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R3_ROW _u(0x43)
#define OTP_DATA_CRIT1_R3_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R3_RESET  "-"
#define OTP_DATA_CRIT1_R3_MSB    _u(23)
#define OTP_DATA_CRIT1_R3_LSB    _u(0)
#define OTP_DATA_CRIT1_R3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R4
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R4_ROW _u(0x44)
#define OTP_DATA_CRIT1_R4_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R4_RESET  "-"
#define OTP_DATA_CRIT1_R4_MSB    _u(23)
#define OTP_DATA_CRIT1_R4_LSB    _u(0)
#define OTP_DATA_CRIT1_R4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R5
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R5_ROW _u(0x45)
#define OTP_DATA_CRIT1_R5_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R5_RESET  "-"
#define OTP_DATA_CRIT1_R5_MSB    _u(23)
#define OTP_DATA_CRIT1_R5_LSB    _u(0)
#define OTP_DATA_CRIT1_R5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R6
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R6_ROW _u(0x46)
#define OTP_DATA_CRIT1_R6_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R6_RESET  "-"
#define OTP_DATA_CRIT1_R6_MSB    _u(23)
#define OTP_DATA_CRIT1_R6_LSB    _u(0)
#define OTP_DATA_CRIT1_R6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_CRIT1_R7
// Description : Redundant copy of CRIT1
#define OTP_DATA_CRIT1_R7_ROW _u(0x47)
#define OTP_DATA_CRIT1_R7_BITS   _u(0x00ffffff)
#define OTP_DATA_CRIT1_R7_RESET  "-"
#define OTP_DATA_CRIT1_R7_MSB    _u(23)
#define OTP_DATA_CRIT1_R7_LSB    _u(0)
#define OTP_DATA_CRIT1_R7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY0
// Description : Bits 15:0 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY0_ROW _u(0x48)
#define OTP_DATA_BOOTKEY0_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY0_RESET  "-"
#define OTP_DATA_BOOTKEY0_MSB    _u(15)
#define OTP_DATA_BOOTKEY0_LSB    _u(0)
#define OTP_DATA_BOOTKEY0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY1
// Description : Bits 31:16 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY1_ROW _u(0x49)
#define OTP_DATA_BOOTKEY1_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY1_RESET  "-"
#define OTP_DATA_BOOTKEY1_MSB    _u(15)
#define OTP_DATA_BOOTKEY1_LSB    _u(0)
#define OTP_DATA_BOOTKEY1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY2
// Description : Bits 47:32 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY2_ROW _u(0x4a)
#define OTP_DATA_BOOTKEY2_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY2_RESET  "-"
#define OTP_DATA_BOOTKEY2_MSB    _u(15)
#define OTP_DATA_BOOTKEY2_LSB    _u(0)
#define OTP_DATA_BOOTKEY2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY3
// Description : Bits 63:48 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY3_ROW _u(0x4b)
#define OTP_DATA_BOOTKEY3_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY3_RESET  "-"
#define OTP_DATA_BOOTKEY3_MSB    _u(15)
#define OTP_DATA_BOOTKEY3_LSB    _u(0)
#define OTP_DATA_BOOTKEY3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY4
// Description : Bits 79:64 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY4_ROW _u(0x4c)
#define OTP_DATA_BOOTKEY4_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY4_RESET  "-"
#define OTP_DATA_BOOTKEY4_MSB    _u(15)
#define OTP_DATA_BOOTKEY4_LSB    _u(0)
#define OTP_DATA_BOOTKEY4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY5
// Description : Bits 95:80 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY5_ROW _u(0x4d)
#define OTP_DATA_BOOTKEY5_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY5_RESET  "-"
#define OTP_DATA_BOOTKEY5_MSB    _u(15)
#define OTP_DATA_BOOTKEY5_LSB    _u(0)
#define OTP_DATA_BOOTKEY5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY6
// Description : Bits 111:96 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY6_ROW _u(0x4e)
#define OTP_DATA_BOOTKEY6_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY6_RESET  "-"
#define OTP_DATA_BOOTKEY6_MSB    _u(15)
#define OTP_DATA_BOOTKEY6_LSB    _u(0)
#define OTP_DATA_BOOTKEY6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY7
// Description : Bits 127:112 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY7_ROW _u(0x4f)
#define OTP_DATA_BOOTKEY7_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY7_RESET  "-"
#define OTP_DATA_BOOTKEY7_MSB    _u(15)
#define OTP_DATA_BOOTKEY7_LSB    _u(0)
#define OTP_DATA_BOOTKEY7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY8
// Description : Bits 143:128 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY8_ROW _u(0x50)
#define OTP_DATA_BOOTKEY8_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY8_RESET  "-"
#define OTP_DATA_BOOTKEY8_MSB    _u(15)
#define OTP_DATA_BOOTKEY8_LSB    _u(0)
#define OTP_DATA_BOOTKEY8_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY9
// Description : Bits 159:144 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY9_ROW _u(0x51)
#define OTP_DATA_BOOTKEY9_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY9_RESET  "-"
#define OTP_DATA_BOOTKEY9_MSB    _u(15)
#define OTP_DATA_BOOTKEY9_LSB    _u(0)
#define OTP_DATA_BOOTKEY9_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY10
// Description : Bits 175:160 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY10_ROW _u(0x52)
#define OTP_DATA_BOOTKEY10_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY10_RESET  "-"
#define OTP_DATA_BOOTKEY10_MSB    _u(15)
#define OTP_DATA_BOOTKEY10_LSB    _u(0)
#define OTP_DATA_BOOTKEY10_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY11
// Description : Bits 191:176 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY11_ROW _u(0x53)
#define OTP_DATA_BOOTKEY11_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY11_RESET  "-"
#define OTP_DATA_BOOTKEY11_MSB    _u(15)
#define OTP_DATA_BOOTKEY11_LSB    _u(0)
#define OTP_DATA_BOOTKEY11_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY12
// Description : Bits 207:192 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY12_ROW _u(0x54)
#define OTP_DATA_BOOTKEY12_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY12_RESET  "-"
#define OTP_DATA_BOOTKEY12_MSB    _u(15)
#define OTP_DATA_BOOTKEY12_LSB    _u(0)
#define OTP_DATA_BOOTKEY12_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY13
// Description : Bits 223:208 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY13_ROW _u(0x55)
#define OTP_DATA_BOOTKEY13_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY13_RESET  "-"
#define OTP_DATA_BOOTKEY13_MSB    _u(15)
#define OTP_DATA_BOOTKEY13_LSB    _u(0)
#define OTP_DATA_BOOTKEY13_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY14
// Description : Bits 239:224 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY14_ROW _u(0x56)
#define OTP_DATA_BOOTKEY14_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY14_RESET  "-"
#define OTP_DATA_BOOTKEY14_MSB    _u(15)
#define OTP_DATA_BOOTKEY14_LSB    _u(0)
#define OTP_DATA_BOOTKEY14_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTKEY15
// Description : Bits 255:240 of SHA-256 hash of boot signing key (ECC)
#define OTP_DATA_BOOTKEY15_ROW _u(0x57)
#define OTP_DATA_BOOTKEY15_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTKEY15_RESET  "-"
#define OTP_DATA_BOOTKEY15_MSB    _u(15)
#define OTP_DATA_BOOTKEY15_LSB    _u(0)
#define OTP_DATA_BOOTKEY15_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION0
// Description : Default boot version thermometer counter, bits 23:0 (RBIT-3)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_ROW _u(0x58)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION0_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION0_R1
// Description : Redundant copy of DEFAULT_BOOT_VERSION0
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_ROW _u(0x59)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION0_R2
// Description : Redundant copy of DEFAULT_BOOT_VERSION0
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_ROW _u(0x5a)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION0_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION1
// Description : Default boot version thermometer counter, bits 47:24 (RBIT-3)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_ROW _u(0x5b)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION1_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION1_R1
// Description : Redundant copy of DEFAULT_BOOT_VERSION1
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_ROW _u(0x5c)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_DEFAULT_BOOT_VERSION1_R2
// Description : Redundant copy of DEFAULT_BOOT_VERSION1
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_ROW _u(0x5d)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_RESET  "-"
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_MSB    _u(23)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_LSB    _u(0)
#define OTP_DATA_DEFAULT_BOOT_VERSION1_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_FLAGS
// Description : Disable/Enable boot paths/features in the RP2350 mask ROM.
//               Disables always supersede enables. Enables are provided where
//               there are other configurations in OTP that must be valid.
//               (RBIT-3)
#define OTP_DATA_BOOT_FLAGS_ROW _u(0x5e)
#define OTP_DATA_BOOT_FLAGS_BITS   _u(0x0007ffff)
#define OTP_DATA_BOOT_FLAGS_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE
// Description : Require a partition table to be hashed (if not signed)
#define OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE_BITS   _u(0x00040000)
#define OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE_MSB    _u(18)
#define OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE_LSB    _u(18)
#define OTP_DATA_BOOT_FLAGS_HASHED_PARTITION_TABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE
// Description : Require a partition table to be signed
#define OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE_BITS   _u(0x00020000)
#define OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE_MSB    _u(17)
#define OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE_LSB    _u(17)
#define OTP_DATA_BOOT_FLAGS_SECURE_PARTITION_TABLE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH
// Description : Disable auto-switch of CPU architecture on boot when the (only)
//               binary to be booted is for the other ARM/RISC-V architecture
//               and both architectures are enabled
#define OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH_BITS   _u(0x00010000)
#define OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH_MSB    _u(16)
#define OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH_LSB    _u(16)
#define OTP_DATA_BOOT_FLAGS_DISABLE_AUTO_SWITCH_ARCH_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY
// Description : Restrict flash boot path to use of a single binary at the start
//               of flash
#define OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY_BITS   _u(0x00008000)
#define OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY_MSB    _u(15)
#define OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY_LSB    _u(15)
#define OTP_DATA_BOOT_FLAGS_SINGLE_FLASH_BINARY_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE
// Description : Override the limit for default flash metadata scanning. the
//               value is specified in FLASH_PARTITION_SLOT_SIZE. Make sure the
//               field is valid before setting this bit
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE_BITS   _u(0x00004000)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE_MSB    _u(14)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE_LSB    _u(14)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_PARTITION_SLOT_SIZE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE
// Description : Override the limit for default flash metadata scanning. the
//               value is specified in FLASH_METADATA_MAX_SCAN_SIZE. Make sure
//               the field is valid before setting this bit
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE_BITS   _u(0x00002000)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE_MSB    _u(13)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE_LSB    _u(13)
#define OTP_DATA_BOOT_FLAGS_OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DOUBLE_TAP
// Description : Enable entering BOOTSEL mode via double-tap of the RUN/RSTn
//               pin. Adds a significant delay to boot time.
#define OTP_DATA_BOOT_FLAGS_DOUBLE_TAP_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DOUBLE_TAP_BITS   _u(0x00001000)
#define OTP_DATA_BOOT_FLAGS_DOUBLE_TAP_MSB    _u(12)
#define OTP_DATA_BOOT_FLAGS_DOUBLE_TAP_LSB    _u(12)
#define OTP_DATA_BOOT_FLAGS_DOUBLE_TAP_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG
// Description : Use ROSC for BOOTSEL mode. Note ROSC should not be used for USB
//               boot, but is sufficient for UART boot. Ignored if
//               ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG is set.
//
//               Ensure that BOOTSEL_ROSC_DIV, BOOTSEL_ROSC_FREQA and
//               BOOTSEL_ROSC_FREQB are correctly programmed before setting this
//               bit.
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG_BITS   _u(0x00000800)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG_MSB    _u(11)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG_LSB    _u(11)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_ROSC_CFG_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG
// Description : Enable loading of the non-default XOSC and PLL configuration
//               before entering BOOTSEL mode.
//
//               Ensure that BOOTSEL_XOSC_CFG and BOOTSEL_PLL_CFG are correctly
//               programmed before setting this bit.
//
//               If this bit is set, user software may use the contents of
//               BOOTSEL_PLL_CFG to calculated the expected ROSC frequency based
//               on the fixed USB boot frequency of 48 MHz.
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG_BITS   _u(0x00000400)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG_MSB    _u(10)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG_LSB    _u(10)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_NON_DEFAULT_PLL_XOSC_CFG_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED
// Description : Enable bootloader activity LED. If set, bootsel_led_cfg is
//               assumed to be valid
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED_BITS   _u(0x00000200)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED_MSB    _u(9)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED_LSB    _u(9)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_LED_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT
// Description : None
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT_BITS   _u(0x00000100)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT_MSB    _u(8)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT_LSB    _u(8)
#define OTP_DATA_BOOT_FLAGS_ENABLE_BOOTSEL_UART_BOOT_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT_BITS   _u(0x00000080)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT_MSB    _u(7)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT_LSB    _u(7)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_UART_BOOT_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC_BITS   _u(0x00000040)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC_MSB    _u(6)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC_LSB    _u(6)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_PICOBOOT_IFC_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC_BITS   _u(0x00000020)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC_MSB    _u(5)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC_LSB    _u(5)
#define OTP_DATA_BOOT_FLAGS_DISABLE_BOOTSEL_USB_MSD_IFC_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH_BITS   _u(0x00000010)
#define OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH_MSB    _u(4)
#define OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH_LSB    _u(4)
#define OTP_DATA_BOOT_FLAGS_DISABLE_WATCHDOG_SCRATCH_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH_BITS   _u(0x00000008)
#define OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH_MSB    _u(3)
#define OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH_LSB    _u(3)
#define OTP_DATA_BOOT_FLAGS_DISABLE_POWER_SCRATCH_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT
// Description : None
#define OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT_BITS   _u(0x00000004)
#define OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT_MSB    _u(2)
#define OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT_LSB    _u(2)
#define OTP_DATA_BOOT_FLAGS_ENABLE_OTP_BOOT_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT_BITS   _u(0x00000002)
#define OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT_MSB    _u(1)
#define OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT_LSB    _u(1)
#define OTP_DATA_BOOT_FLAGS_DISABLE_OTP_BOOT_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT
// Description : None
#define OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT_BITS   _u(0x00000001)
#define OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT_MSB    _u(0)
#define OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT_LSB    _u(0)
#define OTP_DATA_BOOT_FLAGS_DISABLE_FLASH_BOOT_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_FLAGS_R1
// Description : Redundant copy of BOOT_FLAGS
#define OTP_DATA_BOOT_FLAGS_R1_ROW _u(0x5f)
#define OTP_DATA_BOOT_FLAGS_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOT_FLAGS_R1_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_R1_MSB    _u(23)
#define OTP_DATA_BOOT_FLAGS_R1_LSB    _u(0)
#define OTP_DATA_BOOT_FLAGS_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_FLAGS_R2
// Description : Redundant copy of BOOT_FLAGS
#define OTP_DATA_BOOT_FLAGS_R2_ROW _u(0x60)
#define OTP_DATA_BOOT_FLAGS_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOT_FLAGS_R2_RESET  "-"
#define OTP_DATA_BOOT_FLAGS_R2_MSB    _u(23)
#define OTP_DATA_BOOT_FLAGS_R2_LSB    _u(0)
#define OTP_DATA_BOOT_FLAGS_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE
// Description : Amount of flash to scan looking for partition table or image
//               metadata blocks (ECC) Enabled by the
//               OVERRIDE_FLASH_METADATA_MAX_SCAN_SIZE bit in BOOT_FLAGS, the
//               max number of bytes scanned is 512 * (value + 1).
//
//               Note that when scanning at the beginning of flash, rather than
//               in a partition, no more than the FLASH_PARTITION_SLOT_SIZE is
//               scanned.
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_ROW _u(0x61)
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_BITS   _u(0x0000ffff)
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_RESET  "-"
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_MSB    _u(15)
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_LSB    _u(0)
#define OTP_DATA_FLASH_METADATA_MAX_SCAN_SIZE_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_FLASH_PARTITION_SLOT_SIZE
// Description : Gap between partition table slot 0 and slot 1 at the start of
//               flash (the default size is 4096 bytes) (ECC) Enabled by the
//               OVERRIDE_FLASH_PARTITION_SLOT_SIZE bit in BOOT_FLAGS, the size
//               is 4096 * (value + 1)
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_ROW _u(0x62)
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_BITS   _u(0x0000ffff)
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_RESET  "-"
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_MSB    _u(15)
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_LSB    _u(0)
#define OTP_DATA_FLASH_PARTITION_SLOT_SIZE_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_LED_CFG
// Description : Pin configuration for LED status (used for anything in bootrom
//               that outputs to LED if enabled elsewhere
#define OTP_DATA_BOOTSEL_LED_CFG_ROW _u(0x63)
#define OTP_DATA_BOOTSEL_LED_CFG_BITS   _u(0x000000ff)
#define OTP_DATA_BOOTSEL_LED_CFG_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_LED_CFG_LED_PIN
// Description : GPIO index to use for bootloader activity LED.
#define OTP_DATA_BOOTSEL_LED_CFG_LED_PIN_RESET  "-"
#define OTP_DATA_BOOTSEL_LED_CFG_LED_PIN_BITS   _u(0x000000fc)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_PIN_MSB    _u(7)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_PIN_LSB    _u(2)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_PIN_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW
// Description : LED is active-low. (Default: active-high.)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW_RESET  "-"
#define OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW_BITS   _u(0x00000002)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW_MSB    _u(1)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW_LSB    _u(1)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_ACTIVELOW_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_LED_CFG_LED_VALID
// Description : Must be set to 1 for other LED configuration to be considered
//               valid
#define OTP_DATA_BOOTSEL_LED_CFG_LED_VALID_RESET  "-"
#define OTP_DATA_BOOTSEL_LED_CFG_LED_VALID_BITS   _u(0x00000001)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_VALID_MSB    _u(0)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_VALID_LSB    _u(0)
#define OTP_DATA_BOOTSEL_LED_CFG_LED_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_LED_CFG_R1
// Description : Redundant copy of BOOTSEL_LED_CFG
#define OTP_DATA_BOOTSEL_LED_CFG_R1_ROW _u(0x64)
#define OTP_DATA_BOOTSEL_LED_CFG_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOTSEL_LED_CFG_R1_RESET  "-"
#define OTP_DATA_BOOTSEL_LED_CFG_R1_MSB    _u(23)
#define OTP_DATA_BOOTSEL_LED_CFG_R1_LSB    _u(0)
#define OTP_DATA_BOOTSEL_LED_CFG_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_LED_CFG_R2
// Description : Redundant copy of BOOTSEL_LED_CFG
#define OTP_DATA_BOOTSEL_LED_CFG_R2_ROW _u(0x65)
#define OTP_DATA_BOOTSEL_LED_CFG_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOTSEL_LED_CFG_R2_RESET  "-"
#define OTP_DATA_BOOTSEL_LED_CFG_R2_MSB    _u(23)
#define OTP_DATA_BOOTSEL_LED_CFG_R2_LSB    _u(0)
#define OTP_DATA_BOOTSEL_LED_CFG_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_PLL_CFG
// Description : Optional PLL configuration for BOOTSEL mode. (ECC)
//
//               This should be configured to produce an exact 48 MHz based on
//               the crystal oscillator frequency. User mode software may also
//               use this value to calculate the expected crystal frequency
//               based on an assumed 48 MHz PLL output.
//
//               If no configuration is given, the crystal is assumed to be 12
//               MHz.
//
//               The PLL frequency can be calculated as:
//
//               PLL out = (XOSC frequency / (REFDIV+1)) x FBDIV / (POSTDIV1 x
//               POSTDIV2)
//
//               Conversely the crystal frequency can be calculated as:
//
//               XOSC frequency = 48 MHz x (REFDIV+1) x (POSTDIV1 x POSTDIV2) /
//               FBDIV
//
//               (Note the  +1 on REFDIV is because the value stored in this OTP
//               location is the actual divisor value minus one.)
//
//               Valid if and only if BOOTSEL_CFG_NONDEFAULT_CLOCK_CFG bit is
//               set. That bit should be set only after this row and
//               BOOTSEL_XOSC_CFG are both correctly programmed.
#define OTP_DATA_BOOTSEL_PLL_CFG_ROW _u(0x66)
#define OTP_DATA_BOOTSEL_PLL_CFG_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTSEL_PLL_CFG_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_PLL_CFG_REFDIV
// Description : PLL reference divisor, minus one.
//
//               Programming a value of 0 means a reference divisor of 1.
//               Programming a value of 1 means a reference divisor of 2 (for
//               exceptionally fast XIN inputs)
#define OTP_DATA_BOOTSEL_PLL_CFG_REFDIV_RESET  "-"
#define OTP_DATA_BOOTSEL_PLL_CFG_REFDIV_BITS   _u(0x00008000)
#define OTP_DATA_BOOTSEL_PLL_CFG_REFDIV_MSB    _u(15)
#define OTP_DATA_BOOTSEL_PLL_CFG_REFDIV_LSB    _u(15)
#define OTP_DATA_BOOTSEL_PLL_CFG_REFDIV_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2
// Description : PLL post-divide 2 divisor, in the range 1..7 inclusive.
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2_RESET  "-"
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2_BITS   _u(0x00007000)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2_MSB    _u(14)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2_LSB    _u(12)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1
// Description : PLL post-divide 1 divisor, in the range 1..7 inclusive.
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1_RESET  "-"
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1_BITS   _u(0x00000e00)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1_MSB    _u(11)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1_LSB    _u(9)
#define OTP_DATA_BOOTSEL_PLL_CFG_POSTDIV1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_PLL_CFG_FBDIV
// Description : PLL feedback divisor, in the range 16..320 inclusive.
#define OTP_DATA_BOOTSEL_PLL_CFG_FBDIV_RESET  "-"
#define OTP_DATA_BOOTSEL_PLL_CFG_FBDIV_BITS   _u(0x000001ff)
#define OTP_DATA_BOOTSEL_PLL_CFG_FBDIV_MSB    _u(8)
#define OTP_DATA_BOOTSEL_PLL_CFG_FBDIV_LSB    _u(0)
#define OTP_DATA_BOOTSEL_PLL_CFG_FBDIV_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_XOSC_CFG
// Description : Non-default crystal oscillator configuration for the USB
//               bootloader. (ECC)
//
//               These values may also be used by user code configuring the
//               crystal oscillator.
//
//               Valid if and only if BOOTSEL_CFG_NONDEFAULT_CLOCK_CFG bit is
//               set. That bit should be set only after this row and
//               BOOTSEL_PLL_CFG are both correctly programmed.
#define OTP_DATA_BOOTSEL_XOSC_CFG_ROW _u(0x67)
#define OTP_DATA_BOOTSEL_XOSC_CFG_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_XOSC_CFG_RANGE
// Description : Value of the XOSC_CTRL_FREQ_RANGE register.
//               0x0 -> 1_15MHZ
//               0x1 -> 10_30MHZ
//               0x2 -> 25_60MHZ
//               0x3 -> 40_100MHZ
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_RESET           "-"
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_BITS            _u(0x0000c000)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_MSB             _u(15)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_LSB             _u(14)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_ACCESS          "RO"
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_VALUE_1_15MHZ   _u(0x0)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_VALUE_10_30MHZ  _u(0x1)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_VALUE_25_60MHZ  _u(0x2)
#define OTP_DATA_BOOTSEL_XOSC_CFG_RANGE_VALUE_40_100MHZ _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP
// Description : Value of the XOSC_STARTUP register
#define OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP_RESET  "-"
#define OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP_BITS   _u(0x00003fff)
#define OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP_MSB    _u(13)
#define OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP_LSB    _u(0)
#define OTP_DATA_BOOTSEL_XOSC_CFG_STARTUP_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_ROSC_DIV
// Description : Divider and range settings for the ring oscillator used by
//               BOOTSEL mode. (ECC)
//
//               These fields correspond to the ROSC_CTRL_FREQ_RANGE and
//               ROSC_DIV register fields in the ROSC control registers.
//
//               Valid if and only if BOOTSEL_CFG_NONDEFAULT_ROSC_CFG is set.
#define OTP_DATA_BOOTSEL_ROSC_DIV_ROW _u(0x68)
#define OTP_DATA_BOOTSEL_ROSC_DIV_BITS   _u(0x000003ff)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_ROSC_DIV_RANGE
// Description : 0x0 -> LOW
//               0x1 -> MEDIUM
//               0x3 -> HIGH
//               0x2 -> TOOHIGH
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_RESET         "-"
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_BITS          _u(0x00000300)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_MSB           _u(9)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_LSB           _u(8)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_ACCESS        "RO"
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_VALUE_LOW     _u(0x0)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_VALUE_MEDIUM  _u(0x1)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_VALUE_HIGH    _u(0x3)
#define OTP_DATA_BOOTSEL_ROSC_DIV_RANGE_VALUE_TOOHIGH _u(0x2)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOTSEL_ROSC_DIV_DIV
// Description : None
#define OTP_DATA_BOOTSEL_ROSC_DIV_DIV_RESET  "-"
#define OTP_DATA_BOOTSEL_ROSC_DIV_DIV_BITS   _u(0x000000ff)
#define OTP_DATA_BOOTSEL_ROSC_DIV_DIV_MSB    _u(7)
#define OTP_DATA_BOOTSEL_ROSC_DIV_DIV_LSB    _u(0)
#define OTP_DATA_BOOTSEL_ROSC_DIV_DIV_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_ROSC_FREQA
// Description : FREQA settings for the ring oscillator, used by BOOTSEL mode.
//               (ECC)
//
//               Valid if and only if BOOTSEL_CFG_NONDEFAULT_ROSC_CFG is set.
#define OTP_DATA_BOOTSEL_ROSC_FREQA_ROW _u(0x69)
#define OTP_DATA_BOOTSEL_ROSC_FREQA_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTSEL_ROSC_FREQA_RESET  "-"
#define OTP_DATA_BOOTSEL_ROSC_FREQA_MSB    _u(15)
#define OTP_DATA_BOOTSEL_ROSC_FREQA_LSB    _u(0)
#define OTP_DATA_BOOTSEL_ROSC_FREQA_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOTSEL_ROSC_FREQB
// Description : FREQB settings for the ring oscillator, used by BOOTSEL mode.
//               (ECC)
//
//               Valid if and only if BOOTSEL_CFG_NONDEFAULT_ROSC_CFG is set.
#define OTP_DATA_BOOTSEL_ROSC_FREQB_ROW _u(0x6a)
#define OTP_DATA_BOOTSEL_ROSC_FREQB_BITS   _u(0x0000ffff)
#define OTP_DATA_BOOTSEL_ROSC_FREQB_RESET  "-"
#define OTP_DATA_BOOTSEL_ROSC_FREQB_MSB    _u(15)
#define OTP_DATA_BOOTSEL_ROSC_FREQB_LSB    _u(0)
#define OTP_DATA_BOOTSEL_ROSC_FREQB_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_USB_BOOT_FLAGS
// Description : USB boot specific feature flags (RBIT-3)
#define OTP_DATA_USB_BOOT_FLAGS_ROW _u(0x6b)
#define OTP_DATA_USB_BOOT_FLAGS_BITS   _u(0x00c0ffff)
#define OTP_DATA_USB_BOOT_FLAGS_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP
// Description : Swap DM/DP during USB boot, to support board layouts with
//               mirrored USB routing (deliberate or accidental).
#define OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP_BITS   _u(0x00800000)
#define OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP_MSB    _u(23)
#define OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP_LSB    _u(23)
#define OTP_DATA_USB_BOOT_FLAGS_DP_DM_SWAP_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID
// Description : valid flag for INFO_UF2_TXT_BOARD_ID_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 15)
#define OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID_BITS   _u(0x00400000)
#define OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID_MSB    _u(22)
#define OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID_LSB    _u(22)
#define OTP_DATA_USB_BOOT_FLAGS_WHITE_LABEL_ADDR_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID
// Description : valid flag for the USB_WHITE_LABEL_ADDR field
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID_BITS   _u(0x00008000)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID_MSB    _u(15)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID_LSB    _u(15)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_BOARD_ID_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID
// Description : valid flag for INFO_UF2_TXT_MODEL_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 14)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID_BITS   _u(0x00004000)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID_MSB    _u(14)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID_LSB    _u(14)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INFO_UF2_TXT_MODEL_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID
// Description : valid flag for INDEX_HTM_REDIRECT_NAME_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 13)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID_BITS   _u(0x00002000)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID_MSB    _u(13)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID_LSB    _u(13)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_NAME_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID
// Description : valid flag for INDEX_HTM_REDIRECT_URL_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 12)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID_BITS   _u(0x00001000)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID_MSB    _u(12)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID_LSB    _u(12)
#define OTP_DATA_USB_BOOT_FLAGS_WL_INDEX_HTM_REDIRECT_URL_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID
// Description : valid flag for SCSI_INQUIRY_VERSION_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 11)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID_BITS   _u(0x00000800)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID_MSB    _u(11)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID_LSB    _u(11)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VERSION_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID
// Description : valid flag for SCSI_INQUIRY_PRODUCT_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 10)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID_BITS   _u(0x00000400)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID_MSB    _u(10)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID_LSB    _u(10)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_PRODUCT_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID
// Description : valid flag for SCSI_INQUIRY_VENDOR_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 9)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID_BITS   _u(0x00000200)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID_MSB    _u(9)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID_LSB    _u(9)
#define OTP_DATA_USB_BOOT_FLAGS_WL_SCSI_INQUIRY_VENDOR_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID
// Description : valid flag for VOLUME_LABEL_STRDEF entry of the USB_WHITE_LABEL
//               struct (index 8)
#define OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID_BITS   _u(0x00000100)
#define OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID_MSB    _u(8)
#define OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID_LSB    _u(8)
#define OTP_DATA_USB_BOOT_FLAGS_WL_VOLUME_LABEL_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALU
//               ES_VALID
// Description : valid flag for USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES entry of
//               the USB_WHITE_LABEL struct (index 7)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES_VALID_BITS   _u(0x00000080)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES_VALID_MSB    _u(7)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES_VALID_LSB    _u(7)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALI
//               D
// Description : valid flag for USB_DEVICE_SERIAL_NUMBER_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 6)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALID_BITS   _u(0x00000040)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALID_MSB    _u(6)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALID_LSB    _u(6)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID
// Description : valid flag for USB_DEVICE_PRODUCT_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 5)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID_BITS   _u(0x00000020)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID_MSB    _u(5)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID_LSB    _u(5)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PRODUCT_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID
// Description : valid flag for USB_DEVICE_MANUFACTURER_STRDEF entry of the
//               USB_WHITE_LABEL struct (index 4)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID_BITS   _u(0x00000010)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID_MSB    _u(4)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID_LSB    _u(4)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_MANUFACTURER_STRDEF_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID
// Description : valid flag for USB_DEVICE_LANG_ID_VALUE entry of the
//               USB_WHITE_LABEL struct (index 3)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID_BITS   _u(0x00000008)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID_MSB    _u(3)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID_LSB    _u(3)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_LANG_ID_VALUE_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID
// Description : valid flag for USB_DEVICE_BCD_DEVICEVALUE entry of the
//               USB_WHITE_LABEL struct (index 2)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID_BITS   _u(0x00000004)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID_MSB    _u(2)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID_LSB    _u(2)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_SERIAL_NUMBER_VALUE_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID
// Description : valid flag for USB_DEVICE_PID_VALUE entry of the
//               USB_WHITE_LABEL struct (index 1)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID_BITS   _u(0x00000002)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID_MSB    _u(1)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID_LSB    _u(1)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_PID_VALUE_VALID_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID
// Description : valid flag for USB_DEVICE_VID_VALUE entry of the
//               USB_WHITE_LABEL struct (index 0)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID_BITS   _u(0x00000001)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID_MSB    _u(0)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID_LSB    _u(0)
#define OTP_DATA_USB_BOOT_FLAGS_WL_USB_DEVICE_VID_VALUE_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_USB_BOOT_FLAGS_R1
// Description : Redundant copy of USB_BOOT_FLAGS
#define OTP_DATA_USB_BOOT_FLAGS_R1_ROW _u(0x6c)
#define OTP_DATA_USB_BOOT_FLAGS_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_USB_BOOT_FLAGS_R1_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_R1_MSB    _u(23)
#define OTP_DATA_USB_BOOT_FLAGS_R1_LSB    _u(0)
#define OTP_DATA_USB_BOOT_FLAGS_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_USB_BOOT_FLAGS_R2
// Description : Redundant copy of USB_BOOT_FLAGS
#define OTP_DATA_USB_BOOT_FLAGS_R2_ROW _u(0x6d)
#define OTP_DATA_USB_BOOT_FLAGS_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_USB_BOOT_FLAGS_R2_RESET  "-"
#define OTP_DATA_USB_BOOT_FLAGS_R2_MSB    _u(23)
#define OTP_DATA_USB_BOOT_FLAGS_R2_LSB    _u(0)
#define OTP_DATA_USB_BOOT_FLAGS_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_USB_WHITE_LABEL_ADDR
// Description : Row index of the USB_WHITE_LABEL structure within OTP (ECC) The
//               table has 16 rows, each of which are also ECC and marked valid
//               by the corresponding valid bit in USB_BOOT_FLAGS (ECC).
//
//               The entries are either _VALUEs where the 16 bit value is used
//               as is, or _STRDEFs which acts as a pointers to a string value.
//
//               The value stored in a _STRDEF is two separate bytes: The low
//               seven bits of the first (LSB) byte indicates the number of
//               characters in the string, and the top bit of the first (LSB)
//               byte if set to indicate that each character in the string is
//               two bytes (Unicode) versus one byte if unset. The second (MSB)
//               byte represents the location of the string data, and is encoded
//               as the number of rows from this USB_WHITE_LABEL_ADDR; i.e. the
//               row of the start of the string is USB_WHITE_LABEL_ADDR value +
//               msb_byte.
//
//               In each case, the corresponding valid bit enables replacing the
//               default value for the corresponding item provided by the boot
//               rom.
//
//               Note that Unicode _STRDEFs are only supported for
//               USB_DEVICE_PRODUCT_STRDEF, USB_DEVICE_SERIAL_NUMBER_STRDEF and
//               USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES. Unicode values will be
//               ignored if specified for other fields, and non unicode values
//               for these three items will be converted to Unicode characters
//               by setting the upper 8 bits to zero.
//
//               Note that if the USB_WHITE_LABEL structure or the corresponding
//               strings are not readable by BOOTSEL mode based on OTP
//               permissions, or if alignment requirements are not met, then the
//               corresponding default values are used.
//
//               The index values indicate where each field is located (row
//               USB_WHITE_LABEL_ADDR value + index):
//               0x0000 -> INDEX_USB_DEVICE_VID_VALUE
//               0x0001 -> INDEX_USB_DEVICE_PID_VALUE
//               0x0002 -> INDEX_USB_DEVICE_BCD_DEVICE_VALUE
//               0x0003 -> INDEX_USB_DEVICE_LANG_ID_VALUE
//               0x0004 -> INDEX_USB_DEVICE_MANUFACTURER_STRDEF
//               0x0005 -> INDEX_USB_DEVICE_PRODUCT_STRDEF
//               0x0006 -> INDEX_USB_DEVICE_SERIAL_NUMBER_STRDEF
//               0x0007 -> INDEX_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES
//               0x0008 -> INDEX_VOLUME_LABEL_STRDEF
//               0x0009 -> INDEX_SCSI_INQUIRY_VENDOR_STRDEF
//               0x000a -> INDEX_SCSI_INQUIRY_PRODUCT_STRDEF
//               0x000b -> INDEX_SCSI_INQUIRY_VERSION_STRDEF
//               0x000c -> INDEX_INDEX_HTM_REDIRECT_URL_STRDEF
//               0x000d -> INDEX_INDEX_HTM_REDIRECT_NAME_STRDEF
//               0x000e -> INDEX_INFO_UF2_TXT_MODEL_STRDEF
//               0x000f -> INDEX_INFO_UF2_TXT_BOARD_ID_STRDEF
#define OTP_DATA_USB_WHITE_LABEL_ADDR_ROW _u(0x6e)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_BITS                                               _u(0x0000ffff)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_RESET                                              "-"
#define OTP_DATA_USB_WHITE_LABEL_ADDR_MSB                                                _u(15)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_LSB                                                _u(0)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_ACCESS                                             "RO"
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_VID_VALUE                   _u(0x0000)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_PID_VALUE                   _u(0x0001)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_BCD_DEVICE_VALUE            _u(0x0002)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_LANG_ID_VALUE               _u(0x0003)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_MANUFACTURER_STRDEF         _u(0x0004)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_PRODUCT_STRDEF              _u(0x0005)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_DEVICE_SERIAL_NUMBER_STRDEF        _u(0x0006)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_USB_CONFIG_ATTRIBUTES_MAX_POWER_VALUES _u(0x0007)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_VOLUME_LABEL_STRDEF                    _u(0x0008)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_SCSI_INQUIRY_VENDOR_STRDEF             _u(0x0009)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_SCSI_INQUIRY_PRODUCT_STRDEF            _u(0x000a)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_SCSI_INQUIRY_VERSION_STRDEF            _u(0x000b)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_INDEX_HTM_REDIRECT_URL_STRDEF          _u(0x000c)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_INDEX_HTM_REDIRECT_NAME_STRDEF         _u(0x000d)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_INFO_UF2_TXT_MODEL_STRDEF              _u(0x000e)
#define OTP_DATA_USB_WHITE_LABEL_ADDR_VALUE_INDEX_INFO_UF2_TXT_BOARD_ID_STRDEF           _u(0x000f)
// =============================================================================
// Register    : OTP_DATA_OTPBOOT_SRC_ADDR
// Description : OTP start row for the OTP boot image. (ECC)
//
//               If OTP boot is enabled, the bootrom will load from this
//               location into SRAM and then directly enter the loaded image.
//               Note that the image must be signed if SECURE_BOOT_ENABLE is
//               set. The image itself is assumed to be ECC-protected.
#define OTP_DATA_OTPBOOT_SRC_ADDR_ROW _u(0x6f)
#define OTP_DATA_OTPBOOT_SRC_ADDR_BITS   _u(0x0000ffff)
#define OTP_DATA_OTPBOOT_SRC_ADDR_RESET  "-"
#define OTP_DATA_OTPBOOT_SRC_ADDR_MSB    _u(15)
#define OTP_DATA_OTPBOOT_SRC_ADDR_LSB    _u(0)
#define OTP_DATA_OTPBOOT_SRC_ADDR_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_OTPBOOT_LEN
// Description : Length in rows of the OTP boot image. (ECC)
#define OTP_DATA_OTPBOOT_LEN_ROW _u(0x70)
#define OTP_DATA_OTPBOOT_LEN_BITS   _u(0x0000ffff)
#define OTP_DATA_OTPBOOT_LEN_RESET  "-"
#define OTP_DATA_OTPBOOT_LEN_MSB    _u(15)
#define OTP_DATA_OTPBOOT_LEN_LSB    _u(0)
#define OTP_DATA_OTPBOOT_LEN_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_OTPBOOT_DST0
// Description : Bits 15:0 of the OTP boot image load destination (and entry
//               point). (ECC)
//
//               Generally this should be a location in main SRAM. Must be at
//               least halfword-aligned.
#define OTP_DATA_OTPBOOT_DST0_ROW _u(0x71)
#define OTP_DATA_OTPBOOT_DST0_BITS   _u(0x0000ffff)
#define OTP_DATA_OTPBOOT_DST0_RESET  "-"
#define OTP_DATA_OTPBOOT_DST0_MSB    _u(15)
#define OTP_DATA_OTPBOOT_DST0_LSB    _u(0)
#define OTP_DATA_OTPBOOT_DST0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_OTPBOOT_DST1
// Description : Bits 31:16 of the OTP boot image load destination (and entry
//               point). (ECC)
//
//               Must be at least halfword-aligned.
#define OTP_DATA_OTPBOOT_DST1_ROW _u(0x72)
#define OTP_DATA_OTPBOOT_DST1_BITS   _u(0x0000ffff)
#define OTP_DATA_OTPBOOT_DST1_RESET  "-"
#define OTP_DATA_OTPBOOT_DST1_MSB    _u(15)
#define OTP_DATA_OTPBOOT_DST1_LSB    _u(0)
#define OTP_DATA_OTPBOOT_DST1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_UARTBOOT_BAUD
// Description : UART boot baud rate divided by 100. (ECC)
//
//               Note UART boot uses the USB clock setup to get a 48 MHz
//               reference.
#define OTP_DATA_UARTBOOT_BAUD_ROW _u(0x73)
#define OTP_DATA_UARTBOOT_BAUD_BITS   _u(0x0000ffff)
#define OTP_DATA_UARTBOOT_BAUD_RESET  "-"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_UARTBOOT_BAUD_BAUD
// Description : None
#define OTP_DATA_UARTBOOT_BAUD_BAUD_RESET  "-"
#define OTP_DATA_UARTBOOT_BAUD_BAUD_BITS   _u(0x0000ffff)
#define OTP_DATA_UARTBOOT_BAUD_BAUD_MSB    _u(15)
#define OTP_DATA_UARTBOOT_BAUD_BAUD_LSB    _u(0)
#define OTP_DATA_UARTBOOT_BAUD_BAUD_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_UARTBOOT_PINS
// Description : UART boot pin configuration
#define OTP_DATA_UARTBOOT_PINS_ROW _u(0x74)
#define OTP_DATA_UARTBOOT_PINS_BITS   _u(0x00003f3f)
#define OTP_DATA_UARTBOOT_PINS_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_UARTBOOT_PINS_PIN_RX
// Description : None
#define OTP_DATA_UARTBOOT_PINS_PIN_RX_RESET  "-"
#define OTP_DATA_UARTBOOT_PINS_PIN_RX_BITS   _u(0x00003f00)
#define OTP_DATA_UARTBOOT_PINS_PIN_RX_MSB    _u(13)
#define OTP_DATA_UARTBOOT_PINS_PIN_RX_LSB    _u(8)
#define OTP_DATA_UARTBOOT_PINS_PIN_RX_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_UARTBOOT_PINS_PIN_TX
// Description : None
#define OTP_DATA_UARTBOOT_PINS_PIN_TX_RESET  "-"
#define OTP_DATA_UARTBOOT_PINS_PIN_TX_BITS   _u(0x0000003f)
#define OTP_DATA_UARTBOOT_PINS_PIN_TX_MSB    _u(5)
#define OTP_DATA_UARTBOOT_PINS_PIN_TX_LSB    _u(0)
#define OTP_DATA_UARTBOOT_PINS_PIN_TX_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_I2CBOOT_PINS
// Description : None
#define OTP_DATA_I2CBOOT_PINS_ROW _u(0x75)
#define OTP_DATA_I2CBOOT_PINS_BITS   _u(0x00003f3f)
#define OTP_DATA_I2CBOOT_PINS_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_I2CBOOT_PINS_PIN_SCK
// Description : None
#define OTP_DATA_I2CBOOT_PINS_PIN_SCK_RESET  "-"
#define OTP_DATA_I2CBOOT_PINS_PIN_SCK_BITS   _u(0x00003f00)
#define OTP_DATA_I2CBOOT_PINS_PIN_SCK_MSB    _u(13)
#define OTP_DATA_I2CBOOT_PINS_PIN_SCK_LSB    _u(8)
#define OTP_DATA_I2CBOOT_PINS_PIN_SCK_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_I2CBOOT_PINS_PIN_SDA
// Description : None
#define OTP_DATA_I2CBOOT_PINS_PIN_SDA_RESET  "-"
#define OTP_DATA_I2CBOOT_PINS_PIN_SDA_BITS   _u(0x0000003f)
#define OTP_DATA_I2CBOOT_PINS_PIN_SDA_MSB    _u(5)
#define OTP_DATA_I2CBOOT_PINS_PIN_SDA_LSB    _u(0)
#define OTP_DATA_I2CBOOT_PINS_PIN_SDA_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE2_WATERMARK
// Description : Generate an error if I try to put more than 128 bytes in page
//               1. TODO remove this.
#define OTP_DATA_PAGE2_WATERMARK_ROW _u(0x80)
#define OTP_DATA_PAGE2_WATERMARK_BITS   _u(0x0000ffff)
#define OTP_DATA_PAGE2_WATERMARK_RESET  "-"
#define OTP_DATA_PAGE2_WATERMARK_MSB    _u(15)
#define OTP_DATA_PAGE2_WATERMARK_LSB    _u(0)
#define OTP_DATA_PAGE2_WATERMARK_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN
// Description : bits to opt in behavior for A0 only
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_ROW _u(0x81)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_BITS   _u(0x0000083f)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV
// Description : Enable quartering of ROSC divisor during signature check, to
//               reduce boot time
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV_BITS   _u(0x00000800)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV_MSB    _u(11)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV_LSB    _u(11)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_FASTER_SIGCHECK_ROSC_DIV_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUN
//               T
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUNT_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUNT_BITS   _u(0x00000020)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUNT_MSB    _u(5)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUNT_LSB    _u(5)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_SAMPLE_COUNT_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG_BITS   _u(0x00000010)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG_MSB    _u(4)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG_LSB    _u(4)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_OVERRIDE_TRNG_CONFIG_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS_BITS   _u(0x00000008)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS_MSB    _u(3)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS_LSB    _u(3)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_VNC_BYPASS_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS_BITS   _u(0x00000004)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS_MSB    _u(2)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS_LSB    _u(2)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_CRNGT_BYPASS_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELAT
//               E_BYPASS
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELATE_BYPASS_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELATE_BYPASS_BITS   _u(0x00000002)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELATE_BYPASS_MSB    _u(1)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELATE_BYPASS_LSB    _u(1)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_TRNG_DISABLE_AUTOCORRELATE_BYPASS_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG
// Description : None
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG_BITS   _u(0x00000001)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG_MSB    _u(0)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG_LSB    _u(0)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_USE_TRNG_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1
// Description : Redundant copy of BOOT_TEMP_CHICKEN_BIT_OPT_IN
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_ROW _u(0x82)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_MSB    _u(23)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_LSB    _u(0)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2
// Description : Redundant copy of BOOT_TEMP_CHICKEN_BIT_OPT_IN
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_ROW _u(0x83)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_BITS   _u(0x00ffffff)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_RESET  "-"
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_MSB    _u(23)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_LSB    _u(0)
#define OTP_DATA_BOOT_TEMP_CHICKEN_BIT_OPT_IN_R2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_TRNG_CONFIG
// Description : Override value for trng_config if enabled
#define OTP_DATA_TRNG_CONFIG_ROW _u(0x84)
#define OTP_DATA_TRNG_CONFIG_BITS   _u(0x00000003)
#define OTP_DATA_TRNG_CONFIG_RESET  "-"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_TRNG_CONFIG_CONFIG
// Description : None
#define OTP_DATA_TRNG_CONFIG_CONFIG_RESET  "-"
#define OTP_DATA_TRNG_CONFIG_CONFIG_BITS   _u(0x00000003)
#define OTP_DATA_TRNG_CONFIG_CONFIG_MSB    _u(1)
#define OTP_DATA_TRNG_CONFIG_CONFIG_LSB    _u(0)
#define OTP_DATA_TRNG_CONFIG_CONFIG_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_TRNG_SAMPLE_COUNT
// Description : Override value for trng_sample_cnt1 if enabled
#define OTP_DATA_TRNG_SAMPLE_COUNT_ROW _u(0x85)
#define OTP_DATA_TRNG_SAMPLE_COUNT_BITS   _u(0x0000ffff)
#define OTP_DATA_TRNG_SAMPLE_COUNT_RESET  "-"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_TRNG_SAMPLE_COUNT_COUNT
// Description : None
#define OTP_DATA_TRNG_SAMPLE_COUNT_COUNT_RESET  "-"
#define OTP_DATA_TRNG_SAMPLE_COUNT_COUNT_BITS   _u(0x0000ffff)
#define OTP_DATA_TRNG_SAMPLE_COUNT_COUNT_MSB    _u(15)
#define OTP_DATA_TRNG_SAMPLE_COUNT_COUNT_LSB    _u(0)
#define OTP_DATA_TRNG_SAMPLE_COUNT_COUNT_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_0
// Description : Bits 15:0 of key 1 (ECC)
#define OTP_DATA_KEY1_0_ROW _u(0xf48)
#define OTP_DATA_KEY1_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_0_RESET  "-"
#define OTP_DATA_KEY1_0_MSB    _u(15)
#define OTP_DATA_KEY1_0_LSB    _u(0)
#define OTP_DATA_KEY1_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_1
// Description : Bits 31:16 of key 1 (ECC)
#define OTP_DATA_KEY1_1_ROW _u(0xf49)
#define OTP_DATA_KEY1_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_1_RESET  "-"
#define OTP_DATA_KEY1_1_MSB    _u(15)
#define OTP_DATA_KEY1_1_LSB    _u(0)
#define OTP_DATA_KEY1_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_2
// Description : Bits 47:32 of key 1 (ECC)
#define OTP_DATA_KEY1_2_ROW _u(0xf4a)
#define OTP_DATA_KEY1_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_2_RESET  "-"
#define OTP_DATA_KEY1_2_MSB    _u(15)
#define OTP_DATA_KEY1_2_LSB    _u(0)
#define OTP_DATA_KEY1_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_3
// Description : Bits 63:48 of key 1 (ECC)
#define OTP_DATA_KEY1_3_ROW _u(0xf4b)
#define OTP_DATA_KEY1_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_3_RESET  "-"
#define OTP_DATA_KEY1_3_MSB    _u(15)
#define OTP_DATA_KEY1_3_LSB    _u(0)
#define OTP_DATA_KEY1_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_4
// Description : Bits 79:64 of key 1 (ECC)
#define OTP_DATA_KEY1_4_ROW _u(0xf4c)
#define OTP_DATA_KEY1_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_4_RESET  "-"
#define OTP_DATA_KEY1_4_MSB    _u(15)
#define OTP_DATA_KEY1_4_LSB    _u(0)
#define OTP_DATA_KEY1_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_5
// Description : Bits 95:80 of key 1 (ECC)
#define OTP_DATA_KEY1_5_ROW _u(0xf4d)
#define OTP_DATA_KEY1_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_5_RESET  "-"
#define OTP_DATA_KEY1_5_MSB    _u(15)
#define OTP_DATA_KEY1_5_LSB    _u(0)
#define OTP_DATA_KEY1_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_6
// Description : Bits 111:96 of key 1 (ECC)
#define OTP_DATA_KEY1_6_ROW _u(0xf4e)
#define OTP_DATA_KEY1_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_6_RESET  "-"
#define OTP_DATA_KEY1_6_MSB    _u(15)
#define OTP_DATA_KEY1_6_LSB    _u(0)
#define OTP_DATA_KEY1_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_7
// Description : Bits 127:112 of key 1 (ECC)
#define OTP_DATA_KEY1_7_ROW _u(0xf4f)
#define OTP_DATA_KEY1_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY1_7_RESET  "-"
#define OTP_DATA_KEY1_7_MSB    _u(15)
#define OTP_DATA_KEY1_7_LSB    _u(0)
#define OTP_DATA_KEY1_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_0
// Description : Bits 15:0 of key 2 (ECC)
#define OTP_DATA_KEY2_0_ROW _u(0xf50)
#define OTP_DATA_KEY2_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_0_RESET  "-"
#define OTP_DATA_KEY2_0_MSB    _u(15)
#define OTP_DATA_KEY2_0_LSB    _u(0)
#define OTP_DATA_KEY2_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_1
// Description : Bits 31:16 of key 2 (ECC)
#define OTP_DATA_KEY2_1_ROW _u(0xf51)
#define OTP_DATA_KEY2_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_1_RESET  "-"
#define OTP_DATA_KEY2_1_MSB    _u(15)
#define OTP_DATA_KEY2_1_LSB    _u(0)
#define OTP_DATA_KEY2_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_2
// Description : Bits 47:32 of key 2 (ECC)
#define OTP_DATA_KEY2_2_ROW _u(0xf52)
#define OTP_DATA_KEY2_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_2_RESET  "-"
#define OTP_DATA_KEY2_2_MSB    _u(15)
#define OTP_DATA_KEY2_2_LSB    _u(0)
#define OTP_DATA_KEY2_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_3
// Description : Bits 63:48 of key 2 (ECC)
#define OTP_DATA_KEY2_3_ROW _u(0xf53)
#define OTP_DATA_KEY2_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_3_RESET  "-"
#define OTP_DATA_KEY2_3_MSB    _u(15)
#define OTP_DATA_KEY2_3_LSB    _u(0)
#define OTP_DATA_KEY2_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_4
// Description : Bits 79:64 of key 2 (ECC)
#define OTP_DATA_KEY2_4_ROW _u(0xf54)
#define OTP_DATA_KEY2_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_4_RESET  "-"
#define OTP_DATA_KEY2_4_MSB    _u(15)
#define OTP_DATA_KEY2_4_LSB    _u(0)
#define OTP_DATA_KEY2_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_5
// Description : Bits 95:80 of key 2 (ECC)
#define OTP_DATA_KEY2_5_ROW _u(0xf55)
#define OTP_DATA_KEY2_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_5_RESET  "-"
#define OTP_DATA_KEY2_5_MSB    _u(15)
#define OTP_DATA_KEY2_5_LSB    _u(0)
#define OTP_DATA_KEY2_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_6
// Description : Bits 111:96 of key 2 (ECC)
#define OTP_DATA_KEY2_6_ROW _u(0xf56)
#define OTP_DATA_KEY2_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_6_RESET  "-"
#define OTP_DATA_KEY2_6_MSB    _u(15)
#define OTP_DATA_KEY2_6_LSB    _u(0)
#define OTP_DATA_KEY2_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_7
// Description : Bits 127:112 of key 2 (ECC)
#define OTP_DATA_KEY2_7_ROW _u(0xf57)
#define OTP_DATA_KEY2_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY2_7_RESET  "-"
#define OTP_DATA_KEY2_7_MSB    _u(15)
#define OTP_DATA_KEY2_7_LSB    _u(0)
#define OTP_DATA_KEY2_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_0
// Description : Bits 15:0 of key 3 (ECC)
#define OTP_DATA_KEY3_0_ROW _u(0xf58)
#define OTP_DATA_KEY3_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_0_RESET  "-"
#define OTP_DATA_KEY3_0_MSB    _u(15)
#define OTP_DATA_KEY3_0_LSB    _u(0)
#define OTP_DATA_KEY3_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_1
// Description : Bits 31:16 of key 3 (ECC)
#define OTP_DATA_KEY3_1_ROW _u(0xf59)
#define OTP_DATA_KEY3_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_1_RESET  "-"
#define OTP_DATA_KEY3_1_MSB    _u(15)
#define OTP_DATA_KEY3_1_LSB    _u(0)
#define OTP_DATA_KEY3_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_2
// Description : Bits 47:32 of key 3 (ECC)
#define OTP_DATA_KEY3_2_ROW _u(0xf5a)
#define OTP_DATA_KEY3_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_2_RESET  "-"
#define OTP_DATA_KEY3_2_MSB    _u(15)
#define OTP_DATA_KEY3_2_LSB    _u(0)
#define OTP_DATA_KEY3_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_3
// Description : Bits 63:48 of key 3 (ECC)
#define OTP_DATA_KEY3_3_ROW _u(0xf5b)
#define OTP_DATA_KEY3_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_3_RESET  "-"
#define OTP_DATA_KEY3_3_MSB    _u(15)
#define OTP_DATA_KEY3_3_LSB    _u(0)
#define OTP_DATA_KEY3_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_4
// Description : Bits 79:64 of key 3 (ECC)
#define OTP_DATA_KEY3_4_ROW _u(0xf5c)
#define OTP_DATA_KEY3_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_4_RESET  "-"
#define OTP_DATA_KEY3_4_MSB    _u(15)
#define OTP_DATA_KEY3_4_LSB    _u(0)
#define OTP_DATA_KEY3_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_5
// Description : Bits 95:80 of key 3 (ECC)
#define OTP_DATA_KEY3_5_ROW _u(0xf5d)
#define OTP_DATA_KEY3_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_5_RESET  "-"
#define OTP_DATA_KEY3_5_MSB    _u(15)
#define OTP_DATA_KEY3_5_LSB    _u(0)
#define OTP_DATA_KEY3_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_6
// Description : Bits 111:96 of key 3 (ECC)
#define OTP_DATA_KEY3_6_ROW _u(0xf5e)
#define OTP_DATA_KEY3_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_6_RESET  "-"
#define OTP_DATA_KEY3_6_MSB    _u(15)
#define OTP_DATA_KEY3_6_LSB    _u(0)
#define OTP_DATA_KEY3_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_7
// Description : Bits 127:112 of key 3 (ECC)
#define OTP_DATA_KEY3_7_ROW _u(0xf5f)
#define OTP_DATA_KEY3_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY3_7_RESET  "-"
#define OTP_DATA_KEY3_7_MSB    _u(15)
#define OTP_DATA_KEY3_7_LSB    _u(0)
#define OTP_DATA_KEY3_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_0
// Description : Bits 15:0 of key 4 (ECC)
#define OTP_DATA_KEY4_0_ROW _u(0xf60)
#define OTP_DATA_KEY4_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_0_RESET  "-"
#define OTP_DATA_KEY4_0_MSB    _u(15)
#define OTP_DATA_KEY4_0_LSB    _u(0)
#define OTP_DATA_KEY4_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_1
// Description : Bits 31:16 of key 4 (ECC)
#define OTP_DATA_KEY4_1_ROW _u(0xf61)
#define OTP_DATA_KEY4_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_1_RESET  "-"
#define OTP_DATA_KEY4_1_MSB    _u(15)
#define OTP_DATA_KEY4_1_LSB    _u(0)
#define OTP_DATA_KEY4_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_2
// Description : Bits 47:32 of key 4 (ECC)
#define OTP_DATA_KEY4_2_ROW _u(0xf62)
#define OTP_DATA_KEY4_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_2_RESET  "-"
#define OTP_DATA_KEY4_2_MSB    _u(15)
#define OTP_DATA_KEY4_2_LSB    _u(0)
#define OTP_DATA_KEY4_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_3
// Description : Bits 63:48 of key 4 (ECC)
#define OTP_DATA_KEY4_3_ROW _u(0xf63)
#define OTP_DATA_KEY4_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_3_RESET  "-"
#define OTP_DATA_KEY4_3_MSB    _u(15)
#define OTP_DATA_KEY4_3_LSB    _u(0)
#define OTP_DATA_KEY4_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_4
// Description : Bits 79:64 of key 4 (ECC)
#define OTP_DATA_KEY4_4_ROW _u(0xf64)
#define OTP_DATA_KEY4_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_4_RESET  "-"
#define OTP_DATA_KEY4_4_MSB    _u(15)
#define OTP_DATA_KEY4_4_LSB    _u(0)
#define OTP_DATA_KEY4_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_5
// Description : Bits 95:80 of key 4 (ECC)
#define OTP_DATA_KEY4_5_ROW _u(0xf65)
#define OTP_DATA_KEY4_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_5_RESET  "-"
#define OTP_DATA_KEY4_5_MSB    _u(15)
#define OTP_DATA_KEY4_5_LSB    _u(0)
#define OTP_DATA_KEY4_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_6
// Description : Bits 111:96 of key 4 (ECC)
#define OTP_DATA_KEY4_6_ROW _u(0xf66)
#define OTP_DATA_KEY4_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_6_RESET  "-"
#define OTP_DATA_KEY4_6_MSB    _u(15)
#define OTP_DATA_KEY4_6_LSB    _u(0)
#define OTP_DATA_KEY4_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_7
// Description : Bits 127:112 of key 4 (ECC)
#define OTP_DATA_KEY4_7_ROW _u(0xf67)
#define OTP_DATA_KEY4_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY4_7_RESET  "-"
#define OTP_DATA_KEY4_7_MSB    _u(15)
#define OTP_DATA_KEY4_7_LSB    _u(0)
#define OTP_DATA_KEY4_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_0
// Description : Bits 15:0 of key 5 (ECC)
#define OTP_DATA_KEY5_0_ROW _u(0xf68)
#define OTP_DATA_KEY5_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_0_RESET  "-"
#define OTP_DATA_KEY5_0_MSB    _u(15)
#define OTP_DATA_KEY5_0_LSB    _u(0)
#define OTP_DATA_KEY5_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_1
// Description : Bits 31:16 of key 5 (ECC)
#define OTP_DATA_KEY5_1_ROW _u(0xf69)
#define OTP_DATA_KEY5_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_1_RESET  "-"
#define OTP_DATA_KEY5_1_MSB    _u(15)
#define OTP_DATA_KEY5_1_LSB    _u(0)
#define OTP_DATA_KEY5_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_2
// Description : Bits 47:32 of key 5 (ECC)
#define OTP_DATA_KEY5_2_ROW _u(0xf6a)
#define OTP_DATA_KEY5_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_2_RESET  "-"
#define OTP_DATA_KEY5_2_MSB    _u(15)
#define OTP_DATA_KEY5_2_LSB    _u(0)
#define OTP_DATA_KEY5_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_3
// Description : Bits 63:48 of key 5 (ECC)
#define OTP_DATA_KEY5_3_ROW _u(0xf6b)
#define OTP_DATA_KEY5_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_3_RESET  "-"
#define OTP_DATA_KEY5_3_MSB    _u(15)
#define OTP_DATA_KEY5_3_LSB    _u(0)
#define OTP_DATA_KEY5_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_4
// Description : Bits 79:64 of key 5 (ECC)
#define OTP_DATA_KEY5_4_ROW _u(0xf6c)
#define OTP_DATA_KEY5_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_4_RESET  "-"
#define OTP_DATA_KEY5_4_MSB    _u(15)
#define OTP_DATA_KEY5_4_LSB    _u(0)
#define OTP_DATA_KEY5_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_5
// Description : Bits 95:80 of key 5 (ECC)
#define OTP_DATA_KEY5_5_ROW _u(0xf6d)
#define OTP_DATA_KEY5_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_5_RESET  "-"
#define OTP_DATA_KEY5_5_MSB    _u(15)
#define OTP_DATA_KEY5_5_LSB    _u(0)
#define OTP_DATA_KEY5_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_6
// Description : Bits 111:96 of key 5 (ECC)
#define OTP_DATA_KEY5_6_ROW _u(0xf6e)
#define OTP_DATA_KEY5_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_6_RESET  "-"
#define OTP_DATA_KEY5_6_MSB    _u(15)
#define OTP_DATA_KEY5_6_LSB    _u(0)
#define OTP_DATA_KEY5_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_7
// Description : Bits 127:112 of key 5 (ECC)
#define OTP_DATA_KEY5_7_ROW _u(0xf6f)
#define OTP_DATA_KEY5_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY5_7_RESET  "-"
#define OTP_DATA_KEY5_7_MSB    _u(15)
#define OTP_DATA_KEY5_7_LSB    _u(0)
#define OTP_DATA_KEY5_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_0
// Description : Bits 15:0 of key 6 (ECC)
#define OTP_DATA_KEY6_0_ROW _u(0xf70)
#define OTP_DATA_KEY6_0_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_0_RESET  "-"
#define OTP_DATA_KEY6_0_MSB    _u(15)
#define OTP_DATA_KEY6_0_LSB    _u(0)
#define OTP_DATA_KEY6_0_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_1
// Description : Bits 31:16 of key 6 (ECC)
#define OTP_DATA_KEY6_1_ROW _u(0xf71)
#define OTP_DATA_KEY6_1_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_1_RESET  "-"
#define OTP_DATA_KEY6_1_MSB    _u(15)
#define OTP_DATA_KEY6_1_LSB    _u(0)
#define OTP_DATA_KEY6_1_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_2
// Description : Bits 47:32 of key 6 (ECC)
#define OTP_DATA_KEY6_2_ROW _u(0xf72)
#define OTP_DATA_KEY6_2_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_2_RESET  "-"
#define OTP_DATA_KEY6_2_MSB    _u(15)
#define OTP_DATA_KEY6_2_LSB    _u(0)
#define OTP_DATA_KEY6_2_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_3
// Description : Bits 63:48 of key 6 (ECC)
#define OTP_DATA_KEY6_3_ROW _u(0xf73)
#define OTP_DATA_KEY6_3_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_3_RESET  "-"
#define OTP_DATA_KEY6_3_MSB    _u(15)
#define OTP_DATA_KEY6_3_LSB    _u(0)
#define OTP_DATA_KEY6_3_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_4
// Description : Bits 79:64 of key 6 (ECC)
#define OTP_DATA_KEY6_4_ROW _u(0xf74)
#define OTP_DATA_KEY6_4_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_4_RESET  "-"
#define OTP_DATA_KEY6_4_MSB    _u(15)
#define OTP_DATA_KEY6_4_LSB    _u(0)
#define OTP_DATA_KEY6_4_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_5
// Description : Bits 95:80 of key 6 (ECC)
#define OTP_DATA_KEY6_5_ROW _u(0xf75)
#define OTP_DATA_KEY6_5_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_5_RESET  "-"
#define OTP_DATA_KEY6_5_MSB    _u(15)
#define OTP_DATA_KEY6_5_LSB    _u(0)
#define OTP_DATA_KEY6_5_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_6
// Description : Bits 111:96 of key 6 (ECC)
#define OTP_DATA_KEY6_6_ROW _u(0xf76)
#define OTP_DATA_KEY6_6_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_6_RESET  "-"
#define OTP_DATA_KEY6_6_MSB    _u(15)
#define OTP_DATA_KEY6_6_LSB    _u(0)
#define OTP_DATA_KEY6_6_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_7
// Description : Bits 127:112 of key 6 (ECC)
#define OTP_DATA_KEY6_7_ROW _u(0xf77)
#define OTP_DATA_KEY6_7_BITS   _u(0x0000ffff)
#define OTP_DATA_KEY6_7_RESET  "-"
#define OTP_DATA_KEY6_7_MSB    _u(15)
#define OTP_DATA_KEY6_7_LSB    _u(0)
#define OTP_DATA_KEY6_7_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY1_VALID
// Description : Valid flag for key 1. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY1_VALID_ROW _u(0xf79)
#define OTP_DATA_KEY1_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY1_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY1_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY1_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY1_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY1_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY1_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY1_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY1_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY1_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY1_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY1_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY1_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY1_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY1_VALID_VALID
// Description : None
#define OTP_DATA_KEY1_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY1_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY1_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY1_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY1_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY2_VALID
// Description : Valid flag for key 2. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY2_VALID_ROW _u(0xf7a)
#define OTP_DATA_KEY2_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY2_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY2_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY2_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY2_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY2_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY2_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY2_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY2_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY2_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY2_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY2_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY2_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY2_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY2_VALID_VALID
// Description : None
#define OTP_DATA_KEY2_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY2_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY2_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY2_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY2_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY3_VALID
// Description : Valid flag for key 3. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY3_VALID_ROW _u(0xf7b)
#define OTP_DATA_KEY3_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY3_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY3_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY3_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY3_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY3_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY3_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY3_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY3_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY3_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY3_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY3_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY3_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY3_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY3_VALID_VALID
// Description : None
#define OTP_DATA_KEY3_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY3_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY3_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY3_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY3_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY4_VALID
// Description : Valid flag for key 4. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY4_VALID_ROW _u(0xf7c)
#define OTP_DATA_KEY4_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY4_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY4_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY4_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY4_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY4_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY4_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY4_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY4_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY4_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY4_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY4_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY4_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY4_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY4_VALID_VALID
// Description : None
#define OTP_DATA_KEY4_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY4_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY4_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY4_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY4_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY5_VALID
// Description : Valid flag for key 5. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY5_VALID_ROW _u(0xf7d)
#define OTP_DATA_KEY5_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY5_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY5_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY5_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY5_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY5_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY5_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY5_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY5_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY5_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY5_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY5_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY5_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY5_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY5_VALID_VALID
// Description : None
#define OTP_DATA_KEY5_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY5_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY5_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY5_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY5_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_KEY6_VALID
// Description : Valid flag for key 6. Once the valid flag is set, the key can
//               no longer be read or written, and becomes a valid fixed key for
//               protecting OTP pages.
#define OTP_DATA_KEY6_VALID_ROW _u(0xf7e)
#define OTP_DATA_KEY6_VALID_BITS   _u(0x00010101)
#define OTP_DATA_KEY6_VALID_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY6_VALID_VALID_R2
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY6_VALID_VALID_R2_RESET  "-"
#define OTP_DATA_KEY6_VALID_VALID_R2_BITS   _u(0x00010000)
#define OTP_DATA_KEY6_VALID_VALID_R2_MSB    _u(16)
#define OTP_DATA_KEY6_VALID_VALID_R2_LSB    _u(16)
#define OTP_DATA_KEY6_VALID_VALID_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY6_VALID_VALID_R1
// Description : Redundant copy of VALID, with 3-way majority vote
#define OTP_DATA_KEY6_VALID_VALID_R1_RESET  "-"
#define OTP_DATA_KEY6_VALID_VALID_R1_BITS   _u(0x00000100)
#define OTP_DATA_KEY6_VALID_VALID_R1_MSB    _u(8)
#define OTP_DATA_KEY6_VALID_VALID_R1_LSB    _u(8)
#define OTP_DATA_KEY6_VALID_VALID_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_KEY6_VALID_VALID
// Description : None
#define OTP_DATA_KEY6_VALID_VALID_RESET  "-"
#define OTP_DATA_KEY6_VALID_VALID_BITS   _u(0x00000001)
#define OTP_DATA_KEY6_VALID_VALID_MSB    _u(0)
#define OTP_DATA_KEY6_VALID_VALID_LSB    _u(0)
#define OTP_DATA_KEY6_VALID_VALID_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE0_LOCK0
// Description : Lock configuration LSBs for page 0 (addresses 0x0 through
//               0x7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE0_LOCK0_ROW _u(0xf80)
#define OTP_DATA_PAGE0_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE0_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE0_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE0_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE0_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE0_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE0_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE0_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE0_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE0_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE0_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE0_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE0_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE0_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE0_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE0_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE0_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE0_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE0_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE0_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE0_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE0_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE0_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE0_LOCK1
// Description : Lock configuration MSBs for page 0 (addresses 0x0 through
//               0x7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE0_LOCK1_ROW _u(0xf81)
#define OTP_DATA_PAGE0_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE0_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE0_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE0_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE0_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE0_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE0_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE0_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE0_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE0_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE0_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE0_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE0_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE0_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE0_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE0_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE1_LOCK0
// Description : Lock configuration LSBs for page 1 (addresses 0x80 through
//               0xff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE1_LOCK0_ROW _u(0xf82)
#define OTP_DATA_PAGE1_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE1_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE1_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE1_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE1_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE1_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE1_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE1_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE1_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE1_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE1_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE1_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE1_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE1_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE1_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE1_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE1_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE1_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE1_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE1_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE1_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE1_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE1_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE1_LOCK1
// Description : Lock configuration MSBs for page 1 (addresses 0x80 through
//               0xff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE1_LOCK1_ROW _u(0xf83)
#define OTP_DATA_PAGE1_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE1_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE1_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE1_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE1_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE1_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE1_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE1_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE1_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE1_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE1_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE1_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE1_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE1_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE1_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE1_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE2_LOCK0
// Description : Lock configuration LSBs for page 2 (addresses 0x100 through
//               0x17f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE2_LOCK0_ROW _u(0xf84)
#define OTP_DATA_PAGE2_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE2_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE2_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE2_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE2_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE2_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE2_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE2_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE2_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE2_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE2_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE2_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE2_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE2_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE2_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE2_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE2_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE2_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE2_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE2_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE2_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE2_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE2_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE2_LOCK1
// Description : Lock configuration MSBs for page 2 (addresses 0x100 through
//               0x17f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE2_LOCK1_ROW _u(0xf85)
#define OTP_DATA_PAGE2_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE2_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE2_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE2_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE2_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE2_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE2_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE2_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE2_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE2_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE2_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE2_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE2_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE2_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE2_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE2_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE3_LOCK0
// Description : Lock configuration LSBs for page 3 (addresses 0x180 through
//               0x1ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE3_LOCK0_ROW _u(0xf86)
#define OTP_DATA_PAGE3_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE3_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE3_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE3_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE3_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE3_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE3_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE3_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE3_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE3_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE3_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE3_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE3_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE3_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE3_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE3_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE3_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE3_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE3_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE3_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE3_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE3_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE3_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE3_LOCK1
// Description : Lock configuration MSBs for page 3 (addresses 0x180 through
//               0x1ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE3_LOCK1_ROW _u(0xf87)
#define OTP_DATA_PAGE3_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE3_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE3_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE3_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE3_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE3_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE3_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE3_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE3_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE3_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE3_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE3_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE3_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE3_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE3_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE3_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE4_LOCK0
// Description : Lock configuration LSBs for page 4 (addresses 0x200 through
//               0x27f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE4_LOCK0_ROW _u(0xf88)
#define OTP_DATA_PAGE4_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE4_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE4_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE4_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE4_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE4_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE4_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE4_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE4_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE4_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE4_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE4_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE4_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE4_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE4_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE4_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE4_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE4_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE4_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE4_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE4_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE4_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE4_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE4_LOCK1
// Description : Lock configuration MSBs for page 4 (addresses 0x200 through
//               0x27f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE4_LOCK1_ROW _u(0xf89)
#define OTP_DATA_PAGE4_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE4_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE4_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE4_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE4_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE4_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE4_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE4_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE4_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE4_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE4_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE4_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE4_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE4_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE4_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE4_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE5_LOCK0
// Description : Lock configuration LSBs for page 5 (addresses 0x280 through
//               0x2ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE5_LOCK0_ROW _u(0xf8a)
#define OTP_DATA_PAGE5_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE5_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE5_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE5_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE5_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE5_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE5_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE5_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE5_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE5_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE5_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE5_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE5_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE5_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE5_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE5_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE5_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE5_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE5_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE5_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE5_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE5_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE5_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE5_LOCK1
// Description : Lock configuration MSBs for page 5 (addresses 0x280 through
//               0x2ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE5_LOCK1_ROW _u(0xf8b)
#define OTP_DATA_PAGE5_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE5_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE5_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE5_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE5_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE5_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE5_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE5_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE5_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE5_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE5_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE5_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE5_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE5_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE5_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE5_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE6_LOCK0
// Description : Lock configuration LSBs for page 6 (addresses 0x300 through
//               0x37f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE6_LOCK0_ROW _u(0xf8c)
#define OTP_DATA_PAGE6_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE6_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE6_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE6_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE6_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE6_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE6_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE6_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE6_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE6_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE6_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE6_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE6_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE6_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE6_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE6_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE6_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE6_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE6_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE6_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE6_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE6_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE6_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE6_LOCK1
// Description : Lock configuration MSBs for page 6 (addresses 0x300 through
//               0x37f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE6_LOCK1_ROW _u(0xf8d)
#define OTP_DATA_PAGE6_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE6_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE6_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE6_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE6_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE6_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE6_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE6_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE6_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE6_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE6_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE6_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE6_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE6_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE6_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE6_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE7_LOCK0
// Description : Lock configuration LSBs for page 7 (addresses 0x380 through
//               0x3ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE7_LOCK0_ROW _u(0xf8e)
#define OTP_DATA_PAGE7_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE7_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE7_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE7_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE7_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE7_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE7_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE7_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE7_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE7_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE7_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE7_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE7_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE7_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE7_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE7_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE7_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE7_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE7_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE7_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE7_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE7_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE7_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE7_LOCK1
// Description : Lock configuration MSBs for page 7 (addresses 0x380 through
//               0x3ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE7_LOCK1_ROW _u(0xf8f)
#define OTP_DATA_PAGE7_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE7_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE7_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE7_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE7_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE7_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE7_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE7_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE7_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE7_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE7_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE7_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE7_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE7_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE7_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE7_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE8_LOCK0
// Description : Lock configuration LSBs for page 8 (addresses 0x400 through
//               0x47f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE8_LOCK0_ROW _u(0xf90)
#define OTP_DATA_PAGE8_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE8_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE8_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE8_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE8_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE8_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE8_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE8_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE8_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE8_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE8_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE8_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE8_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE8_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE8_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE8_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE8_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE8_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE8_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE8_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE8_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE8_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE8_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE8_LOCK1
// Description : Lock configuration MSBs for page 8 (addresses 0x400 through
//               0x47f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE8_LOCK1_ROW _u(0xf91)
#define OTP_DATA_PAGE8_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE8_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE8_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE8_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE8_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE8_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE8_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE8_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE8_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE8_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE8_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE8_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE8_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE8_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE8_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE8_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE9_LOCK0
// Description : Lock configuration LSBs for page 9 (addresses 0x480 through
//               0x4ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE9_LOCK0_ROW _u(0xf92)
#define OTP_DATA_PAGE9_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE9_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE9_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE9_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE9_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE9_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE9_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE9_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE9_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE9_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE9_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE9_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE9_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE9_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE9_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE9_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE9_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE9_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE9_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE9_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE9_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE9_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE9_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE9_LOCK1
// Description : Lock configuration MSBs for page 9 (addresses 0x480 through
//               0x4ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE9_LOCK1_ROW _u(0xf93)
#define OTP_DATA_PAGE9_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE9_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE9_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE9_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE9_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE9_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE9_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE9_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE9_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE9_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE9_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE9_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE9_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE9_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE9_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE9_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE10_LOCK0
// Description : Lock configuration LSBs for page 10 (addresses 0x500 through
//               0x57f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE10_LOCK0_ROW _u(0xf94)
#define OTP_DATA_PAGE10_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE10_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE10_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE10_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE10_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE10_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE10_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE10_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE10_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE10_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE10_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE10_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE10_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE10_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE10_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE10_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE10_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE10_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE10_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE10_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE10_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE10_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE10_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE10_LOCK1
// Description : Lock configuration MSBs for page 10 (addresses 0x500 through
//               0x57f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE10_LOCK1_ROW _u(0xf95)
#define OTP_DATA_PAGE10_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE10_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE10_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE10_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE10_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE10_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE10_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE10_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE10_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE10_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE10_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE10_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE10_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE10_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE10_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE10_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE11_LOCK0
// Description : Lock configuration LSBs for page 11 (addresses 0x580 through
//               0x5ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE11_LOCK0_ROW _u(0xf96)
#define OTP_DATA_PAGE11_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE11_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE11_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE11_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE11_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE11_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE11_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE11_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE11_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE11_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE11_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE11_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE11_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE11_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE11_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE11_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE11_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE11_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE11_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE11_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE11_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE11_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE11_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE11_LOCK1
// Description : Lock configuration MSBs for page 11 (addresses 0x580 through
//               0x5ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE11_LOCK1_ROW _u(0xf97)
#define OTP_DATA_PAGE11_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE11_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE11_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE11_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE11_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE11_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE11_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE11_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE11_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE11_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE11_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE11_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE11_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE11_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE11_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE11_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE12_LOCK0
// Description : Lock configuration LSBs for page 12 (addresses 0x600 through
//               0x67f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE12_LOCK0_ROW _u(0xf98)
#define OTP_DATA_PAGE12_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE12_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE12_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE12_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE12_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE12_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE12_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE12_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE12_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE12_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE12_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE12_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE12_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE12_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE12_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE12_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE12_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE12_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE12_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE12_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE12_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE12_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE12_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE12_LOCK1
// Description : Lock configuration MSBs for page 12 (addresses 0x600 through
//               0x67f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE12_LOCK1_ROW _u(0xf99)
#define OTP_DATA_PAGE12_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE12_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE12_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE12_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE12_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE12_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE12_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE12_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE12_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE12_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE12_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE12_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE12_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE12_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE12_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE12_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE13_LOCK0
// Description : Lock configuration LSBs for page 13 (addresses 0x680 through
//               0x6ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE13_LOCK0_ROW _u(0xf9a)
#define OTP_DATA_PAGE13_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE13_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE13_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE13_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE13_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE13_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE13_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE13_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE13_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE13_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE13_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE13_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE13_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE13_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE13_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE13_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE13_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE13_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE13_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE13_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE13_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE13_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE13_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE13_LOCK1
// Description : Lock configuration MSBs for page 13 (addresses 0x680 through
//               0x6ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE13_LOCK1_ROW _u(0xf9b)
#define OTP_DATA_PAGE13_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE13_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE13_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE13_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE13_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE13_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE13_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE13_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE13_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE13_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE13_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE13_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE13_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE13_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE13_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE13_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE14_LOCK0
// Description : Lock configuration LSBs for page 14 (addresses 0x700 through
//               0x77f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE14_LOCK0_ROW _u(0xf9c)
#define OTP_DATA_PAGE14_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE14_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE14_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE14_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE14_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE14_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE14_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE14_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE14_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE14_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE14_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE14_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE14_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE14_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE14_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE14_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE14_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE14_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE14_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE14_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE14_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE14_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE14_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE14_LOCK1
// Description : Lock configuration MSBs for page 14 (addresses 0x700 through
//               0x77f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE14_LOCK1_ROW _u(0xf9d)
#define OTP_DATA_PAGE14_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE14_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE14_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE14_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE14_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE14_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE14_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE14_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE14_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE14_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE14_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE14_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE14_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE14_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE14_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE14_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE15_LOCK0
// Description : Lock configuration LSBs for page 15 (addresses 0x780 through
//               0x7ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE15_LOCK0_ROW _u(0xf9e)
#define OTP_DATA_PAGE15_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE15_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE15_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE15_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE15_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE15_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE15_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE15_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE15_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE15_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE15_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE15_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE15_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE15_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE15_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE15_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE15_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE15_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE15_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE15_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE15_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE15_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE15_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE15_LOCK1
// Description : Lock configuration MSBs for page 15 (addresses 0x780 through
//               0x7ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE15_LOCK1_ROW _u(0xf9f)
#define OTP_DATA_PAGE15_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE15_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE15_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE15_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE15_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE15_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE15_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE15_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE15_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE15_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE15_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE15_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE15_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE15_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE15_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE15_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE16_LOCK0
// Description : Lock configuration LSBs for page 16 (addresses 0x800 through
//               0x87f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE16_LOCK0_ROW _u(0xfa0)
#define OTP_DATA_PAGE16_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE16_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE16_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE16_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE16_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE16_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE16_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE16_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE16_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE16_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE16_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE16_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE16_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE16_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE16_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE16_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE16_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE16_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE16_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE16_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE16_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE16_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE16_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE16_LOCK1
// Description : Lock configuration MSBs for page 16 (addresses 0x800 through
//               0x87f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE16_LOCK1_ROW _u(0xfa1)
#define OTP_DATA_PAGE16_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE16_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE16_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE16_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE16_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE16_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE16_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE16_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE16_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE16_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE16_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE16_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE16_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE16_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE16_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE16_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE17_LOCK0
// Description : Lock configuration LSBs for page 17 (addresses 0x880 through
//               0x8ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE17_LOCK0_ROW _u(0xfa2)
#define OTP_DATA_PAGE17_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE17_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE17_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE17_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE17_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE17_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE17_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE17_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE17_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE17_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE17_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE17_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE17_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE17_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE17_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE17_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE17_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE17_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE17_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE17_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE17_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE17_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE17_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE17_LOCK1
// Description : Lock configuration MSBs for page 17 (addresses 0x880 through
//               0x8ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE17_LOCK1_ROW _u(0xfa3)
#define OTP_DATA_PAGE17_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE17_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE17_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE17_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE17_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE17_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE17_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE17_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE17_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE17_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE17_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE17_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE17_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE17_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE17_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE17_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE18_LOCK0
// Description : Lock configuration LSBs for page 18 (addresses 0x900 through
//               0x97f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE18_LOCK0_ROW _u(0xfa4)
#define OTP_DATA_PAGE18_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE18_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE18_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE18_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE18_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE18_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE18_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE18_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE18_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE18_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE18_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE18_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE18_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE18_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE18_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE18_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE18_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE18_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE18_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE18_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE18_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE18_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE18_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE18_LOCK1
// Description : Lock configuration MSBs for page 18 (addresses 0x900 through
//               0x97f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE18_LOCK1_ROW _u(0xfa5)
#define OTP_DATA_PAGE18_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE18_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE18_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE18_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE18_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE18_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE18_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE18_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE18_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE18_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE18_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE18_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE18_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE18_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE18_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE18_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE19_LOCK0
// Description : Lock configuration LSBs for page 19 (addresses 0x980 through
//               0x9ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE19_LOCK0_ROW _u(0xfa6)
#define OTP_DATA_PAGE19_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE19_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE19_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE19_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE19_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE19_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE19_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE19_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE19_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE19_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE19_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE19_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE19_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE19_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE19_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE19_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE19_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE19_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE19_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE19_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE19_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE19_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE19_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE19_LOCK1
// Description : Lock configuration MSBs for page 19 (addresses 0x980 through
//               0x9ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE19_LOCK1_ROW _u(0xfa7)
#define OTP_DATA_PAGE19_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE19_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE19_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE19_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE19_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE19_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE19_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE19_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE19_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE19_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE19_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE19_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE19_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE19_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE19_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE19_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE20_LOCK0
// Description : Lock configuration LSBs for page 20 (addresses 0xa00 through
//               0xa7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE20_LOCK0_ROW _u(0xfa8)
#define OTP_DATA_PAGE20_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE20_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE20_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE20_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE20_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE20_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE20_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE20_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE20_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE20_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE20_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE20_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE20_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE20_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE20_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE20_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE20_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE20_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE20_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE20_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE20_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE20_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE20_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE20_LOCK1
// Description : Lock configuration MSBs for page 20 (addresses 0xa00 through
//               0xa7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE20_LOCK1_ROW _u(0xfa9)
#define OTP_DATA_PAGE20_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE20_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE20_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE20_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE20_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE20_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE20_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE20_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE20_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE20_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE20_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE20_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE20_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE20_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE20_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE20_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE21_LOCK0
// Description : Lock configuration LSBs for page 21 (addresses 0xa80 through
//               0xaff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE21_LOCK0_ROW _u(0xfaa)
#define OTP_DATA_PAGE21_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE21_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE21_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE21_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE21_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE21_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE21_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE21_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE21_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE21_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE21_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE21_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE21_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE21_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE21_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE21_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE21_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE21_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE21_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE21_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE21_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE21_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE21_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE21_LOCK1
// Description : Lock configuration MSBs for page 21 (addresses 0xa80 through
//               0xaff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE21_LOCK1_ROW _u(0xfab)
#define OTP_DATA_PAGE21_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE21_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE21_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE21_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE21_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE21_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE21_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE21_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE21_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE21_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE21_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE21_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE21_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE21_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE21_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE21_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE22_LOCK0
// Description : Lock configuration LSBs for page 22 (addresses 0xb00 through
//               0xb7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE22_LOCK0_ROW _u(0xfac)
#define OTP_DATA_PAGE22_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE22_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE22_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE22_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE22_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE22_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE22_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE22_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE22_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE22_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE22_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE22_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE22_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE22_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE22_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE22_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE22_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE22_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE22_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE22_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE22_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE22_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE22_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE22_LOCK1
// Description : Lock configuration MSBs for page 22 (addresses 0xb00 through
//               0xb7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE22_LOCK1_ROW _u(0xfad)
#define OTP_DATA_PAGE22_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE22_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE22_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE22_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE22_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE22_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE22_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE22_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE22_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE22_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE22_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE22_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE22_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE22_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE22_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE22_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE23_LOCK0
// Description : Lock configuration LSBs for page 23 (addresses 0xb80 through
//               0xbff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE23_LOCK0_ROW _u(0xfae)
#define OTP_DATA_PAGE23_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE23_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE23_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE23_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE23_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE23_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE23_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE23_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE23_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE23_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE23_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE23_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE23_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE23_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE23_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE23_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE23_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE23_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE23_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE23_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE23_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE23_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE23_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE23_LOCK1
// Description : Lock configuration MSBs for page 23 (addresses 0xb80 through
//               0xbff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE23_LOCK1_ROW _u(0xfaf)
#define OTP_DATA_PAGE23_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE23_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE23_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE23_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE23_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE23_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE23_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE23_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE23_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE23_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE23_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE23_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE23_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE23_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE23_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE23_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE24_LOCK0
// Description : Lock configuration LSBs for page 24 (addresses 0xc00 through
//               0xc7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE24_LOCK0_ROW _u(0xfb0)
#define OTP_DATA_PAGE24_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE24_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE24_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE24_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE24_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE24_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE24_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE24_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE24_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE24_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE24_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE24_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE24_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE24_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE24_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE24_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE24_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE24_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE24_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE24_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE24_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE24_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE24_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE24_LOCK1
// Description : Lock configuration MSBs for page 24 (addresses 0xc00 through
//               0xc7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE24_LOCK1_ROW _u(0xfb1)
#define OTP_DATA_PAGE24_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE24_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE24_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE24_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE24_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE24_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE24_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE24_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE24_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE24_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE24_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE24_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE24_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE24_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE24_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE24_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE25_LOCK0
// Description : Lock configuration LSBs for page 25 (addresses 0xc80 through
//               0xcff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE25_LOCK0_ROW _u(0xfb2)
#define OTP_DATA_PAGE25_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE25_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE25_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE25_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE25_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE25_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE25_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE25_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE25_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE25_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE25_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE25_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE25_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE25_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE25_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE25_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE25_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE25_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE25_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE25_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE25_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE25_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE25_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE25_LOCK1
// Description : Lock configuration MSBs for page 25 (addresses 0xc80 through
//               0xcff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE25_LOCK1_ROW _u(0xfb3)
#define OTP_DATA_PAGE25_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE25_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE25_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE25_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE25_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE25_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE25_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE25_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE25_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE25_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE25_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE25_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE25_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE25_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE25_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE25_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE26_LOCK0
// Description : Lock configuration LSBs for page 26 (addresses 0xd00 through
//               0xd7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE26_LOCK0_ROW _u(0xfb4)
#define OTP_DATA_PAGE26_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE26_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE26_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE26_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE26_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE26_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE26_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE26_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE26_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE26_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE26_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE26_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE26_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE26_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE26_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE26_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE26_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE26_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE26_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE26_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE26_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE26_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE26_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE26_LOCK1
// Description : Lock configuration MSBs for page 26 (addresses 0xd00 through
//               0xd7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE26_LOCK1_ROW _u(0xfb5)
#define OTP_DATA_PAGE26_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE26_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE26_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE26_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE26_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE26_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE26_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE26_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE26_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE26_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE26_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE26_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE26_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE26_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE26_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE26_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE27_LOCK0
// Description : Lock configuration LSBs for page 27 (addresses 0xd80 through
//               0xdff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE27_LOCK0_ROW _u(0xfb6)
#define OTP_DATA_PAGE27_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE27_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE27_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE27_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE27_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE27_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE27_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE27_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE27_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE27_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE27_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE27_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE27_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE27_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE27_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE27_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE27_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE27_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE27_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE27_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE27_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE27_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE27_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE27_LOCK1
// Description : Lock configuration MSBs for page 27 (addresses 0xd80 through
//               0xdff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE27_LOCK1_ROW _u(0xfb7)
#define OTP_DATA_PAGE27_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE27_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE27_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE27_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE27_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE27_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE27_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE27_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE27_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE27_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE27_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE27_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE27_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE27_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE27_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE27_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE28_LOCK0
// Description : Lock configuration LSBs for page 28 (addresses 0xe00 through
//               0xe7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE28_LOCK0_ROW _u(0xfb8)
#define OTP_DATA_PAGE28_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE28_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE28_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE28_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE28_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE28_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE28_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE28_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE28_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE28_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE28_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE28_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE28_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE28_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE28_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE28_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE28_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE28_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE28_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE28_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE28_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE28_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE28_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE28_LOCK1
// Description : Lock configuration MSBs for page 28 (addresses 0xe00 through
//               0xe7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE28_LOCK1_ROW _u(0xfb9)
#define OTP_DATA_PAGE28_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE28_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE28_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE28_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE28_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE28_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE28_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE28_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE28_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE28_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE28_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE28_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE28_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE28_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE28_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE28_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE29_LOCK0
// Description : Lock configuration LSBs for page 29 (addresses 0xe80 through
//               0xeff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE29_LOCK0_ROW _u(0xfba)
#define OTP_DATA_PAGE29_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE29_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE29_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE29_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE29_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE29_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE29_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE29_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE29_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE29_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE29_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE29_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE29_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE29_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE29_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE29_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE29_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE29_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE29_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE29_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE29_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE29_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE29_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE29_LOCK1
// Description : Lock configuration MSBs for page 29 (addresses 0xe80 through
//               0xeff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE29_LOCK1_ROW _u(0xfbb)
#define OTP_DATA_PAGE29_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE29_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE29_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE29_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE29_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE29_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE29_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE29_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE29_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE29_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE29_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE29_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE29_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE29_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE29_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE29_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE30_LOCK0
// Description : Lock configuration LSBs for page 30 (addresses 0xf00 through
//               0xf7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE30_LOCK0_ROW _u(0xfbc)
#define OTP_DATA_PAGE30_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE30_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE30_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE30_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE30_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE30_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE30_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE30_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE30_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE30_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE30_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE30_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE30_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE30_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE30_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE30_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE30_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE30_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE30_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE30_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE30_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE30_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE30_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE30_LOCK1
// Description : Lock configuration MSBs for page 30 (addresses 0xf00 through
//               0xf7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE30_LOCK1_ROW _u(0xfbd)
#define OTP_DATA_PAGE30_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE30_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE30_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE30_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE30_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE30_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE30_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE30_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE30_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE30_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE30_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE30_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE30_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE30_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE30_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE30_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE31_LOCK0
// Description : Lock configuration LSBs for page 31 (addresses 0xf80 through
//               0xfff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE31_LOCK0_ROW _u(0xfbe)
#define OTP_DATA_PAGE31_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE31_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE31_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE31_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE31_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE31_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE31_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE31_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE31_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE31_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE31_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE31_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE31_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE31_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE31_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE31_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE31_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE31_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE31_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE31_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE31_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE31_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE31_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE31_LOCK1
// Description : Lock configuration MSBs for page 31 (addresses 0xf80 through
//               0xfff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE31_LOCK1_ROW _u(0xfbf)
#define OTP_DATA_PAGE31_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE31_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE31_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE31_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE31_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE31_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE31_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE31_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE31_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE31_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE31_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE31_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE31_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE31_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE31_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE31_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE32_LOCK0
// Description : Lock configuration LSBs for page 32 (addresses 0x1000 through
//               0x107f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE32_LOCK0_ROW _u(0xfc0)
#define OTP_DATA_PAGE32_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE32_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE32_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE32_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE32_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE32_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE32_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE32_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE32_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE32_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE32_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE32_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE32_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE32_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE32_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE32_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE32_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE32_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE32_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE32_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE32_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE32_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE32_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE32_LOCK1
// Description : Lock configuration MSBs for page 32 (addresses 0x1000 through
//               0x107f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE32_LOCK1_ROW _u(0xfc1)
#define OTP_DATA_PAGE32_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE32_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE32_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE32_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE32_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE32_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE32_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE32_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE32_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE32_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE32_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE32_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE32_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE32_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE32_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE32_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE33_LOCK0
// Description : Lock configuration LSBs for page 33 (addresses 0x1080 through
//               0x10ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE33_LOCK0_ROW _u(0xfc2)
#define OTP_DATA_PAGE33_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE33_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE33_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE33_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE33_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE33_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE33_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE33_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE33_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE33_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE33_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE33_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE33_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE33_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE33_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE33_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE33_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE33_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE33_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE33_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE33_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE33_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE33_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE33_LOCK1
// Description : Lock configuration MSBs for page 33 (addresses 0x1080 through
//               0x10ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE33_LOCK1_ROW _u(0xfc3)
#define OTP_DATA_PAGE33_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE33_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE33_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE33_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE33_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE33_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE33_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE33_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE33_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE33_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE33_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE33_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE33_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE33_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE33_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE33_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE34_LOCK0
// Description : Lock configuration LSBs for page 34 (addresses 0x1100 through
//               0x117f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE34_LOCK0_ROW _u(0xfc4)
#define OTP_DATA_PAGE34_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE34_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE34_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE34_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE34_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE34_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE34_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE34_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE34_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE34_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE34_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE34_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE34_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE34_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE34_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE34_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE34_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE34_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE34_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE34_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE34_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE34_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE34_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE34_LOCK1
// Description : Lock configuration MSBs for page 34 (addresses 0x1100 through
//               0x117f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE34_LOCK1_ROW _u(0xfc5)
#define OTP_DATA_PAGE34_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE34_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE34_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE34_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE34_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE34_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE34_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE34_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE34_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE34_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE34_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE34_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE34_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE34_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE34_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE34_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE35_LOCK0
// Description : Lock configuration LSBs for page 35 (addresses 0x1180 through
//               0x11ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE35_LOCK0_ROW _u(0xfc6)
#define OTP_DATA_PAGE35_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE35_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE35_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE35_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE35_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE35_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE35_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE35_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE35_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE35_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE35_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE35_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE35_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE35_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE35_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE35_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE35_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE35_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE35_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE35_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE35_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE35_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE35_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE35_LOCK1
// Description : Lock configuration MSBs for page 35 (addresses 0x1180 through
//               0x11ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE35_LOCK1_ROW _u(0xfc7)
#define OTP_DATA_PAGE35_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE35_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE35_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE35_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE35_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE35_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE35_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE35_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE35_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE35_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE35_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE35_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE35_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE35_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE35_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE35_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE36_LOCK0
// Description : Lock configuration LSBs for page 36 (addresses 0x1200 through
//               0x127f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE36_LOCK0_ROW _u(0xfc8)
#define OTP_DATA_PAGE36_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE36_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE36_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE36_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE36_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE36_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE36_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE36_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE36_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE36_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE36_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE36_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE36_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE36_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE36_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE36_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE36_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE36_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE36_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE36_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE36_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE36_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE36_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE36_LOCK1
// Description : Lock configuration MSBs for page 36 (addresses 0x1200 through
//               0x127f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE36_LOCK1_ROW _u(0xfc9)
#define OTP_DATA_PAGE36_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE36_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE36_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE36_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE36_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE36_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE36_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE36_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE36_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE36_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE36_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE36_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE36_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE36_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE36_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE36_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE37_LOCK0
// Description : Lock configuration LSBs for page 37 (addresses 0x1280 through
//               0x12ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE37_LOCK0_ROW _u(0xfca)
#define OTP_DATA_PAGE37_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE37_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE37_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE37_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE37_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE37_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE37_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE37_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE37_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE37_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE37_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE37_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE37_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE37_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE37_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE37_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE37_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE37_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE37_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE37_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE37_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE37_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE37_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE37_LOCK1
// Description : Lock configuration MSBs for page 37 (addresses 0x1280 through
//               0x12ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE37_LOCK1_ROW _u(0xfcb)
#define OTP_DATA_PAGE37_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE37_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE37_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE37_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE37_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE37_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE37_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE37_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE37_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE37_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE37_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE37_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE37_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE37_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE37_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE37_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE38_LOCK0
// Description : Lock configuration LSBs for page 38 (addresses 0x1300 through
//               0x137f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE38_LOCK0_ROW _u(0xfcc)
#define OTP_DATA_PAGE38_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE38_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE38_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE38_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE38_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE38_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE38_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE38_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE38_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE38_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE38_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE38_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE38_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE38_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE38_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE38_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE38_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE38_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE38_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE38_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE38_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE38_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE38_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE38_LOCK1
// Description : Lock configuration MSBs for page 38 (addresses 0x1300 through
//               0x137f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE38_LOCK1_ROW _u(0xfcd)
#define OTP_DATA_PAGE38_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE38_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE38_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE38_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE38_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE38_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE38_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE38_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE38_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE38_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE38_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE38_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE38_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE38_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE38_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE38_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE39_LOCK0
// Description : Lock configuration LSBs for page 39 (addresses 0x1380 through
//               0x13ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE39_LOCK0_ROW _u(0xfce)
#define OTP_DATA_PAGE39_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE39_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE39_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE39_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE39_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE39_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE39_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE39_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE39_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE39_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE39_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE39_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE39_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE39_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE39_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE39_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE39_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE39_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE39_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE39_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE39_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE39_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE39_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE39_LOCK1
// Description : Lock configuration MSBs for page 39 (addresses 0x1380 through
//               0x13ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE39_LOCK1_ROW _u(0xfcf)
#define OTP_DATA_PAGE39_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE39_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE39_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE39_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE39_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE39_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE39_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE39_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE39_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE39_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE39_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE39_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE39_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE39_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE39_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE39_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE40_LOCK0
// Description : Lock configuration LSBs for page 40 (addresses 0x1400 through
//               0x147f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE40_LOCK0_ROW _u(0xfd0)
#define OTP_DATA_PAGE40_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE40_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE40_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE40_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE40_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE40_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE40_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE40_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE40_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE40_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE40_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE40_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE40_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE40_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE40_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE40_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE40_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE40_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE40_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE40_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE40_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE40_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE40_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE40_LOCK1
// Description : Lock configuration MSBs for page 40 (addresses 0x1400 through
//               0x147f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE40_LOCK1_ROW _u(0xfd1)
#define OTP_DATA_PAGE40_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE40_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE40_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE40_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE40_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE40_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE40_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE40_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE40_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE40_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE40_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE40_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE40_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE40_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE40_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE40_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE41_LOCK0
// Description : Lock configuration LSBs for page 41 (addresses 0x1480 through
//               0x14ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE41_LOCK0_ROW _u(0xfd2)
#define OTP_DATA_PAGE41_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE41_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE41_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE41_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE41_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE41_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE41_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE41_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE41_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE41_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE41_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE41_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE41_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE41_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE41_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE41_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE41_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE41_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE41_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE41_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE41_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE41_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE41_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE41_LOCK1
// Description : Lock configuration MSBs for page 41 (addresses 0x1480 through
//               0x14ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE41_LOCK1_ROW _u(0xfd3)
#define OTP_DATA_PAGE41_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE41_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE41_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE41_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE41_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE41_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE41_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE41_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE41_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE41_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE41_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE41_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE41_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE41_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE41_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE41_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE42_LOCK0
// Description : Lock configuration LSBs for page 42 (addresses 0x1500 through
//               0x157f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE42_LOCK0_ROW _u(0xfd4)
#define OTP_DATA_PAGE42_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE42_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE42_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE42_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE42_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE42_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE42_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE42_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE42_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE42_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE42_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE42_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE42_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE42_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE42_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE42_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE42_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE42_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE42_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE42_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE42_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE42_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE42_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE42_LOCK1
// Description : Lock configuration MSBs for page 42 (addresses 0x1500 through
//               0x157f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE42_LOCK1_ROW _u(0xfd5)
#define OTP_DATA_PAGE42_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE42_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE42_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE42_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE42_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE42_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE42_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE42_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE42_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE42_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE42_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE42_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE42_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE42_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE42_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE42_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE43_LOCK0
// Description : Lock configuration LSBs for page 43 (addresses 0x1580 through
//               0x15ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE43_LOCK0_ROW _u(0xfd6)
#define OTP_DATA_PAGE43_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE43_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE43_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE43_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE43_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE43_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE43_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE43_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE43_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE43_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE43_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE43_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE43_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE43_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE43_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE43_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE43_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE43_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE43_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE43_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE43_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE43_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE43_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE43_LOCK1
// Description : Lock configuration MSBs for page 43 (addresses 0x1580 through
//               0x15ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE43_LOCK1_ROW _u(0xfd7)
#define OTP_DATA_PAGE43_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE43_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE43_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE43_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE43_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE43_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE43_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE43_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE43_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE43_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE43_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE43_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE43_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE43_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE43_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE43_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE44_LOCK0
// Description : Lock configuration LSBs for page 44 (addresses 0x1600 through
//               0x167f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE44_LOCK0_ROW _u(0xfd8)
#define OTP_DATA_PAGE44_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE44_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE44_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE44_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE44_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE44_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE44_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE44_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE44_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE44_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE44_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE44_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE44_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE44_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE44_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE44_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE44_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE44_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE44_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE44_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE44_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE44_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE44_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE44_LOCK1
// Description : Lock configuration MSBs for page 44 (addresses 0x1600 through
//               0x167f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE44_LOCK1_ROW _u(0xfd9)
#define OTP_DATA_PAGE44_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE44_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE44_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE44_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE44_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE44_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE44_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE44_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE44_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE44_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE44_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE44_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE44_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE44_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE44_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE44_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE45_LOCK0
// Description : Lock configuration LSBs for page 45 (addresses 0x1680 through
//               0x16ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE45_LOCK0_ROW _u(0xfda)
#define OTP_DATA_PAGE45_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE45_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE45_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE45_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE45_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE45_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE45_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE45_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE45_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE45_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE45_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE45_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE45_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE45_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE45_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE45_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE45_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE45_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE45_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE45_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE45_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE45_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE45_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE45_LOCK1
// Description : Lock configuration MSBs for page 45 (addresses 0x1680 through
//               0x16ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE45_LOCK1_ROW _u(0xfdb)
#define OTP_DATA_PAGE45_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE45_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE45_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE45_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE45_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE45_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE45_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE45_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE45_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE45_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE45_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE45_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE45_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE45_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE45_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE45_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE46_LOCK0
// Description : Lock configuration LSBs for page 46 (addresses 0x1700 through
//               0x177f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE46_LOCK0_ROW _u(0xfdc)
#define OTP_DATA_PAGE46_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE46_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE46_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE46_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE46_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE46_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE46_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE46_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE46_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE46_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE46_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE46_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE46_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE46_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE46_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE46_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE46_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE46_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE46_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE46_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE46_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE46_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE46_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE46_LOCK1
// Description : Lock configuration MSBs for page 46 (addresses 0x1700 through
//               0x177f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE46_LOCK1_ROW _u(0xfdd)
#define OTP_DATA_PAGE46_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE46_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE46_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE46_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE46_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE46_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE46_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE46_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE46_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE46_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE46_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE46_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE46_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE46_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE46_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE46_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE47_LOCK0
// Description : Lock configuration LSBs for page 47 (addresses 0x1780 through
//               0x17ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE47_LOCK0_ROW _u(0xfde)
#define OTP_DATA_PAGE47_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE47_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE47_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE47_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE47_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE47_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE47_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE47_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE47_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE47_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE47_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE47_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE47_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE47_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE47_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE47_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE47_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE47_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE47_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE47_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE47_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE47_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE47_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE47_LOCK1
// Description : Lock configuration MSBs for page 47 (addresses 0x1780 through
//               0x17ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE47_LOCK1_ROW _u(0xfdf)
#define OTP_DATA_PAGE47_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE47_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE47_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE47_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE47_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE47_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE47_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE47_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE47_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE47_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE47_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE47_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE47_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE47_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE47_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE47_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE48_LOCK0
// Description : Lock configuration LSBs for page 48 (addresses 0x1800 through
//               0x187f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE48_LOCK0_ROW _u(0xfe0)
#define OTP_DATA_PAGE48_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE48_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE48_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE48_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE48_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE48_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE48_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE48_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE48_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE48_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE48_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE48_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE48_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE48_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE48_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE48_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE48_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE48_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE48_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE48_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE48_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE48_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE48_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE48_LOCK1
// Description : Lock configuration MSBs for page 48 (addresses 0x1800 through
//               0x187f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE48_LOCK1_ROW _u(0xfe1)
#define OTP_DATA_PAGE48_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE48_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE48_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE48_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE48_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE48_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE48_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE48_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE48_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE48_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE48_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE48_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE48_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE48_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE48_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE48_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE49_LOCK0
// Description : Lock configuration LSBs for page 49 (addresses 0x1880 through
//               0x18ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE49_LOCK0_ROW _u(0xfe2)
#define OTP_DATA_PAGE49_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE49_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE49_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE49_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE49_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE49_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE49_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE49_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE49_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE49_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE49_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE49_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE49_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE49_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE49_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE49_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE49_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE49_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE49_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE49_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE49_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE49_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE49_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE49_LOCK1
// Description : Lock configuration MSBs for page 49 (addresses 0x1880 through
//               0x18ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE49_LOCK1_ROW _u(0xfe3)
#define OTP_DATA_PAGE49_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE49_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE49_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE49_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE49_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE49_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE49_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE49_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE49_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE49_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE49_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE49_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE49_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE49_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE49_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE49_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE50_LOCK0
// Description : Lock configuration LSBs for page 50 (addresses 0x1900 through
//               0x197f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE50_LOCK0_ROW _u(0xfe4)
#define OTP_DATA_PAGE50_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE50_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE50_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE50_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE50_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE50_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE50_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE50_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE50_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE50_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE50_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE50_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE50_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE50_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE50_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE50_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE50_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE50_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE50_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE50_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE50_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE50_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE50_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE50_LOCK1
// Description : Lock configuration MSBs for page 50 (addresses 0x1900 through
//               0x197f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE50_LOCK1_ROW _u(0xfe5)
#define OTP_DATA_PAGE50_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE50_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE50_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE50_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE50_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE50_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE50_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE50_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE50_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE50_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE50_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE50_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE50_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE50_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE50_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE50_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE51_LOCK0
// Description : Lock configuration LSBs for page 51 (addresses 0x1980 through
//               0x19ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE51_LOCK0_ROW _u(0xfe6)
#define OTP_DATA_PAGE51_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE51_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE51_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE51_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE51_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE51_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE51_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE51_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE51_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE51_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE51_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE51_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE51_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE51_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE51_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE51_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE51_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE51_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE51_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE51_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE51_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE51_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE51_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE51_LOCK1
// Description : Lock configuration MSBs for page 51 (addresses 0x1980 through
//               0x19ff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE51_LOCK1_ROW _u(0xfe7)
#define OTP_DATA_PAGE51_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE51_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE51_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE51_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE51_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE51_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE51_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE51_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE51_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE51_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE51_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE51_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE51_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE51_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE51_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE51_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE52_LOCK0
// Description : Lock configuration LSBs for page 52 (addresses 0x1a00 through
//               0x1a7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE52_LOCK0_ROW _u(0xfe8)
#define OTP_DATA_PAGE52_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE52_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE52_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE52_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE52_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE52_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE52_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE52_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE52_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE52_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE52_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE52_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE52_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE52_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE52_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE52_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE52_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE52_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE52_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE52_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE52_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE52_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE52_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE52_LOCK1
// Description : Lock configuration MSBs for page 52 (addresses 0x1a00 through
//               0x1a7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE52_LOCK1_ROW _u(0xfe9)
#define OTP_DATA_PAGE52_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE52_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE52_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE52_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE52_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE52_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE52_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE52_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE52_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE52_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE52_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE52_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE52_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE52_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE52_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE52_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE53_LOCK0
// Description : Lock configuration LSBs for page 53 (addresses 0x1a80 through
//               0x1aff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE53_LOCK0_ROW _u(0xfea)
#define OTP_DATA_PAGE53_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE53_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE53_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE53_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE53_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE53_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE53_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE53_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE53_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE53_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE53_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE53_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE53_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE53_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE53_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE53_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE53_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE53_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE53_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE53_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE53_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE53_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE53_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE53_LOCK1
// Description : Lock configuration MSBs for page 53 (addresses 0x1a80 through
//               0x1aff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE53_LOCK1_ROW _u(0xfeb)
#define OTP_DATA_PAGE53_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE53_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE53_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE53_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE53_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE53_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE53_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE53_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE53_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE53_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE53_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE53_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE53_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE53_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE53_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE53_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE54_LOCK0
// Description : Lock configuration LSBs for page 54 (addresses 0x1b00 through
//               0x1b7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE54_LOCK0_ROW _u(0xfec)
#define OTP_DATA_PAGE54_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE54_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE54_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE54_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE54_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE54_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE54_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE54_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE54_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE54_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE54_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE54_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE54_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE54_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE54_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE54_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE54_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE54_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE54_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE54_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE54_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE54_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE54_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE54_LOCK1
// Description : Lock configuration MSBs for page 54 (addresses 0x1b00 through
//               0x1b7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE54_LOCK1_ROW _u(0xfed)
#define OTP_DATA_PAGE54_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE54_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE54_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE54_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE54_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE54_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE54_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE54_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE54_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE54_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE54_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE54_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE54_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE54_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE54_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE54_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE55_LOCK0
// Description : Lock configuration LSBs for page 55 (addresses 0x1b80 through
//               0x1bff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE55_LOCK0_ROW _u(0xfee)
#define OTP_DATA_PAGE55_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE55_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE55_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE55_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE55_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE55_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE55_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE55_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE55_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE55_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE55_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE55_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE55_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE55_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE55_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE55_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE55_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE55_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE55_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE55_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE55_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE55_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE55_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE55_LOCK1
// Description : Lock configuration MSBs for page 55 (addresses 0x1b80 through
//               0x1bff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE55_LOCK1_ROW _u(0xfef)
#define OTP_DATA_PAGE55_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE55_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE55_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE55_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE55_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE55_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE55_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE55_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE55_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE55_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE55_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE55_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE55_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE55_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE55_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE55_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE56_LOCK0
// Description : Lock configuration LSBs for page 56 (addresses 0x1c00 through
//               0x1c7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE56_LOCK0_ROW _u(0xff0)
#define OTP_DATA_PAGE56_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE56_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE56_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE56_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE56_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE56_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE56_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE56_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE56_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE56_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE56_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE56_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE56_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE56_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE56_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE56_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE56_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE56_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE56_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE56_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE56_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE56_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE56_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE56_LOCK1
// Description : Lock configuration MSBs for page 56 (addresses 0x1c00 through
//               0x1c7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE56_LOCK1_ROW _u(0xff1)
#define OTP_DATA_PAGE56_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE56_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE56_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE56_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE56_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE56_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE56_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE56_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE56_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE56_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE56_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE56_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE56_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE56_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE56_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE56_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE57_LOCK0
// Description : Lock configuration LSBs for page 57 (addresses 0x1c80 through
//               0x1cff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE57_LOCK0_ROW _u(0xff2)
#define OTP_DATA_PAGE57_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE57_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE57_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE57_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE57_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE57_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE57_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE57_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE57_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE57_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE57_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE57_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE57_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE57_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE57_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE57_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE57_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE57_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE57_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE57_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE57_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE57_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE57_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE57_LOCK1
// Description : Lock configuration MSBs for page 57 (addresses 0x1c80 through
//               0x1cff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE57_LOCK1_ROW _u(0xff3)
#define OTP_DATA_PAGE57_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE57_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE57_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE57_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE57_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE57_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE57_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE57_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE57_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE57_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE57_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE57_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE57_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE57_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE57_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE57_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE58_LOCK0
// Description : Lock configuration LSBs for page 58 (addresses 0x1d00 through
//               0x1d7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE58_LOCK0_ROW _u(0xff4)
#define OTP_DATA_PAGE58_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE58_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE58_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE58_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE58_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE58_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE58_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE58_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE58_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE58_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE58_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE58_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE58_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE58_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE58_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE58_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE58_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE58_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE58_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE58_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE58_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE58_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE58_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE58_LOCK1
// Description : Lock configuration MSBs for page 58 (addresses 0x1d00 through
//               0x1d7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE58_LOCK1_ROW _u(0xff5)
#define OTP_DATA_PAGE58_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE58_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE58_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE58_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE58_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE58_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE58_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE58_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE58_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE58_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE58_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE58_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE58_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE58_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE58_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE58_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE59_LOCK0
// Description : Lock configuration LSBs for page 59 (addresses 0x1d80 through
//               0x1dff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE59_LOCK0_ROW _u(0xff6)
#define OTP_DATA_PAGE59_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE59_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE59_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE59_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE59_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE59_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE59_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE59_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE59_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE59_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE59_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE59_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE59_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE59_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE59_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE59_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE59_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE59_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE59_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE59_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE59_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE59_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE59_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE59_LOCK1
// Description : Lock configuration MSBs for page 59 (addresses 0x1d80 through
//               0x1dff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE59_LOCK1_ROW _u(0xff7)
#define OTP_DATA_PAGE59_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE59_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE59_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE59_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE59_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE59_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE59_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE59_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE59_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE59_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE59_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE59_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE59_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE59_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE59_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE59_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE60_LOCK0
// Description : Lock configuration LSBs for page 60 (addresses 0x1e00 through
//               0x1e7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE60_LOCK0_ROW _u(0xff8)
#define OTP_DATA_PAGE60_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE60_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE60_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE60_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE60_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE60_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE60_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE60_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE60_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE60_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE60_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE60_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE60_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE60_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE60_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE60_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE60_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE60_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE60_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE60_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE60_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE60_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE60_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE60_LOCK1
// Description : Lock configuration MSBs for page 60 (addresses 0x1e00 through
//               0x1e7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE60_LOCK1_ROW _u(0xff9)
#define OTP_DATA_PAGE60_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE60_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE60_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE60_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE60_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE60_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE60_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE60_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE60_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE60_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE60_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE60_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE60_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE60_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE60_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE60_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE61_LOCK0
// Description : Lock configuration LSBs for page 61 (addresses 0x1e80 through
//               0x1eff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE61_LOCK0_ROW _u(0xffa)
#define OTP_DATA_PAGE61_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE61_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE61_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE61_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE61_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE61_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE61_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE61_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE61_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE61_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE61_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE61_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE61_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE61_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE61_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE61_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE61_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE61_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE61_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE61_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE61_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE61_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE61_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE61_LOCK1
// Description : Lock configuration MSBs for page 61 (addresses 0x1e80 through
//               0x1eff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE61_LOCK1_ROW _u(0xffb)
#define OTP_DATA_PAGE61_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE61_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE61_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE61_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE61_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE61_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE61_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE61_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE61_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE61_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE61_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE61_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE61_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE61_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE61_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE61_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE62_LOCK0
// Description : Lock configuration LSBs for page 62 (addresses 0x1f00 through
//               0x1f7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE62_LOCK0_ROW _u(0xffc)
#define OTP_DATA_PAGE62_LOCK0_BITS   _u(0x00ffff7f)
#define OTP_DATA_PAGE62_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE62_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE62_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE62_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE62_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE62_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE62_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE62_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE62_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE62_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE62_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE62_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE62_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE62_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE62_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE62_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE62_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE62_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE62_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE62_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE62_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE62_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE62_LOCK1
// Description : Lock configuration MSBs for page 62 (addresses 0x1f00 through
//               0x1f7f). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE62_LOCK1_ROW _u(0xffd)
#define OTP_DATA_PAGE62_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE62_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE62_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE62_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE62_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE62_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE62_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE62_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE62_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE62_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE62_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE62_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE62_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE62_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE62_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE62_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
// Register    : OTP_DATA_PAGE63_LOCK0
// Description : Lock configuration LSBs for page 63 (addresses 0x1f80 through
//               0x1fff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE63_LOCK0_ROW _u(0xffe)
#define OTP_DATA_PAGE63_LOCK0_BITS   _u(0x00ffffff)
#define OTP_DATA_PAGE63_LOCK0_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE63_LOCK0_R2_RESET  "-"
#define OTP_DATA_PAGE63_LOCK0_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE63_LOCK0_R2_MSB    _u(23)
#define OTP_DATA_PAGE63_LOCK0_R2_LSB    _u(16)
#define OTP_DATA_PAGE63_LOCK0_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE63_LOCK0_R1_RESET  "-"
#define OTP_DATA_PAGE63_LOCK0_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE63_LOCK0_R1_MSB    _u(15)
#define OTP_DATA_PAGE63_LOCK0_R1_LSB    _u(8)
#define OTP_DATA_PAGE63_LOCK0_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_RMA
// Description : Decommission for RMA of a suspected faulty device. This
//               re-enables the factory test JTAG interface, and makes pages 2
//               through 61 of the OTP permanently inaccessible.
#define OTP_DATA_PAGE63_LOCK0_RMA_RESET  "-"
#define OTP_DATA_PAGE63_LOCK0_RMA_BITS   _u(0x00000080)
#define OTP_DATA_PAGE63_LOCK0_RMA_MSB    _u(7)
#define OTP_DATA_PAGE63_LOCK0_RMA_LSB    _u(7)
#define OTP_DATA_PAGE63_LOCK0_RMA_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE
// Description : State when at least one key is registered for this page and no
//               matching key has been entered.
//               0x0 -> read_only
//               0x1 -> inaccessible
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_RESET              "-"
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_BITS               _u(0x00000040)
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_MSB                _u(6)
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_LSB                _u(6)
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_ACCESS             "RO"
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_VALUE_READ_ONLY    _u(0x0)
#define OTP_DATA_PAGE63_LOCK0_NO_KEY_STATE_VALUE_INACCESSIBLE _u(0x1)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_KEY_R
// Description : Index 1-6 of a hardware key which must be entered to grant read
//               access, or 0 if no such key is required.
#define OTP_DATA_PAGE63_LOCK0_KEY_R_RESET  "-"
#define OTP_DATA_PAGE63_LOCK0_KEY_R_BITS   _u(0x00000038)
#define OTP_DATA_PAGE63_LOCK0_KEY_R_MSB    _u(5)
#define OTP_DATA_PAGE63_LOCK0_KEY_R_LSB    _u(3)
#define OTP_DATA_PAGE63_LOCK0_KEY_R_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK0_KEY_W
// Description : Index 1-6 of a hardware key which must be entered to grant
//               write access, or 0 if no such key is required.
#define OTP_DATA_PAGE63_LOCK0_KEY_W_RESET  "-"
#define OTP_DATA_PAGE63_LOCK0_KEY_W_BITS   _u(0x00000007)
#define OTP_DATA_PAGE63_LOCK0_KEY_W_MSB    _u(2)
#define OTP_DATA_PAGE63_LOCK0_KEY_W_LSB    _u(0)
#define OTP_DATA_PAGE63_LOCK0_KEY_W_ACCESS "RO"
// =============================================================================
// Register    : OTP_DATA_PAGE63_LOCK1
// Description : Lock configuration MSBs for page 63 (addresses 0x1f80 through
//               0x1fff). Locks are stored with 3-way majority vote encoding, so
//               that bits can be set independently.
//
//               This OTP location is always readable, and is write-protected by
//               its own permissions.
#define OTP_DATA_PAGE63_LOCK1_ROW _u(0xfff)
#define OTP_DATA_PAGE63_LOCK1_BITS   _u(0x00ffff3f)
#define OTP_DATA_PAGE63_LOCK1_RESET  _u(0x00000000)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK1_R2
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE63_LOCK1_R2_RESET  "-"
#define OTP_DATA_PAGE63_LOCK1_R2_BITS   _u(0x00ff0000)
#define OTP_DATA_PAGE63_LOCK1_R2_MSB    _u(23)
#define OTP_DATA_PAGE63_LOCK1_R2_LSB    _u(16)
#define OTP_DATA_PAGE63_LOCK1_R2_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK1_R1
// Description : Redundant copy of bits 7:0
#define OTP_DATA_PAGE63_LOCK1_R1_RESET  "-"
#define OTP_DATA_PAGE63_LOCK1_R1_BITS   _u(0x0000ff00)
#define OTP_DATA_PAGE63_LOCK1_R1_MSB    _u(15)
#define OTP_DATA_PAGE63_LOCK1_R1_LSB    _u(8)
#define OTP_DATA_PAGE63_LOCK1_R1_ACCESS "RO"
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK1_LOCK_BL
// Description : Dummy lock bits reserved for bootloaders (including the RP2350
//               USB bootloader) to store their own OTP access permissions. No
//               hardware effect, and no corresponding SW_LOCKx registers.
//               0x0 -> Bootloader permits user reads and writes to this page
//               0x1 -> Bootloader permits user reads of this page
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE
//               0x3 -> Bootloader does not permit user access to this page
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_RESET              "-"
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_BITS               _u(0x00000030)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_MSB                _u(5)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_LSB                _u(4)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_ACCESS             "RO"
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE63_LOCK1_LOCK_BL_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK1_LOCK_NS
// Description : Lock state for NonSecure accesses to this page.
//               Thermometer-coded, so lock state can be advanced permanently
//               from any state to any less-permissive state by programming OTP.
//               Software can also advance the lock state temporarily (until
//               next OTP reset) using the SW_LOCKx registers.
//
//               Note that READ_WRITE and READ_ONLY are equivalent in hardware,
//               as the SBPI programming interface is not accessible to
//               NonSecure software. However, Secure software may check these
//               bits to apply write permissions to a NonSecure OTP programming
//               API.
//               0x0 -> Page can be read by NonSecure software, and Secure
//               software may permit NonSecure writes.
//               0x1 -> Page can be read by NonSecure software
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by NonSecure software.
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_RESET              "-"
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_BITS               _u(0x0000000c)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_MSB                _u(3)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_LSB                _u(2)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_ACCESS             "RO"
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE63_LOCK1_LOCK_NS_VALUE_INACCESSIBLE _u(0x3)
// -----------------------------------------------------------------------------
// Field       : OTP_DATA_PAGE63_LOCK1_LOCK_S
// Description : Lock state for Secure accesses to this page. Thermometer-coded,
//               so lock state can be advanced permanently from any state to any
//               less-permissive state by programming OTP. Software can also
//               advance the lock state temporarily (until next OTP reset) using
//               the SW_LOCKx registers.
//               0x0 -> Page is fully accessible by Secure software.
//               0x1 -> Page can be read by Secure software, but can not be
//               written.
//               0x2 -> Do not use. Behaves the same as INACCESSIBLE.
//               0x3 -> Page can not be accessed by Secure software.
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_RESET              "-"
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_BITS               _u(0x00000003)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_MSB                _u(1)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_LSB                _u(0)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_ACCESS             "RO"
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_VALUE_READ_WRITE   _u(0x0)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_VALUE_READ_ONLY    _u(0x1)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_VALUE_RESERVED     _u(0x2)
#define OTP_DATA_PAGE63_LOCK1_LOCK_S_VALUE_INACCESSIBLE _u(0x3)
// =============================================================================
#endif // HARDWARE_REGS_OTP_DATA_DEFINED
