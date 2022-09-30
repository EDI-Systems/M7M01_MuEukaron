/******************************************************************************
Filename    : rme_platform_cav7.h
Author      : pry
Date        : 01/04/2017
Licence     : LGPL v3+; see COPYING for details.
Description : The header of "rme_platform_cav7.c".
******************************************************************************/

/* Defines *******************************************************************/
#ifdef __HDR_DEFS__
#ifndef __RME_PLATFORM_CAV7_H_DEFS__
#define __RME_PLATFORM_CAV7_H_DEFS__
/*****************************************************************************/
/* Basic Types ***************************************************************/
#ifndef __RME_S32_T__
#define __RME_S32_T__
typedef signed int rme_s32_t;
#endif

#ifndef __RME_S16_T__
#define __RME_S16_T__
typedef signed short rme_s16_t;
#endif

#ifndef __RME_S8_T__
#define __RME_S8_T__
typedef signed char rme_s8_t;
#endif

#ifndef __RME_U32_T__
#define __RME_U32_T__
typedef unsigned int rme_u32_t;
#endif

#ifndef __RME_U16_T__
#define __RME_U16_T__
typedef unsigned short rme_u16_t;
#endif

#ifndef __RME_U8_T__
#define __RME_U8_T__
typedef unsigned char rme_u8_t;
#endif
/* End Basic Types ***********************************************************/

/* Begin Extended Types ******************************************************/
#ifndef __RME_TID_T__
#define __RME_TID_T__
/* The typedef for the Thread ID */
typedef rme_s32_t rme_tid_t;
#endif

#ifndef __RME_PTR_T__
#define __RME_PTR_T__
/* The typedef for the pointers - This is the raw style. Pointers must be unsigned */
typedef rme_u32_t rme_ptr_t;
#endif

#ifndef __RME_CNT_T__
#define __RME_CNT_T__
/* The typedef for the count variables */
typedef rme_s32_t rme_cnt_t;
#endif

#ifndef __RME_CID_T__
#define __RME_CID_T__
/* The typedef for capability ID */
typedef rme_s32_t rme_cid_t;
#endif

#ifndef __RME_RET_T__
#define __RME_RET_T__
/* The type for process return value */
typedef rme_s32_t rme_ret_t;
#endif
/* End Extended Types ********************************************************/

/* System macros *************************************************************/
/* Compiler "extern" keyword setting */
#define EXTERN                          extern
/* Compiler "inline" keyword setting */
#define INLINE                          inline
/* Compiler likely & unlikely setting */
#ifdef likely
#define RME_LIKELY(X)                   (likely(X))
#else
#define RME_LIKELY(X)                   (X)
#endif
#ifdef unlikely
#define RME_UNLIKELY(X)                 (unlikely(X))
#else
#define RME_UNLIKELY(X)                 (X)
#endif
/* CPU-local data structure location macro */
#define RME_CPU_LOCAL()                 (__RME_CAV7_CPU_Local_Get())
/* The order of bits in one CPU machine word */
#define RME_WORD_ORDER                  5
/* Forcing VA=PA in user memory segments */
#define RME_VA_EQU_PA                   (RME_FALSE)
/* Quiescence timeslice value */
#define RME_QUIE_TIME                   0
/* Captbl size limit - not restricted */
#define RME_CAPTBL_LIMIT                0
/* Normal page directory size calculation macro */
#define RME_PGTBL_SIZE_NOM(NUM_ORDER)   (1<<(NUM_ORDER))
/* Top-level page directory size calculation macro */
#define RME_PGTBL_SIZE_TOP(NUM_ORDER)   RME_PGTBL_SIZE_NOM(NUM_ORDER)
/* The kernel object allocation table address - original */
#define RME_KOTBL                       RME_Kotbl
/* Compare-and-Swap(CAS) */
#define RME_COMP_SWAP(PTR,OLD,NEW)      __RME_CAV7_Comp_Swap(PTR,OLD,NEW)
/* Fetch-and-Add(FAA) */
#define RME_FETCH_ADD(PTR,ADDEND)       __RME_CAV7_Fetch_Add(PTR,ADDEND)
/* Fetch-and-And(FAND) */
#define RME_FETCH_AND(PTR,OPERAND)      __RME_CAV7_Fetch_And(PTR,OPERAND)
/* Get most significant bit */
#define RME_MSB_GET(VAL)                __RME_CAV7_MSB_Get(VAL)
/* Read/write barrier both needed on MPCore, because ARM is weakly ordered */
#define RME_READ_ACQUIRE(X)             __RME_CAV7_Read_Acquire(X)
#define RME_WRITE_RELEASE(X,V)          __RME_CAV7_Write_Release(X,V)

/* The CPU and application specific macros are here */
#include "rme_platform_cav7_conf.h"
/* End System macros *********************************************************/

/* Cortex-A specific macros **************************************************/
/* Initial boot capabilities */
/* The capability table of the init process */
#define RME_BOOT_CAPTBL                 0
/* The top-level page table of the init process - always 4GB full range split into 8 pages */
#define RME_BOOT_PGTBL                  1
/* The init process */
#define RME_BOOT_INIT_PROC              2
/* The init thread */
#define RME_BOOT_INIT_THD               3
/* The initial kernel function capability */
#define RME_BOOT_INIT_KERN              4
/* The initial kernel memory capability */
#define RME_BOOT_INIT_KMEM              5
/* The initial timer endpoint */
#define RME_BOOT_INIT_TIMER             6
/* The initial default endpoint for all other interrupts */
#define RME_BOOT_INIT_INT               7

/* Booting capability layout */
#define RME_CAV7_CPT                     ((struct RME_Cap_Captbl*)(RME_KMEM_VA_START))
/* Kernel virtual address base - this is fixed */
#define RME_CAV7_VA_BASE                 (0x80000000U)
/* For Cortex-A:
 * The layout of the page entry is complicated.
 * Refer to ARMv7-AR architecture reference manual (ARM) for details.
 * Physical address extension is NOT supported in RME */
/* Get the actual table positions */
#define RME_CAV7_PGTBL_TBL_NOM(X)        (X)
#define RME_CAV7_PGTBL_TBL_TOP(X)        (X)

