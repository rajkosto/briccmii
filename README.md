# gptrestore ![License](https://img.shields.io/badge/License-GPLv2-blue.svg)
Restores the original Nintendo Switch GPT to your eMMC if you somehow messed it up ;)

## Usage
 1. Build `gptrestore.bin` using make from the repository root directory, or download a binary release from https://switchtools.sshnuke.net
 2. Send the gptrestore.bin to your Switch running in RCM mode via a fusee-launcher (sudo ./fusee-launcher.py gptrestore.bin or just drag and drop it onto TegraRcmSmash.exe on Windows)
 3. Follow the on-screen prompts.

## Changes

This section is required by the GPLv2 license

 * initial code based on https://github.com/Atmosphere-NX/Atmosphere
 * everything except fusee-primary been removed (from Atmosphere)
 * all hwinit code has been replaced by the updated versions from https://github.com/nwert/hekate
 * Files pinmux.c/h, carveout.c/h, flow.h, sdram.c/h, decomp.h,lz4_wrapper.c,lzma.c,lzmadecode.c,lz4.c.inc,cbmem.c/h are based on https://github.com/fail0verflow/switch-coreboot.git sources
 * main.c has been modified to prepare the replacement GPT, initialize the eMMC, check its size, and write the prepared GPT to both primary and secondary location on the eMMC if chosen to.

## Responsibility

**I am not responsible for anything, including dead switches, loss of life, or total nuclear annihilation.**
