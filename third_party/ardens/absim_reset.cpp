#include "absim.hpp"

#ifndef ARDENS_EMBEDDED
#include <random>
#endif

namespace absim
{

void atmega32u4_t::reset()
{
    cycle_count = 0;

#ifdef ARDENS_EMBEDDED
    // A power-on reset detaches the host display sink. Guest watchdog resets
    // use soft_reset() and must preserve it or display capture silently stops.
    embedded_spi_sink = nullptr;
    embedded_spi_bulk_sink = nullptr;
    embedded_spi_sink_context = nullptr;
    embedded_port_sink = nullptr;
    embedded_port_sink_context = nullptr;
    embedded_hle_sink = nullptr;
    embedded_hle_sink_context = nullptr;
#endif

    soft_reset();

    // power-on reset flag
    MCUSR() |= (1 << 0);

    for(auto& byte : eeprom) byte = 0xff;

    eeprom_modified_bytes.reset();
    eeprom_modified = false;
    eeprom_dirty = false;

#ifndef ARDENS_NO_SERIAL_BUFFER
    serial_bytes.clear();
#endif

    for(auto& h : ld_handlers) h = nullptr;
    for(auto& h : st_handlers) h = nullptr;

    adc_seed = 0xcafebabe;
#ifndef ARDENS_EMBEDDED
    if(adc_nondeterminism)
        adc_seed = (uint32_t)std::random_device{}();
#endif

#ifndef ARDENS_NO_AUDIO
    sound_buffer.clear();
#endif

    st_handlers[0x3f] = eeprom_handle_st_eecr;
    st_handlers[0x49] = pll_handle_st_pllcsr;
    st_handlers[0x4c] = spi_handle_st_spcr_or_spsr;
    st_handlers[0x4d] = spi_handle_st_spcr_or_spsr;
    st_handlers[0x4e] = spi_handle_st_spdr;
    st_handlers[0x64] = st_handle_prr0;
    st_handlers[0x7a] = adc_st_handle_adcsra;

    st_handlers[0x23] = st_handle_pin;
    st_handlers[0x26] = st_handle_pin;
    st_handlers[0x29] = st_handle_pin;
    st_handlers[0x2c] = st_handle_pin;
    st_handlers[0x2f] = st_handle_pin;

    //st_handlers[0x25] = st_handle_port;
    //st_handlers[0x28] = st_handle_port;
#ifdef ARDENS_EMBEDDED
    st_handlers[0x2b] = st_handle_port;
    st_handlers[0x2e] = st_handle_port;
#else
    //st_handlers[0x2b] = st_handle_port;
    //st_handlers[0x2e] = st_handle_port;
#endif
    //st_handlers[0x31] = st_handle_port;

    for(int i = 0x44; i <= 0x48; ++i)
        st_handlers[i] = timer0_handle_st_regs;
    st_handlers[0x46] = timer0_handle_st_tcnt;

    for(int i = 0x80; i <= 0x8d; ++i)
        st_handlers[i] = timer1_handle_st_regs;
    for(int i = 0x90; i <= 0x9d; ++i)
        st_handlers[i] = timer3_handle_st_regs;
    for(int i = 0xbe; i <= 0xc4; ++i)
        st_handlers[i] = timer4_handle_st_regs;
    for(int i = 0xcf; i <= 0xd2; ++i)
        st_handlers[i] = timer4_handle_st_ocrN;

    // TIMSK4
    st_handlers[0x72] = timer4_handle_st_regs;

    st_handlers[0x35] = timer0_handle_st_tifr;
    st_handlers[0x36] = timer1_handle_st_tifr;
    st_handlers[0x38] = timer3_handle_st_tifr;
    st_handlers[0x39] = timer4_handle_st_tifr;

#ifndef ARDENS_NO_AUDIO
    st_handlers[0x27] = sound_st_handler_ddrc;
#endif

    ld_handlers[0x4d] = spi_handle_ld_spsr;
    ld_handlers[0x4e] = spi_handle_ld_spdr;
    ld_handlers[0x46] = timer0_handle_ld_tcnt;

    st_handlers[0x54] = st_handle_mcusr;
    st_handlers[0x55] = st_handle_mcucr;
    st_handlers[0x57] = st_handle_spmcsr;
    st_handlers[0x60] = st_handle_wdtcsr;

    st_handlers[0x63] = st_handler_timsk;
    st_handlers[0x6f] = st_handler_timsk;
    st_handlers[0x71] = st_handler_timsk;
    st_handlers[0x72] = st_handler_timsk;

    for(int i = 0x84; i <= 0x8d; ++i)
        ld_handlers[i] = timer1_handle_ld_regs;
    for(int i = 0x94; i <= 0x9d; ++i)
        ld_handlers[i] = timer3_handle_ld_regs;
    ld_handlers[0xbe] = timer4_handle_ld_tcnt;
    ld_handlers[0xbf] = timer4_handle_ld_tcnt;

    st_handlers[0xd7] = usb_st_handler;
    st_handlers[0xd8] = usb_st_handler;
    st_handlers[0xd9] = usb_st_handler;
    st_handlers[0xda] = usb_st_handler;
    for(int i = 0xe0; i <= 0xe6; ++i)
        st_handlers[i] = usb_st_handler;
    for(int i = 0xe8; i <= 0xf4; ++i)
        st_handlers[i] = usb_st_handler;

    ld_handlers[0xf1] = usb_ld_handler_uedatx;
}

void atmega32u4_t::soft_reset()
{
    // clear all registers and RAM, reset state

    // preserve button pins
    uint8_t pinb = data[0x23];
    uint8_t pine = data[0x2c];
    uint8_t pinf = data[0x2f];

    data = {};

    data[0x23] = pinb;
    data[0x2c] = pine;
    data[0x2f] = pinf;

    // turn off TX/RX LEDs at reset (assume bootloader has turned them off)
    data[0x25] |= 0x01;
    data[0x2b] |= 0x20;

    pc = BOOTRST() ? bootloader_address() : 0;
    executing_instr_pc = pc;

    just_read = 0xffffffff;
    just_written = 0xffffffff;

    active = true;
    wakeup_cycles = false;
    just_interrupted = false;

    num_stack_frames = 0;
    pushed_at_least_once = false;
    autobreaks.reset();

    prev_sreg = 0;

    timer0 = {};
    timer1 = {};
    timer3 = {};
    timer4 = {};

    data[0xd1] = timer4.ocrNc = timer4.ocrNc_next = timer4.top = 0xff;

    timer1.base_addr = 0x80;
    timer3.base_addr = 0x90;
    timer1.tifrN_addr = 0x36;
    timer3.tifrN_addr = 0x38;
    timer1.timskN_addr = 0x6f;
    timer3.timskN_addr = 0x71;
    timer1.prr_addr = 0x64;
    timer3.prr_addr = 0x65;
    timer1.prr_mask = 1 << 3;
    timer3.prr_mask = 1 << 3;

    timer0.prev_update_cycle = cycle_count;
    timer1.prev_update_cycle = cycle_count;
    timer3.prev_update_cycle = cycle_count;
    timer4.prev_update_cycle = cycle_count;

    timer0.next_update_cycle = UINT64_MAX;
    timer1.next_update_cycle = UINT64_MAX;
    timer3.next_update_cycle = UINT64_MAX;
    timer4.next_update_cycle = UINT64_MAX;

    pll_prev_cycle = cycle_count;
    pll_lock_cycle = 0;
    pll_num12 = 0;
    pll_busy = false;

    spsr_read_after_transmit = false;
    spi_busy = false;
    spi_busy_clear = false;
    spi_latch_read = false;
    spi_data_latched = false;
    spi_data_byte = 0;
    spi_datain_byte = 0;
    spi_clock_cycles = 4;
    spi_done_cycle = UINT64_MAX;
    spi_transmit_zero_cycle = UINT64_MAX;

    eeprom_prev_cycle = cycle_count;
    eeprom_clear_eempe_cycles = 0;
    eeprom_write_addr = 0;
    eeprom_write_data = 0;
    eeprom_program_cycles = 0;
    eeprom_busy = false;

    adc_prev_cycle = cycle_count;
    adc_prescaler_cycle = 0;
    adc_cycle = 0;
    adc_ref = 0;
    adc_result = 0;
    adc_busy = false;

    sound_prev_cycle = cycle_count;
    sound_cycle = 0;
    sound_enabled = 0;
    sound_pwm = false;
    sound_pwm_val = 0;

    min_stack = 0xffff;
    pushed_at_least_once = false;

    reset_usb();
#ifndef ARDENS_NO_USB_BUFFER
    usb_dpram = {};
#endif

    spm_prev_cycle = cycle_count;
    spm_busy = false;
    spm_op = SPM_OP_NONE;
    spm_cycles = 0;
    erase_spm_buffer();

    watchdog_divider = 0;
    watchdog_divider_cycle = 0;
    watchdog_prev_cycle = cycle_count;
    update_watchdog_prescaler();
#ifdef ARDENS_GAME_ONLY
    watchdog_next_cycle = UINT64_MAX;
#else
    watchdog_next_cycle = cycle_count + watchdog_divider;
    peripheral_queue.schedule(watchdog_next_cycle, PQ_WATCHDOG);
#endif

#ifdef ARDENS_EMBEDDED
    embedded_delay_pc = EMBEDDED_HLE_NONE;
    embedded_micros_pc = EMBEDDED_HLE_NONE;
    embedded_timer0_millis_addr = EMBEDDED_HLE_NONE;
    embedded_timer0_fract_addr = EMBEDDED_HLE_NONE;
    embedded_timer0_overflow_addr = EMBEDDED_HLE_NONE;
    embedded_delay_hits = 0;
    embedded_delay_last_ms = 0;
    embedded_delay_ms_total = 0;
    embedded_delay_cycles = 0;
    embedded_timer0_fast_updates = 0;
    embedded_spi_fast_writes = 0;
#endif

    OSCCAL() = 0x6d;

    peripheral_queue.clear();
}

}
