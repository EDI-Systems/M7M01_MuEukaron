/******************************************************************************
Filename   : rme_platform_c66x.h
Author     : pry
Date       : 01/04/2017
Licence     : The Unlicense; see LICENSE for details.
Description: The header of "platform_C66X.h".
******************************************************************************/

/* Defines *******************************************************************/
#ifdef __HDR_DEFS__
#ifndef __RME_PLATFORM_C66X_H_DEFS__
#define __RME_PLATFORM_C66X_H_DEFS__
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
/* CPU-local data structure */
#define RME_CPU_LOCAL()                 (&(RME_C66X_CPU_Local[__RME_C66X_CPUID_Get()]))
/* The order of bits in one CPU machine word */
#define RME_WORD_ORDER                  5
/* Forcing VA=PA in user memory segments */
#define RME_VA_EQU_PA                   (RME_TRUE)
/* Quiescence timeslice value */
#define RME_QUIE_TIME                   0
/* Captbl size limit - not restricted */
#define RME_CAPTBL_LIMIT                0
/* Normal page directory size calculation macro */
#define RME_PGTBL_SIZE_NOM(NUM_ORDER)   ((1<<(NUM_ORDER))*sizeof(rme_ptr_t)+sizeof(struct __RME_C66X_Pgtbl_Meta))
/* Top-level page directory size calculation macro */
#define RME_PGTBL_SIZE_TOP(NUM_ORDER)   (RME_PGTBL_SIZE_NOM(NUM_ORDER)+sizeof(struct __RME_C66X_MMU_Data))
/* The kernel object allocation table address - original */
#define RME_KOTBL                       RME_Kotbl
/* Compare-and-Swap(CAS) */
#define RME_COMP_SWAP(PTR,OLD,NEW)      __RME_C66X_Comp_Swap(PTR,OLD,NEW)
/* Fetch-and-Add(FAA) */
#define RME_FETCH_ADD(PTR,ADDEND)       __RME_C66X_Fetch_Add(PTR,ADDEND)
/* Fetch-and-And(FAND) */
#define RME_FETCH_AND(PTR,OPERAND)      __RME_C66X_Fetch_And(PTR,OPERAND)
/* Get most significant bit */
#define RME_MSB_GET(VAL)                __RME_C66X_MSB_Get(VAL)
/* Read barrier on C66X - C66X only guarantees read-write ordering, thus this is required */
#define RME_READ_ACQUIRE(X)             __RME_C66X_Read_Acquire(X)
/* Write barrier on C66X - C66X only guarantees read-write ordering, thus this is required */
#define RME_WRITE_RELEASE(X,V)          __RME_C66X_Write_Release(X,V)

/* The CPU and application specific macros are here */
#include "rme_platform_c66x_conf.h"
/* End System macros *********************************************************/

/* C66X specific macros ******************************************************/
/* Initial boot capabilities */
/* The capability table of the init process */
#define RME_BOOT_CAPTBL                 0
/* The top-level page table of the init process - an array */
#define RME_BOOT_INIT_PGTBL             1
/* The init process */
#define RME_BOOT_INIT_PROC              2
/* The init thread - this is a per-core array */
#define RME_BOOT_TBL_THD                3
/* The initial kernel function capability */
#define RME_BOOT_INIT_KERN              4
/* The initial kernel memory capability */
#define RME_BOOT_INIT_KMEM              5
/* The initial timer endpoint - this is a per-core array */
#define RME_BOOT_TBL_TIMER              6
/* The initial default endpoint for all other interrupts - this is a per-core array */
#define RME_BOOT_TBL_INT                7

/* Booting capability layout */
#define RME_C66X_CPT                    (Captbl)
/* For C66X:
 * The layout of the page entry is:
 * [31:12] Paddr - The physical address to map this page to, or the physical
 *                 address of the next layer of page table. This address is
 *                 always aligned to 4kB.
 * [7] Static - Is this page a static page?
 * [6:5] Reserved - The cacheability flags are not fit for C6000 as it uses cache registers.
 *                  We will implement cacheability as kernel functions.
 * [4] Execute - Do we allow execution(instruction fetch) on this page?
 * [3] Write - Is this page user writable?
 * [2] Read - Is this page user readable?
 * [1] Terminal - Is this page a terminal page, or points to another page table?
 * [0] Present - Is this entry present?
 *
 * The layout of a directory entry is:
 * [31:2] Paddr - The in-kernel physical address of the lower page directory.
 * [1] Terminal - Is this page a terminal page, or points to another page table?
 * [0] Present - Is this entry present?
 */
/* Get the actual table positions */
#define RME_C66X_PGTBL_TBL_NOM(X)       ((X)+(sizeof(struct __RME_C66X_Pgtbl_Meta)/sizeof(rme_ptr_t)))
#define RME_C66X_PGTBL_TBL_TOP(X)       ((X)+(sizeof(struct __RME_C66X_Pgtbl_Meta)+sizeof(struct __RME_C66X_MMU_Data))/sizeof(rme_ptr_t))

/* Page entry bit definitions */
#define RME_C66X_PGTBL_PRESENT          (1<<0)
#define RME_C66X_PGTBL_TERMINAL         (1<<1)
#define RME_C66X_PGTBL_READONLY         (1<<2)
#define RME_C66X_PGTBL_READWRITE        (1<<3)
#define RME_C66X_PGTBL_EXECUTE          (1<<4)
#define RME_C66X_PGTBL_STATIC           (1<<7)
/* The address mask for the actual page address */
#define RME_C66X_PGTBL_PTE_ADDR(X)      RME_ROUND_DOWN(X,12)
/* The address mask for the next level page table address */
#define RME_C66X_PGTBL_PGD_ADDR(X)      ((X)&0xFFFFFFFC)
/* Extract flags from the table itself */
#define RME_C66X_PGTBL_FLAG(X)          (((X)&0x0000000F)>>2)
/* Page table metadata definitions */
#define RME_C66X_PGTBL_SIZEORD(X)       ((X)>>16)
#define RME_C66X_PGTBL_NUMORD(X)        ((X)&0x0000FFFF)
/* Extract address for MMU */
#define RME_C66X_PGTBL_MPAXH_VA(X)      ((X)&0xFFFFF000)
#define RME_C66X_PGTBL_MPAXL_PA(X)      (((X)&0xFFFFF000)<<4)
/* Get info from MMU */
#define RME_C66X_PGTBL_MPAXL_SZORD(X)   ((((X)&0x3F)>>1)-2)

