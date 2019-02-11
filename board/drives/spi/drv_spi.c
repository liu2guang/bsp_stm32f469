/*
 * File      : gpio.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2015, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author            Notes
 * 2017-11-08     ZYH            the first version
 */
#include "board.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

struct stm32_hw_spi_cs
{
    rt_uint32_t pin;
};

struct stm32_spi
{
    SPI_TypeDef *Instance;
    uint32_t idx;
    struct rt_spi_configuration *cfg;
#ifdef BSP_SPI_ENABLE_DMA
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
    struct rt_completion transmit_completion;
#endif
};
static int spi_clock_value = 0;
static int spi_clock(void)
{
    rt_kprintf("SPI Clock: %d\n", spi_clock_value);
    return 0;
}
MSH_CMD_EXPORT(spi_clock, show spi clock);

#ifdef BSP_SPI_ENABLE_DMA
struct spi_dma
{
    DMA_Stream_TypeDef * dma_stream;
    uint32_t channel;
    IRQn_Type irq_n;
};


#define SPI_DMA(dma, stream, channel)   {DMA##dma##_Stream##stream, DMA_CHANNEL_##channel, DMA##dma##_Stream##stream##_IRQn}
#define SPI1_DMA_IRQ_HANDLE DMA2_Stream5_IRQHandler
#define SPI2_DMA_IRQ_HANDLE DMA1_Stream4_IRQHandler
#define SPI3_DMA_IRQ_HANDLE DMA1_Stream5_IRQHandler
static struct spi_dma tx_dma_table[] = 
{
    SPI_DMA(2, 5, 3),
    SPI_DMA(1, 4, 0),
    SPI_DMA(1, 5, 0),
};

static struct spi_dma rx_dma_table[] = 
{
    SPI_DMA(2, 0, 3),
    SPI_DMA(1, 3, 0),
    SPI_DMA(1, 0, 0),
};

#else
#define SPIRXEVENT 0x01
#define SPITXEVENT 0x02
#define SPITIMEOUT 2
#define SPICRCEN 0
#define SPISTEP(datalen) (((datalen) == 8) ? 1 : 2)
#define SPISEND_1(reg, ptr, datalen)       \
    do                                     \
    {                                      \
        if (datalen == 8)                  \
        {                                  \
            (reg) = *(rt_uint8_t *)(ptr);  \
        }                                  \
        else                               \
        {                                  \
            (reg) = *(rt_uint16_t *)(ptr); \
        }                                  \
    } while (0)
#define SPIRECV_1(reg, ptr, datalen)      \
    do                                    \
    {                                     \
        if (datalen == 8)                 \
        {                                 \
            *(rt_uint8_t *)(ptr) = (reg); \
        }                                 \
        else                              \
        {                                 \
            *(rt_uint16_t *)(ptr) = reg;  \
        }                                 \
    } while (0)

static rt_err_t spitxrx1b(struct stm32_spi *hspi, void *rcvb, const void *sndb)
{
    rt_uint32_t padrcv = 0;
    rt_uint32_t padsnd = 0xFF;
    if (!rcvb && !sndb)
    {
        return RT_ERROR;
    }
    if (!rcvb)
    {
        rcvb = &padrcv;
    }
    if (!sndb)
    {
        sndb = &padsnd;
    }
    while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE) == RESET)
        ;
    SPISEND_1(hspi->Instance->DR, sndb, hspi->cfg->data_width);
    while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE) == RESET)
        ;
    SPIRECV_1(hspi->Instance->DR, rcvb, hspi->cfg->data_width);
    return RT_EOK;
}
#endif

struct rt_spi_bus _spi_bus1, _spi_bus2;
struct stm32_spi _spi1, _spi2;

#ifdef BSP_SPI_ENABLE_DMA
void SPI1_DMA_IRQ_HANDLE(void)
{
    HAL_DMA_IRQHandler(&_spi1.hdma_tx);
}

void SPI2_DMA_IRQ_HANDLE(void)
{
    HAL_DMA_IRQHandler(&_spi2.hdma_tx);
}

void spi_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    struct stm32_spi * spi = rt_container_of(hdma, struct stm32_spi, hdma_tx);
    rt_completion_done(&spi->transmit_completion);
}
#endif

