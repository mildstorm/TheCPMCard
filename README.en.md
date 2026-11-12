# ALS The CP/M Card for MAME

[한국어](README.md) | [English](README.en.md)

This repository provides the changes needed to add the ALS The CP/M Card to MAME's Apple II bus, along with CP/M disk images and reference documents. It does not include the full MAME source tree.

## Contents

- `src/devices/bus/a2bus/a2cpmcard.cpp` and `a2cpmcard.h`: the 6 MHz Z80B card, onboard RAM, Apple II memory and DMA access, bank switching, and slot control
- `src/lib/formats/fdi_dsk.cpp`: a wrapper that reuses MAME's PC-98 FDI format implementation
- `patches/mame-cpmcard.patch`: adds the card files to MAME's build, registers the card in Apple II slots, and adds `/IRQ` change notification to the a2bus (`a2bus.cpp/.h`, `bus_irq_w()`)
- `patches/mame-apple2e-backspace.patch`: fixes Apple IIe Backspace input
- `CPM/`: CP/M disk images and card reference documents
- `screenshots/cpm-card-boot.png`: a complete CP/M startup screen with the card installed
- `photos/`: photos of a real card (component and solder side scans plus close-ups)

## Apply the patches and build MAME

These changes are based on MAME commit `84981cc15cfc1429e48f098d80c6dd97bc82a0bc`. Clone MAME and this repository into sibling directories, then apply both patches from the MAME checkout:

```bash
git clone https://github.com/mamedev/mame.git
git clone https://github.com/mildstorm/TheCPMCard.git
cd mame
git checkout 84981cc15cfc1429e48f098d80c6dd97bc82a0bc
git apply ../TheCPMCard/patches/mame-cpmcard.patch
git apply ../TheCPMCard/patches/mame-apple2e-backspace.patch
```

The Backspace patch maps the Apple IIe Delete key, which is bound to the host Backspace key, to ASCII Backspace (`0x08`) so CP/M command-line editing works as expected.

Build the Apple IIe target from the MAME source directory:

```bash
make -j$(nproc) SUBTARGET=apple2e SOURCES=src/mame/apple/apple2e.cpp REGENIE=1
```

See the [MAME compilation guide](https://docs.mamedev.org/initialsetup/compilingmame.html) for platform-specific prerequisites and build instructions.

## Run CP/M

Install the ALS CP/M card in slot 4 or 7 and a 5.25-inch Disk II controller in slot 6. The command below keeps the default slot 6 Disk II controller, puts the CP/M card in slot 4, and mounts the CP/M Plus system disk and GSX-80 D1 disk in the first two drives.

Run it from the MAME source directory. This example assumes the MAME and TheCPMCard repositories are sibling directories:

```bash
./apple2e.exe apple2e -window -nomaximize -sl4 cpmcard \
  -flop1 "../TheCPMCard/CPM/CPM.Disk1.DSK" \
  -flop2 "../TheCPMCard/CPM/GSX-80 VERSION 1.1/GSX80D1.DSK"
```

MAME defaults to full-screen mode. `-window -nomaximize` opens it in a smaller window; drag a window edge to resize it.

At the startup menu, press `4` to reach the `A>` prompt. Pressing the host Backspace key removes the previous character.

## Screenshot

The image below shows the complete CP/M Plus 3.01C startup menu:

![ALS The CP/M Card running in MAME](screenshots/cpm-card-boot.png)

## Hardware model

The emulation follows photos of a real board (PCB "DIGITAL RESEARCH / Advanced Logic Systems", 08-83) and `CPM/CPM Card Schematic.pdf` (rev A, 05-04-83).

| Part | Function |
|------|----------|
| U15 Z0840006PSC | Z80B, 12 MHz crystal (Y1) / 2 = 6 MHz |
| U2-U5, U7-U10 M5K4164S-15 | 8 × 64K×1 DRAM = 64K card RAM |
| U26 Raytheon R29623 (20-pin bipolar PROM) | Z80 → Apple address translation, one entry per 256-byte page |
| U22 74S113 | 6502/Z80 bus ownership toggle + Z80 clock divider |
| U6 74LS74 | M1 clock stretch (DRAM timing) |
| U27 74LS74 | Bank latch + 62 Hz tick flip-flop |
| U1 4020 | Apple Φ1 / 2^14 ≈ 62 Hz real-time-clock tick |

Component side: [photos/Scan_20261112_233251.jpg](photos/Scan_20261112_233251.jpg), solder side: [photos/Scan_20261112_233343.jpg](photos/Scan_20261112_233343.jpg)

The card has **no firmware ROM**; everything is loaded from disk by the 6502. The only programmable part is the U26 PROM, which has not been dumped yet.

- **Hand-off**: every write to `$Cnxx` (from the 6502, or from the Z80 through DMA) toggles bus ownership, and the card asserts /DMA while the Z80 owns the bus. The Z80 is never stopped as a whole: /WAIT is asserted only when it accesses Apple memory while the 6502 owns the bus, so it can keep running from card DRAM in parallel with the 6502 (emulated with MAME's restartable memory access). Reads of `$Cnxx` are not decoded (open bus).
- **Bank latch (U27)**: D0 is latched on every Z80 I/O write; the port address is not decoded. The CP/M 3 BIOS writes `F0` (bank 0) / `FF` (bank 1) to port 00. Z80 I/O reads return D0 = Apple /IRQ, D1-D7 = 1.
- **Memory map**: in bank 0 (reset state) Z80 `$0000-$EFFF` goes to Apple memory through U26. The table is synthesized as Z80 + `$9000` with the `$Cxxx`/`$Fxxx` pages swapped (Z80 `$0000-$2FFF` → `$9000-$BFFF`, `$3000` → `$F000`, `$4000-$5FFF` → `$D000-$EFFF`, `$6000` → `$C000` I/O, `$7000-$EFFF` → `$0000-$7FFF`). The PROM sees Z80 A15 inverted. In bank 1, `$0000-$EFFF` is card DRAM. `$F000-$FFFF` is card DRAM in both banks.
- **Real-time clock**: U1 divides Apple Φ1 by 2^14 (≈62.3 Hz). Z80 /INT = tick pending OR Apple /IRQ (slot pin 30). Interrupt acknowledge clears the tick and puts `$F7` (`RST 30h` in IM 0) on the bus. The 3.01C BIOS clock handler at `$FB94` counts 60 ticks per second, so `DATE` shows a running clock that is about 4% fast, as on the real card.
- **Z80 clock**: in bank 1 U6 removes one 12 MHz pulse per opcode fetch (M1 = 4.5 T-states, emulated as one extra T-state every second fetch). In bank 0 the divider only runs while Apple Φ0 is high, which is exactly 50% of every Apple cycle, so the Z80 averages 3 MHz (0.5 clock scale; the two crystals are not phase-locked).

## Disk images and reference documents

The `CPM/` directory includes CP/M 2.2 and CP/M Plus disks, a ProDOS disk, and reference PDFs for the card. Some disk images and documents may be third-party works; check the applicable rights and licenses before using or distributing them.