/* Special function register definitions */
#define RME_C66X_SFR(BASE,OFFSET)       (*((volatile rme_ptr_t*)((rme_ptr_t)((BASE)+(OFFSET)))))

/* XMC base */
#define RME_C66X_XMC                    (0x08000000)
/* C66X XMC definitions */
#define RME_C66X_XMC_XMPAXL(X)          RME_C66X_SFR(RME_C66X_XMC,(X)<<3)
#define RME_C66X_XMC_XMPAXH(X)          RME_C66X_SFR(RME_C66X_XMC,0x0004+((X)<<3))
#define RME_C66X_XMC_XMPFAR             RME_C66X_SFR(RME_C66X_XMC,0x0200)
#define RME_C66X_XMC_XMPFSR             RME_C66X_SFR(RME_C66X_XMC,0x0204)
#define RME_C66X_XMC_XMPFCR             RME_C66X_SFR(RME_C66X_XMC,0x0208)
#define RME_C66X_XMC_MDMAARBX           RME_C66X_SFR(RME_C66X_XMC,0x0280)
#define RME_C66X_XMC_XPFCMD             RME_C66X_SFR(RME_C66X_XMC,0x0300)
#define RME_C66X_XMC_XPFACS             RME_C66X_SFR(RME_C66X_XMC,0x0304)
#define RME_C66X_XMC_XPFAC(X)           RME_C66X_SFR(RME_C66X_XMC,0x0310+((X)<<2))
#define RME_C66X_XMC_XPFADDR(X)         RME_C66X_SFR(RME_C66X_XMC,0x0400+((X)<<2))
/* MPAX locks */
#define RME_C66X_XMC_LOCK               (0x0184AD00)
#define RME_C66X_XMC_MPLK(X)            RME_C66X_SFR(RME_C66X_XMC_LOCK,(X)<<2) /* 0-3 */
#define RME_C66X_XMC_MPLKCMD            RME_C66X_SFR(RME_C66X_XMC_LOCK,0x0010)
#define RME_C66X_XMC_MPLKSTAT           RME_C66X_SFR(RME_C66X_XMC_LOCK,0x0014)
/* Number of regions */
#define RME_C66X_XMC_REGIONS            (16)
/* XMC bit fields */
#define RME_C66X_XMC_XMPAXH_SIZE(X)     ((X)-1)
#define RME_C66X_XMC_XMPAXH_DISABLE     (0)
#define RME_C66X_XMC_XMPAXL_SR          (1<<5)
#define RME_C66X_XMC_XMPAXL_SW          (1<<4)
#define RME_C66X_XMC_XMPAXL_SX          (1<<3)
#define RME_C66X_XMC_XMPAXL_UR          (1<<2)
#define RME_C66X_XMC_XMPAXL_UW          (1<<1)
#define RME_C66X_XMC_XMPAXL_UX          (1<<0)
#define RME_C66X_XMC_XMPAXL_FLAG(FLAG)  (((FLAG)&RME_PGTBL_READ)<<2)| \
                                         ((FLAG)&RME_PGTBL_WRITE)| \
                                         (((FLAG)&RME_PGTBL_EXECUTE)>>2)
#define RME_C66X_XMC_XMPFSR_LOCAL       (1<<8)
#define RME_C66X_XMC_XMPFCR_MPFCLR      (1<<0)
/* Lock bitfields */
#define RME_C66X_XMC_MPLK0_KEY          (0xBFBFBFBF)
#define RME_C66X_XMC_MPLK1_KEY          (0x0000FE29)
#define RME_C66X_XMC_MPLK2_KEY          (0x00000003)
#define RME_C66X_XMC_MPLK3_KEY          (0x00000004)
#define RME_C66X_XMC_MPLKCMD_KEYR       (1<<2)
#define RME_C66X_XMC_MPLKCMD_LOCK       (1<<1)
#define RME_C66X_XMC_MPLKCMD_UNLOCK     (1<<0)
#define RME_C66X_XMC_MPLKSTAT_LK        (1<<0)
/* Convert RME representation to machine representation */
#define RME_C66X_XMC_XMPAXH_V(VA,SIZE)  ((VA)|RME_C66X_XMC_XMPAXH_SIZE(SIZE))
#define RME_C66X_XMC_XMPAXL_V(PA,FLAG)  (((PA)>>4)|RME_C66X_XMC_XMPAXL_FLAG(FLAG))
/* Default values for region 0 & 1 (kernel) */
#define RME_C66X_XMC_XMPAXL0_DEF        (RME_C66X_XMC_XMPAXL_SR|RME_C66X_XMC_XMPAXL_SW|RME_C66X_XMC_XMPAXL_SX)
#define RME_C66X_XMC_XMPAXH0_DEF        (RME_C66X_XMC_XMPAXH_SIZE(RME_PGTBL_SIZE_2G))
#define RME_C66X_XMC_XMPAXL1_DEF        ((0x80000000UL)|RME_C66X_XMC_XMPAXL_SR|RME_C66X_XMC_XMPAXL_SW|RME_C66X_XMC_XMPAXL_SX)
#define RME_C66X_XMC_XMPAXH1_DEF        ((0x80000000UL)|RME_C66X_XMC_XMPAXH_SIZE(RME_PGTBL_SIZE_2G))

