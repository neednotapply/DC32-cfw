from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> bool:
    if condition:
        print(f"PASS: {message}")
        return True
    print(f"FAIL: {message}", file=sys.stderr)
    return False


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", text)
    if not match:
        return ""

    pos = match.end()
    depth = 1
    while pos < len(text) and depth:
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
        pos += 1
    return text[match.end():pos - 1] if depth == 0 else ""


def main() -> int:
    ui = read("src/ui.c")
    badusb = read("src/badUsb.c")
    usb_device = read("src/usbDevice.c")
    usb_hid = read("src/usbHid.c")
    ir_remote = read("src/irRemote.c")
    pio_irda = read("src/pioIrdaSIR.c")
    dcd = read("third_party/tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c")
    ok = True

    badusb_ui = function_body(ui, "uiPrvRunBadUsbLocator")
    badusb_runner = function_body(badusb, "badUsbPrvRunPreparedFileWithScratch")
    badusb_script_runner = function_body(badusb, "badUsbPrvRunScript")
    badusb_read_line = function_body(badusb, "badUsbPrvReadLine")
    badusb_id_scan = function_body(badusb, "badUsbPrvReadDeviceInfoWithScratch")
    drop_now = function_body(usb_hid, "usbHidDropNow")
    device_drop_now = function_body(usb_device, "usbDeviceDropNow")
    ir_remote_end = function_body(ir_remote, "irRemoteEnd")
    ir_read_line = function_body(ui, "uiPrvReadLine")
    ir_rewind = function_body(ui, "uiPrvIrRewindRead")
    ir_detect_format = function_body(ui, "uiPrvIrDetectFormat")
    flipper = function_body(ui, "uiPrvIrBlastFlipper")
    send_ctx = function_body(ui, "uiPrvIrSendCtxKeepGoing")

    ok &= require("usbHidDropNow();" in badusb_ui, "BadUSB UI hard-drops HID during terminal cleanup")
    ok &= require("uiPrvBadUsbWaitKeysReleasedBounded" in badusb_ui and "uiPrvWaitKeysReleased();" not in badusb_ui, "BadUSB UI uses bounded key-release waits")
    ok &= require("BadUsbStateDone" not in badusb_runner, "BadUSB runner returns done without final done-status callback")
    ok &= require("fileSize = fatfsFileGetSize(fil)" in badusb_read_line and "*bytesReadP >= fileSize" in badusb_read_line, "BadUSB line reader stops at known file size")
    ok &= require("atFileEnd" in badusb_script_runner and "st->status.bytesRead >= st->status.fileSize" in badusb_script_runner, "BadUSB runner avoids an extra EOF read after the final line")
    ok &= require("truncated" in badusb_id_scan and "return fatfsFileSeek(fil, 0);" in badusb_id_scan, "BadUSB ID scan tolerates first-line parse errors")
    ok &= require("usbHidPrvReleaseAll" not in drop_now and "usbDeviceDropNow();" in drop_now, "usbHidDropNow skips final release report")
    ok &= require("tud_disconnect" not in device_drop_now and "dcd_disconnect(0);" in device_drop_now and "tud_deinit(0)" in device_drop_now, "USB hard-drop bypasses normal disconnect wrapper")

    ok &= require("RP2040_USB_ABORT_DONE_WAIT_LOOPS" in dcd and re.search(r"abort_done.*wait_count--", dcd, re.S) is not None, "TinyUSB endpoint abort wait is bounded")
    ok &= require("RP2040_USB_RESET_DONE_WAIT_LOOPS" in dcd and re.search(r"reset_done.*wait_count--", dcd, re.S) is not None, "TinyUSB USBCTRL reset wait is bounded")
    ok &= require("dcd_force_idle" in dcd and "usb_hw_clear->buf_status = 0xffffffffu;" in dcd and "usb_hw->main_ctrl = 0;" in dcd, "TinyUSB deinit force-clears controller state")

    ok &= require("#define IR_RAW_CANCEL_CHUNK_USEC\t1000" in ui, "IR mark/space cancellation chunk is 1 ms")
    ok &= require("IR_PARSED_RECORD_TIMEOUT_USEC" in ui, "IR parsed records have a wall-clock timeout")
    ok &= require("bytesReadP" in ir_read_line and "*bytesReadP >= fileSize" in ir_read_line, "IR line reader stops at known file size")
    ok &= require("stats->bytesRead = 0;" in ir_rewind and "uiPrvIrRewindRead(fil, stats)" in ir_detect_format, "IR rewind resets EOF byte counters")
    ok &= require("uiPrvIrSendCtxStalled" in send_ctx and "stats->cancelled = true" in send_ctx, "IR send context records timeout and cancel")
    ok &= require("!stats->timedOut" in flipper and "uiPrvFlipperRecordFinish" in flipper, "Flipper IR parser stops finalization after timeout")
    ok &= require("uiPrvIrAlertStalled" in ui and "stats.timedOut" in ui, "IR UI reports stalled records")
    ok &= require("badgeIrdaInit" not in ir_remote_end and "irdaSIRuartConfig(&offCfg" in ir_remote_end, "IR remote end leaves IrDA RX disabled")

    ok &= require("SIR_TX_QUEUE_TIMEOUT_MS" in pio_irda and "IRDA TX queue timeout" in pio_irda, "IrDA blocking TX queue has a timeout")
    ok &= require("SIR_TX_IRQ_MAX_FEEDS" in pio_irda and "IRDA TX IRQ feed limit" in pio_irda, "IrDA TX IRQ feed loop is bounded")
    ok &= require("return mCircBufWnext == mCircBufR;" in pio_irda, "IrDA TX full status uses the correct full condition")

    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
