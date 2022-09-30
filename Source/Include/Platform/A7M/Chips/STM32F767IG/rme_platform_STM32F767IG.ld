/******************************************************************************
Filename    : rme_platform_STM32F767IG.ld
Author      : pry
Date        : 24/06/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The scatter file for Cortex-M7 layout. This file is intended 
              to be used with STM32F767IGT6, and the GNU toolchain.
              ROM: 0x08000000 0x00100000
              RAM: 0x20000000 0x00080000
              System ROM layout:
              |0x08000000            0x0800FFFF|0x08010000         0x080FFFFF|
              |<-           Kernel           ->|<-           User          ->|
              System RAM layout:
              |0x20000000            0x20001FFF|0x20002000         0x20003FFF|
              |<-        Kernel Stack        ->|<-        Kernel Data      ->|
              |0x20004000            0x2000FFFF|0x20010000         0x2007FFFF|
              |<-       Kernel Objects       ->|<-           User          ->|
******************************************************************************/

/* Memory Definitions *********************************************************
Description : This section will define the memory layout of the system. 
Component   : ORIGIN - Starting address of the memory region.
              LENGTH - Length of the region.
******************************************************************************/
MEMORY
{
    /* Kernel stack segment */
    KSRAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 4K
    /* Kernel initalized segment */
    KIRAM   (xrw) : ORIGIN = 0x20001000, LENGTH = 8K
    /* Kernel memory segment for kernel object allocations */
    KORAM   (xrw) : ORIGIN = 0x20003000, LENGTH = 52K
    /* Available memory for ths user */
    URAM    (xrw) : ORIGIN = 0x20010000, LENGTH = 384K
    
    /* Kernel flash segment */
    KFLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    /* Init process flash segment */
    IFLASH  (rx)  : ORIGIN = 0x08010000, LENGTH = 64K
    /* Virtual machine flash segment */
    UFLASH  (rx)  : ORIGIN = 0x08020000, LENGTH = 896K
}
/* End Memory Definitions ****************************************************/

/* Stack Definitions *********************************************************/
/* The '__stack' definition is required by crt0, do not remove it. */
__stack = ORIGIN(KSRAM) + LENGTH(KSRAM) - 0x100;
__initial_sp = __stack;
/* End Stack Definitions *****************************************************/

/* Entry Point Definitions ***************************************************/
/* The entry point is informative, for debuggers and simulators,
 * since the Cortex-M vector points to it anyway. */
ENTRY(_start);
SECTIONS
{
/* End Entry Point Definitions ***********************************************/

/* Begin Section:.init_text ***************************************************
Description : The program code for the init process, which goes to flash.
Location    : Flash
Component   : .text - The code segment.
              .rodata.* - The read-only data segment.
******************************************************************************/
.init_text : ALIGN(4)
{
    /* Place the init process here */
    KEEP(*M7M2_Init.o(.text .text.*))
    KEEP(*M7M2_Init.o(.rodata .rodata.* .constdata .constdata.*))
} > IFLASH
/* End Section:.init_text ****************************************************/

/* Begin Section:.vm_text *****************************************************
Description : The program code for all the virtual machines, which goes to flash.
Location    : Flash
Component   : .text - The code segment.
              .rodata.* - The read-only data segment.
******************************************************************************/
.vm_text : ALIGN(4)
{
    /* Place all the virtual machines here */
    KEEP(*VM*.o(.text .text.*))
    KEEP(*VM*.o(.rodata .rodata.* .constdata .constdata.*))
} > UFLASH
/* End Section:.vm_text ******************************************************/

/* Begin Section:.text ********************************************************
Description : The program code is stored in the .text section, which goes to FLASH.
Location    : Flash
Component   : .text - The code segment.
              .rodata.* - The read-only data segment.
******************************************************************************/
.text : ALIGN(4)
{
    /* the startup code */
    KEEP(*rme_platform_a7m_asm_gcc.o(.text .text.*))
    /* All remaining code */
    *(.text .text.*)
    /* Read-only data (constants) */
    *(.rodata .rodata.* .constdata .constdata.*)
} > KFLASH
/* End Section:.text *********************************************************/

/* Begin Section:.data ********************************************************
Description : The main initialized data section. The program executes knowing that
              the data is in the RAM but the loader puts the initial values in the
              FLASH (inidata). It is one task of the startup to copy the initial
              values from FLASH to RAM. The RME kernel does not really rely on the
              data section to be initialized because it never uses preinitialized
              global variables.
Location    : RAM
Component   : .data - The sections to put into the RAM.
******************************************************************************/
/* Used by the startup code to initialise the .data section */
_sidata = LOADADDR(.data);
.data : ALIGN(8192)
{
    FILL(0xFF)
    /* This is used by the startup code to initialise the .data section */
    _sdata = . ;        	/* STM specific definition */
    __data_start__ = . ;
    *(.data_begin .data_begin.*)
    *(.data .data.*)
	*(.data_end .data_end.*)
	. = ALIGN(4);

    /* This is used by the startup code to initialise the .data section */
    _edata = . ;        	/* STM specific definition */
    __data_end__ = . ;
} > KIRAM AT > KFLASH
/* End Section:.data *********************************************************/

/* Begin Section:.bss *********************************************************
Description : The initialised-to-0 data sections. NOLOAD is used to avoid
              the "section `.bss' type changed to PROGBITS" warning. This is the
              main region which is placed in RAM. Actually the RME does not
              initialize its bss because there is no such need.
Location    : RAM
Component   : .bss - The sections to put into the RAM, and initialized to 0.
******************************************************************************/
.bss (NOLOAD) : ALIGN(4)
{
    __bss_start__ = .;     	/* standard newlib definition */
    _sbss = .;              /* STM specific definition */
    *(.bss_begin .bss_begin.*)
    *(.bss .bss.*)
    *(COMMON)
    *(.bss_end .bss_end.*)
	. = ALIGN(4);

    __bss_end__ = .;        /* standard newlib definition */
    _ebss = . ;             /* STM specific definition */
} > KIRAM
/* End Section:.bss **********************************************************/

/* Begin Section:.noinit ******************************************************
Description : The uninitialised data sections. NOLOAD is used to avoid
              the "section `.noinit' type changed to PROGBITS" warning.
Location    : RAM
Component   : .noinit - The sections to put into the RAM, and not initialized.
******************************************************************************/
.noinit (NOLOAD) : ALIGN(4)
{
    _noinit = .;
    *(.noinit .noinit.*) 
    . = ALIGN(4) ;
    _end_noinit = .;   
} > KIRAM
/* Mandatory to be word aligned, _sbrk assumes this */
PROVIDE ( end = _end_noinit ); /* was _ebss */
PROVIDE ( _end = _end_noinit );
PROVIDE ( __end = _end_noinit );
PROVIDE ( __end__ = _end_noinit );
/* End Section:.noinit *******************************************************/

/* End Of File ***************************************************************/
}
/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/

