/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h" 
#include "rtdevice.h" 
#include "board.h" 

#if defined(PKG_USING_PLAYER)

rt_err_t rtquick_audio_init(void)
{
    rt_err_t ret = (RT_EOK); 

    extern int player_system_init(void); 
    player_system_init();
    
    void audio_device_set_volume(int value); 
    audio_device_set_volume(65);

    return ret; 
}
#endif 
