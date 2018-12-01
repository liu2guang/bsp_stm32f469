#  STM32F469NI

![STM32F469NI](https://i.imgur.com/4YN7mOR.png)

## 1. 简介

该 BSP 是基于官方的 STM32F469IDISCOVERY探索板 定制开发, 具备以下简要的特性：

| 介绍 | 描述 |
| ---- | ---- |
| 主CPU平台 | ARM Cortex-M4 |
| 最高频率 | 214MHz |
| 内部存储器 | 2MB Flash 384KB+4KB RAM |
| 外部存储器 | 16MB 32bit SDRAM 8MB QSPI |

## 2. 编译说明

STM32F469NI板级包支持MDK5开发环境和GCC编译器，以下是具体版本信息：

| IDE/编译器 | 已测试版本 |
| ---------- | --------- |
| MDK5 | MDK522 |
| GCC | GCC 5.4.1 20160919 (release) |
| IAR | 非常讨厌IAR开发环境, 需要的自己去处理 |

## 3. 驱动支持情况及计划

| 驱动 | 支持情况  |
| :------ | :----  |
| UART | 支持串口1/2/3/6 |
| GPIO | 支持所有GPIO口 |
| IIC | 支持模拟IIC2总线 |
| SPI | 支持SPI1/2, 2支持dma |
| ETH | 不支持 |
| LCD | 支持单framebuffer模式 |
| RTC | 未支持 |
| SDCARD | 支持块设备和SDIO设备驱动 |
| SDRAM | 支持 |
| AUDIO | 支持 |
| USB | 支持 | |
| RAMDISK | 支持 |
| QSPI | 未支持 |
| WDG | 未支持 |
| PWM | 未支持 |

## 5. 联系人信息

维护人：
- [liu2guang](https://github.com/liu2guang)
