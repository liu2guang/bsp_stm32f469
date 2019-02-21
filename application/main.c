/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtquick.h" 

int main(void)
{   
    rtquick_init();
    
#if defined(APP_ENABLE_KANIME) && defined(PKG_USING_PLAYER)
    extern int kanime_init(void); 
    kanime_init(); 
#endif 
    
#if defined(APP_ENABLE_KNETEASE) && defined(PKG_USING_PLAYER)
    int knetease_init(void); 
    knetease_init();
#endif

    return 0; 
}
