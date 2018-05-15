#include "lib/printk.h"
#include "lib/heap.h"
#include "display/video_fb.h"

#include "hwinit/btn.h"
#include "hwinit/hwinit.h"
#include "hwinit/di.h"
#include "hwinit/mc.h"
#include "hwinit/t210.h"
#include "hwinit/sdmmc.h"
#include "hwinit/timer.h"
#include "hwinit/util.h"
#include "lib/decomp.h"
#include "lib/crc32.h"
#include "gpt_nintendo_32.inc"
#include <string.h>
#define XVERSION 1

static size_t decompress_lz4_buffer(const uint8_t* srcBytes, size_t srcLen, uint8_t* dstBytes, size_t dstLen, const uint32_t goodCrc)
{
    size_t newSize = ulz4fn(srcBytes, srcLen, dstBytes, dstLen);
    if (newSize == 0)
    {
        printk("FAIL!\n");
        return 0;
    }
    else if (dstLen > newSize)
        memset(&dstBytes[newSize], 0, dstLen-newSize);

    printk("%u bytes", newSize);
    unsigned int newCrc = crc32b(dstBytes, newSize);
    printk(" CRC32: 0x%08X", newCrc);
    if (newCrc != goodCrc)
    {
        printk(" - FAIL!\n");
        return 0;
    }
    else
        printk(" - OK!\n");

    return newSize;
}

static const u32 SECTOR_SIZE = 512;
static int restore_gpt_to_storage(sdmmc_storage_t* storage, uint32_t gptStartSector, uint32_t gptSizeSectors, uint8_t* gptDataBuffer, uint32_t expectedCrc)
{
    if (!sdmmc_storage_write(storage, gptStartSector, gptSizeSectors, gptDataBuffer))
    {
        printk("WRITE FAIL! \n");
        return 0;
    }
    memset(gptDataBuffer, 0, gptSizeSectors*SECTOR_SIZE);
    if (!sdmmc_storage_read(storage, gptStartSector, gptSizeSectors, gptDataBuffer))
    {
        printk("READ FAIL! \n");
        return 0;
    }
    uint32_t calcCrc = crc32b(gptDataBuffer, gptSizeSectors*SECTOR_SIZE);
    if (calcCrc != expectedCrc)
    {
        printk("CRC32 FAIL! (expected: 0x%08x got %0x08x)\n", expectedCrc, calcCrc);
        return 0;
    }    
    
    printk("OK!\n");
    return gptSizeSectors*SECTOR_SIZE;
}

int main(void) 
{
    u32* lfb_base;    

    config_hw();
    display_enable_backlight(0);
    display_init();

    // Set up the display, and register it as a printk provider.
    lfb_base = display_init_framebuffer();
    video_init(lfb_base);

    //Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
	heap_init(0x90020000);

    printk("                               gptrestore v%d by rajkosto\n", XVERSION);
    printk("\n atmosphere base by team reswitched, hwinit by naehrwert, some parts taken from coreboot\n\n");

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_enable_backlight(1);
    mc_enable_ahb_redirect();

    static const u32 ALLOC_SIZE = 32768;

    void* allocPrefix = NULL;
    void* allocSuffix = NULL;
    
    allocPrefix = malloc(ALLOC_SIZE);
    uint8_t* gptPrefix = (void*)ALIGN_UP((uintptr_t)(allocPrefix), SECTOR_SIZE);
    size_t gptPrefixSize = ALLOC_SIZE - ((uintptr_t)gptPrefix - (uintptr_t)allocPrefix);

    printk("Decompressing GPT prefix..."); 
    gptPrefixSize = decompress_lz4_buffer(_gpt_prefix_lz4, sizeof(_gpt_prefix_lz4), gptPrefix, gptPrefixSize, _gpt_prefix_crc32);
    if (gptPrefixSize == 0)
        goto progend;

    allocSuffix = malloc(ALLOC_SIZE);
    uint8_t* gptSuffix = (void*)ALIGN_UP((uintptr_t)(allocSuffix), SECTOR_SIZE);
    size_t gptSuffixSize = ALLOC_SIZE - ((uintptr_t)gptSuffix - (uintptr_t)allocSuffix);

    printk("Decompressing GPT suffix..."); 
    gptSuffixSize = decompress_lz4_buffer(_gpt_suffix_lz4, sizeof(_gpt_suffix_lz4), gptSuffix, gptSuffixSize, _gpt_suffix_crc32);
    if (gptSuffixSize == 0)
        goto progend;

    sdmmc_storage_t storage;
    memset(&storage, 0, sizeof(storage));
	sdmmc_t sdmmc;
    memset(&sdmmc, 0, sizeof(sdmmc));

	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
        memset(&storage, 0, sizeof(storage));
		printk("Failed to init eMMC.\n");
		goto progend;
	}

    //unnecessary since default should already be large area, but do it just in case
    if (!sdmmc_storage_set_mmc_partition(&storage, 0)) 
    {
        printk("Failed to switch eMMC partition.\n");
		goto progend;
    }

    uint32_t emmcNumSectors = storage.ext_csd.sectors;
    if (emmcNumSectors != 0x3A3E000) //capacity of switch 32GB eMMC
    {
        printk("Invalid eMMC capacity ! Only standard 32GB eMMC are supported.");
        goto progend;
    }
    else
        printk("eMMC capacity: %llu bytes\n", (uint64_t)emmcNumSectors * SECTOR_SIZE);

    printk("\nPRESS VOL- TO RESTORE STOCK NINTENDO GPT TO eMMC OR ANY OTHER BUTTON TO QUIT\n");
    for (;;) 
    {
        uint32_t btn = btn_read();
        if (btn == BTN_VOL_DOWN)
            break;

        btn &= ~BTN_VOL_DOWN;
        if (btn != 0)
            goto progend;        
    }

    uint32_t gptPrefixOffset = 0;
    uint32_t gptPrefixSectors = ALIGN_UP(gptPrefixSize, SECTOR_SIZE) / SECTOR_SIZE;
    printk("Writing GPT prefix to LBA %u size %u sectors...", gptPrefixOffset, gptPrefixSectors);
    if (!restore_gpt_to_storage(&storage, gptPrefixOffset, gptPrefixSectors, gptPrefix, _gpt_prefix_crc32))
        goto progend;

    uint32_t gptSuffixSectors = ALIGN_UP(gptSuffixSize, SECTOR_SIZE) / SECTOR_SIZE;
    uint32_t gptSuffixOffset = emmcNumSectors - gptSuffixSectors;
    printk("Writing GPT suffix to LBA %u size %u sectors...", gptSuffixOffset, gptSuffixSectors);
    if (!restore_gpt_to_storage(&storage, gptSuffixOffset, gptSuffixSectors, gptSuffix, _gpt_suffix_crc32))
        goto progend;

progend:
    if (storage.sdmmc != NULL)
        sdmmc_storage_end(&storage, 0);

    mc_disable_ahb_redirect();
    if (allocPrefix != NULL) { free(allocPrefix); allocPrefix = NULL; }
    if (allocSuffix != NULL) { free(allocSuffix); allocSuffix = NULL; }

    printk("\nPress the POWER button to turn off the console.\n");
    while (btn_read() != BTN_POWER) { sleep(10000); }

    // Tell the PMIC to turn everything off
    shutdown_using_pmic();

    /* Do nothing for now */
    return 0;
}