/* Cache controller base */
#define RME_C66X_CACHE                  (0x01840000)
/* C66X level 1 program cache definitions */
#define RME_C66X_CACHE_L1PCFG           RME_C66X_SFR(RME_C66X_CACHE,0x0020)
#define RME_C66X_CACHE_L1PCC            RME_C66X_SFR(RME_C66X_CACHE,0x0024)
#define RME_C66X_CACHE_L1PIBAR          RME_C66X_SFR(RME_C66X_CACHE,0x4020)
#define RME_C66X_CACHE_L1PIWC           RME_C66X_SFR(RME_C66X_CACHE,0x4024)
#define RME_C66X_CACHE_L1PINV           RME_C66X_SFR(RME_C66X_CACHE,0x5028)
/* C66X level 1 data cache definitions */
#define RME_C66X_CACHE_L1DCFG           RME_C66X_SFR(RME_C66X_CACHE,0x0040)
#define RME_C66X_CACHE_L1DCC            RME_C66X_SFR(RME_C66X_CACHE,0x0044)
#define RME_C66X_CACHE_L1DWIBAR         RME_C66X_SFR(RME_C66X_CACHE,0x4030)
#define RME_C66X_CACHE_L1DWIWC          RME_C66X_SFR(RME_C66X_CACHE,0x4034)
#define RME_C66X_CACHE_L1DIBAR          RME_C66X_SFR(RME_C66X_CACHE,0x4048)
#define RME_C66X_CACHE_L1DIWC           RME_C66X_SFR(RME_C66X_CACHE,0x404C)
#define RME_C66X_CACHE_L1DWB            RME_C66X_SFR(RME_C66X_CACHE,0x5040)
#define RME_C66X_CACHE_L1DWBINV         RME_C66X_SFR(RME_C66X_CACHE,0x5044)
#define RME_C66X_CACHE_L1DINV           RME_C66X_SFR(RME_C66X_CACHE,0x5048)
/* C66X level 2 cache definitions */
#define RME_C66X_CACHE_L2CFG            RME_C66X_SFR(RME_C66X_CACHE,0x0000)
#define RME_C66X_CACHE_L2WBAR           RME_C66X_SFR(RME_C66X_CACHE,0x4000)
#define RME_C66X_CACHE_L2WWC            RME_C66X_SFR(RME_C66X_CACHE,0x4004)
#define RME_C66X_CACHE_L2IBAR           RME_C66X_SFR(RME_C66X_CACHE,0x4018)
#define RME_C66X_CACHE_L2IWC            RME_C66X_SFR(RME_C66X_CACHE,0x401C)
#define RME_C66X_CACHE_L2WB             RME_C66X_SFR(RME_C66X_CACHE,0x5000)
#define RME_C66X_CACHE_L2WBINV          RME_C66X_SFR(RME_C66X_CACHE,0x5004)
#define RME_C66X_CACHE_L2INV            RME_C66X_SFR(RME_C66X_CACHE,0x5008)
#define RME_C66X_CACHE_L2MAR(X)         RME_C66X_SFR(RME_C66X_CACHE,0x8000+((X)<<2)) /* 12-255 */
#define RME_C66X_CACHE_L2MAR_PFX        (1<<3)
#define RME_C66X_CACHE_L2MAR_PC         (1<<0)

/* Semaphore base */
#define RME_C66X_SEM                    (0x02640000)
/* C66X semaphore definitions */
#define RME_C66X_SEM_NUM                (32)
#define RME_C66X_SEM_PID                RME_C66X_SFR(RME_C66X_SEM,0x0000)
#define RME_C66X_SEM_RST_RUN            RME_C66X_SFR(RME_C66X_SEM,0x0004)
#define RME_C66X_SEM_EOI                RME_C66X_SFR(RME_C66X_SEM,0x000C)
#define RME_C66X_SEM_DIRECT(X)          RME_C66X_SFR(RME_C66X_SEM,0x0100+((X)<<2)) /* 0-63 */
#define RME_C66X_SEM_INDIRECT(X)        RME_C66X_SFR(RME_C66X_SEM,0x0200+((X)<<2)) /* 0-63 */
#define RME_C66X_SEM_QUERY(X)           RME_C66X_SFR(RME_C66X_SEM,0x0300+((X)<<2)) /* 0-63 */
#define RME_C66X_SEM_FLAGL(X)           RME_C66X_SFR(RME_C66X_SEM,0x0400+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_FLAGL_CLEAR(X)     RME_C66X_SFR(RME_C66X_SEM,0x0400+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_FLAGH(X)           RME_C66X_SFR(RME_C66X_SEM,0x0440+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_FLAGH_CLEAR(X)     RME_C66X_SFR(RME_C66X_SEM,0x0440+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_FLAGL_SET(X)       RME_C66X_SFR(RME_C66X_SEM,0x0480+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_FLAGH_SET(X)       RME_C66X_SFR(RME_C66X_SEM,0x04C0+((X)<<2)) /* 0-15 */
#define RME_C66X_SEM_ERR                RME_C66X_SFR(RME_C66X_SEM,0x0500)
#define RME_C66X_SEM_ERR_CLEAR          RME_C66X_SFR(RME_C66X_SEM,0x0504)
#define RME_C66X_SEM_ERR_SET            RME_C66X_SFR(RME_C66X_SEM,0x0508)
/* Get the semaphore positions - what semaphore is responsible for this memory range? */
#define RME_C66X_SEM_POS(ADDR,POS) \
{ \
    if((ADDR)<RME_KMEM_VA_START) \
        (POS)=0; \
    else if((ADDR)>=(RME_KMEM_VA_START+RME_KMEM_SIZE)) \
        (POS)=RME_C66X_SEM_NUM-1; \
    else \
        (POS)=((ADDR)-RME_KMEM_VA_START)/(RME_KMEM_SIZE/RME_C66X_SEM_NUM);\
}

