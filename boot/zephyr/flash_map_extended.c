/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash.h>

#include "target.h"

#include <flash_map_backend/flash_map_backend.h>
#include <sysflash/sysflash.h>

#include "bootutil/bootutil_log.h"

MCUBOOT_LOG_MODULE_DECLARE(mcuboot);

#if (!defined(CONFIG_XTENSA) && defined(DT_FLASH_DEV_NAME))
#define FLASH_DEVICE_ID SOC_FLASH_0_ID
#define FLASH_DEVICE_BASE CONFIG_FLASH_BASE_ADDRESS
#elif (defined(CONFIG_XTENSA) && defined(DT_SPI_NOR_DRV_NAME))
#define FLASH_DEVICE_ID SPI_FLASH_0_ID
#else
#error "FLASH_DEVICE_ID could not be determined"
#endif

static struct device *flash_dev;

struct device *flash_device_get_binding(char *dev_name)
{
    if (!flash_dev) {
        flash_dev = device_get_binding(dev_name);
    }
    return flash_dev;
}

int flash_device_base(uint8_t fd_id, uintptr_t *ret)
{
#if (!defined(CONFIG_XTENSA) && defined(DT_FLASH_DEV_NAME))
    if (fd_id != FLASH_DEVICE_ID) {
        BOOT_LOG_ERR("invalid flash ID %d; expected %d",
                     fd_id, FLASH_DEVICE_ID);
        return -EINVAL;
    }
    *ret = FLASH_DEVICE_BASE;
#elif (defined(CONFIG_XTENSA) && defined(DT_SPI_NOR_DRV_NAME))
    *ret = 0;
#endif
    return 0;
}

/*
 * This depends on the mappings defined in sysflash.h, and assumes
 * that slot 0, slot 1, and the scratch areas are contiguous.
 */
int flash_area_id_from_image_slot(int slot)
{
    return slot + FLASH_AREA_IMAGE_0;
}

int flash_area_sector_from_off(off_t off, struct flash_sector *sector)
{
    int rc;
    struct flash_pages_info page;

    rc = flash_get_page_info_by_offs(flash_dev, off, &page);
    if (rc) {
        return rc;
    }

    sector->fs_off = page.start_offset;
    sector->fs_size = page.size;

    return rc;
}

#define ERASED_VAL 0xff
uint8_t flash_area_erased_val(const struct flash_area *fap)
{
    (void)fap;
    return ERASED_VAL;
}

int flash_area_read_is_empty(const struct flash_area *fa, uint32_t off,
        void *dst, uint32_t len)
{
    uint8_t i;
    uint8_t *u8dst;
    int rc;

    rc = flash_area_read(fa, off, dst, len);
    if (rc) {
        return -1;
    }

    for (i = 0, u8dst = (uint8_t *)dst; i < len; i++) {
        if (u8dst[i] != ERASED_VAL) {
            return 0;
        }
    }

    return 1;
}