/* Cortex-A (ARMv7) */
#define RME_CAV7_PGREG_POS(TABLE)        (((union __RME_CAV7_Pgreg*)RME_CAV7_PGREG_START) \
		                                  [(((rme_ptr_t)(TABLE))-RME_CAV7_VA_BASE)>>RME_PGTBL_SIZE_1K])

/* MMU definitions operation flags, assuming the following changes:
 * TTBCR=0 : TTBR1 not used,
 * SCTLR.AFE=1 : AP[2:1] is the permission flags, AP[0] is now the access flag.
 * DACR=0x55555555 : All pages/tables are client and access permissions always checked.
 * SCTLR.TRE=1 : TEX remap engaged, CACHEABLE and BUFFERABLE works as RME defined.
 *               {MSB TEX[0], C, B LSB} will index PRRR and NMRR.
 * PRRR=0b 00 00 00 00 00 00 10 10 00 00 00 00 10 10 01 00=0x000A00A4
 *         OUTER_SHARE PTBL_DECIDE xx xx xx xx CB C- -B --
 *      In PRRR configuration, shareability is decided by the page table. This
 *      facilitates cache-incoherent user-level systems. Also, if the memory
 *      is shareable, then it is both inner and outer shareable.
 * NMRR=0b 00 00 00 00 01 10 11 00 00 00 00 00  01 10 11 00=0x006C006C
 *         xx xx xx xx CB C- -B -- xx xx xx xx  CB C- -B --
 * -- : Non-cacheable non-bufferable - strongly ordered, caching not allowed at all
 * -B : Non-cacheable bufferable - device, write-back without write-allocate (since
 *      reads will still have to load words from memory)
 * C- : Cacheable non-bufferable - normal, write-through without write allocate
 * CB : Cacheable bufferable - normal memory, write-back, write-allocate */
#define RME_CAV7_MMU_1M_PGDIR_PRESENT    (0x02U)
#define RME_CAV7_MMU_1M_PGDIR_NOTSECURE  (1U<<3U)
#define RME_CAV7_MMU_1M_PAGE_PRESENT     (0x01U)
#define RME_CAV7_MMU_1M_BUFFERABLE       (1U<<2U)
#define RME_CAV7_MMU_1M_CACHEABLE        (1U<<3U)
#define RME_CAV7_MMU_1M_EXECUTENEVER     (1U<<4U)
#define RME_CAV7_MMU_1M_USER             (1U<<11U)
#define RME_CAV7_MMU_1M_READONLY         (1U<<15U)
#define RME_CAV7_MMU_1M_SHAREABLE        (1U<<16U)
#define RME_CAV7_MMU_1M_NOTGLOBAL        (1U<<17U)
#define RME_CAV7_MMU_1M_NOTSECURE        (1U<<19U)

#define RME_CAV7_MMU_1M_PAGE_USER_COMMON (RME_CAV7_MMU_1M_PAGE_PRESENT|RME_CAV7_MMU_1M_USER| \
		                                  RME_CAV7_MMU_1M_SHAREABLE|RME_CAV7_MMU_1M_NOTGLOBAL)
#define RME_CAV7_MMU_1M_PAGE_KERN_COMMON (RME_CAV7_MMU_1M_PAGE_PRESENT|RME_CAV7_MMU_1M_SHAREABLE|RME_CAV7_MMU_1M_NOTGLOBAL)

/* These definitions are only used by the initial page table */
#define RME_CAV7_MMU_1M_PAGE_USER_DEF    (RME_CAV7_MMU_1M_PAGE_USER_COMMON|RME_CAV7_MMU_1M_BUFFERABLE|RME_CAV7_MMU_1M_CACHEABLE)
#define RME_CAV7_MMU_1M_PAGE_KERN_DEF    (RME_CAV7_MMU_1M_PAGE_KERN_COMMON|RME_CAV7_MMU_1M_BUFFERABLE|RME_CAV7_MMU_1M_CACHEABLE)
#define RME_CAV7_MMU_1M_PAGE_KERN_DEV    (RME_CAV7_MMU_1M_PAGE_KERN_COMMON|RME_CAV7_MMU_1M_BUFFERABLE)
#define RME_CAV7_MMU_1M_PAGE_KERN_SEQ    (RME_CAV7_MMU_1M_PAGE_KERN_COMMON)

#define RME_CAV7_MMU_4K_EXECUTENEVER     (1U<<0U)
#define RME_CAV7_MMU_4K_PAGE_PRESENT     (1U<<1U)
#define RME_CAV7_MMU_4K_BUFFERABLE       (1U<<2U)
#define RME_CAV7_MMU_4K_CACHEABLE        (1U<<3U)
#define RME_CAV7_MMU_4K_USER             (1U<<5U)
#define RME_CAV7_MMU_4K_READONLY         (1U<<9U)
#define RME_CAV7_MMU_4K_SHAREABLE        (1U<<10U)
#define RME_CAV7_MMU_4K_NOTGLOBAL        (1U<<11U)

#define RME_CAV7_MMU_4K_PAGE_USER_COMMON (RME_CAV7_MMU_4K_PAGE_PRESENT|RME_CAV7_MMU_4K_USER| \
                                          RME_CAV7_MMU_4K_SHAREABLE|RME_CAV7_MMU_4K_NOTGLOBAL)

#define RME_CAV7_MMU_4G_PGTBL_ADDR(X)    ((X)&0xFFFFC000U)
#define RME_CAV7_MMU_1M_PGTBL_ADDR(X)    ((X)&0xFFFFFC00U)
#define RME_CAV7_MMU_1M_PAGE_ADDR(X)     ((X)&0xFFF00000U)
#define RME_CAV7_MMU_4K_PAGE_ADDR(X)     ((X)&0xFFFFF000U)

#define RME_CAV7_PGFLG_1M_RME2NAT(X)     (RME_CAV7_Pgflg_1M_RME2NAT[X])
#define RME_CAV7_PGFLG_1M_PREPROC(X)     ((((X)&RME_CAV7_MMU_1M_READONLY)>>12)| \
                                          (((X)&RME_CAV7_MMU_1M_EXECUTENEVER)>>2)| \
		                                  (((X)&RME_CAV7_MMU_1M_CACHEABLE)>>2)| \
										  (((X)&RME_CAV7_MMU_1M_BUFFERABLE)>>2))
