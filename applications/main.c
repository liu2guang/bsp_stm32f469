/*
 * File      : main.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-07-29     Arda.Fu      first implementation
 */
#include <rtthread.h>
#include <board.h>

#if defined(RT_USING_DFS)
#include <dfs_file.h>
#include <dfs_posix.h>
#endif

#if defined(BSP_USING_RW00X) && defined(PKG_USING_RW007) 
#include <spi_wifi_rw007.h>
#endif

#if defined(PKG_USING_LUDIO)
#include "audio.h" 
#include "audio_control.h"
#endif

#if defined(RT_USING_LWIP) && defined(BSP_USING_RW00X) && defined(PKG_USING_RW007) 
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

static int get_wifi_status(void)
{
    ip_addr_t ip_addr;
    int result = 0;

    ip_addr_set_zero(&ip_addr);
    if (ip_addr_cmp(&(netif_list->ip_addr), &ip_addr))
    {
        result = 0;
    }
    else
    {
        result = 1;
        rt_kprintf("IP address: %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    }

    return result;
}
#endif 

int main(void)
{   
    extern struct romfs_dirent romfs_root; 
    dfs_mount(RT_NULL, "/", "rom", 0, &romfs_root);
    
#if defined(BSP_USING_RAMDISK) && defined(BSP_USING_RAMDISK_MOUNT) 
    dfs_mkfs("elm", "ram0"); 
    
    if(dfs_mount("ram0", BSP_USING_RAMDISK_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_RAMDISK_PATH_MOUNT);  
    }
#endif
    
#if defined(BSP_USING_SDCARD_SDIO_BUS) && defined(BSP_USING_SDCARD_MOUNT)
    int result = mmcsd_wait_cd_changed(RT_TICK_PER_SECOND);
    if (result == MMCSD_HOST_PLUGED)
    {
        /* mount sd card fat partition 1 as root directory */
        if (dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) == 0)
        {
            rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT);  
        }
        extern int chdir(const char *path); 
        chdir(BSP_USING_SDCARD_PATH_MOUNT);
    }
    else
    {
        rt_kprintf("sdcard not inserted!\n");
    }
#elif defined(BSP_USING_SDCARD_BLOCK) && defined(BSP_USING_SDCARD_MOUNT)
    if(dfs_mount("sd0", BSP_USING_SDCARD_PATH_MOUNT, "elm", 0, 0) != 0)
    {
        rt_kprintf("sdcard mount '%s' failed.\n", BSP_USING_SDCARD_PATH_MOUNT); 
    }
    
    extern int chdir(const char *path); 
    chdir(BSP_USING_SDCARD_PATH_MOUNT); 
#endif
    
#if defined(RT_USING_LWIP) && defined(BSP_USING_RW00X) && defined(PKG_USING_RW007) 
    int cnt = 1; 
    int point = (-1); 
    rw007_ap_info *ap_info = RT_NULL;
    
    #define SSID_NUM (2)
    const char *ssids[] = 
    {
        "rtthread-ap", "TP-LINK_9D8F"
    }; 
    
    const char *passwords[] = 
    {
        "12345678910", "12345678910"
    }; 
    
    extern void wifi_spi_device_init(const char * device_name);
    wifi_spi_device_init("wspi");
    rt_hw_wifi_init("wspi", MODE_STATION);    

    rt_err_t ret = (-RT_ERROR);
    struct rw007_wifi *wifi = RT_NULL;

    wifi = (struct rw007_wifi *)rt_device_find("w0");

    ret = rt_device_control((rt_device_t)wifi, RW007_CMD_SCAN, RT_NULL);
    if (ret == RT_EOK)
    {
        uint32_t i = 0, j = 0;
        
        for (i = 0; i < wifi->ap_scan_count; i++)
        {
            ap_info = &wifi->ap_scan[i];
            
            for(j = 0; j < SSID_NUM; j++)
            {
                if(rt_strcmp(ap_info->ssid, ssids[j]) == 0) 
                {
                    point = j; 
                    goto _connect;
                }
            }
        }
    }
    
_connect:
    rt_kprintf("\nTry connect SSID: %s ...\n", ap_info->ssid);
    rw007_join(ssids[point], passwords[point]); 
    
    while(!get_wifi_status()) 
    {
        if(cnt++ % (50) == 0)
        {
            rt_kprintf("Try reconnecting...\n");
            rw007_join(ssids[point], passwords[point]); 
        }
        
        rt_thread_mdelay(100); 
    }
    rt_kprintf("Net connect successful!\n");
#endif

#if defined(PKG_USING_PLAYER)
    extern int msh_exec(char *cmd, rt_size_t length);

    #define _cmd2 "lp --play" 	
    msh_exec(_cmd2, rt_strlen(_cmd2)); 	
#endif
        
#ifdef PKG_USING_LUDIO
    rt_audio_control_t control = RT_NULL; 
    struct rt_audio_control_cfg cfg = {"i2c2", 1}; 
    
    control = rt_audio_control_find("CS43L22"); 
    rt_audio_control_load(control, &cfg); 
#endif
}

#if defined(PKG_USING_PLAYER)
static int _player_init(void)
{
    extern int player_system_init(void); 
    player_system_init(); 
    
    return RT_EOK; 
}
INIT_COMPONENT_EXPORT(_player_init); 
#endif

int reboot(void)
{
    NVIC_SystemReset(); 
    return RT_EOK; 
}
MSH_CMD_EXPORT(reboot, reboot board.); 
