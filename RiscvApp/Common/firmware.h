// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "common.h"

#define VGA_CONSOLE          0

#define FLASH_SIZE           0x800000
#define FLASH_WR_SIZE        0x100

#define FREQ_US              80
#define FREQ_MS              80000

#define HYRAM_BASE           0xF0000000

// HyperRAM VGA memory
#define VGA_GFX_BASE         0xF07A0000   // Graphics framebuffer

#define VGA_SPR_B_CTL_BASE     0xF07EC000   // Spirit attributes big
#define VGA_SPR_B_CTL_BASE_N(n)  (VGA_SPR_B_CTL_BASE + ((n) << 3))
#define VGA_SPR_S_CTL_BASE     0xF07EC100   // Spirit attributes small
#define VGA_SPR_S_CTL_BASE_N(n)  (VGA_SPR_S_CTL_BASE + ((n) << 3))
#define   VGA_SPR_ID(n)         *(unsigned char *)(VGA_SPR_B_CTL_BASE_N(n)  + 0)
#define   VGA_SPR_CTL(n)        *(unsigned char *)(VGA_SPR_B_CTL_BASE_N(n)  + 3)
#define     VGA_SPR_CTL_EN      BIT7   // 0:disable 1:enable
#define     VGA_SPR_CTL_DIM     BIT6   // 0:16x16   1:8x8
#define     VGA_SPR_CTL_IMG    (BIT4 | BIT5)   // 0~3: Sub-image index (8x8 only)
#define     VGA_SPR_CTL_MIR      BIT0   // 1: Mirror image
#define   VGA_SPR_POSX(n)       *(unsigned short *)(VGA_SPR_B_CTL_BASE_N(n) + 4)
#define   VGA_SPR_POSY(n)       *(unsigned short *)(VGA_SPR_B_CTL_BASE_N(n) + 6)

#define VGA_TXT_BASE         0xF07ED000   // Text buffer
#define VGA_TXT_BASE_PTR     ((unsigned char *)0xF07ED000)

#define VGA_SPR_B_BASE       0xF07F0000   // Spirit bitmap big
#define VGA_SPR_S_BASE       0xF07FC000   // Spirit bitmap small

#define VGA_SPR_BASE         0xF0740000   // Spirit bitmap small, big, huge  (64K each)
#define VGA_TILE_BASE        0xF0770000

// HyperRAM 66WVH8M8ALL
#define HYRAM_ID_0           0xF1000000
#define HYRAM_ID_1           0xF1100000
#define HYRAM_REG_0          0xF1001000
#define HYRAM_REG_1          0xF1101000
#define HYRAM_DIE_2          0x00800000


#define SPI_FLASH_BASE       0xFFFF0100
#define SPI_CMD_OFF          0x00
#define SPI_CTL_OFF          0x04
#define   B_SPI_STS_BUSY       0x80000000
#define   B_SPI_CTL_RUN        0x80000000
#define   B_SPI_CTL_CS_ASSERT    0x00000300
#define   B_SPI_CTL_CS_RELEASE   0x00000100
#define   B_SPI_CTL_CNT        0x000000FF  // 0:4 bytes 1:1 bytes  2:2 bytes  3:3 bytes
#define SPI_DATA_OFF         0x08

#define GPIO_BASE            0xFFFF0200
#define GPIO_BTN_DIP         0xFFFF0210
#define   B_DIP_1                BIT6
#define   B_DIP_2                BIT5
#define   B_DIP_3                BIT4

#define CLK_FREQ_MHZ         80
#define TIMER_BASE           0xFFFF0300
#define TIMER_CNT0           0xFFFF0300
#define TIMER_CNT1           0xFFFF0304
#define TIMER_CNT2           0xFFFF0308
#define TIMER_CNT3           0xFFFF030C
#define TIMER_CNT4           0xFFFF0310
#define TIMER_CNT5           0xFFFF0314

#define TIMER_CNTN           0xFFFF0310
#define TIMER_CNTN_LI        0xFFFF0310
#define TIMER_CNTN_HI        0xFFFF0314

#define SYS_CTRL_BASE        0xFFFF0500
#define SYS_CTRL_JOYSTICK    0xFFFF0500
#define SYS_CTRL_RESET       0xFFFF05FC

#define UART_DAT             0xFFFF0400
#define UART_LSR             0xFFFF0414
#define LSR_TXRDY                  0x20
#define LSR_RXDA                   0x01
#define LSR_ERR_OVR                0x02
#define LSR_ERR_FRM                0x08