#define RME_CAV7_PGFLG_1M_NAT2RME(X)     (RME_CAV7_Pgflg_1M_NAT2RME[RME_CAV7_PGFLG_1M_PREPROC(X)])

#define RME_CAV7_PGFLG_4K_RME2NAT(X)     (RME_CAV7_Pgflg_4K_RME2NAT[X])
#define RME_CAV7_PGFLG_4K_PREPROC(X)     ((((X)&RME_CAV7_MMU_4K_READONLY)>>6)| \
		                                  (((X)&RME_CAV7_MMU_4K_CACHEABLE)>>1)| \
		                                  (((X)&RME_CAV7_MMU_4K_BUFFERABLE)>>1)| \
										  (((X)&RME_CAV7_MMU_4K_EXECUTENEVER)>>0))
#define RME_CAV7_PGFLG_4K_NAT2RME(X)     (RME_CAV7_Pgflg_4K_NAT2RME[RME_CAV7_PGFLG_4K_PREPROC(X)])

/* Processor type definitions */
#define RME_CAV7_CPU_CORTEX_A5           (0)
#define RME_CAV7_CPU_CORTEX_A7           (1)
#define RME_CAV7_CPU_CORTEX_A8           (2)
#define RME_CAV7_CPU_CORTEX_A9           (3)
#define RME_CAV7_CPU_CORTEX_A15          (4)
#define RME_CAV7_CPU_CORTEX_A17          (5)

/* FPU type definitions */
#define RME_CAV7_FPU_NONE                (0)
#define RME_CAV7_FPU_VFPV3               (1)
#define RME_CAV7_FPU_VFPV3U              (2)
#define RME_CAV7_FPU_VFPV3_HPE           (3)
#define RME_CAV7_FPU_VFPV4               (4)
#define RME_CAV7_FPU_VFPV5               (5)

/* GIC type definitions */
#define RME_CAV7_GIC_V1                  (0)
#define RME_CAV7_GIC_V2                  (1)

/* MPU definitions */
#define RME_CAV7_MPU_PRIVDEF             0x00000004

#define RME_CAV7_SFR(BASE,OFFSET)        (*((volatile rme_ptr_t*)((rme_ptr_t)((BASE)+(OFFSET)))))
/* GIC definitions - This is only present in MPCore platform. Most Cortex-A9s,
 * all Cortex-A7s and Cortex-A5s are on the multicore platform */
/* Distributor control register */
#define RME_CAV7_GICD_CTLR               RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0000)
#define RME_CAV7_GICD_CTLR_GRP1EN        (1U<<1U)
#define RME_CAV7_GICD_CTLR_GRP0EN        (1U<<0U)
/* Interrupt controller type register */
#define RME_CAV7_GICD_TYPER              RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0004)
/* Distributor implementer identification register */
#define RME_CAV7_GICD_IIDR               RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0008)
/* Interrupt group registers */
#define RME_CAV7_GICD_IGROUPR(X)         RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0080+(X)*4) /* 0-31 */
/* Interrupt enabling registers */
#define RME_CAV7_GICD_ISENABLER(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0100+(X)*4) /* 0-31 */
/* Interrupt disabling registers */
#define RME_CAV7_GICD_ICENABLER(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0180+(X)*4) /* 0-31 */
/* Interrupt set-pending registers */
#define RME_CAV7_GICD_ISPENDR(X)         RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0200+(X)*4) /* 0-31 */
/* Interrupt clear-pending registers */
#define RME_CAV7_GICD_ICPENDR(X)         RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0280+(X)*4) /* 0-31 */
/* Interrupt set-active registers */
#define RME_CAV7_GICD_ISACTIVER(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0300+(X)*4) /* 0-31 */
/* Interrupt clear-active registers */
#define RME_CAV7_GICD_ICACTIVER(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0380+(X)*4) /* 0-31 */
/* Interrupt priority registers */
#define RME_CAV7_GICD_IPRIORITYR(X)      RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0400+(X)*4) /* 0-254 */
/* Interrupt processor targets registers */
#define RME_CAV7_GICD_ITARGETSR(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0800+(X)*4) /* 0-7 */
/* Interrupt configuration registers */
#define RME_CAV7_GICD_ICFGR(X)           RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0C00+(X)*4) /* 0-63 */
/* Non-secure access control registers */
#define RME_CAV7_GICD_NSACR(X)           RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0E00+(X)*4) /* 0-63 */
/* Software generated interrupt register */
#define RME_CAV7_GICD_SGIR               RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0F00)
/* SGI clear-pending registers */
#define RME_CAV7_GICD_CPENDSGIR(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0F10+(X)*4) /* 0-3 */
/* SGI set-pending registers */
#define RME_CAV7_GICD_SPENDSGIR(X)       RME_CAV7_SFR(RME_CAV7_GICD_BASE,0x0F20+(X)*4) /* 0-3 */

/* CPU interface control register */
#define RME_CAV7_GICC_CTLR               RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0000)
/* Interrupt priority mask register */
#define RME_CAV7_GICC_PMR                RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0004)
/* Binary pointer register */
#define RME_CAV7_GICC_BPR                RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0004)
/* Interrupt acknowledge register */
#define RME_CAV7_GICC_IAR                RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x000C)
/* End of interrupt register */
#define RME_CAV7_GICC_EOIR               RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0010)
/* Running priority register */
#define RME_CAV7_GICC_RPR                RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0014)
/* Highest priority pending interrupt register */
#define RME_CAV7_GICC_HPPIR              RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0018)
/* Aliased binary point register */
#define RME_CAV7_GICC_ABPR               RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x001C)
/* Aliased interrupt acknowledge register */
#define RME_CAV7_GICC_AIAR               RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0020)
/* Aliased end of interrupt register */
#define RME_CAV7_GICC_AEOIR              RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0024)
/* Aliased highest priority pending interrupt register */
#define RME_CAV7_GICC_AHPPIR             RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x0028)
/* Active priorities registers */
#define RME_CAV7_GICC_APR(X)             RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x00D0+(X)*4) /* 0-3 */
/* Non-secure active priorities registers */
#define RME_CAV7_GICC_NSAPR(X)           RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x00E0+(X)*4) /* 0-3 */
/* CPU interface identification register */
#define RME_CAV7_GICC_IIDR               RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x00FC)
/* Deactivate interrupt register */
#define RME_CAV7_GICC_DIR                RME_CAV7_SFR(RME_CAV7_GICC_BASE,0x1000)