/* UART base */
#define RME_C66X_UART                   (0x02540000)
/* C66X UART definitions */
#define RME_C66X_UART_LCR               RME_C66X_SFR(RME_C66X_UART,0x000C)
#define RME_C66X_UART_DLL               RME_C66X_SFR(RME_C66X_UART,0x0020)
#define RME_C66X_UART_DLH               RME_C66X_SFR(RME_C66X_UART,0x0024)
#define RME_C66X_UART_IER               RME_C66X_SFR(RME_C66X_UART,0x0004)
#define RME_C66X_UART_MCR               RME_C66X_SFR(RME_C66X_UART,0x0010)
#define RME_C66X_UART_PWREMU_MGMT       RME_C66X_SFR(RME_C66X_UART,0x0030)
#define RME_C66X_UART_FCR               RME_C66X_SFR(RME_C66X_UART,0x0008)
#define RME_C66X_UART_THR               RME_C66X_SFR(RME_C66X_UART,0x0000)
#define RME_C66X_UART_LSR               RME_C66X_SFR(RME_C66X_UART,0x0014)
/* UART bits */
#define RME_C66X_UART_LCR_DLAB          (1<<7)
#define RME_C66X_UART_LCR_BC            (1<<6)
#define RME_C66X_UART_LCR_SP            (1<<5)
#define RME_C66X_UART_LCR_EPS           (1<<4)
#define RME_C66X_UART_LCR_PEN           (1<<3)
#define RME_C66X_UART_LCR_STB           (1<<2)
#define RME_C66X_UART_LCR_WLS5          (0)
#define RME_C66X_UART_LCR_WLS6          (1)
#define RME_C66X_UART_LCR_WLS7          (2)
#define RME_C66X_UART_LCR_WLS8          (3)
#define RME_C66X_UART_MGMT_UTRST        (1<<14)
#define RME_C66X_UART_MGMT_URRST        (1<<13)
#define RME_C66X_UART_MGMT_FREE         (1<<0)
#define RME_C66X_UART_FCR_TXCLR         (1<<2)
#define RME_C66X_UART_FCR_RXCLR         (1<<1)
#define RME_C66X_UART_FCR_FIFOEN        (1<<0)
#define RME_C66X_UART_LSR_THRE          (1<<5)

/* Local interrupt controller base */
#define RME_C66X_LIC                    (0x01800000)
/* C66X Corepac local interrupt controller definitions */
#define RME_C66X_LIC_EVTFLAG(X)         RME_C66X_SFR(RME_C66X_LIC,(X)<<2) /* 0-3 */
#define RME_C66X_LIC_EVTSET(X)          RME_C66X_SFR(RME_C66X_LIC,0x0020+((X)<<2)) /* 0-3 */
#define RME_C66X_LIC_EVTCLR(X)          RME_C66X_SFR(RME_C66X_LIC,0x0040+((X)<<2)) /* 0-3 */
#define RME_C66X_LIC_EVTMASK(X)         RME_C66X_SFR(RME_C66X_LIC,0x0080+((X)<<2)) /* 0-3 */
#define RME_C66X_LIC_MEVTFLAG(X)        RME_C66X_SFR(RME_C66X_LIC,0x00A0+((X)<<2)) /* 0-3 */
#define RME_C66X_LIC_INTMUX(X)          RME_C66X_SFR(RME_C66X_LIC,0x0100+((X)<<2)) /* 1-3 */
#define RME_C66X_LIC_AEGMUX(X)          RME_C66X_SFR(RME_C66X_LIC,0x0140+((X)<<2)) /* 0-1 */
#define RME_C66X_LIC_INTXSTAT           RME_C66X_SFR(RME_C66X_LIC,0x0180)
#define RME_C66X_LIC_INTXCLR            RME_C66X_SFR(RME_C66X_LIC,0x0184)
#define RME_C66X_LIC_INTDMASK           RME_C66X_SFR(RME_C66X_LIC,0x0188)
#define RME_C66X_LIC_EXPMASK(X)         RME_C66X_SFR(RME_C66X_LIC,0x00C0+((X)<<2)) /* 0-3 */
#define RME_C66X_LIC_MEXPFLAG(X)        RME_C66X_SFR(RME_C66X_LIC,0x00E0+((X)<<2)) /* 0-3 */

/* CIC base - 4 controllers */
#define RME_C66X_CIC(N)                 (0x02600000+(0x4000*(N))) /* 0-3 */
/* C66X Chip-level interrupt controller definitions */
/* Revision register */
#define RME_C66X_CIC_REV(N)             RME_C66X_SFR(RME_C66X_CIC(N),0x0000)
/* Control register */
#define RME_C66X_CIC_CTL(N)             RME_C66X_SFR(RME_C66X_CIC(N),0x0004)
/* Global enable register */
#define RME_C66X_CIC_GER(N)             RME_C66X_SFR(RME_C66X_CIC(N),0x0010)
/* System interrupt status indexed set register */
#define RME_C66X_CIC_SISISR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x0020)
/* System interrupt status indexed clear register */
#define RME_C66X_CIC_SISICR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x0024)
/* System interrupt enable indexed set register */
#define RME_C66X_CIC_SIEISR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x0028)
/* System interrupt enable indexed clear register */
#define RME_C66X_CIC_SIEICR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x002C)
/* Host interrupt enable indexed set register */
#define RME_C66X_CIC_HIEISR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x0034)
/* Host interrupt enable indexed clear register */
#define RME_C66X_CIC_HIEICR(N)          RME_C66X_SFR(RME_C66X_CIC(N),0x0038)
/* Global prioritized index register */
#define RME_C66X_CIC_GPIR(N)            RME_C66X_SFR(RME_C66X_CIC(N),0x0080)
/* System interrupt status raw/set registers */
#define RME_C66X_CIC_SISROSR(N,X)       RME_C66X_SFR(RME_C66X_CIC(N),0x0200+((X)<<2)) /* 0-31, 1024 device interrupts */
/* System interrupt status enabled/clear registers */
#define RME_C66X_CIC_SISEOCR(N,X)       RME_C66X_SFR(RME_C66X_CIC(N),0x0280+((X)<<2)) /* 0-31, 1024 device interrupts */
/* System interrupt staus enable set registers */
#define RME_C66X_CIC_SISESR(N,X)        RME_C66X_SFR(RME_C66X_CIC(N),0x0300+((X)<<2)) /* 0-31, 1024 device interrupts */
/* System interrupt status enable clear registers */
#define RME_C66X_CIC_SISECR(N,X)        RME_C66X_SFR(RME_C66X_CIC(N),0x0380+((X)<<2)) /* 0-31, 1024 device interrupts */
/* Channel interrupt map registers */
#define RME_C66X_CIC_CIMR(N,X)          RME_C66X_SFR(RME_C66X_CIC(N),0x0400+((X)<<2)) /* 0-255, 1024 device interrupts */
/* Host map registers */
#define RME_C66X_CIC_HIMR(N,X)          RME_C66X_SFR(RME_C66X_CIC(N),0x0800+((X)<<2)) /* 0-64, 256 channels */
/* Host interrupt prioritized index registers */
#define RME_C66X_CIC_HIPIR(N,X)         RME_C66X_SFR(RME_C66X_CIC(N),0x0900+((X)<<2)) /* 0-255, 256 host interrupts */
/* Host interrupt enable registers */
#define RME_C66X_CIC_HIER(N,X)          RME_C66X_SFR(RME_C66X_CIC(N),0x1500+((X)<<2)) /* 0-7, 256 host interrupts */

