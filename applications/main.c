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

#ifdef BSP_USING_RW00X
#include <spi_wifi_rw007.h>
#endif

#ifdef PKG_USING_LUDIO
#include "audio.h" 
#include "audio_control.h"
#endif

#ifdef RT_USING_LWIP
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
        rt_kprintf("Got IP address : %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    }

    return result;
}
#endif 

int main(void)
{   
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

#ifdef BSP_USING_RW00X
    #define _SSID     ((const char *)"rtthread-ap")
    #define _PASSWORD ((const char *)"12345678910")
    
    extern void wifi_spi_device_init(const char * device_name);
    wifi_spi_device_init("wspi");
    rt_hw_wifi_init("wspi",MODE_STATION);
		
    rw007_join(_SSID, _PASSWORD); 
#endif
    
#ifdef RT_USING_LWIP
    while(!get_wifi_status())
    {
        rt_thread_mdelay(100); 
    }
    rt_kprintf("--------------wifi connect\n");
#endif

#if defined(PKG_USING_PLAYER)
    extern int msh_exec(char *cmd, rt_size_t length);

    #define _cmd1 "player -v 65" 	
    msh_exec(_cmd1, rt_strlen(_cmd1)); 	

    #define _cmd2 "listplayer --play" 	
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