/* Control register */
#define RME_CAV7_GICC_CBPR               (1U<<4U)
#define RME_CAV7_GICC_FIQEN              (1U<<3U)
#define RME_CAV7_GICC_ACKCTL             (1U<<2U)
#define RME_CAV7_GICC_ENABLEGRP1         (1U<<1U)
#define RME_CAV7_GICC_ENABLEGRP0         (1U<<0U)

/* Priority grouping */
#define RME_CAV7_GIC_GROUPING_P7S1       (0U)
#define RME_CAV7_GIC_GROUPING_P6S2       (1U)
#define RME_CAV7_GIC_GROUPING_P5S3       (2U)
#define RME_CAV7_GIC_GROUPING_P4S4       (3U)
#define RME_CAV7_GIC_GROUPING_P3S5       (4U)
#define RME_CAV7_GIC_GROUPING_P2S6       (5U)
#define RME_CAV7_GIC_GROUPING_P1S7       (6U)
#define RME_CAV7_GIC_GROUPING_P0S8       (7U)

/* Timer definitions */
/* Private timer load register */
#define RME_CAV7_PTWD_PTLR               RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0000)
/* Private timer counter register */
#define RME_CAV7_PTWD_PTCNTR             RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0004)
/* Private timer control register */
#define RME_CAV7_PTWD_PTCTLR             RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0008)
#define RME_CAV7_PTWD_PTCTLR_PRESC(X)    ((X)<<8U)
#define RME_CAV7_PTWD_PTCTLR_IRQEN       (1U<<2U)
#define RME_CAV7_PTWD_PTCTLR_AUTOREL     (1U<<1U)
#define RME_CAV7_PTWD_PTCTLR_TIMEN       (1U<<0U)
/* Private timer interrupt status register */
#define RME_CAV7_PTWD_PTISR              RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x000C)

/* Watchdog load register */
#define RME_CAV7_PTWD_WDLR               RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0020)
/* Watchdog counter register */
#define RME_CAV7_PTWD_WDCNTR             RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0024)
/* Watchdog control register */
#define RME_CAV7_PTWD_WDCTLR             RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0028)
/* Watchdog interrupt status register */
#define RME_CAV7_PTWD_WDISR              RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x002C)
/* Watchdog reset status register */
#define RME_CAV7_PTWD_WDRSR              RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0030)
/* Watchdog disable register */
#define RME_CAV7_PTWD_WDDR               RME_CAV7_SFR(RME_CAV7_PTWD_BASE,0x0034)

/* Fault definitions */
/* These faults cannot be recovered and will lead to termination immediately */
#define RME_CAV7_FAULT_FATAL             (0)
/*****************************************************************************/
/* __RME_PLATFORM_CAV7_H_DEFS__ */
#endif
/* __HDR_DEFS__ */
#endif
/* End Defines ***************************************************************/

/* Structs *******************************************************************/
#ifdef __HDR_STRUCTS__
#ifndef __RME_PLATFORM_CAV7_H_STRUCTS__
#define __RME_PLATFORM_CAV7_H_STRUCTS__
/* We used structs in the header */

/* Use defines in these headers */
#define __HDR_DEFS__
#undef __HDR_DEFS__
/*****************************************************************************/
/* The register set struct - R0-R3, R12, PC, LR, xPSR is automatically pushed.
 * Here we need LR to decide EXC_RETURN, that's why it is here */
struct RME_Reg_Struct
{
    rme_ptr_t CPSR;
    rme_ptr_t R0;
    rme_ptr_t R1;
    rme_ptr_t R2;
    rme_ptr_t R3;
    rme_ptr_t R4;
    rme_ptr_t R5;
    rme_ptr_t R6;
    rme_ptr_t R7;
    rme_ptr_t R8;
    rme_ptr_t R9;
    rme_ptr_t R10;
    rme_ptr_t R11;
    rme_ptr_t SP;
    rme_ptr_t LR;
    rme_ptr_t PC;
};

/* The coprocessor register set structure. In Cortex-M, if there is a 
 * single-precision FPU, then the FPU S0-S15 is automatically pushed */
struct RME_Cop_Struct
{
    rme_ptr_t F0;
    rme_ptr_t F1;
    rme_ptr_t F2;
    rme_ptr_t F3;
    rme_ptr_t F4;
    rme_ptr_t F5;
    rme_ptr_t F6;
    rme_ptr_t F7;
    rme_ptr_t FPS;
};

/* The registers to keep to remember where to return after an invocation */
struct RME_Iret_Struct
{
    rme_ptr_t PC;
    rme_ptr_t SP;
};

/* Memory information - the layout is (offset from VA base):
 * |----------16MB|-----|-----|-----|-----|
 * |Kernel&Globals|Kotbl|Pgreg|Kmem1|Kmem2|
 *  Kernel&Globals : Initial kernel text segment and all static variables.
 *                   Also includes per-CPU variables and all other stuff.
 *  Kotbl          : Kernel object registration table.
 *  Pgreg          : Page table registration table.
 *  Kmem1          : Kernel memory 1, linear mapping, allow creation of page tables.
 *  Kmem2          : Kernel memory 2, non-linear mapping, allow creation of all other stuff.
 *  All values are in bytes, and are virtual addresses. If running the kernel or creating
 *  page tables on on-chip SRAM is desired, then the VA2PA and PA2VA macros must consider
 *  that issue properly, and the Kmem1 description also needs to take care of that. */
struct RME_CAV7_Mem_Layout
{
	rme_ptr_t Kotbl_Start;
	rme_ptr_t Kotbl_Size;

	rme_ptr_t Pgreg_Start;
	rme_ptr_t Pgreg_Size;

	rme_ptr_t Kmem1_Start;
	rme_ptr_t Kmem1_Size;

	rme_ptr_t Kmem2_Start;
	rme_ptr_t Kmem2_Size;
};

