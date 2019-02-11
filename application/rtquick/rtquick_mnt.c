/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h" 
#include "rtdevice.h" 

#if defined(RT_USING_DFS)
#include <dfs_file.h>
#include <dfs_posix.h>
#endif

rt_err_t rtquick_mnt_init(void)
{
    rt_err_t ret = RT_EOK; 

    extern struct romfs_dirent romfs_root;
    dfs_mount(RT_NULL, "/", "rom", 0, &romfs_root); 

    /* 内存磁盘自动挂载 */ 
#if defined(BSP_RAMDISK_ENABLE_AUTO_MOUNT) 
    dfs_mkfs("elm", "ram0"); 
    
    if(dfs_mount("ram0", BSP_RAMDISK_CONFIG_MOUNT_POINT, "elm", 0, 0) != 0)
    {
        rt_kprintf("ram0 mount '%s' failed.\n", BSP_RAMDISK_CONFIG_MOUNT_POINT);  
    }
#endif 

#if defined(BSP_SDCARD_ENABLE_AUTO_MOUNT)
    /* SDIO设备模式的SD卡自动挂载 */ 
#if defined(BSP_SDCARD_USING_SDIO)
    ret = mmcsd_wait_cd_changed(RT_TICK_PER_SECOND);
    if (ret == MMCSD_HOST_PLUGED)
    {
        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", BSP_SDCARD_CONFIG_MOUNT_POINT, "elm", 0, 0) == 0)
        {
            rt_kprintf("sd0 mount '%s' failed.\n", BSP_SDCARD_CONFIG_MOUNT_POINT);  
        }
    }
    else
    {
        rt_kprintf("sdcard not inserted!\n");
    }

    /* 块设备模式的SD卡自动挂载 */ 
#elif defined(BSP_SDCARD_USING_BLOCK)
    if(dfs_mount("sd0", BSP_SDCARD_CONFIG_MOUNT_POINT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sd0 mount '%s' failed.\n", BSP_SDCARD_CONFIG_MOUNT_POINT); 
    }
#endif

    /* 切换目录到SD卡 */ 
    extern int chdir(const char *path); 
    chdir(BSP_SDCARD_CONFIG_MOUNT_POINT); 
#endif

    /* 初始化USB */ 
#if defined(BSP_ENABLE_USB)
    extern int stm_usbh_register(void); 
    stm_usbh_register(); 
#endif

    return ret; 
}