#define PS2_DAT              0xFFFF0600
#define PS2_STS              0xFFFF0604
#define   PS2_STS_AVL              0x01

#define DPRAM_BASE           0xFFFF1000
#define DPRAM_DAT            (DPRAM_BASE + 0x000)
#define DPRAM_CMD            (DPRAM_BASE + 0x600)
#define DPRAM_SHELL_CMD      (DPRAM_BASE + 0x7C0)
#define DPRAM_ARG(x)         (DPRAM_CMD + (x) * 4)

#define VGA_CTLBASE          0xFFFF4000
#define   VGA_CTL_EN          BIT0  // Enable screen display
#define   VGA_CTL_FREEZE      BIT2  // Freeze all VGA signals
#define   VGA_CTL_EN_HPC      BIT3  // Enable HyperRAM access
#define   VGA_CTL_CHR8        BIT4  // Use char height 8 instead of 16
#define   VGA_CTL_CHR_2BPP    BIT5  // 2bits per pixel for char
#define   VGA_CTL_TILE_EN     BIT6  // Use char height 8 instead of 16
#define VGA_TXT_START        (VGA_CTLBASE + 1)
#define VGA_INFO              0xFFFF400C
#define VGA_GFX_BUF_BASE      0xFFFF4010
#define VGA_SPR_BUF_BASE      0xFFFF4014
#define VGA_BUF_BASE(n)       (VGA_GFX_BUF_BASE + ((n)<<2))
#define VGA_SPR_OAM(n)        (0xFFFF4400 + ((n)<<2))
#define   VGA_SPR_ATTR_MIR     BIT0
#define   VGA_SPR_ATTR_SUB_IDX (BIT3|BIT2)  // Index
#define   VGA_SPR_ATTR_SIZE    (BIT4|BIT5)  // 11: 64bit  10: 32bit  01:16bit  00:8bit
#define   VGA_SPR_ATTR_SIZE_64    (BIT5|BIT4)
#define   VGA_SPR_ATTR_SIZE_32    (BIT5)
#define   VGA_SPR_ATTR_SIZE_16    (BIT4)
#define   VGA_SPR_ATTR_SIZE_8     (0)
#define   VGA_SPR_ATTR_FRONT   BIT6
#define   VGA_SPR_ATTR_EN      BIT7
#define VGA_SPR_ATTR_SUB_IDX_VAL(x)  (((x) & 3) << 2)
#define SET_VGA_SPR_ATTR(n,i)    MMIO_AND_OR(VGA_SPR_OAM(n), 0xFFFFFF00, ((i)&0xFF)<<0)
#define SET_VGA_SPR_POS_Y(n,i)   MMIO_AND_OR(VGA_SPR_OAM(n), 0xFFFF00FF, ((i)&0xFF)<<8)
#define SET_VGA_SPR_POS_X(n,i)   MMIO_AND_OR(VGA_SPR_OAM(n), 0xFE00FFFF, ((i)&0x1FF)<<16)
#define SET_VGA_SPR_BUF_IDX(n,i) MMIO_AND_OR(VGA_SPR_OAM(n), 0x01FFFFFF, ((i)&0x7F)<<25)
#define SET_VGA_SPR_SUB_IDX(n,i) MMIO_AND_OR(VGA_SPR_OAM(n), 0xFFFFFFF3, ((i)&0x03)<<2)
#define SET_VGA_SPR_MIR(n,i)     MMIO_AND_OR(VGA_SPR_OAM(n), 0xFFFFFFFE, ((i)&0x01)<<0)

#define VGA_CHARBASE         0xFFFF5000


#define GET_RAND(x)        (MMIO(TIMER_CNTN_LI))
#define GET_TSC(Curr)      (Curr.Part.Lo = MMIO(TIMER_CNTN_LI),  Curr.Part.Hi = MMIO(TIMER_CNTN_HI),  Curr.Value)

void  delay (DWORD us);
DWORD tick_to_us (UINT64 Tick);

void  putchar_uart (char ch);

void  putchar_vga (char ch);

uint32_t _picorv32_irq_timer( uint32_t tVal );
uint32_t _picorv32_irq_disable( uint32_t irqsToDisable );
uint32_t _picorv32_irq_enable( uint32_t irqsToEnable );
uint32_t _picorv32_irq_mask( uint32_t iMask );

#endif
