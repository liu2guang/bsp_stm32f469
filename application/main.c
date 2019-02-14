/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtquick.h" 

int main(void)
{   
    rtquick_init();
    
#if defined(PKG_USING_PLAYER)
    extern int kanime_init(void)
    kanime_init(); 
#endif 

    return 0; 
}
