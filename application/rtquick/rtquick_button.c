/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h" 
#include "rtdevice.h" 
#include "board.h" 

#if defined(PKG_USING_PLAYER)
#include "player.h"
#endif 

#if defined(PKG_USING_MULTIBUTTON) 

#include "multi_button.h"

static struct button btn;

#define BUTTON_PIN (48) /* PA0 */ 

static uint8_t button_read_pin(void) 
{
    return !rt_pin_read(BUTTON_PIN); 
}

static void button_callback(void *btn)
{
    uint32_t event; 
    
    event = get_button_event((struct button *)btn); 
    
    switch(event)
    {
        case PRESS_DOWN:
        break; 

        case PRESS_UP: 
        break; 

        case PRESS_REPEAT: 
        break; 

        case SINGLE_CLICK: 
#if defined(PKG_USING_PLAYER)
            player_stop(); 
#endif
        break; 

        case DOUBLE_CLICK: 
        break; 

        case LONG_RRESS_START: 
        break; 

        case LONG_PRESS_HOLD: 
        break; 
    }
}

static void btn_thread_entry(void* p)
{
    while(1)
    {
        rt_thread_mdelay(5); 
        button_ticks(); 
    }
}

rt_err_t rtquick_button_init(void)
{
    rt_thread_t thread = RT_NULL;
    
    /* Create background ticks thread */
    thread = rt_thread_create("button", btn_thread_entry, RT_NULL, 1024, 10, 10);
    if(thread == RT_NULL)
    {
        return RT_ERROR; 
    }
    rt_thread_startup(thread);

    /* low level drive */
    rt_pin_mode  (BUTTON_PIN, PIN_MODE_INPUT); 
    button_init  (&btn, button_read_pin, PIN_LOW);
    button_attach(&btn, PRESS_DOWN,       button_callback);
    button_attach(&btn, PRESS_UP,         button_callback);
    button_attach(&btn, PRESS_REPEAT,     button_callback);
    button_attach(&btn, SINGLE_CLICK,     button_callback);
    button_attach(&btn, DOUBLE_CLICK,     button_callback);
    button_attach(&btn, LONG_RRESS_START, button_callback);
    button_attach(&btn, LONG_PRESS_HOLD,  button_callback);
    button_start (&btn);

    return RT_EOK; 
}

#endif 
