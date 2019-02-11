/*
 * Copyright (c) 2018-2019, liu2guang Personal 
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h> 
#include "board.h"
 
static struct rt_memheap sdram_heap;

static void SystemClock_Config(void)
{
    HAL_StatusTypeDef ret = HAL_OK;

    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
    
    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLR = 6;

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (ret != HAL_OK)
    {
        while(1); 
    }

    /* Activate the OverDrive to reach the 180 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if (ret != HAL_OK)
    {
        while(1);
    }
    
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDIO | RCC_PERIPHCLK_CLK48;
    PeriphClkInitStruct.SdioClockSelection   = RCC_SDIOCLKSOURCE_CLK48;
    PeriphClkInitStruct.Clk48ClockSelection  = RCC_CK48CLKSOURCE_PLLSAIP;
    PeriphClkInitStruct.PLLSAI.PLLSAIN       = 384;
    PeriphClkInitStruct.PLLSAI.PLLSAIP       = RCC_PLLSAIP_DIV8;

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
    if (ret != HAL_OK)
    {
        while(1);
    }
}

void SysTick_Handler(void)
{
    rt_interrupt_enter();

    HAL_IncTick();
    rt_tick_increase();
    
    rt_interrupt_leave();
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / RT_TICK_PER_SECOND);
    HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0);

    return HAL_OK;
}

void HAL_Delay(__IO uint32_t Delay)
{
    rt_thread_delay(Delay);
}

void HAL_SuspendTick(void)
{
    /* we should not suspend tick */
}

void HAL_ResumeTick(void)
{
    /* we should not resume tick */
}

void HAL_MspInit(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
    HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
    HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

void rt_hw_board_init(void)
{
    /* Hardware floating point support */ 
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

    /* Configure the system clock @ 180 MHz */
    SystemClock_Config(); 
    HAL_Init();

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_HEAP
    rt_system_heap_init(BOARD_HEAP_BEGIN, BOARD_HEAP_END);
    rt_memheap_init(&sdram_heap, "sdram", BOARD_DRAM_BEGIN, BOARD_DRAM_SIZE);
#endif
}
