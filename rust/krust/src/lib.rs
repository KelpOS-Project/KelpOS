#![no_std]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

/// CRC32 (IEEE), wird vom Kernel-Befehl `crc` benutzt.
/// Das Kernel-Makefile linkt libkrust.a automatisch, wenn sie gebaut wurde
/// (`make rust`, Target x86_64-unknown-none, braucht i. d. R. nightly).
#[no_mangle]
pub extern "C" fn krust_crc32(data: *const u8, len: usize) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    let slice = unsafe { core::slice::from_raw_parts(data, len) };
    for &b in slice {
        crc ^= b as u32;
        for _ in 0..8 {
            let mask = (crc & 1).wrapping_neg();
            crc = (crc >> 1) ^ (0xEDB8_8320 & mask);
        }
    }
    !crc
}
