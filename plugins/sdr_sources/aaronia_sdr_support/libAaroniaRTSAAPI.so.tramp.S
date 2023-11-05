/*
 * Copyright 2018-2023 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .section .note.GNU-stack,"",@progbits

  .data

  .globl _libAaroniaRTSAAPI_so_tramp_table
  .hidden _libAaroniaRTSAAPI_so_tramp_table
  .align 8
_libAaroniaRTSAAPI_so_tramp_table:
  .zero 144

  .text

  .globl _libAaroniaRTSAAPI_so_tramp_resolve
  .hidden _libAaroniaRTSAAPI_so_tramp_resolve

  .globl _libAaroniaRTSAAPI_so_save_regs_and_resolve
  .hidden _libAaroniaRTSAAPI_so_save_regs_and_resolve
  .type _libAaroniaRTSAAPI_so_save_regs_and_resolve, %function
_libAaroniaRTSAAPI_so_save_regs_and_resolve:
  .cfi_startproc

#define PUSH_REG(reg) pushq %reg ; .cfi_adjust_cfa_offset 8; .cfi_rel_offset reg, 0
#define POP_REG(reg) popq %reg ; .cfi_adjust_cfa_offset -8; .cfi_restore reg

#define DEC_STACK(d) subq $d, %rsp; .cfi_adjust_cfa_offset d
#define INC_STACK(d) addq $d, %rsp; .cfi_adjust_cfa_offset -d

#define PUSH_XMM_REG(reg) DEC_STACK(16); movdqa %reg, (%rsp); .cfi_rel_offset reg, 0
#define POP_XMM_REG(reg) movdqa (%rsp), %reg; .cfi_restore reg; INC_STACK(16)

  // Slow path which calls dlsym, taken only on first call.
  // All registers are stored to handle arbitrary calling conventions
  // (except x87 FPU registers which do not have to be preserved).
  // For Dwarf directives, read https://www.imperialviolet.org/2017/01/18/cfi.html.

  .cfi_def_cfa_offset 8  // Return address

  // FIXME: AVX (YMM, ZMM) registers are NOT saved to simplify code.

  PUSH_REG(rdi)  // 16
  mov 0x10(%rsp), %rdi
  PUSH_REG(rax)
  PUSH_REG(rbx)  // 16
  PUSH_REG(rcx)
  PUSH_REG(rdx)  // 16
  PUSH_REG(rbp)
  PUSH_REG(rsi)  // 16
  PUSH_REG(r8)
  PUSH_REG(r9)  // 16
  PUSH_REG(r10)
  PUSH_REG(r11)  // 16
  PUSH_REG(r12)
  PUSH_REG(r13)  // 16
  PUSH_REG(r14)
  PUSH_REG(r15)  // 16
  PUSH_XMM_REG(xmm0)
  PUSH_XMM_REG(xmm1)
  PUSH_XMM_REG(xmm2)
  PUSH_XMM_REG(xmm3)
  PUSH_XMM_REG(xmm4)
  PUSH_XMM_REG(xmm5)
  PUSH_XMM_REG(xmm6)
  PUSH_XMM_REG(xmm7)

  // Stack is just 8-byte aligned but callee will re-align to 16
  call _libAaroniaRTSAAPI_so_tramp_resolve

  POP_XMM_REG(xmm7)
  POP_XMM_REG(xmm6)
  POP_XMM_REG(xmm5)
  POP_XMM_REG(xmm4)
  POP_XMM_REG(xmm3)
  POP_XMM_REG(xmm2)
  POP_XMM_REG(xmm1)
  POP_XMM_REG(xmm0)  // 16
  POP_REG(r15)
  POP_REG(r14)  // 16
  POP_REG(r13)
  POP_REG(r12)  // 16
  POP_REG(r11)
  POP_REG(r10)  // 16
  POP_REG(r9)
  POP_REG(r8)  // 16
  POP_REG(rsi)
  POP_REG(rbp)  // 16
  POP_REG(rdx)
  POP_REG(rcx)  // 16
  POP_REG(rbx)
  POP_REG(rax)  // 16
  POP_REG(rdi)

  ret

  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_Open
  .p2align 4
  .type AARTSAAPI_Open, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_Open
#endif
AARTSAAPI_Open:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+0(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+0(%rip)
2:
  pushq $0
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_RescanDevices
  .p2align 4
  .type AARTSAAPI_RescanDevices, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_RescanDevices
#endif
AARTSAAPI_RescanDevices:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+8(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+8(%rip)
2:
  pushq $1
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_EnumDevice
  .p2align 4
  .type AARTSAAPI_EnumDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_EnumDevice
#endif
AARTSAAPI_EnumDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+16(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+16(%rip)
2:
  pushq $2
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_Close
  .p2align 4
  .type AARTSAAPI_Close, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_Close
#endif
AARTSAAPI_Close:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+24(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+24(%rip)
2:
  pushq $3
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConfigFind
  .p2align 4
  .type AARTSAAPI_ConfigFind, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConfigFind
#endif
AARTSAAPI_ConfigFind:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+32(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+32(%rip)
2:
  pushq $4
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConfigSetFloat
  .p2align 4
  .type AARTSAAPI_ConfigSetFloat, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConfigSetFloat
#endif
AARTSAAPI_ConfigSetFloat:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+40(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+40(%rip)
2:
  pushq $5
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_Close
  .p2align 4
  .type AARTSAAPI_Close, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_Close
#endif
AARTSAAPI_Close:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+48(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+48(%rip)
2:
  pushq $6
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_CloseDevice
  .p2align 4
  .type AARTSAAPI_CloseDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_CloseDevice
#endif
AARTSAAPI_CloseDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+56(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+56(%rip)
2:
  pushq $7
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_DisconnectDevice
  .p2align 4
  .type AARTSAAPI_DisconnectDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_DisconnectDevice
#endif
AARTSAAPI_DisconnectDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+64(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+64(%rip)
2:
  pushq $8
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_StopDevice
  .p2align 4
  .type AARTSAAPI_StopDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_StopDevice
#endif
AARTSAAPI_StopDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+72(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+72(%rip)
2:
  pushq $9
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_GetPacket
  .p2align 4
  .type AARTSAAPI_GetPacket, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_GetPacket
#endif
AARTSAAPI_GetPacket:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+80(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+80(%rip)
2:
  pushq $10
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_StartDevice
  .p2align 4
  .type AARTSAAPI_StartDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_StartDevice
#endif
AARTSAAPI_StartDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+88(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+88(%rip)
2:
  pushq $11
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConnectDevice
  .p2align 4
  .type AARTSAAPI_ConnectDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConnectDevice
#endif
AARTSAAPI_ConnectDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+96(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+96(%rip)
2:
  pushq $12
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConfigSetString
  .p2align 4
  .type AARTSAAPI_ConfigSetString, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConfigSetString
#endif
AARTSAAPI_ConfigSetString:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+104(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+104(%rip)
2:
  pushq $13
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConfigFind
  .p2align 4
  .type AARTSAAPI_ConfigFind, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConfigFind
#endif
AARTSAAPI_ConfigFind:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+112(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+112(%rip)
2:
  pushq $14
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_ConfigRoot
  .p2align 4
  .type AARTSAAPI_ConfigRoot, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_ConfigRoot
#endif
AARTSAAPI_ConfigRoot:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+120(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+120(%rip)
2:
  pushq $15
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

/*
 * Copyright 2018-2022 Yury Gribov
 *
 * The MIT License (MIT)
 *
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE.txt file.
 */

  .globl AARTSAAPI_OpenDevice
  .p2align 4
  .type AARTSAAPI_OpenDevice, %function
#ifndef IMPLIB_EXPORT_SHIMS
  .hidden AARTSAAPI_OpenDevice
#endif
AARTSAAPI_OpenDevice:
  .cfi_startproc
  .cfi_def_cfa_offset 8  // Return address
  // Intel opt. manual says to
  // "make the fall-through code following a conditional branch be the likely target for a branch with a forward target"
  // to hint static predictor.
  cmpq $0, _libAaroniaRTSAAPI_so_tramp_table+128(%rip)
  je 2f
1:
  jmp *_libAaroniaRTSAAPI_so_tramp_table+128(%rip)
2:
  pushq $16
  .cfi_adjust_cfa_offset 8
  call _libAaroniaRTSAAPI_so_save_regs_and_resolve
  addq $8, %rsp
  .cfi_adjust_cfa_offset -8
  jmp 1b
  .cfi_endproc