/* PSC base */
#define RME_C66X_PSC                    (0x02350000)
/* Power state control registers */
#define RME_C66X_PSC_PID                RME_C66X_SFR(RME_C66X_PSC,0x0000)
#define RME_C66X_PSC_VCNTLID            RME_C66X_SFR(RME_C66X_PSC,0x0014)
#define RME_C66X_PSC_PTCMD              RME_C66X_SFR(RME_C66X_PSC,0x0120)
#define RME_C66X_PSC_PTSTAT             RME_C66X_SFR(RME_C66X_PSC,0x0128)
#define RME_C66X_PSC_PDSTAT(X)          RME_C66X_SFR(RME_C66X_PSC,0x0200+((X)<<2)) /* 0-31 - 8 is CorePac 0 */
#define RME_C66X_PSC_PDCTL(X)           RME_C66X_SFR(RME_C66X_PSC,0x0300+((X)<<2)) /* 0-31 - 8 is CorePac 0 */
#define RME_C66X_PSC_MDSTAT(X)          RME_C66X_SFR(RME_C66X_PSC,0x0800+((X)<<2)) /* 0-31 - 15 is CorePac 0 */
#define RME_C66X_PSC_MDCTL(X)           RME_C66X_SFR(RME_C66X_PSC,0x0A00+((X)<<2)) /* 0-31 - 15 is CorePac 0 */
#define RME_C66X_PSC_MDCTL_LRST         (1<<8)
#define RME_C66X_PSC_MDCTL_NEXT_ENABLE  (0x03)

