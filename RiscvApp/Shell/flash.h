//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#ifndef _FLASH_H_
#define _FLASH_H_

#include "common.h"
#include "firmware.h"

INT32 EraseFlash (UINT32 FlashAddr, UINT32 Length);
INT32 WriteFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length);
INT32 ReadFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length);
INT32 ProgramFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length);

#endif