/* Interrupt flags - this type of flags will only appear on MPU-based systems */
struct __RME_CAV7_Flag_Set
{
    rme_ptr_t Lock;
    rme_ptr_t Group;
    rme_ptr_t Flags[32];
};

struct __RME_CAV7_Flags
{
    struct __RME_CAV7_Flag_Set Set0;
    struct __RME_CAV7_Flag_Set Set1;
};

/* Page table registration table - actually we can take advantage of the fact that
 * ARM page tables never have more than 16384 childs, and if the page have a child,
 * then there must be no parent (because there are only 2 levels). Also, this can save
 * us some memory, and shred down the pgreg memory consumption to 1/256 of the total */
union __RME_CAV7_Pgreg
{
	/* What is the ASID of this page table? - This can be set through kernel function caps */
	rme_ptr_t ASID_Child_Cnt;
    /* How many parent page tables does this page table have? */
	rme_ptr_t Parent_Cnt;
};
/*****************************************************************************/
/* __RME_PLATFORM_CAV7_H_STRUCTS__ */
#endif
/* __HDR_STRUCTS__ */
#endif
/* End Structs ***************************************************************/

/* Private Global Variables **************************************************/
#if(!(defined __HDR_DEFS__||defined __HDR_STRUCTS__))
#ifndef __RME_PLATFORM_CAV7_MEMBERS__
#define __RME_PLATFORM_CAV7_MEMBERS__

/* In this way we can use the data structures and definitions in the headers */
#define __HDR_DEFS__

#undef __HDR_DEFS__

#define __HDR_STRUCTS__

#undef __HDR_STRUCTS__

/* If the header is not used in the public mode */
#ifndef __HDR_PUBLIC_MEMBERS__
/*****************************************************************************/
/* Translate the flags into Cortex-A specific ones - the STATIC bit will never be
 * set thus no need to consider about it here. The flag bits order is shown below:
 * [MSB                                                                                         LSB]
 * RME_PGTBL_BUFFERABLE | RME_PGTBL_CACHEABLE | RME_PGTBL_EXECUTE | RME_PGTBL_WRITE | RME_PGTBL_READ
 * The C snippet to generate this (gcc x64):

#include <stdio.h>

#define CAV7_1M_PRESENT      (0x01U)
#define CAV7_1M_USER         (1U<<11U)
#define CAV7_1M_SHAREABLE    (1U<<16U)
#define CAV7_1M_NOTGLOBAL    (1U<<17U)

#define CAV7_1M_EXECUTENEVER (1U<<4U)
#define CAV7_1M_READONLY     (1U<<15U)
#define CAV7_1M_BUFFERABLE   (1U<<2U)
#define CAV7_1M_CACHEABLE    (1U<<3U)

#define CAV7_1M_COMMON       (CAV7_1M_PRESENT|CAV7_1M_USER|CAV7_1M_SHAREABLE|CAV7_1M_NOTGLOBAL)

#define RME_READ             (1<<0)
#define RME_WRITE            (1<<1)
#define RME_EXECUTE          (1<<2)
#define RME_CACHEABLE        (1<<3)
#define RME_BUFFERABLE       (1<<4)

int main(void)
{
	unsigned long result;
	int count;
	for(count=0;count<32;count++)
	{
	    result=CAV7_1M_COMMON;
		if((count&RME_WRITE)==0)
			result|=CAV7_1M_READONLY;
		if((count&RME_EXECUTE)==0)
			result|=CAV7_1M_EXECUTENEVER;
		if((count&RME_CACHEABLE)!=0)
			result|=CAV7_1M_CACHEABLE;
		if((count&RME_BUFFERABLE)!=0)
			result|=CAV7_1M_BUFFERABLE;
	    printf("0x%08lX,",result);
	    if(count%4==3)
	    	printf("\n");
	}
	return 0;
} */
static const rme_ptr_t RME_CAV7_Pgflg_1M_RME2NAT[32]=
{
	0x00038811,0x00038811,0x00030811,0x00030811,
	0x00038801,0x00038801,0x00030801,0x00030801,
	0x00038819,0x00038819,0x00030819,0x00030819,
	0x00038809,0x00038809,0x00030809,0x00030809,
	0x00038815,0x00038815,0x00030815,0x00030815,
	0x00038805,0x00038805,0x00030805,0x00030805,
	0x0003881D,0x0003881D,0x0003081D,0x0003081D,
	0x0003880D,0x0003880D,0x0003080D,0x0003080D
};

/* Translate the flags into Cortex-A specific ones - the STATIC bit will never be
 * set thus no need to consider about it here. The flag bits order is shown below:
 * [MSB                                                                                         LSB]
 * RME_PGTBL_BUFFERABLE | RME_PGTBL_CACHEABLE | RME_PGTBL_EXECUTE | RME_PGTBL_WRITE | RME_PGTBL_READ
 * The C snippet to generate this (gcc x64):

#include <stdio.h>

#define CAV7_4K_PRESENT      (1U<<1U)
#define CAV7_4K_USER         (1U<<5U)
#define CAV7_4K_SHAREABLE    (1U<<10U)
#define CAV7_4K_NOTGLOBAL    (1U<<11U)

#define CAV7_4K_EXECUTENEVER (1U<<0U)
#define CAV7_4K_READONLY     (1U<<9U)
#define CAV7_4K_BUFFERABLE   (1U<<2U)
#define CAV7_4K_CACHEABLE    (1U<<3U)

#define CAV7_4K_COMMON       (CAV7_4K_PRESENT|CAV7_4K_USER|CAV7_4K_SHAREABLE|CAV7_4K_NOTGLOBAL)

#define RME_READ             (1<<0)
#define RME_WRITE            (1<<1)
#define RME_EXECUTE          (1<<2)
#define RME_CACHEABLE        (1<<3)
#define RME_BUFFERABLE       (1<<4)

int main(void)
{
	unsigned long result;
	int count;
	for(count=0;count<32;count++)
	{
	    result=CAV7_4K_COMMON;
		if((count&RME_WRITE)==0)
			result|=CAV7_4K_READONLY;
		if((count&RME_EXECUTE)==0)
			result|=CAV7_4K_EXECUTENEVER;
		if((count&RME_CACHEABLE)!=0)
			result|=CAV7_4K_CACHEABLE;
		if((count&RME_BUFFERABLE)!=0)
			result|=CAV7_4K_BUFFERABLE;
	    printf("0x%08lX,",result);
	    if(count%4==3)
	    	printf("\n");
	}
	return 0;
} */
static const rme_ptr_t RME_CAV7_Pgflg_4K_RME2NAT[32]=
{
	0x00000E23,0x00000E23,0x00000C23,0x00000C23,
	0x00000E22,0x00000E22,0x00000C22,0x00000C22,
	0x00000E2B,0x00000E2B,0x00000C2B,0x00000C2B,
	0x00000E2A,0x00000E2A,0x00000C2A,0x00000C2A,
	0x00000E27,0x00000E27,0x00000C27,0x00000C27,
	0x00000E26,0x00000E26,0x00000C26,0x00000C26,
	0x00000E2F,0x00000E2F,0x00000C2F,0x00000C2F,
	0x00000E2E,0x00000E2E,0x00000C2E,0x00000C2E
};