static rt_uint32_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
#ifdef BSP_SPI_ENABLE_DMA
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    {
        struct stm32_spi *hspi = (struct stm32_spi *)device->bus->parent.user_data;
        struct stm32_hw_spi_cs *cs = device->parent.user_data;
        static char dummy = 0;
        
        uint32_t send_addr = (uint32_t)&dummy;
        uint32_t recv_addr = (uint32_t)&dummy;
        uint32_t transfer_size = 0;
        uint32_t remain_size = message->length;
        
        dummy = 0;
        /* Config DMA */
        
        hspi->hdma_tx.Instance                 = tx_dma_table[hspi->idx].dma_stream;
        hspi->hdma_tx.Init.Channel             = tx_dma_table[hspi->idx].channel;
        hspi->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        hspi->hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hspi->hdma_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
        hspi->hdma_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
        hspi->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hspi->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        
        if(message->send_buf == RT_NULL)
        {
            hspi->hdma_tx.Init.MemInc          = DMA_MINC_DISABLE;
        }
        else
        {
            hspi->hdma_tx.Init.MemInc          = DMA_MINC_ENABLE;
            send_addr = (uint32_t)message->send_buf;
        }
        
        hspi->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hspi->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hspi->hdma_tx.Init.Mode                = DMA_NORMAL;
        hspi->hdma_tx.Init.Priority            = DMA_PRIORITY_HIGH;
        
        
        hspi->hdma_rx.Instance                 = rx_dma_table[hspi->idx].dma_stream;
        hspi->hdma_rx.Init.Channel             = rx_dma_table[hspi->idx].channel;
        hspi->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
        hspi->hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
        hspi->hdma_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
        hspi->hdma_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
        hspi->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hspi->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        
        if(message->recv_buf == RT_NULL)
        {
            hspi->hdma_rx.Init.MemInc          = DMA_MINC_DISABLE;
        }
        else
        {
            hspi->hdma_rx.Init.MemInc          = DMA_MINC_ENABLE;
            recv_addr = (uint32_t)message->recv_buf;
        }
        
        hspi->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hspi->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hspi->hdma_rx.Init.Mode                = DMA_NORMAL;
        hspi->hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
        
        HAL_DMA_DeInit(&hspi->hdma_tx);
        HAL_DMA_DeInit(&hspi->hdma_rx);
        
        HAL_NVIC_SetPriority(tx_dma_table[hspi->idx].irq_n, 1, 0);
        HAL_NVIC_EnableIRQ  (tx_dma_table[hspi->idx].irq_n);
        /* Transmit Begin */
        
        if (message->cs_take)
        {
            rt_pin_write(cs->pin, 0);
        }
        
        while(remain_size)
        {
            transfer_size = remain_size > 65535 ? 65535 : remain_size;
            
            HAL_DMA_Init(&hspi->hdma_tx);
            HAL_DMA_Init(&hspi->hdma_rx);
            hspi->hdma_tx.XferCpltCallback = spi_XferCpltCallback;
            
            
            rt_completion_init(&hspi->transmit_completion);
            
            HAL_DMA_Start_IT(&hspi->hdma_tx, send_addr, (uint32_t)&hspi->Instance->DR, transfer_size);
            HAL_DMA_Start(&hspi->hdma_rx, (uint32_t)&hspi->Instance->DR, recv_addr, transfer_size);
            
            __HAL_SPI_ENABLE(hspi);
            SET_BIT(hspi->Instance->CR2, SPI_CR2_RXDMAEN);
            SET_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);
            
            rt_completion_wait(&hspi->transmit_completion, RT_WAITING_FOREVER);
            
            CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_RXDMAEN);
            CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);
            remain_size -= transfer_size;
            if(send_addr != (uint32_t)&dummy)
            {
                send_addr += transfer_size;
            }
            
            if(recv_addr != (uint32_t)&dummy)
            {
                recv_addr += transfer_size;
            }
            HAL_DMA_DeInit(&hspi->hdma_rx);
            HAL_DMA_DeInit(&hspi->hdma_tx);
        }
        
        /* Release DMA */
        
        HAL_NVIC_DisableIRQ(tx_dma_table[hspi->idx].irq_n);
        
        if (message->cs_release)
        {
            rt_pin_write(cs->pin, 1);
        }
        __HAL_SPI_DISABLE(hspi);
    }
    return message->length;
}
#else
{
    rt_err_t res;
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    struct stm32_spi *hspi = (struct stm32_spi *)device->bus->parent.user_data;
    struct stm32_hw_spi_cs *cs = device->parent.user_data;
    __HAL_SPI_ENABLE(hspi);
    if (message->cs_take)
    {
        rt_pin_write(cs->pin, 0);
    }
    const rt_uint8_t *sndb = message->send_buf;
    rt_uint8_t *rcvb = message->recv_buf;
    rt_int32_t length = message->length;
    while (length)
    {
        res = spitxrx1b(hspi, rcvb, sndb);
        if (rcvb)
        {
            rcvb += SPISTEP(hspi->cfg->data_width);
        }
        if (sndb)
        {
            sndb += SPISTEP(hspi->cfg->data_width);
        }
        if (res != RT_EOK)
        {
            break;
        }
        length--;
    }
    /* Wait until Busy flag is reset before disabling SPI */
    while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) == SET)
        ;
    if (message->cs_release)
    {
        rt_pin_write(cs->pin, 1);
    }
    __HAL_SPI_DISABLE(hspi);
    return message->length - length;
}
#endif

