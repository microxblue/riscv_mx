//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#ifndef _SHELL_H_
#define _SHELL_H_

#include  "common.h"
#include  "firmware.h"
#include  "memory.h"

#define VGA_DEMO  1

void bootloader(void);

void vga_demo (void);
void raiden_demo (void);

#endif