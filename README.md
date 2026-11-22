# ZenithOS: Developer & Robotics Operating System

ZenithOS is a lightweight, 64-bit developer and robotics operating system written in freestanding C. Built on the Limine boot protocol for modern x86_64 hardware, ZenithOS combines a responsive desktop environment with low-latency device communication, a bounded serial subsystem, and real-time sensor telemetry visualization for embedded and robotics engineering.

---

## Key Highlights

- **Direct Hardware & Desktop Experience**: Boots directly into a dark-themed desktop with dock, topbar metrics, draggable windows, and built-in developer tools—no browser or heavy userland stack required.
- **Zenith Device Studio**: Integrated serial workstation and robotics telemetry dashboard supporting bounded text console logging, serial command transmission, and live attitude/voltage waveforms.
- **Safe Kernel Boundaries**: Validated memory helpers (`copy_from_user`, `copy_to_user`, `copy_str_from_user`), isolated Ring 3 user mode, safe page fault termination for faulty user processes, and automated file descriptor cleanup on process exit.
- **VFS Device Integration**: Full 16550A UART COM1 driver exposed as `/dev/ttyS0` via the Virtual File System, accessible through standard `open()`, `read()`, `write()`, and `close()` syscalls or `SYS_SERIAL_STATUS`.
- **Telemetry Protocol (`#Z1`)**: Documented ASCII protocol for microcontrollers (STM32, ESP32, RP2040, Arduino) with sequence validation, drop detection, and live attitude/battery monitoring.

---

## Architecture Overview

```
+------------------------------------------------------------------------+
|                          ZenithOS Desktop GUI                          |
|  Device Studio  |  Terminal (Ring 3)  |  Files  |  SysMon  |  Calc   |
+------------------------------------------------------------------------+
                                    | Syscalls / VFS
+------------------------------------------------------------------------+
|                             Zenith Kernel                              |
|  - Process Scheduler & Virtual Memory Manager (PML4, Demand Paging)   |
|  - Validated User/Kernel Boundary Helpers (user_access)                |
|  - Virtual File System (RAMFS, /dev/ttyS0, Initramfs TAR)             |
|  - Telemetry Parser & History Ring Buffer (#Z1 Protocol)               |
+------------------------------------------------------------------------+
                                    | Interrupts & I/O
+------------------------------------------------------------------------+
|                            Hardware Drivers                            |
|  16550A UART (COM1)  |  PS/2 Mouse & Keyboard  |  PIT Timer  |  PIC   |
+------------------------------------------------------------------------+
```

---

## Robotics Telemetry Protocol (`#Z1`)

ZenithOS defines a lightweight, human-readable ASCII telemetry format tailored for microcontrollers, UAVs, and mobile robots:

```
#Z1,<seq>,<roll>,<pitch>,<yaw>,<volt_mv>,<temp_c>,<status>
```

### Field Definitions

| Field | Type | Unit / Range | Description |
|---|---|---|---|
| `Header` | String | `#Z1` | Protocol identifier for Zenith Telemetry v1 |
| `<seq>` | Unsigned 32-bit | `1 .. 4294967295` | Packet sequence counter (tracks dropped/lost packets) |
| `<roll>` | Signed 32-bit | `-180 .. +180` deg | Roll angle in degrees (center 0) |
| `<pitch>` | Signed 32-bit | `-90 .. +90` deg | Pitch angle in degrees |
| `<yaw>` | Signed 32-bit | `0 .. 359` deg | Heading / Yaw with 8 cardinal directions |
| `<volt_mv>`| Unsigned 32-bit | `0 .. 60000` mV | Main battery / DC bus voltage (e.g. `12450` = 12.45 V) |
| `<temp_c>` | Signed 32-bit | `-50 .. +150` °C | Onboard sensor or microcontroller temperature |
| `<status>` | String | Max 15 chars | Status label (e.g., `OK`, `ARMED`, `CALIB`, `STANDBY`) |

### Example Records

