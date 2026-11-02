# ALS The CP/M Card for MAME

[한국어](README.md) | [English](README.en.md)

This repository provides the changes needed to add the ALS The CP/M Card to MAME's Apple II bus, along with CP/M disk images and reference documents. It does not include the full MAME source tree.

## Contents

- `src/devices/bus/a2bus/a2cpmcard.cpp` and `a2cpmcard.h`: the 6 MHz Z80B card, onboard RAM, Apple II memory and DMA access, bank switching, and slot control
- `src/lib/formats/fdi_dsk.cpp`: a wrapper that reuses MAME's PC-98 FDI format implementation
- `patches/mame-cpmcard.patch`: adds the card files to MAME's build and registers the card in Apple II slots
- `patches/mame-apple2e-backspace.patch`: fixes Apple IIe Backspace input
- `CPM/`: CP/M disk images and card reference documents
- `screenshots/cpm-card-boot.png`: a complete CP/M startup screen with the card installed

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

## Disk images and reference documents

The `CPM/` directory includes CP/M 2.2 and CP/M Plus disks, a ProDOS disk, and reference PDFs for the card. Some disk images and documents may be third-party works; check the applicable rights and licenses before using or distributing them.
