/******************************************************************************
Filename    : rme_platform_XC7Z020.h
Author      : pry
Date        : 24/06/2017
Licence     : LGPL v3+; see COPYING for details.
Description : The configuration file for XC7Z020, with 1MB memory.
              Kernel : 0x80100000 - 0x80EFFFFF.
              Pgreg  : 0x80F00000 - 0x80FFFFFF.
              Kmem1  : 0x81000000 - 0xC0FFFFFF.
              Kmem2  : None.
              Periph : 0xC1000000 - 0xFFFFFFFF.
******************************************************************************/

/* Defines *******************************************************************/
/* The HAL library */
/* The virtual memory start address for the kernel objects */
#define RME_KMEM_VA_START            0x81000000U
/* The size of the kernel object virtual memory - the excess 16MB is for mapping
 * in extra memory, like on-chip SRAM, if they are to be used */
#define RME_KMEM_SIZE                0x40000000U
/* The virtual memory start address for the virtual machines - If no virtual machines is used, set to 0 */
#define RME_HYP_VA_START             0x0U
/* The size of the hypervisor reserved virtual memory */
#define RME_HYP_SIZE                 0x0U
/* The granularity of kernel memory allocation, in bytes */
#define RME_KMEM_SLOT_ORDER          4U
/* Kernel stack size and address */
#define RME_KMEM_STACK_ADDR          (((rme_ptr_t)&__RME_CAV7_Stack_Start)+0x10000U)
/* The maximum number of preemption priority levels in the system.
 * This parameter must be divisible by the word length - 32 is usually sufficient */
#define RME_MAX_PREEMPT_PRIO         32U

/* Memory mapping PA-VA relationships for the page tables. Tailor this to your own
 * memory mappings if your chip have idiosyncrasies. */
#define RME_CAV7_VA2PA(X)            (((rme_ptr_t)(X))-RME_CAV7_VA_BASE)
#define RME_CAV7_PA2VA(X)            (((rme_ptr_t)(X))+RME_CAV7_VA_BASE)
/* Shared interrupt flag region address - always use 256*4 = 1kB memory */
#define RME_CAV7_INT_FLAG_ADDR       0x10008000U
/* Initial kenel object frontier limit */
#define RME_CAV7_KMEM_BOOT_FRONTIER  0x10003000U
/* Init process's first thread's entry point address */
#define RME_CAV7_INIT_ENTRY          0x08004001U
/* Init process's first thread's stack address */
#define RME_CAV7_INIT_STACK          0x1000FFF0U
/* What is the Systick value? in ticks. For XC7Z020, always clocked at 1/2 Fcpu */
#define RME_CAV7_SYSTICK_VAL         (7670000U/2U)

/* Processor type - This can make a huge difference when it comes
 * to timer configurations, as Cortex-A5/9 use private timer and
 * Cortex-A7/15/17 use generic timer. Cortex-A8 doesn't have a timer */
#define RME_CAV7_CPU_TYPE			 RME_CAV7_CPU_CORTEX_A9
/* What is the FPU type? */
#define RME_CAV7_FPU_TYPE            RME_CAV7_FPU_VFPV3U
/* What is the GIC type? */
#define RME_CAV7_GIC_TYPE            RME_CAV7_GIC_V1
/* What is the interrupt priority grouping? */
#define RME_CAV7_GIC_GROUPING        RME_CAV7_GIC_GROUPING_P7S1
/* GIC distributor base address */
#define RME_CAV7_GICD_BASE           0xF8F01000
/* GIC CPU interface base address */
#define RME_CAV7_GICC_BASE           0xF8F00100
/* Private timer and watchdog block base */
#define RME_CAV7_PTWD_BASE           0xF8F00600

/* Because the system memory map of Cortex-A based systems are not decided by
 * a particular standard (unlike x86-64), and they cannot be probed as well
 * (memory probing is dangerous). Additionally, the layout of embedded systems
 * are usually fairly volatile, and there is no good policy to dictate the
 * allocation of anything. As a result, we are forced to write down all the
 * mappings that we wish to make. This also needs to be in accordance with
 * the linker script. */