```text
[BOOT] Zenith Flight Controller v2.1 Online
#Z1,1,0,0,90,12450,28,OK
#Z1,2,-4,2,92,12425,29,ACTIVE
#Z1,3,-8,6,95,12400,29,ARMED
```

Device Studio displays normal serial strings (like boot logs) in the bounded console log, while lines matching `#Z1,` are automatically parsed, validated, and plotted on the **Telemetry Dashboard**.

---

## Desktop Applications

1. **Device Studio**:
   - **Console Tab**: Bounded 64-line scrollable serial log, active baud and status metrics, byte RX/TX counters, command input box, and non-blocking UART polling.
   - **Telemetry Dashboard Tab**: Real-time IMU roll/pitch tilt meters, compass heading, battery voltage level, temperature gauge, and multi-waveform sparkline plot for recent attitude history.
   - **Simulation Mode**: Built-in test generator toggle (`Sim: ON/OFF`) for testing telemetry without external hardware.
2. **Terminal**: Interactive shell running in Ring 3 user mode. Supports Unix-like commands including `help`, `ls`, `cat`, `echo`, `uptime`, `uname`, `ps`, and serial utilities.
3. **Files**: File explorer browsing the RAM filesystem tree (`/`, `/bin`, `/dev`, `/etc`, `/home`).
4. **System Monitor**: Real-time memory allocation (used vs. free pages) and uptime tracking.
5. **Calculator**: Math workstation with decimal and hexadecimal conversion.
6. **Settings**: Desktop theme and wallpaper accent controls.
7. **Applications**: Quick-launch grid for all installed system tools.

---

## Building and Running

### Prerequisites

- macOS or Linux
- GNU Make (`make` or `gmake`)
- Clang / LLVM (with `lld` linker)
- `xorriso` (for ISO creation)
- `qemu-system-x86_64` (for emulation)
- Python 3 (for the host mock telemetry runner)

### Build the Bootable ISO

```bash
make template-x86_64.iso
```

### Run with COM1 Serial Bridge (Recommended)

To connect real external hardware, microcontrollers, or host simulation scripts, run QEMU with the serial TCP bridge:

```bash
make run-serial-bios
```

This starts ZenithOS with COM1 mapped to host TCP port `127.0.0.1:4444`.

### Connecting a Mock Telemetry Streamer

In another host terminal, run the included mock telemetry script to stream realistic attitude waveforms and interact with Device Studio:

```bash
python3 scripts/mock_robot_telemetry.py --port 4444 --rate 5
```

You can also connect interactively via netcat:

```bash
nc 127.0.0.1 4444
```

### Interactive Shell Commands

Inside the ZenithOS Terminal:

```sh
# Inspect serial port status, baud rate, and byte counters
serial status

# Send text out of COM1
serial send PING_ROBOT

# Read buffered bytes from /dev/ttyS0
serial read
```

---

## Platform Roadmap

- **Stage 0 (Completed)**: Baseline audit of boot, interrupts, scheduler, VFS, and GUI compositor.
- **Stage 1 (Completed)**: Kernel/user boundary safety with validated memory access helpers and process exit FD cleanup.
- **Stage 2 (Completed)**: 16550A UART COM1 driver, bounded circular ring buffer, `/dev/ttyS0` VFS node, and QEMU host TCP serial bridge.
- **Stage 3 (Completed)**: Zenith Device Studio desktop workstation with non-blocking serial console and command input.
- **Stage 4 (Completed)**: `#Z1` robotics telemetry specification, bounded parser, and live attitude/voltage dashboard.
- **Stage 5 (Future Platform Growth)**:
  - Persistent block device storage (AHCI / NVMe / FAT32) for logs and flight records.
  - USB core and USB CDC ACM serial support for USB-to-UART bridge dongles.
  - User-space GUI application API and lightweight windowing IPC.
  - Onboard firmware flashing and serial bootloader integration (e.g. STM32 DFU / serial bootloaders).
