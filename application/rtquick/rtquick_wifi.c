/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h" 
#include "rtdevice.h"

#if defined(BSP_ENABLE_WIFI) 

#include "spi_wifi_rw007.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

const char *ssids[] = 
{
    "rtthread-ap", "TP-LINK_9D8F", "realthread"
}; 

const char *passwords[] = 
{
    "12345678910", "12345678910", "02158995663"
}; 

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

rt_err_t rtquick_wifi_init(void)
{
    rt_err_t ret = RT_EOK; 

    int cnt = 1; 
    int point = (-1); 
    rw007_ap_info *ap_info = RT_NULL;
    
    struct rw007_wifi *wifi = RT_NULL;

    wifi = (struct rw007_wifi *)rt_device_find("w0");

    ret = rt_device_control((rt_device_t)wifi, RW007_CMD_SCAN, RT_NULL);
    if (ret == RT_EOK)
    {
        uint32_t i = 0, j = 0;
        
        for (i = 0; i < wifi->ap_scan_count; i++)
        {
            ap_info = &wifi->ap_scan[i];
            
            for(j = 0; j < (sizeof(ssids)/sizeof(char *)); j++)
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

    return ret; 
}

#endif 
