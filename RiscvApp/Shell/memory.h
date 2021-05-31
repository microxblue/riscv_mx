//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "common.h"

void WriteMem (UINT32 addr, UINT32 value, int unit);
void ReadMem  (UINT32 addr, UINT32 len, int unit);

#endif