/* Translate the 1M flags back to RME format. In order to use this table, it is needed to extract the
 * Cortex-A bits: [15](READONLY) [4](EXECUTENEVER) [3](CACHEABLE) [2](BUFFERABLE). The C snippet to
 * generate this (gcc x64):

#include <stdio.h>
#define CAV7_1M_READONLY     (1U<<3U)
#define CAV7_1M_EXECUTENEVER (1U<<2U)
#define CAV7_1M_CACHEABLE    (1U<<1U)
#define CAV7_1M_BUFFERABLE   (1U<<0U)

#define RME_READ             (1U<<0U)
#define RME_WRITE            (1U<<1U)
#define RME_EXECUTE          (1U<<2U)
#define RME_CACHEABLE        (1U<<3U)
#define RME_BUFFERABLE       (1U<<4U)
#define RME_STATIC           (1U<<5U)

int main(void)
{
    unsigned long int flag;
    int count;
    for(count=0;count<16;count++)
    {
        flag=RME_READ;
        if((count&CAV7_1M_EXECUTENEVER)==0)
            flag|=RME_EXECUTE;
        if((count&CAV7_1M_CACHEABLE)!=0)
            flag|=RME_CACHEABLE;
        if((count&CAV7_1M_BUFFERABLE)!=0)
            flag|=RME_BUFFERABLE;
        if((count&CAV7_1M_READONLY)==0)
            flag|=RME_WRITE;
        printf("0x%08lX,",flag);
	    if(count%4==3)
	    	printf("\n");
    }
} */
static const rme_ptr_t RME_CAV7_Pgflg_1M_NAT2RME[16]=
{
	0x00000007,0x00000017,0x0000000F,0x0000001F,
	0x00000003,0x00000013,0x0000000B,0x0000001B,
	0x00000005,0x00000015,0x0000000D,0x0000001D,
	0x00000001,0x00000011,0x00000009,0x00000019
};

/* Translate the 4K flags back to RME format. In order to use this table, it is needed to extract the
 * Cortex-A bits: [9](READONLY) [3](CACHEABLE) [2](BUFFERABLE) [0](EXECUTENEVER). The C snippet to
 * generate this (gcc x64):

#include <stdio.h>
#define CAV7_4K_READONLY     (1U<<3U)
#define CAV7_4K_CACHEABLE    (1U<<2U)
#define CAV7_4K_BUFFERABLE   (1U<<1U)
#define CAV7_4K_EXECUTENEVER (1U<<0U)

#define RME_READ             (1U<<0U)
#define RME_WRITE            (1U<<1U)
#define RME_EXECUTE          (1U<<2U)
#define RME_CACHEABLE        (1U<<3U)
#define RME_BUFFERABLE       (1U<<4U)
#define RME_STATIC           (1U<<5U)

int main(void)
{
    unsigned long int flag;
    int count;
    for(count=0;count<16;count++)
    {
        flag=RME_READ;
        if((count&CAV7_4K_EXECUTENEVER)==0)
            flag|=RME_EXECUTE;
        if((count&CAV7_4K_CACHEABLE)!=0)
            flag|=RME_CACHEABLE;
        if((count&CAV7_4K_BUFFERABLE)!=0)
            flag|=RME_BUFFERABLE;
        if((count&CAV7_4K_READONLY)==0)
            flag|=RME_WRITE;
        printf("0x%08lX,",flag);
	    if(count%4==3)
	    	printf("\n");
    }
} */
static const rme_ptr_t RME_CAV7_Pgflg_4K_NAT2RME[16]=
{
	0x00000007,0x00000003,0x00000017,0x00000013,
	0x0000000F,0x0000000B,0x0000001F,0x0000001B,
	0x00000005,0x00000001,0x00000015,0x00000011,
	0x0000000D,0x00000009,0x0000001D,0x00000019
};
/*****************************************************************************/
/* End Private Global Variables **********************************************/

/* Private C Function Prototypes *********************************************/ 
/*****************************************************************************/
static void __RME_CAV7_Int_Init(void);
static void __RME_CAV7_Timer_Init(void);
/*****************************************************************************/
#define __EXTERN__
/* End Private C Function Prototypes *****************************************/

/* Public Global Variables ***************************************************/
/* __HDR_PUBLIC_MEMBERS__ */
#else
#define __EXTERN__ EXTERN 
/* __HDR_PUBLIC_MEMBERS__ */
#endif

/*****************************************************************************/
/* Cortex-A9 can have up to 4 cores. We hard-code it here */
__EXTERN__ struct RME_CPU_Local RME_CAV7_Local[4];
/* The memory layout of this chip */
__EXTERN__ const rme_ptr_t RME_CAV7_Mem_Info[RME_CAV7_MEM_ENTRIES];
/* The start of the contiguous stack area for all processors */
EXTERN rme_ptr_t __RME_CAV7_Stack_Start;
/* The CPU initialization counter */
__EXTERN__ rme_ptr_t RME_CAV7_CPU_Cnt;
/* The initial page table */
EXTERN rme_ptr_t __RME_CAV7_Kern_Pgtbl;
/* The interrupt vector */
EXTERN rme_ptr_t __RME_CAV7_Vector_Table;
/* The memory layout struct - currently not used */
/* __EXTERN__ struct RME_CAV7_Mem_Layout RME_CAV7_Layout; */
/*****************************************************************************/

