# Drivers

- `vga_terminal.c` writes early text output to VGA text memory.
- `keyboard.c` polls PS/2 scan-code set 1 and translates basic US keyboard input, including Escape.
- `mouse.c` initializes and polls a standard three-byte PS/2 mouse packet.
- `e1000.c` scans PCI configuration space and provides polled raw-frame send/receive for selected Intel E1000 IDs.
- `serial.c` initializes COM1 and provides bounded-polling serial diagnostics.
- `x86_io.c` provides the x86 port read/write instructions used by drivers.

Port I/O is privileged and must only be called by kernel code. The root build
compiles these objects but does not execute them on the host. The serial port is
optional; VGA output remains available if COM1 is not detected.

Keyboard and mouse input are polled rather than interrupt-driven. The E1000
driver has no IP, DHCP, or socket layer and requires DMA-visible identity-mapped
memory. It is an early prototype, not a general-purpose network stack.