/* DSC base */
#define RME_C66X_DSC                    (0x02620000)
/* Device state control registers */
#define RME_C66X_DSC_JTAGID             RME_C66X_SFR(RME_C66X_DSC,0x0018)
#define RME_C66X_DSC_DEVSTAT            RME_C66X_SFR(RME_C66X_DSC,0x0020)
#define RME_C66X_DSC_KICK0              RME_C66X_SFR(RME_C66X_DSC,0x0038)
#define RME_C66X_DSC_KICK1              RME_C66X_SFR(RME_C66X_DSC,0x003C)
#define RME_C66X_DSC_BOOT_ADDR(X)       RME_C66X_SFR(RME_C66X_DSC,0x0040+((X)<<2)) /* 0-7, 8 cores */
#define RME_C66X_DSC_MACIDL             RME_C66X_SFR(RME_C66X_DSC,0x0110)
#define RME_C66X_DSC_MACIDH             RME_C66X_SFR(RME_C66X_DSC,0x0114)
#define RME_C66X_DSC_LRSTNMISTAT_CLR    RME_C66X_SFR(RME_C66X_DSC,0x0130)
#define RME_C66X_DSC_RESETSTAT_CLR      RME_C66X_SFR(RME_C66X_DSC,0x0134)
#define RME_C66X_DSC_BOOTCOMPLETE       RME_C66X_SFR(RME_C66X_DSC,0x013C)
#define RME_C66X_DSC_RESET_STAT         RME_C66X_SFR(RME_C66X_DSC,0x0144)
#define RME_C66X_DSC_LRSTNMISTAT        RME_C66X_SFR(RME_C66X_DSC,0x0148)
#define RME_C66X_DSC_DEVCFG             RME_C66X_SFR(RME_C66X_DSC,0x014C)
#define RME_C66X_DSC_PWRSTATECTL        RME_C66X_SFR(RME_C66X_DSC,0x0150)
#define RME_C66X_DSC_SRIO_STS           RME_C66X_SFR(RME_C66X_DSC,0x0154)
#define RME_C66X_DSC_SMGII_STS          RME_C66X_SFR(RME_C66X_DSC,0x0158)
#define RME_C66X_DSC_PCIE_STS           RME_C66X_SFR(RME_C66X_DSC,0x015C)
#define RME_C66X_DSC_HYPERLINK_STS      RME_C66X_SFR(RME_C66X_DSC,0x0160)
#define RME_C66X_DSC_NMIGR(X)           RME_C66X_SFR(RME_C66X_DSC,0x0200+((X)<<2)) /* 0-7, 8 cores */
#define RME_C66X_DSC_IPCGR(X)           RME_C66X_SFR(RME_C66X_DSC,0x0240+((X)<<2)) /* 0-7, 8 cores */
#define RME_C66X_DSC_IPCGRH             RME_C66X_SFR(RME_C66X_DSC,0x027C)
#define RME_C66X_DSC_IPCAR(X)           RME_C66X_SFR(RME_C66X_DSC,0x0280+((X)<<2)) /* 0-7, 8 cores */
#define RME_C66X_DSC_IPCARH             RME_C66X_SFR(RME_C66X_DSC,0x02BC)
#define RME_C66X_DSC_TINPSEL            RME_C66X_SFR(RME_C66X_DSC,0x0300)
#define RME_C66X_DSC_TOUTPSEL           RME_C66X_SFR(RME_C66X_DSC,0x0304)
#define RME_C66X_DSC_RSTMUX(X)          RME_C66X_SFR(RME_C66X_DSC,0x0308+((X)<<2)) /* 0-7, 8 cores */
#define RME_C66X_DSC_MAINPLLCTL(X)      RME_C66X_SFR(RME_C66X_DSC,0x0328+((X)<<2)) /* 0-1 */
#define RME_C66X_DSC_DDR3PLLCTL(X)      RME_C66X_SFR(RME_C66X_DSC,0x0330+((X)<<2)) /* 0-1 */
#define RME_C66X_DSC_PASSPLLCTL(X)      RME_C66X_SFR(RME_C66X_DSC,0x0338+((X)<<2)) /* 0-1 */
#define RME_C66X_DSC_SGMII_CFGPLL       RME_C66X_SFR(RME_C66X_DSC,0x0340)
#define RME_C66X_DSC_SGMII_CFGRX(X)     RME_C66X_SFR(RME_C66X_DSC,0x0344+((X)<<3)) /* 0-1 */
#define RME_C66X_DSC_SGMII_CFGTX(X)     RME_C66X_SFR(RME_C66X_DSC,0x0348+((X)<<3)) /* 0-1 */
#define RME_C66X_DSC_PCIE_CFGPLL        RME_C66X_SFR(RME_C66X_DSC,0x0358)
#define RME_C66X_DSC_SRIO_CFGPLL        RME_C66X_SFR(RME_C66X_DSC,0x0360)
#define RME_C66X_DSC_SRIO_CFGRX(X)      RME_C66X_SFR(RME_C66X_DSC,0x0364+((X)<<3)) /* 0-3 */
#define RME_C66X_DSC_SRIO_CFGTX(X)      RME_C66X_SFR(RME_C66X_DSC,0368+((X)<<3)) /* 0-3 */
#define RME_C66X_DSC_DSP_SUSP_CTL       RME_C66X_SFR(RME_C66X_DSC,0x038C)
#define RME_C66X_DSC_HYPERLINK_CFGPLL   RME_C66X_SFR(RME_C66X_DSC,0x03B4)
#define RME_C66X_DSC_HYPERLINK_CFGRX(X) RME_C66X_SFR(RME_C66X_DSC,0x03B8+((X)<<3)) /* 0-3 */
#define RME_C66X_DSC_HYPERLINK_CFGTX(X) RME_C66X_SFR(RME_C66X_DSC,0x03BC+((X)<<3)) /* 0-3 */
#define RME_C66X_DSC_DEVSPEED           RME_C66X_SFR(RME_C66X_DSC,0x03F8)
#define RME_C66X_DSC_CHIP_MISC_CTL      RME_C66X_SFR(RME_C66X_DSC,0x0400)

/* Timer definitions */
#define RME_C66X_TIMER(N)               (0x02200000+(0x10000*(N))) /* 0-15 */
#define RME_C66X_TIM_EMUMGT_CLKSPD(N)   RME_C66X_SFR(RME_C66X_TIMER(N),0x0004)
#define RME_C66X_TIM_CNTLO(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0010)
#define RME_C66X_TIM_CNTHI(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0014)
#define RME_C66X_TIM_PRDLO(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0018)
#define RME_C66X_TIM_PRDHI(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x001C)
#define RME_C66X_TIM_TCR(N)             RME_C66X_SFR(RME_C66X_TIMER(N),0x0020)
#define RME_C66X_TIM_TGCR(N)            RME_C66X_SFR(RME_C66X_TIMER(N),0x0024)
#define RME_C66X_TIM_WDTCR(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0028)
#define RME_C66X_TIM_RELLO(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0034)
#define RME_C66X_TIM_RELHI(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x0038)
#define RME_C66X_TIM_CAPLO(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x003C)
#define RME_C66X_TIM_CAPHI(N)           RME_C66X_SFR(RME_C66X_TIMER(N),0x003C)
#define RME_C66X_TIM_INTCTL(N)          RME_C66X_SFR(RME_C66X_TIMER(N),0x0044)
/* Option definitions */
#define RME_C66X_TIM_TCR_CONT           (0x02<<6)
#define RME_C66X_TIM_TGCR_TIMHIRS       (1<<1)
#define RME_C66X_TIM_TGCR_TIMLORS       (1<<0)
#define RME_C66X_TIM_INTCTL_PRDINTEN_LO (1<<0)

/* EFR bit definitions */
#define RME_C66X_EFR_NXF                ((rme_ptr_t)1<<31) /* NMI */
#define RME_C66X_EFR_EXF                (1<<30) /* Exception */
#define RME_C66X_EFR_IXF                (1<<1) /* Internal fault */
#define RME_C66X_EFR_SXF                (1<<0) /* System call */

/* IERR bit definitions */
#define RME_C66X_IERR_MSX               (1<<8) /* Missed stall exception */
#define RME_C66X_IERR_LBX               (1<<7) /* SPLOOP buffer exception */
#define RME_C66X_IERR_PRX               (1<<6) /* Privilege exception */
#define RME_C66X_IERR_RAX               (1<<5) /* Resource access exception */
#define RME_C66X_IERR_RCX               (1<<4) /* Resource conflict exception */
#define RME_C66X_IERR_OPX               (1<<3) /* Opcode exception */
#define RME_C66X_IERR_EPX               (1<<2) /* Execute packet exception */
#define RME_C66X_IERR_FPX               (1<<1) /* Fetch packet exception */
#define RME_C66X_IERR_IFX               (1<<0) /* Instruction fetch exception */
/*****************************************************************************/
/* __RME_PLATFORM_C66X_H_DEFS__ */
#endif
/* __HDR_DEFS__ */
#endif
/* End Defines ***************************************************************/