/* End Public Global Variables ***********************************************/

/* Public C Function Prototypes **********************************************/
/*****************************************************************************/
__EXTERN__ void Test(void);
/* Cortex-A (ARMv7) register reads */
EXTERN rme_ptr_t __RME_CAV7_CPSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_SPSR_Get(void);
/* C0 */
EXTERN rme_ptr_t __RME_CAV7_MIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CTR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TCMTR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TLBTR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_MPIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_REVIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_PFR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_PFR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_DFR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_AFR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_MMFR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_MMFR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_MMFR2_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_MMFR3_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR2_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR3_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR4_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_ISAR5_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_CCSIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_CLIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_AIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_CSSELR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_VPIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ID_VMPIDR_Get(void);
/* C1 */
EXTERN rme_ptr_t __RME_CAV7_SCTLR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ACTLR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CPACR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_SCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_SDER_Get(void);
EXTERN rme_ptr_t __RME_CAV7_NSACR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HSCTLR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HACTLR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HDCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HCPTR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HSTR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HACR_Get(void);
/* C2 */
EXTERN rme_ptr_t __RME_CAV7_TTBR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TTBR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TTBCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HTCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_VTCR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_DACR_Get(void);
/* C5 */
EXTERN rme_ptr_t __RME_CAV7_DFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_IFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ADFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_AIFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HADFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HAIFSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HSR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_DFAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_IFAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HDFAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HIFAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HPFAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_PAR_Get(void);
/* C10 */
EXTERN rme_ptr_t __RME_CAV7_TLBLR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_PRRR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_NMRR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_AMAIR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_AMAIR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HMAIR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HMAIR1_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HAMAIR0_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HAMAIR1_Get(void);
/* C12 */
EXTERN rme_ptr_t __RME_CAV7_VBAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_MVBAR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_ISR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HVBAR_Get(void);
/* C13 */
EXTERN rme_ptr_t __RME_CAV7_FCSEIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CONTEXTIDR_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TPIDRURW_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TPIDRURO_Get(void);
EXTERN rme_ptr_t __RME_CAV7_TPIDRPRW_Get(void);
EXTERN rme_ptr_t __RME_CAV7_HTPIDR_Get(void);
/* C14 */
EXTERN rme_ptr_t __RME_CAV7_CNTFRQ_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTKCTL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTP_TVAL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTP_CTL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTV_TVAL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTV_CTL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTHCTL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTHP_TVAL_Get(void);
EXTERN rme_ptr_t __RME_CAV7_CNTHP_CTL_Get(void);
/* Double words */
EXTERN void __RME_CAV7_CNTPCT_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);
EXTERN void __RME_CAV7_CNTVCT_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);
EXTERN void __RME_CAV7_CNTP_CVAL_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);
EXTERN void __RME_CAV7_CNTV_CVAL_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);
EXTERN void __RME_CAV7_CNTVOFF_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);
EXTERN void __RME_CAV7_CNTHP_CVAL_DW_Get(rme_ptr_t* Low, rme_ptr_t* High);

/* Cortex-A (ARMv7) register writes */
EXTERN void __RME_CAV7_CPSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_SPSR_Set(rme_ptr_t Val);
/* C0 */
EXTERN void __RME_CAV7_ID_CSSELR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ID_VPIDR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ID_VMPIDR_Set(rme_ptr_t Val);
/* C1 */
EXTERN void __RME_CAV7_SCTLR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ACTLR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CPACR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_SCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_SDER_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_NSACR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HSCTLR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HACTLR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HDCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HCPTR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HSTR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HACR_Set(rme_ptr_t Val);
/* C2,C3 */
EXTERN void __RME_CAV7_TTBR0_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TTBR1_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TTBCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HTCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_VTCR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DACR_Set(rme_ptr_t Val);
/* C5 */
EXTERN void __RME_CAV7_DFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_IFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ADFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_AIFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HADFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HAIFSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HSR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DFAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_IFAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HDFAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HIFAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HPFAR_Set(rme_ptr_t Val);
/* C7 */
EXTERN void __RME_CAV7_ICIALLUIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_BPIALLIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_PAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ICIALLU_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ICIMVAU_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CP15ISB_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_BPIALL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_BPIMVA_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCIMVAC_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCISW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1CPR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1CPW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1CUR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1CUW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS12NSOPR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS12NSOPW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS12NSOUR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS12NSOUW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCCMVAC_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCCSW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CP15DSB_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CP15DMB_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCCMVAU_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCCIMVAC_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DCCISW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1HR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ATS1HW_Set(rme_ptr_t Val);
/* C8 */
EXTERN void __RME_CAV7_TLBIALLIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVAIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIASIDIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVAAIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ITLBIALL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ITLBIMVA_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_ITLBIASID_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DTLBIALL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DTLBIMVA_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_DTLBIASID_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIALL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVA_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIASID_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVAA_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIALLHIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVAHIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIALLNSNHIS_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIALLH_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIMVAH_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TLBIALLNSNH_Set(rme_ptr_t Val);
/* C10 */
EXTERN void __RME_CAV7_TLBLR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_PRRR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_NMRR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_AMAIR0_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_AMAIR1_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HMAIR0_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HMAIR1_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HAMAIR0_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HAMAIR1_Set(rme_ptr_t Val);
/* C12 */
EXTERN void __RME_CAV7_VBAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_MVBAR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HVBAR_Set(rme_ptr_t Val);
/* C13 */
EXTERN void __RME_CAV7_CONTEXTIDR_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TPIDRURW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TPIDRURO_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_TPIDRPRW_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_HTPIDR_Set(rme_ptr_t Val);
/* C14 */
EXTERN void __RME_CAV7_CNTFRQ_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTKCTL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTP_TVAL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTP_CTL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTV_TVAL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTV_CTL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTHCTL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTHP_TVAL_Set(rme_ptr_t Val);
EXTERN void __RME_CAV7_CNTHP_CTL_Set(rme_ptr_t Val);
/* Double words */
EXTERN void __RME_CAV7_CNTP_CVAL_DW_Set(rme_ptr_t Low, rme_ptr_t High);
EXTERN void __RME_CAV7_CNTV_CVAL_DW_Set(rme_ptr_t Low, rme_ptr_t High);
EXTERN void __RME_CAV7_CNTVOFF_DW_Set(rme_ptr_t Low, rme_ptr_t High);
EXTERN void __RME_CAV7_CNTHP_CVAL_DW_Set(rme_ptr_t Low, rme_ptr_t High);

