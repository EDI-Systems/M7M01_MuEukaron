/******************************************************************************
Filename   : platform_TMS320C6678.h
Author     : pry
Date       : 24/06/2017
Licence    : The Unlicense; see LICENSE for details.
Description: The configuration file for TMS320C6678.
******************************************************************************/

/* Defines *******************************************************************/
/* Number of CPUs in the system */
#define RME_C66X_CPU_NUM             8
/* The virtual memory start address for the kernel objects */
#define RME_KMEM_VA_START            0x80100000
/* The size of the kernel object virtual memory */
#define RME_KMEM_SIZE                0x03F00000
/* The virtual memory start address for the virtual machines - If no virtual machines is used, set to 0 */
#define RME_HYP_VA_START             0x84000000
/* The size of the hypervisor reserved virtual memory */
#define RME_HYP_SIZE                 0x000C0000
/* The granularity of kernel memory allocation, in bytes */
#define RME_KMEM_SLOT_ORDER          4
/* Kernel stack size and address */
#define RME_KMEM_STACK_ADDR          ((rme_ptr_t)&(__RME_C66X_Stack[4096]))
/* The maximum number of preemption priority levels in the system.
 * This parameter must be divisible by the word length - 32 is usually sufficient */
#define RME_MAX_PREEMPT_PRIO         32

/* Shared interrupt flag region address - always use 256*4 = 1kB memory */
#define RME_C66X_INT_FLAG_ADDR        0x83FC0000
/* Initial kenel object frontier limit */
#define RME_C66X_KMEM_BOOT_FRONTIER   0x80101000
/* Init process's initial thread entry point address */
#define RME_C66X_INIT_ENTRY           0x84000000
/* What is the tick interval value? - 10ms per tick */
#define RME_C66X_SYSTICK_VAL          1500000
/* What is the event number for tick timer? */
#define RME_C66X_EVT_SYSTICK          (64)
/* Event number definitions - only C66X core generic ones are here.
 * If you need more, just add them accordingly. */
#define RME_C66X_EVT_EVT0             (0) /* Event combiner 0 output */
#define RME_C66X_EVT_EVT1             (1) /* Event combiner 1 output */
#define RME_C66X_EVT_EVT2             (2) /* Event combiner 2 output */
#define RME_C66X_EVT_EVT3             (3) /* Event combiner 3 output */
#define RME_C66X_EVT_IDMA0            (13) /* IDMA channel 0 interrupt */
#define RME_C66X_EVT_IDMA1            (14) /* IDMA channel 1 interrupt */
#define RME_C66X_EVT_INTERR           (96) /* Dropped DSP interrupt event */
#define RME_C66X_EVT_EMC_IDMAERR      (97) /* EMC Invalid IDMA parameters */
#define RME_C66X_EVT_MDMAERREVT       (110) /* L2 MDMA bus error event */
#define RME_C66X_EVT_L1P_ED           (113) /* L1P Single bit error detected during DMA read */
#define RME_C66X_EVT_L2_ED1           (116) /* L2 Corrected bit error detected */
#define RME_C66X_EVT_L2_ED2           (117) /* L2 Uncorrected bit error detected */
#define RME_C66X_EVT_PDC_INT          (118) /* PDC sleep interrupt */
#define RME_C66X_EVT_SYS_CMPA         (119) /* DSP memory protection fault */
#define RME_C66X_EVT_L1P_CMPA         (120) /* L1P DSP memory protection fault */
#define RME_C66X_EVT_L1P_DMPA         (121) /* L1P DMA memory protection fault */
#define RME_C66X_EVT_L1D_CMPA         (122) /* L1D DSP memory protection fault */
#define RME_C66X_EVT_L1D_DMPA         (123) /* L1D DMA memory protection fault */
#define RME_C66X_EVT_L2_CMPA          (124) /* L2 DSP memory protection fault */
#define RME_C66X_EVT_L2_DMPA          (125) /* L2 DMA memory protection fault */
#define RME_C66X_EVT_EMC_CMPA         (126) /* EMC DSP memory protection fault */
#define RME_C66X_EVT_EMC_BUSERR       (127) /* EMC CFG bus error event */

/* Kernel functions standard to Cortex-M, interrupt management and power */
#define RME_C66X_KERN_INT(X)           (X)
#define RME_C66X_INT_OP                0
#define RME_C66X_INT_ENABLE            1
#define RME_C66X_INT_DISABLE           0
#define RME_C66X_INT_PRIO              1
#define RME_C66X_KERN_PWR              240

/* Additional peripherals to be initialized when the system boots. This will only be
 * run once on the booting processor. */
#define RME_C66X_ADDITIONAL_LOW_LEVEL_INIT() \
do \
{ \
} \
while(0)
    
/* This is for hooking some real-time stuff in immediate interrupt handlers */
#define RME_C66X_VECT_HOOK(INT_NUM) \
do \
{ \
} \
while(0)

/* This is good for 1GHz, and 115200 baud; Modify it if you use different settings.
 * Math:Fcorepac/6/16/BAUD=Divisor. The actual useful value maybe +/- 1 to 2. */
#define RME_C66X_UART_BAUD                       (91)
/* This is for debugging output */
#define RME_C66X_PUTCHAR(CHAR) \
do \
{ \
    while((RME_C66X_UART_LSR&RME_C66X_UART_LSR_THRE)==0); \
    RME_C66X_UART_THR=(CHAR); \
} \
while(0)
/* End Defines ***************************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