/* Structs *******************************************************************/
#ifdef __HDR_STRUCTS__
#ifndef __RME_PLATFORM_C66X_H_STRUCTS__
#define __RME_PLATFORM_C66X_H_STRUCTS__
/* We used structs in the header */

/* Use defines in these headers */
#define __HDR_DEFS__
#undef __HDR_DEFS__
/*****************************************************************************/
/* The register set struct - The first 10 parameters are passed in
 * A4, B4, A6, B6, A8, B8, A10, B10, A12, and B12 */
struct RME_Reg_Struct
{
    /* Status registers */
    rme_ptr_t RILC;
    rme_ptr_t NTSR;
    rme_ptr_t ILC;
    rme_ptr_t CSR;
    rme_ptr_t SSR;
    rme_ptr_t NRP;

    rme_ptr_t A0;
    rme_ptr_t A1;
    rme_ptr_t A2;
    /* This is the alternative link register */
    rme_ptr_t A3;
    rme_ptr_t A4;
    rme_ptr_t A5;
    rme_ptr_t A6;
    rme_ptr_t A7;
    rme_ptr_t A8;
    rme_ptr_t A9;
    rme_ptr_t A10;
    rme_ptr_t A11;
    rme_ptr_t A12;
    rme_ptr_t A13;
    rme_ptr_t A16;
    rme_ptr_t A17;
    rme_ptr_t A18;
    rme_ptr_t A19;
    rme_ptr_t A20;
    rme_ptr_t A21;
    rme_ptr_t A22;
    rme_ptr_t A23;
    rme_ptr_t A24;
    rme_ptr_t A25;
    rme_ptr_t A26;
    rme_ptr_t A27;
    rme_ptr_t A28;
    rme_ptr_t A29;
    rme_ptr_t A30;
    rme_ptr_t A31;

    rme_ptr_t B0;
    rme_ptr_t B1;
    rme_ptr_t B2;
    /* This is the link register */
    rme_ptr_t B3;
    rme_ptr_t B4;
    rme_ptr_t B5;
    rme_ptr_t B6;
    rme_ptr_t B7;
    rme_ptr_t B8;
    rme_ptr_t B9;
    rme_ptr_t B10;
    rme_ptr_t B11;
    rme_ptr_t B12;
    rme_ptr_t B13;
    rme_ptr_t B16;
    rme_ptr_t B17;
    rme_ptr_t B18;
    rme_ptr_t B19;
    rme_ptr_t B20;
    rme_ptr_t B21;
    rme_ptr_t B22;
    rme_ptr_t B23;
    rme_ptr_t B24;
    rme_ptr_t B25;
    rme_ptr_t B26;
    rme_ptr_t B27;
    rme_ptr_t B28;
    rme_ptr_t B29;
    rme_ptr_t B30;
    rme_ptr_t B31;

    /* This is the data pointer */
    rme_ptr_t B14;
    /* This is the stack pointer */
    rme_ptr_t B15;
    rme_ptr_t A14;
    /* This is the frame pointer */
    rme_ptr_t A15;
};

/* The coprocessor register set structure - This will always be saved & restored upon context switch */
struct RME_Cop_Struct
{
    rme_ptr_t AMR;
    rme_ptr_t GFPGFR;
    rme_ptr_t GPLYA;
    rme_ptr_t GPLYB;
    rme_ptr_t FADCR;
    rme_ptr_t FAUCR;
    rme_ptr_t FMCR;
};

/* The registers to keep to remember where to return after an invocation */
struct RME_Iret_Struct
{
    /* The program counter */
    rme_ptr_t NRP;
    /* The stack pointer */
    rme_ptr_t B15;
};

/* Memory information - the layout is (offset from VA base):
 * |0--------------------------2GB|----------64MB or 128MB|-----|------|------|-----|3.25G-4G|-----|-----|
 * |Peripherals, identical mapping|Kernel&Globals|Kotbl|PerCPU|Kpgtbl|Kmem1|  Hole  |Kmem2|Stack|
 *  Vectors        : Interrupt vectors.
 *  Kernel&Globals : Initial kernel text segment and all static variables.
 *  Kotbl          : Kernel object registration table.
 *  PerCPU         : Per-CPU data structures.
 *  Kpgtbl         : Kernel page tables.
 *  Pgreg          : Page table registration table.
 *  Kmem1          : Kernel memory 1, linear mapping, allow creation of page tables.
 *  Hole           : Memory hole present at 3.25G-4G. For PCI devices.
 *  Kmem2          : Ker
 * The C snippet to generate this table is shown below (gcc x64):nel memory 2, nonlinear mapping, no page table creation allowed.
 *  Stacks         : Kernel stacks, per-CPU.
 *  All values are in bytes, and are virtual addresses.
 */

/* C66X have a region-based MMU, with physical address extension support. We do not support
 * physical address extension here on purpose. The first set of registers are aways reserved
 * for kernel operations (statically mapped), thus is never user-populated */
struct __RME_C66X_XMC_Entry
{
    /* MPAX high register */
    rme_ptr_t XMPAXH;
    /* MPAX low register */
    rme_ptr_t XMPAXL;
};

/* Per-CPU MMU cache data structure */
struct __RME_C66X_MMU_CPU_Local
{
    /* [31:16] Static [15:0] Present */
    rme_ptr_t State;
    struct __RME_C66X_XMC_Entry Data[RME_C66X_XMC_REGIONS];
};

/* Top-level page table organization: Meta - MMU - Table
 * Other page table level organization: Meta - Table */
struct __RME_C66X_Pgtbl_Meta
{
    /* The size/num order of this level */
    rme_ptr_t Size_Num_Order;
    /* How many parent page directories do we have? */
    rme_ptr_t Parent_Cnt;
    /* How many child page directories do we have? */
    rme_ptr_t Child_Cnt;
};