/* Handlers */
__EXTERN__ void __RME_CAV7_Undefined_Handler(struct RME_Reg_Struct* Reg);
__EXTERN__ void __RME_CAV7_Prefetch_Abort_Handler(struct RME_Reg_Struct* Reg);
__EXTERN__ void __RME_CAV7_Data_Abort_Handler(struct RME_Reg_Struct* Reg);
__EXTERN__ void __RME_CAV7_IRQ_Handler(struct RME_Reg_Struct* Reg);
__EXTERN__ void _RME_CAV7_SGI_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t CPUID, rme_ptr_t Int_ID);
/* Interrupts */
EXTERN void __RME_Disable_Int(void);
EXTERN void __RME_Enable_Int(void);
EXTERN void __RME_CAV7_Wait_Int(void);
/* Atomics */
__EXTERN__ rme_ptr_t __RME_CAV7_Comp_Swap(rme_ptr_t* Ptr, rme_ptr_t Old, rme_ptr_t New);
__EXTERN__ rme_ptr_t __RME_CAV7_Fetch_Add(rme_ptr_t* Ptr, rme_cnt_t Addend);
__EXTERN__ rme_ptr_t __RME_CAV7_Fetch_And(rme_ptr_t* Ptr, rme_ptr_t Operand);
/* Memory barriers */
EXTERN rme_ptr_t __RME_CAV7_Read_Acquire(rme_ptr_t* Ptr);
EXTERN void __RME_CAV7_Write_Release(rme_ptr_t* Ptr, rme_ptr_t Val);
/* MSB counting */
EXTERN rme_ptr_t __RME_CAV7_MSB_Get(rme_ptr_t Val);
/* Debugging */
__EXTERN__ rme_ptr_t __RME_Putchar(char Char);
/* Coprocessor */
EXTERN void ___RME_CAV7_Thd_Cop_Save(struct RME_Cop_Struct* Cop_Reg);
EXTERN void ___RME_CAV7_Thd_Cop_Restore(struct RME_Cop_Struct* Cop_Reg);
/* Booting */
EXTERN void _RME_Kmain(rme_ptr_t Stack);
EXTERN void __RME_Enter_User_Mode(rme_ptr_t Entry_Addr, rme_ptr_t Stack_Addr, rme_ptr_t CPUID);
__EXTERN__ rme_ptr_t __RME_Low_Level_Init(void);
__EXTERN__ rme_ptr_t __RME_Boot(void);
__EXTERN__ void __RME_Reboot(void);
__EXTERN__ void __RME_Shutdown(void);
/* Syscall & invocation */
__EXTERN__ struct RME_CPU_Local* __RME_CAV7_CPU_Local_Get(void);
__EXTERN__ void __RME_Get_Syscall_Param(struct RME_Reg_Struct* Reg, rme_ptr_t* Svc,
                                         rme_ptr_t* Capid, rme_ptr_t* Param);
__EXTERN__ void __RME_Set_Syscall_Retval(struct RME_Reg_Struct* Reg, rme_ret_t Retval);
/* Thread register sets */
__EXTERN__ void __RME_Thd_Reg_Init(rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Param, struct RME_Reg_Struct* Reg);
__EXTERN__ void __RME_Thd_Reg_Copy(struct RME_Reg_Struct* Dst, struct RME_Reg_Struct* Src);
__EXTERN__ void __RME_Thd_Cop_Init(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg);
__EXTERN__ void __RME_Thd_Cop_Save(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg);
__EXTERN__ void __RME_Thd_Cop_Restore(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg);
/* Invocation register sets */
__EXTERN__ void __RME_Inv_Reg_Save(struct RME_Iret_Struct* Ret, struct RME_Reg_Struct* Reg);
__EXTERN__ void __RME_Inv_Reg_Restore(struct RME_Reg_Struct* Reg, struct RME_Iret_Struct* Ret);
__EXTERN__ void __RME_Set_Inv_Retval(struct RME_Reg_Struct* Reg, rme_ret_t Retval);
/* Kernel function handler */
__EXTERN__ rme_ptr_t __RME_Kern_Func_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Func_ID, 
                                             rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2);
/* Fault handler */
__EXTERN__ void __RME_CAV7_Fault_Handler(struct RME_Reg_Struct* Reg);
/* Generic interrupt handler */
__EXTERN__ void __RME_CAV7_Generic_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Int_Num);
/* Page table operations */
__EXTERN__ void __RME_Pgtbl_Set(rme_ptr_t Pgtbl);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Kmem_Init(void);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Check(rme_ptr_t Start_Addr, rme_ptr_t Top_Flag, rme_ptr_t Size_Order, rme_ptr_t Num_Order, rme_ptr_t Vaddr);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Init(struct RME_Cap_Pgtbl* Pgtbl_Op);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Del_Check(struct RME_Cap_Pgtbl* Pgtbl_Op);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Page_Map(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Page_Unmap(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Pgdir_Map(struct RME_Cap_Pgtbl* Pgtbl_Parent, rme_ptr_t Pos, 
                                           struct RME_Cap_Pgtbl* Pgtbl_Child, rme_ptr_t Flags);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Pgdir_Unmap(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Lookup(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos, rme_ptr_t* Paddr, rme_ptr_t* Flags);
__EXTERN__ rme_ptr_t __RME_Pgtbl_Walk(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Vaddr, rme_ptr_t* Pgtbl,
                                      rme_ptr_t* Map_Vaddr, rme_ptr_t* Paddr, rme_ptr_t* Size_Order, rme_ptr_t* Num_Order, rme_ptr_t* Flags);
/*****************************************************************************/
/* Undefine "__EXTERN__" to avoid redefinition */
#undef __EXTERN__
/* __RME_PLATFORM_CAV7_MEMBERS__ */
#endif
/* !(defined __HDR_DEFS__||defined __HDR_STRUCTS__) */
#endif
/* End Public C Function Prototypes ******************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
