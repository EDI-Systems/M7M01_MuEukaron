/******************************************************************************
Filename   : platform_X64_SUPER.h
Author     : pry
Date       : 24/06/2017
Licence    : The Unlicense; see LICENSE for details.
Description: The configuration file for X64 supercomputer profile.
******************************************************************************/

/* Defines *******************************************************************/

/* The granularity of kernel memory allocation, in bytes */
#define RME_KMEM_SLOT_ORDER          4
/* The maximum number of preemption priority levels in the system.
 * This parameter must be divisible by the word length - 64 is usually sufficient */
#define RME_MAX_PREEMPT_PRIO         64

/* Number of CPUs in the system - max. 4096 ones are supported */
#define RME_X64_CPU_NUM              256
/* Number of IOAPICs in the system - max. 256 ones are supported */
#define RME_X64_IOAPIC_NUM           8
/* Shared interrupt flag region address - not populated now */
#define RME_X64_INT_FLAG_ADDR        0x00
/* What is the FPU type? */
#define RME_X64_FPU_TYPE             RME_X64_FPU_AVX512
/* Timer frequency - about 1000 ticks per second */
#define RME_X64_TIMER_FREQ           1000
/* End Defines ***************************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