/* When the metadata is filled like a TLB. We provide a kernel call to flush the TLB on
 * a certain core, if the representation changed in a reducing way. On this architecture,
 * we make the kernel memory allocation static, thus we will not have liveness issues */
struct __RME_C66X_MMU_Data
{
    struct __RME_C66X_MMU_CPU_Local Local[RME_C66X_CPU_NUM];
};

/* Interrupt flags */
struct __RME_C66X_Flag_Set
{
    rme_ptr_t Lock;
    rme_ptr_t Group;
    rme_ptr_t Flags[32];
};


struct __RME_C66X_Flags
{
    struct __RME_C66X_Flag_Set Set0;
    struct __RME_C66X_Flag_Set Set1;
};
/*****************************************************************************/
/* __RME_PLATFORM_C66X_H_STRUCTS__ */
#endif
/* __HDR_STRUCTS__ */
#endif
/* End Structs ***************************************************************/

/* Private Global Variables **************************************************/
#if(!(defined __HDR_DEFS__||defined __HDR_STRUCTS__))
#ifndef __RME_PLATFORM_C66X_MEMBERS__
#define __RME_PLATFORM_C66X_MEMBERS__

/* In this way we can use the data structures and definitions in the headers */
#define __HDR_DEFS__

#undef __HDR_DEFS__

#define __HDR_STRUCTS__

#undef __HDR_STRUCTS__

/* If the header is not used in the public mode */
#ifndef __HDR_PUBLIC_MEMBERS__
/*****************************************************************************/
/* CPU booting counter */
static rme_ptr_t RME_C66X_CPU_Cnt;
/* Booting done indicator */
static rme_ptr_t __RME_C66X_Boot_Done[RME_C66X_CPU_NUM];
/*****************************************************************************/
/* End Private Global Variables **********************************************/

/* Private C Function Prototypes *********************************************/ 
/*****************************************************************************/
static rme_ptr_t __RME_C66X_MMU_Update(struct RME_Cap_Pgtbl* Pgtbl, rme_ptr_t CPUID);
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
/* Core local storage */
EXTERN rme_ptr_t __RME_C66X_Stack[4096*8];
EXTERN rme_ptr_t __RME_C66X_Stack_Addr[8];
/* Vector table */
EXTERN rme_ptr_t __RME_C66X_Vector_Table[8*16];
/* Boot done indicator */
EXTERN rme_ptr_t __RME_C66X_Boot_Done[8];
/* Core-local storage */
__EXTERN__ struct RME_CPU_Local RME_C66X_CPU_Local[RME_C66X_CPU_NUM];
/*****************************************************************************/

/* End Public Global Variables ***********************************************/

/* Public C Function Prototypes **********************************************/
/*****************************************************************************/
/* Interrupts */
EXTERN void __RME_Disable_Int(void);
EXTERN void __RME_Enable_Int(void);
EXTERN void __RME_C66X_Idle(void);
EXTERN void __RME_C66X_Set_ISTP(rme_ptr_t ISTP);
EXTERN rme_ptr_t __RME_C66X_Get_EFR(void);
EXTERN void __RME_C66X_Set_ECR(rme_ptr_t ECR);
EXTERN rme_ptr_t __RME_C66X_Get_IERR(void);
/* Atomics */
__EXTERN__ rme_ptr_t __RME_C66X_Comp_Swap(rme_ptr_t* Ptr, rme_ptr_t Old, rme_ptr_t New);
__EXTERN__ rme_ptr_t __RME_C66X_Fetch_Add(rme_ptr_t* Ptr, rme_cnt_t Addend);
__EXTERN__ rme_ptr_t __RME_C66X_Fetch_And(rme_ptr_t* Ptr, rme_ptr_t Operand);
/* Fencing */
EXTERN rme_ptr_t __RME_C66X_Read_Acquire(rme_ptr_t* Ptr);
EXTERN void __RME_C66X_Write_Release(rme_ptr_t* Ptr, rme_ptr_t Value);
/* MSB counting */
EXTERN rme_ptr_t __RME_C66X_MSB_Get(rme_ptr_t Val);
/* Core ID */
EXTERN rme_ptr_t __RME_C66X_CPUID_Get(void);
/* Debugging */
__EXTERN__ rme_ptr_t __RME_Putchar(char Char);
/* Coprocessor */
EXTERN void ___RME_C66X_Thd_Cop_Save(struct RME_Cop_Struct* Cop_Reg);
EXTERN void ___RME_C66X_Thd_Cop_Restore(struct RME_Cop_Struct* Cop_Reg);
/* Booting */
EXTERN void _RME_Kmain(rme_ptr_t Stack);
__EXTERN__ void __RME_SMP_Kmain(void);
EXTERN void _RME_C66X_SMP_Kmain(void);
EXTERN void __RME_Enter_User_Mode(rme_ptr_t Entry_Addr, rme_ptr_t Stack_Addr, rme_ptr_t CPUID);
__EXTERN__ void __RME_C66X_Timer_Init(void);
__EXTERN__ rme_ptr_t __RME_Low_Level_Init(void);
__EXTERN__ rme_ptr_t __RME_SMP_Low_Level_Init(void);
__EXTERN__ rme_ptr_t __RME_Boot(void);
__EXTERN__ void __RME_Reboot(void);
__EXTERN__ void __RME_Shutdown(void);
/* Syscall & invocation */
__EXTERN__ struct RME_CPU_Local* __RME_CPU_Local_Get(void);
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
__EXTERN__ void __RME_C66X_Fault_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Cause);
/* Interrupt handler */
__EXTERN__ void __RME_C66X_Int_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Cause);
/* Generic interrupt handler */
__EXTERN__ void __RME_C66X_Generic_Handler(struct RME_Reg_Struct* Reg);
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
/* __RME_PLATFORM_C66X_MEMBERS__ */
#endif
/* !(defined __HDR_DEFS__||defined __HDR_STRUCTS__) */
#endif
/* End Public C Function Prototypes ******************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
