# briccmii ![License](https://img.shields.io/badge/License-GPLv2-blue.svg)
Corrupts (or fixes) the first byte of every BCT's pubkey in BOOT0 so your Nintendo Switch always enters RCM mode (or boots normally)

## Usage
 1. Build `briccmii.bin` using make from the repository root directory, or download a binary release from https://switchtools.sshnuke.net
 2. Send the briccmii.bin to your Switch running in RCM mode via a fusee-launcher (sudo ./fusee-launcher.py briccmii.bin or just drag and drop it onto TegraRcmSmash.exe on Windows)
 3. Follow the on-screen prompts.

## Changes

This section is required by the GPLv2 license

 * initial code based on https://github.com/Atmosphere-NX/Atmosphere
 * everything except fusee-primary been removed (from Atmosphere)
 * all hwinit code has been replaced by the updated versions from https://github.com/nwert/hekate
 * Files pinmux.c/h, carveout.c/h, flow.h, sdram.c/h, decomp.h,lz4_wrapper.c,lzma.c,lzmadecode.c,lz4.c.inc are based on https://github.com/fail0verflow/switch-coreboot.git sources
 * main.c has been modified to read all the BCTs, figure out which ones are used, and either corrupt byte 0x210 of each one, or restore it to match the fuse SHA256.

## Responsibility

**I am not responsible for anything, including dead switches, loss of life, or total nuclear annihilation.**