static rt_err_t stm32_spi_init(SPI_TypeDef *spix, struct rt_spi_configuration *cfg)
{
    SPI_HandleTypeDef hspi;
    hspi.Instance = spix;
    if (cfg->mode & RT_SPI_SLAVE)
    {
        hspi.Init.Mode = SPI_MODE_SLAVE;
    }
    else
    {
        hspi.Init.Mode = SPI_MODE_MASTER;
    }
    if (cfg->mode & RT_SPI_3WIRE)
    {
        hspi.Init.Direction = SPI_DIRECTION_1LINE;
    }
    else
    {
        hspi.Init.Direction = SPI_DIRECTION_2LINES;
    }
    if (cfg->data_width == 8)
    {
        hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    }
    else if (cfg->data_width == 16)
    {
        hspi.Init.DataSize = SPI_DATASIZE_16BIT;
    }
    else
    {
        return RT_EIO;
    }
    if (cfg->mode & RT_SPI_CPHA)
    {
        hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    else
    {
        hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    }
    if (cfg->mode & RT_SPI_CPOL)
    {
        hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
    }
    else
    {
        hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    }
    
    hspi.Init.NSS = SPI_NSS_SOFT;
    
    if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 2)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 2;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 4)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 4;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 8)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 8;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 16)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 16;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 32)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 32;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 64)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 64;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if (cfg->max_hz >= HAL_RCC_GetPCLK2Freq() / 128)
    {
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 128;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    }
    else
    {
        /*  min prescaler 256 */
        spi_clock_value = HAL_RCC_GetPCLK2Freq() / 256;
        hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }
    if (cfg->mode & RT_SPI_MSB)
    {
        hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    }
    else
    {
        hspi.Init.FirstBit = SPI_FIRSTBIT_LSB;
    }
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = 7;
    hspi.State = HAL_SPI_STATE_RESET;
    if (HAL_SPI_Init(&hspi) != HAL_OK)
    {
        return RT_EIO;
    }
    
    return RT_EOK;
}

rt_err_t spi_configure(struct rt_spi_device *device,
                       struct rt_spi_configuration *configuration)
{
    struct stm32_spi *hspi = (struct stm32_spi *)device->bus->parent.user_data;
    hspi->cfg = configuration;
    return stm32_spi_init(hspi->Instance, configuration);
}

const struct rt_spi_ops stm_spi_ops =
{
    .configure = spi_configure,
    .xfer = spixfer,
};


int stm32_spi_register_bus(SPI_TypeDef *SPIx, const char *name)
{
    struct rt_spi_bus *spi_bus;
    struct stm32_spi *spi;
    if (SPIx == SPI1)
    {
        spi_bus = &_spi_bus1;
        spi = &_spi1;
        spi->idx = 0;
    }
    else if (SPIx == SPI2)
    {
        spi_bus = &_spi_bus2;
        spi = &_spi2;
        spi->idx = 1;
    }
    else
    {
        return -1;
    }
    spi->Instance = SPIx;
    
    spi_bus->parent.user_data = spi;
    return rt_spi_bus_register(spi_bus, name, &stm_spi_ops);
}

//cannot be used before completion init
rt_err_t stm32_spi_bus_attach_device(rt_uint32_t pin, const char *bus_name, const char *device_name)
{
    struct rt_spi_device *spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    RT_ASSERT(spi_device != RT_NULL);
    struct stm32_hw_spi_cs *cs_pin = (struct stm32_hw_spi_cs *)rt_malloc(sizeof(struct stm32_hw_spi_cs));
    RT_ASSERT(cs_pin != RT_NULL);
    cs_pin->pin = pin;
    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, 1);
    return rt_spi_bus_attach_device(spi_device, device_name, bus_name, (void *)cs_pin);
}

int stm32_hw_spi_init(void)
{
    int result = 0;
#ifdef BSP_SPI_ENABLE_PORT1
    result = stm32_spi_register_bus(SPI1, "spi1");
#endif
#ifdef BSP_SPI_ENABLE_PORT2
    result = stm32_spi_register_bus(SPI2, "spi2");
#endif
    return result;
}
INIT_BOARD_EXPORT(stm32_hw_spi_init);

void HAL_SPI_MspInit(SPI_HandleTypeDef *spiHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if (spiHandle->Instance == SPI1)
    {
        /* SPI1 clock enable */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
        /**SPI1 GPIO Configuration
        PA5     ------> SPI1_SCK
        PB4     ------> SPI1_MISO
        PB5     ------> SPI1_MOSI
        */
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (spiHandle->Instance == SPI2)
    {
        /* SPI2 clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();
        /**SPI2 GPIO Configuration
        PD3      ------> SPI2_SCK
        PB14     ------> SPI2_MISO
        PB15     ------> SPI2_MOSI
        */
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiHandle)
{
    if (spiHandle->Instance == SPI1)
    {
        /* Peripheral clock disable */
        __HAL_RCC_SPI1_CLK_DISABLE();
        /**SPI1 GPIO Configuration
        PA5     ------> SPI1_SCK
        PB4     ------> SPI1_MISO
        PB5     ------> SPI1_MOSI
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4 | GPIO_PIN_5);
    }
    else if (spiHandle->Instance == SPI2)
    {
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();
        /**SPI2 GPIO Configuration
        PD3      ------> SPI2_SCK
        PB14     ------> SPI2_MISO
        PB15     ------> SPI2_MOSI
        */
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14 | GPIO_PIN_15);
    }
}
