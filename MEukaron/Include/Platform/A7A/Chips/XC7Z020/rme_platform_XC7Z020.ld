/*******************************************************************************
Filename    : rme_platform_XC7Z020.ld
Author      : pry
Date        : 21/08/2018
Licence     : LGPL v3+; see COPYING for details.
Description : The scatter file for Cortex-A9 layout. This file is intended 
              to be used with XC7Z020, and is adapted from the xv6 port.
              SRAM1: 0x00000000 0x00030000
              SRAM2: 0xFFFF0000 0x00010000
              DRAM:  0x00100000 0x3FF00000
******************************************************************************/

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PROVIDE(__va_offset__ = 0x80000000);

SECTIONS
{
  /* the entry point, before enabling paging. The code to enable paing
   needs to have the same virtual/physical address. entry.S and start.c
   run in this initial setting.*/
  . = 0x80100000;

  .text : AT(__va_offset__) {
    *platform_cav7_asm.o(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
    *(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
  }

  PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }

  /* aligned the data to a (4K) page, so it can be assigned
   different protection than the code*/
  . = ALIGN(0x1000);

  .data : {
    *(.data .data.*)
  }

  PROVIDE (edata = .);

  .bss : {  
    PROVIDE (__bss_start__ = .);
    *(.bss .bss.* COMMON)
    PROVIDE (__bss_end__ = .);
  }

  . = ALIGN(0x1000);
  PROVIDE (end = .);
  ASSERT(. < 0x80F00000, "Kernel size exceeded the limit, may need to adjust memory configs")
}

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/