/* Page table registration table */
#define RME_CAV7_PGREG_START         0x80F00000U
#define REM_CAV7_PGREG_SIZE          0x100000U
/* Kernel memory 1 - creating page tables on these memories is allowed */
#define RME_CAV7_KMEM1_START         0x81000000U
#define RME_CAV7_KMEM1_SIZE          0x40000000U
/* Kernel memory 2 - creating page tables on these memories are not allowed */
#define RME_CAV7_KMEM2_START         0x0U
#define RME_CAV7_KMEM2_SIZE          0x0U

/* Initial page table contents - They must contain only sections and supersections.
 * The initial page table will be generated according to this. Also, the kernel will
 * seek to use supersections when it can. The first word of the table is the length of
 * this section.
 * The user memory mapped in here will become the user portion of the initial process,
 * and the kernel memory mapped in here will become the shared portion of processes.
 * We will map in the kernel entries as supersections whenever we can */
#define RME_CAV7_MEM_ENTRIES        8*4U+1U
#define RME_CAV7_MEM_CONTENTS \
/* Number of entries */ \
  RME_CAV7_MEM_ENTRIES, \
/* Start_PA    Start_VA    Num             Attributes         */ \
  0x01000000, 0x01000000, 0x3F0, RME_CAV7_MMU_1M_PAGE_USER_DEF /* User memory - 1008 MB DDR2 SDRAM identically mapped */, \
  0xFFF00000, 0x00000000, 0x1,   RME_CAV7_MMU_1M_PAGE_USER_DEF /* User memory - 64kB User-OCSRAM */, \
  0x40000000, 0x40000000, 0x200, RME_CAV7_MMU_1M_PAGE_USER_DEF /* User devices - 512MB identical mapping */, \
  0xE0000000, 0x60000000, 0x200, RME_CAV7_MMU_1M_PAGE_USER_DEF /* User devices - 512MB identical mapping */, \
  0x00000000, 0x80000000, 0x400, RME_CAV7_MMU_1M_PAGE_KERN_DEF /* Kernel memory - 1023MB DDR2 SDRAM */, \
  0x00000000, 0xC0000000, 0x1,   RME_CAV7_MMU_1M_PAGE_KERN_DEF /* Kernel memory - 192kB OCSRAM */, \
  0x40100000, 0xC0100000, 0x1FF, RME_CAV7_MMU_1M_PAGE_KERN_SEQ /* Kernel devices - 512MB device space */, \
  0xE0000000, 0xE0000000, 0x200, RME_CAV7_MMU_1M_PAGE_KERN_SEQ /* Kernel devices - 512MB Cortex device space */

/* Kernel functions standard to Cortex-A, interrupt management and power */
#define RME_CAV7_KERN_INT(X)          (X)
#define RME_CAV7_INT_OP               0U
#define RME_CAV7_INT_ENABLE           1U
#define RME_CAV7_INT_DISABLE          0U
#define RME_CAV7_INT_PRIO             1U
#define RME_CAV7_KERN_PWR             240U

/* UART peripheral address */
#define RME_CAV7_UART_CONTROL      (*((volatile rme_ptr_t*)(0xE0001000)))
#define RME_CAV7_UART_MODE         (*((volatile rme_ptr_t*)(0xE0001004)))
#define RME_CAV7_UART_BRGEN        (*((volatile rme_ptr_t*)(0xE0001018)))
#define RME_CAV7_UART_STATUS       (*((volatile rme_ptr_t*)(0xE000102C)))
#define RME_CAV7_UART_FIFO         (*((volatile rme_ptr_t*)(0xE0001030)))
#define RME_CAV7_UART_BRDIV        (*((volatile rme_ptr_t*)(0xE0001034)))
#define RME_CAV7_UART_STATUS_TXE   (1U<<3)

/* Other low-level initialization stuff - The serial port */
#define RME_CAV7_LOW_LEVEL_INIT() \
do \
{ \
	/* Somebody have initialized UART for us so we don't need to do it now */ \
	/* Initialize interrupt controller */ \
	\
} \
while(0)

#define RME_CAV7_SMP_LOW_LEVEL_INIT() \
do \
{ \
	\
} \
while(0)

/* This is for debugging output */
#define RME_CAV7_PUTCHAR(CHAR) \
do \
{ \
    while((RME_CAV7_UART_STATUS&RME_CAV7_UART_STATUS_TXE)==0); \
	RME_CAV7_UART_FIFO=(CHAR); \
} \
while(0)
/* End Defines ***************************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
