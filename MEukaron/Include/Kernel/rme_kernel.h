/******************************************************************************
Filename    : rme_kernel.h
Author      : pry
Date        : 08/04/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The header of the kernel. Whitebox testing of all branches encapsulated
              in macros are inspected manually carefully.
******************************************************************************/

/* Defines *******************************************************************/
#ifdef __HDR_DEFS__
#ifndef __RME_KERNEL_H_DEFS__
#define __RME_KERNEL_H_DEFS__
/*****************************************************************************/
/* Generic *******************************************************************/
#define RME_TRUE                                    (1)
#define RME_FALSE                                   (0)
#define RME_NULL                                    (0)
#define RME_EXIST                                   (1)
#define RME_EMPTY                                   (0)
#define RME_CASFAIL                                 (0)

/* Bit mask/address operations */
#define RME_ALLBITS                                 ((rme_ptr_t)(-1))
#define RME_WORD_BITS                               (sizeof(rme_ptr_t)*8)
#define RME_POSITIVE_BITS                           (RME_ALLBITS>>1)
/* Apply this mask to keep START to MSB bits */
#define RME_MASK_START(START)                       ((RME_ALLBITS)<<(START))
/* Apply this mask to keep LSB to END bits */
#define RME_MASK_END(END)                           ((RME_ALLBITS)>>(RME_WORD_BITS-1-(END)))
/* Apply this mask to keep START to END bits, START < END */
#define RME_MASK(START,END)                         ((RME_MASK_START(START))&(RME_MASK_END(END)))
/* Round the number down & up to a power of 2, or get the power of 2 */
#define RME_ROUND_DOWN(NUM,POW)                     ((NUM)&(RME_MASK_START(POW)))
#define RME_ROUND_UP(NUM,POW)                       RME_ROUND_DOWN((NUM)+RME_MASK_END(POW-1),POW)
#define RME_POW2(POW)                               (1U<<(POW))
/* Check if address is aligned on word boundary */
#define RME_IS_ALIGNED(ADDR)                        (((ADDR)&RME_MASK_END(RME_WORD_ORDER-4))==0)
/* Bit field extraction macros for easy extraction of parameters
[MSB                                 PARAMS                                 LSB]
[                  D1                  ][                  D0                  ]
[        Q3        ][        Q2        ][        Q1        ][        Q0        ]
[   O7   ][   O6   ][   O5   ][   O4   ][   O3   ][   O2   ][   O1   ][   O0   ] 
*/
/* Cut in half */
#define RME_PARAM_D1(X)                             ((X)>>(sizeof(rme_ptr_t)*4))
#define RME_PARAM_D0(X)                             ((X)&RME_MASK_END((sizeof(rme_ptr_t)*4)-1))
/* Cut into 4 parts */
#define RME_PARAM_Q3(X)                             ((X)>>(sizeof(rme_ptr_t)*6))
#define RME_PARAM_Q2(X)                             (((X)>>(sizeof(rme_ptr_t)*4))&RME_MASK_END((sizeof(rme_ptr_t)*2)-1))
#define RME_PARAM_Q1(X)                             (((X)>>(sizeof(rme_ptr_t)*2))&RME_MASK_END((sizeof(rme_ptr_t)*2)-1))
#define RME_PARAM_Q0(X)                             ((X)&RME_MASK_END((sizeof(rme_ptr_t)*2)-1))
/* Cut into 8 parts */
#define RME_PARAM_O7(X)                             ((X)>>(sizeof(rme_ptr_t)*7))
#define RME_PARAM_O6(X)                             (((X)>>(sizeof(rme_ptr_t)*6))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O5(X)                             (((X)>>(sizeof(rme_ptr_t)*5))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O4(X)                             (((X)>>(sizeof(rme_ptr_t)*4))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O3(X)                             (((X)>>(sizeof(rme_ptr_t)*3))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O2(X)                             (((X)>>(sizeof(rme_ptr_t)*2))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O1(X)                             (((X)>>(sizeof(rme_ptr_t)*1))&RME_MASK_END(sizeof(rme_ptr_t)-1))
#define RME_PARAM_O0(X)                             ((X)&RME_MASK_END(sizeof(rme_ptr_t)-1))

/* This is the special one used for delegation, and used for kernel memory
 * capability only because it is very complicated. Other capabilities will not use this */
#define RME_PARAM_KM(SVC,CAPID)                     (((SVC)<<(sizeof(rme_ptr_t)*4))|(CAPID))
/* This is the special one used for page table top-level flags */
#define RME_PARAM_PT(X)                             ((X)&0x01)
/* The page table creation extra parameter packed in the svc number */
#define RME_PARAM_PC(SVC)                           ((SVC)>>((sizeof(rme_ptr_t)<<1)))

/* The return procedure of a possible context switch - If successful, the function itself
 * is responsible for setting the parameters; If failed, we set the parameters for it.
 * Possible categories of context switch includes synchronous invocation and thread switch. */
#define RME_SWITCH_RETURN(REG,RETVAL) \
{ \
    if(RME_UNLIKELY((RETVAL)<0)) \
        __RME_Set_Syscall_Retval((REG),(RETVAL)); \
    \
    return; \
}

/* The system service numbers are defined here. This is included in both user level 
 * and kernel level */
#include "rme.h"

/* Debugging */
#define RME_KERNEL_DEBUG_MAX_STR                    (128)
/* Printk macros */
#define RME_PRINTK_I(INT)                           RME_Print_Int((INT))
#define RME_PRINTK_U(UINT)                          RME_Print_Uint((UINT))
#define RME_PRINTK_S(STR)                           RME_Print_String((rme_s8_t*)(STR))

/* Shutdown debugging */
/* #define RME_ASSERT_CORRECT */
/* Default assert macro - used only when internal development option is on */
#ifndef RME_ASSERT_CORRECT
#define RME_ASSERT(X) \
do \
{ \
    if(RME_UNLIKELY((X)==0)) \
    { \
        RME_PRINTK_S((rme_s8_t*)"\r\n***\r\nKernel panic - not syncing:\r\n"); \
        RME_PRINTK_S((rme_s8_t*)__FILE__); \
        RME_PRINTK_S((rme_s8_t*)" , Line "); \
        RME_PRINTK_I(__LINE__); \
        RME_PRINTK_S((rme_s8_t*)"\r\n"); \
        RME_PRINTK_S((rme_s8_t*)__DATE__); \
        RME_PRINTK_S((rme_s8_t*)" , "); \
        RME_PRINTK_S((rme_s8_t*)__TIME__); \
        RME_PRINTK_S((rme_s8_t*)"\r\n"); \
        while(1); \
    } \
} \
while(0)
#else
#define RME_ASSERT(X) \
do \
{ \
    if(RME_UNLIKELY((X)==0)) \
    { \
    } \
} \
while(0)
#endif

/* Coverage testing marker */
/* #define RME_COVERAGE */
/* Test marker macro */
#ifdef RME_COVERAGE
#define RME_COVERAGE_LINES                          (8192)
#define RME_COVERAGE_MARKER() \
do \
{ \
    RME_Coverage[__LINE__]++; \
    RME_Coverage[0]=RME_Coverage[__LINE__]; \
} \
while(0)
#else
#define RME_COVERAGE_MARKER() \
do \
{ \
    \
} \
while(0)
#endif

/* Kernel Object Table *******************************************************/
/* Bitmap reference error */
#define RME_ERR_KOT_BMP                             (-1)

/* Number of slots, and size of each slot */
#define RME_KOTBL_SLOT_NUM                          (RME_KMEM_SIZE>>RME_KMEM_SLOT_ORDER)
#define RME_KOTBL_SLOT_SIZE                         RME_POW2(RME_KMEM_SLOT_ORDER)
#define RME_KOTBL_WORD_NUM                          (RME_KOTBL_SLOT_NUM>>RME_WORD_ORDER)
/* Round the kernel object size to the entry slot size */
#define RME_KOTBL_ROUND(X)                          RME_ROUND_UP(X,RME_KMEM_SLOT_ORDER)

/* Capability Table **********************************************************/
/* Capability size macro */
#define RME_CAP_SIZE                                (8*sizeof(rme_ptr_t))
/* Capability table size calculation macro */
#define RME_CAPTBL_SIZE(NUM)                        (sizeof(struct RME_Cap_Struct)*(NUM))
/* The operation inline macros on the capabilities */
/* Refcnt_Type:example for 32-bit and 64-bit systems
 * 32-bit system:
 * [31    Type   24][23  Status  16][15              Attribute                0]
 * 64-bit system:
 * [63    Type   48][47  Status  32][31              Attribute                0]
 * Refcnt is used to track delegation, and also in the case of process creation,
 * used to track if the capability table or the page table is referenced.
 * Frozen is a fine-grained lock to lock the entries involved so that no parallel
 *        create/destroy/alteration operations on them can be done. If one of the
 *        locks failed, we will give up all the locks, and retry.
 * Type is a field denoting what is it. */
#define RME_CAP_TYPE_STAT(TYPE,STAT,ATTR)           ((((rme_ptr_t)(TYPE))<<(sizeof(rme_ptr_t)*6))| \
                                                     (((rme_ptr_t)(STAT))<<(sizeof(rme_ptr_t)*4))|(ATTR))
                                                     
/* Capability types */
#define RME_CAP_TYPE(X)                             ((X)>>(sizeof(rme_ptr_t)*6))
/* Empty */
#define RME_CAP_TYPE_NOP                            (0)
/* Kernel function */
#define RME_CAP_TYPE_KERN                           (1)
/* Kernel memory */
#define RME_CAP_TYPE_KMEM                           (2)
/* Capability table */
#define RME_CAP_TYPE_CAPTBL                         (3)
/* Page table */
#define RME_CAP_TYPE_PGTBL                          (4)
/* Process */
#define RME_CAP_TYPE_PROC                           (5)
/* Thread */
#define RME_CAP_TYPE_THD                            (6)
/* Synchronous invocation */
#define RME_CAP_TYPE_INV                            (7)
/* Asynchronous signal endpoint */
#define RME_CAP_TYPE_SIG                            (8)

/* Capability statuses */
#define RME_CAP_STAT(X)                             (((X)>>(sizeof(rme_ptr_t)*4))&RME_MASK_END((sizeof(rme_ptr_t)*2)-1))
/* Valid capability */
#define RME_CAP_STAT_VALID                          (0)
/* Capability under creation */
#define RME_CAP_STAT_CREATING                       (1)
/* Frozen capability */
#define RME_CAP_STAT_FROZEN                         (2)
/* Capability attributes */
#define RME_CAP_ATTR(X)                             ((X)&RME_MASK_END((sizeof(rme_ptr_t)*4)-1))
/* Root capability */
#define RME_CAP_ATTR_ROOT                           (0)
/* Leaf capability */
#define RME_CAP_ATTR_LEAF                           (1)

/* Is this cap quiescent? Yes-1, No-0 */
#if(RME_QUIE_TIME!=0)
#if(RME_WORD_ORDER==5)
/* If this is a 32-bit system, need to consider overflows */
#define RME_CAP_QUIE(X)                             (((RME_Timestamp-(X))>(X)-RME_Timestamp)? \
                                                     (((X)-RME_Timestamp)>RME_QUIE_TIME): \
                                                     ((RME_Timestamp-(X))>RME_QUIE_TIME))
#else
#define RME_CAP_QUIE(X)                             ((RME_Timestamp-(X))>RME_QUIE_TIME)
#endif
#else
#define RME_CAP_QUIE(X)                             (1)
#endif

/* Convert to root */
#define RME_CAP_CONV_ROOT(X,TYPE)                   ((RME_CAP_ATTR((X)->Head.Type_Stat)!=RME_CAP_ATTR_ROOT)? \
                                                     ((TYPE)((X)->Head.Root_Ref)):(X))
/* Get the object */
#define RME_CAP_GETOBJ(X,TYPE)                      ((TYPE)((X)->Head.Object))
/* 1-layer capid addressing:
 * 32-bit systems: Capid range 0x00 - 0x7F
 * [15             Reserved             8][7  2L(0)][6    Table(Master)   0]
 * 64-bit systems: Capid range 0x0000 - 0x7FFF
 * [31             Reserved            16][15 2L(0)][14   Table(Master)   0]
 * 2-layer capid addressing:
 * 32-bit systems: Capid range 0x00 - 0x7F
 * [15 Reserved][14 High Table(Master)  8][7  2L(1)][6   Low Table(Child) 0]
 * 64-bit systems: Capid range 0x0000 - 0x7FFF
 * [31 Reserved][30 High Table(Master) 16][15 2L(1)][14  Low Table(Child) 0] */
#define RME_CAPID_NULL                              (1<<(sizeof(rme_ptr_t)*4-1))
/* See if the capid is a 2-level representation */
#define RME_CAPID_2L                                (1<<(sizeof(rme_ptr_t)*2-1))
/* Make 2-level capability */
#define RME_CAPID(X,Y)                              (((X)<<(sizeof(rme_ptr_t)*2))|(Y)|RME_CAPID_2L)
/* High-level capability table capability position */
#define RME_CAP_H(X)                                ((X)>>(sizeof(rme_ptr_t)*2))
/* Low-level capability table capability position */
#define RME_CAP_L(X)                                ((X)&RME_MASK_END(sizeof(rme_ptr_t)*2-2))

/* When we are clearing capabilities */
#define RME_CAP_CLEAR(X) \
do \
{ \
    /* Do this at last lest that some overlapping operations may happen */ \
    (X)->Head.Type_Stat=0; \
} \
while(0)

/* Replicate the capability. The Type_Ref field is filled in in the last step 
 * of capability creation; the Parent field is also populated in the creation
 * process appropriately. Thus, they are not filled in here.
 * DST - The pointer to the destination slot.
 * SRC - The pointer to the source slot.
 * FLAGS - The new operation flags of this capability. */
#define RME_CAP_COPY(DST,SRC,FLAGS) \
do \
{ \
    /* The suboperation capability flags */ \
    (DST)->Head.Flags=(FLAGS); \
    /* The object address */ \
    (DST)->Head.Object=(SRC)->Head.Object; \
    (DST)->Info[0]=(SRC)->Info[0]; \
    (DST)->Info[1]=(SRC)->Info[1]; \
    (DST)->Info[2]=(SRC)->Info[2]; \
} \
while(0)

/* Check if the capability is ready for some operations.
 * CAP - The pointer to the capability slot to check.
 * FLAG - The operation flags to check against the slot. */
#define RME_CAP_CHECK(CAP,FLAG) \
do \
{ \
    /* See if this capability allows such operations */ \
    if(RME_UNLIKELY(((CAP)->Head.Flags&(FLAG))!=(FLAG))) \
        return RME_ERR_CAP_FLAG; \
} \
while(0)

/* Check if the kernel memory capability range is valid.
 * CAP - The kernel memory capability to check.
 * FLAG - The flags to check against the kernel memory capability.
 * RADDR - The relative start address of the kernel object in kernel memory.
 * VADDR - The true start address of the kernel object in kernel memory, output.
 * SIZE - The size of the kernel memory trunk. */
/* This have wraparounds now */
#define RME_KMEM_CHECK(CAP,FLAG,RADDR,VADDR,SIZE) \
do \
{ \
    /* See if the creation of such capability is allowed */ \
    if(RME_UNLIKELY(((CAP)->Head.Flags&(FLAG))!=(FLAG))) \
        return RME_ERR_CAP_FLAG; \
    /* Convert relative address to virtual address */ \
    (VADDR)=(RADDR)+(CAP)->Start; \
    /* Check start boundary and its possible wraparound */ \
    if(RME_UNLIKELY((VADDR)<(RADDR))) \
        return RME_ERR_CAP_FLAG; \
    if(RME_UNLIKELY(((CAP)->Start>(VADDR)))) \
        return RME_ERR_CAP_FLAG; \
    /* Check end boundary and its possible wraparound */ \
    if(RME_UNLIKELY((((VADDR)+(SIZE))<(VADDR)))) \
        return RME_ERR_CAP_FLAG; \
    if(RME_UNLIKELY((CAP)->End<((VADDR)+(SIZE)))) \
        return RME_ERR_CAP_FLAG; \
} \
while(0)

/* Defrost a frozen cap - we do not check failure because if we fail, someone must 
 * have done it for us. The reason why we need to defrost a capability after we decide
 * that it is unfit for deletion or removal is that the capability may later be used in
 * kernel object deallocation operations. If we keep it freezed, we can't deallocate
 * other resources that may depend on it.
 * CAP - The pointer to the capability slot to defreeze.
 * TEMP - A temporary variable, for compare-and-swap. */
#define RME_CAP_DEFROST(CAP,TEMP) \
do \
{ \
    RME_COMP_SWAP(&((CAP)->Head.Type_Stat),(TEMP), \
                  RME_CAP_TYPE_STAT(RME_CAP_TYPE(TEMP),RME_CAP_STAT_VALID,RME_CAP_ATTR(TEMP))); \
} \
while(0)

/* Checks to be done before deleting - the barrier is for preventing stale timestamp
 * before the FROZEN status is set under read reordering situations. Different from a
 * removal check, the type check is also performed against the slot.
 * CAP - The pointer to the capability slot to check for deletion.
 * TEMP - A temporary variable, for compare-and-swap. 
 * TYPE - What type should we anticipate when we check against the slot? */
#define RME_CAP_DEL_CHECK(CAP,TEMP,TYPE) \
do \
{ \
    /* Atomic read - Need a read acquire barrier here to avoid stale reads below */ \
    (TEMP)=RME_READ_ACQUIRE(&((CAP)->Head.Type_Stat)); \
    /* See if the slot is frozen */ \
    if(RME_UNLIKELY(RME_CAP_STAT(TEMP)!=RME_CAP_STAT_FROZEN)) \
        return RME_ERR_CAP_FROZEN; \
    /* See if the cap type is correct. Only deletion checks type, while removing does not */ \
    if(RME_UNLIKELY(RME_CAP_TYPE(TEMP)!=(TYPE))) \
        return RME_ERR_CAP_TYPE; \
    /* See if the slot is quiescent */ \
    if(RME_UNLIKELY(RME_CAP_QUIE((CAP)->Head.Timestamp)==0)) \
        return RME_ERR_CAP_QUIE; \
    /* To use deletion, we must be an unreferenced root */ \
    if(RME_UNLIKELY(((CAP)->Head.Root_Ref)!=0)) \
    { \
        /* Defrost the cap and return */ \
        RME_CAP_DEFROST(CAP,TEMP); \
        return RME_ERR_CAP_REFCNT; \
    } \
    /* The only case where the Root_Ref is 0 is that this is a unreferenced root cap */ \
    RME_ASSERT(RME_CAP_ATTR(TEMP)==RME_CAP_ATTR_ROOT); \
} \
while(0)

/* Checks to be done before removal - the barrier is for preventing stale timestamp
 * before the FROZEN bit is set under read reordering situations. Different from a 
 * deletion check, the type of the slot will not be checked.
 * CAP - The pointer to the capability slot to check for removal.
 * TEMP - A temporary variable, for compare-and-swap. */
#define RME_CAP_REM_CHECK(CAP,TEMP) \
do \
{ \
    /* Atomic read - Need a read acquire barrier here to avoid stale reads below */ \
    (TEMP)=RME_READ_ACQUIRE(&((CAP)->Head.Type_Stat)); \
    /* See if the slot is frozen */ \
    if(RME_UNLIKELY(RME_CAP_STAT(TEMP)!=RME_CAP_STAT_FROZEN)) \
        return RME_ERR_CAP_FROZEN; \
    /* See if the slot is quiescent */ \
    if(RME_UNLIKELY(RME_CAP_QUIE((CAP)->Head.Timestamp)==0)) \
        return RME_ERR_CAP_QUIE; \
    /* To use removal, we must be a leaf */ \
    if(RME_UNLIKELY(RME_CAP_ATTR(TEMP)==RME_CAP_ATTR_ROOT)) \
        return RME_ERR_CAP_ROOT; \
} \
while(0)

/* Actually remove/delete the cap.
 * CAP - The pointer to the capability slot to delete or remove.
 * TEMP - A temporary variable, for compare-and-swap's old value. */
#define RME_CAP_REMDEL(CAP,TEMP) \
do \
{ \
    /* If this fails, then it means that somebody have deleted/removed it first */ \
    if(RME_UNLIKELY(RME_COMP_SWAP(&((CAP)->Head.Type_Stat),(TEMP),0)==RME_CASFAIL)) \
        return RME_ERR_CAP_NULL; \
} \
while(0)

/* Check if we can take the slot, if we can, just take it. This also updates the timestamp,
 * so that we can enforce creation-freezing quiescence. We must update the counter after we
 * freeze the slot to ensure that we obtain exclusive access to it, and we must ensure that
 * other cores also see it that way.
 * CAP - The pointer to the capability slot to occupy.
 * TEMP - A temporary variable, for compare-and-swap. */
#define RME_CAPTBL_OCCUPY(CAP) \
do \
{ \
    /* Check if anything is there. If there is nothing there, the Type_Ref must be 0 */ \
    if(RME_UNLIKELY(RME_COMP_SWAP(&((CAP)->Head.Type_Stat), 0, \
                                  RME_CAP_TYPE_STAT(RME_CAP_TYPE_NOP,RME_CAP_STAT_CREATING,RME_CAP_ATTR_ROOT))==RME_CASFAIL)) \
        return RME_ERR_CAP_EXIST; \
    /* We have taken the slot. Now log the quiescence counter in. No barrier needed as our atomics are serializing */ \
    (CAP)->Head.Timestamp=RME_Timestamp; \
} \
while(0)

/* Get the capability slot from the master table according to a 1-level encoding.
 * This will not check the validity of the slot; nor will it try to resolve 2-level
 * encodings.
 * CAPTBL - The master capability table.
 * CAP_NUM - The capability number. Only allows 1-level encoding.
 * TYPE -  When assigning the pointer to the PARAM, what struct pointer type should I cast it into?
 * PARAM - The parameter to receive the pointer to the found capability slot. */
#define RME_CAPTBL_GETSLOT(CAPTBL,CAP_NUM,TYPE,PARAM) \
do \
{ \
    /* Check if the captbl is over range */ \
    if(RME_UNLIKELY((CAP_NUM)>=((CAPTBL)->Entry_Num))) \
        return RME_ERR_CAP_RANGE; \
    /* Get the slot position */ \
    (PARAM)=&(RME_CAP_GETOBJ((CAPTBL),TYPE)[(CAP_NUM)]); \
} \
while(0)

/* Get the capability from the master table according to capability number encoding.
 * The read acquire is for handling the case where the other CPU is creating
 * the capability. Suppose we are now operating on an empty slot, and the other CPU
 * have not started the creation yet. If we allow the other operation that follow the
 * TYPE check to reorder with it, then we are checking something that is not even
 * created at all (and by the time type check is taking place, the creation finishes
 * so it passes). This also will cause a race condition.
 * CAPTBL - The current master capability table.
 * CAP_NUM - The capability number. Allows 1- and 2-level encodings.
 * CAP_TYPE - The type of the capability, i.e. CAP_CAPTBL or CAP_PGTBL or CAP_THD, etc?
 * TYPE - When assigning the pointer to the PARAM, what struct pointer type should I cast it into?
 * PARAM - The parameter to receive the pointer to the found capability slot.
 * TEMP - A temporary variable, used to atomically keep a snapshot of Type_Ref when checking.
 */
#define RME_CAPTBL_GETCAP(CAPTBL,CAP_NUM,CAP_TYPE,TYPE,PARAM,TEMP) \
do \
{ \
    /* See if this is a 2-level cap */ \
    if(((CAP_NUM)&RME_CAPID_2L)==0) \
    { \
        /* Check if the captbl is over range */ \
        if(RME_UNLIKELY((CAP_NUM)>=((CAPTBL)->Entry_Num))) \
            return RME_ERR_CAP_RANGE; \
        /* Get the cap slot and check the type */ \
        (PARAM)=(TYPE)(&RME_CAP_GETOBJ(CAPTBL,struct RME_Cap_Struct*)[(CAP_NUM)]); \
        /* Atomic read - Need a read acquire barrier here to avoid stale reads below */ \
        (TEMP)=RME_READ_ACQUIRE(&((PARAM)->Head.Type_Stat)); \
        /* See if the capability is frozen */ \
        if(RME_UNLIKELY(RME_CAP_STAT(TEMP)==RME_CAP_STAT_FROZEN)) \
            return RME_ERR_CAP_FROZEN; \
        /* See if the type is correct */ \
        if(RME_UNLIKELY(RME_CAP_TYPE(TEMP)!=(CAP_TYPE))) \
            return RME_ERR_CAP_TYPE; \
    } \
    /* Yes, this is a 2-level cap */ \
    else \
    { \
        /* Check if the cap to potential captbl is over range */ \
        if(RME_UNLIKELY(RME_CAP_H(CAP_NUM)>=((CAPTBL)->Entry_Num))) \
            return RME_ERR_CAP_RANGE; \
        /* Get the cap slot */ \
        (PARAM)=(TYPE)(&RME_CAP_GETOBJ(CAPTBL,struct RME_Cap_Captbl*)[RME_CAP_H(CAP_NUM)]); \
        /* Atomic read - Need a read acquire barrier here to avoid stale reads below */ \
        (TEMP)=RME_READ_ACQUIRE(&((PARAM)->Head.Type_Stat)); \
        /* See if the capability table is frozen for deletion or removal */ \
        if(RME_UNLIKELY(RME_CAP_STAT(TEMP)==RME_CAP_STAT_FROZEN)) \
            return RME_ERR_CAP_FROZEN; \
        /* See if this is a captbl */ \
        if(RME_UNLIKELY(RME_CAP_TYPE(TEMP)!=RME_CAP_TYPE_CAPTBL)) \
            return RME_ERR_CAP_TYPE; \
        /* Check if the 2nd-layer captbl is over range */ \
        if(RME_UNLIKELY(RME_CAP_L(CAP_NUM)>=(((struct RME_Cap_Captbl*)(PARAM))->Entry_Num))) \
            return RME_ERR_CAP_RANGE; \
        /* Get the cap slot and check the type */ \
        (PARAM)=(TYPE)(&RME_CAP_GETOBJ(PARAM,struct RME_Cap_Struct*)[RME_CAP_L(CAP_NUM)]); \
        /* Atomic read - Need a read acquire barrier here to avoid stale reads below */ \
        (TEMP)=RME_READ_ACQUIRE(&((PARAM)->Head.Type_Stat)); \
        /* See if the capability is frozen */ \
        if(RME_UNLIKELY(RME_CAP_STAT(TEMP)==RME_CAP_STAT_FROZEN)) \
            return RME_ERR_CAP_FROZEN; \
        /* See if the type is correct */ \
        if(RME_UNLIKELY(RME_CAP_TYPE(TEMP)!=(CAP_TYPE))) \
            return RME_ERR_CAP_TYPE; \
    } \
} \
while(0)

/* Page Table ****************************************************************/
/* Driver layer error reporting macro */
#define RME_ERR_PGT_OPFAIL                          ((rme_ptr_t)(-1))

/* Page table flag arrangement
* 32-bit systems: Maximum page table size 2^12 = 4096
* [31    High Limit    20] [19    Low Limit    8][7    Flags    0]
* 64-bit systems: Maximum page table size 2^28 = 268435456
* [63    High Limit    36] [35    Low Limit    8][7    Flags    0] */
/* Maximum number of entries in a page table */
#define RME_PGTBL_MAX_ENTRY                         RME_POW2(sizeof(rme_ptr_t)*4-4)
/* Range high limit */ 
#define RME_PGTBL_FLAG_HIGH(X)                      ((X)>>(sizeof(rme_ptr_t)*4+4))
/* Range low limit */
#define RME_PGTBL_FLAG_LOW(X)                       (((X)>>8)&RME_MASK_END(sizeof(rme_ptr_t)*4-5))
/* Permission flags */
#define RME_PGTBL_FLAG_FLAGS(X)                     ((X)&RME_MASK_END(7))
/* The initial flag of boot-time page table - allows all range delegation access only */
#define RME_PGTBL_FLAG_FULL_RANGE                   RME_MASK_START(sizeof(rme_ptr_t)*4+4)

/* Page table start address/top-level attributes */
#define RME_PGTBL_START(X)                          ((X)&RME_MASK_START(1))
#define RME_PGTBL_TOP                               (1)
#define RME_PGTBL_NOM                               (0)

/* Size order and number order */
#define RME_PGTBL_SIZEORD(X)                        ((X)>>(sizeof(rme_ptr_t)*4))
#define RME_PGTBL_NUMORD(X)                         ((X)&RME_MASK_END(sizeof(rme_ptr_t)*4-1))
#define RME_PGTBL_ORDER(SIZE,NUM)                   (((SIZE)<<(sizeof(rme_ptr_t)*4))|(NUM))
    
/* Kernel Memory *************************************************************/
/* Kernel memory function capability flag arrangement - extended flags used, Granularity always 64 bytes min,
* because the reserved bits is always 6 bits, and the flags field in the Ext_Flag is always 6 bits too.
* 32-bit systems:
* [31          High Limit[31:16]         16] [15       Low Limit[31:16]       0]  Flags
* [31 High Limit[15: 6] 22] [21 Reserved 16] [15 Low Limit[15: 6] 6] [5 Flags 0]  Ext_Flags
* 64-bit systems:
* [63          High Limit[64:32]         32] [31       Low Limit[64:32]       0]  Flags
* [63 High Limit[31: 6] 38] [37 Reserved 32] [31 Low Limit[31: 6] 6] [5 Flags 0]  Ext_Flags
*/
#define RME_KMEM_FLAG_HIGH_F(FLAGS)                 ((FLAGS)&RME_MASK_START(sizeof(rme_ptr_t)*4))
#define RME_KMEM_FLAG_HIGH_E(EFLAGS)                (((EFLAGS)>>(sizeof(rme_ptr_t)*4))&RME_MASK_START(6))
#define RME_KMEM_FLAG_HIGH(FLAGS,EFLAGS)            (RME_KMEM_FLAG_HIGH_F(FLAGS)|RME_KMEM_FLAG_HIGH_E(EFLAGS))
#define RME_KMEM_FLAG_LOW_F(FLAGS)                  ((FLAGS)<<(sizeof(rme_ptr_t)*4))
#define RME_KMEM_FLAG_LOW_E(EFLAGS)                 ((EFLAGS)&RME_MASK(sizeof(rme_ptr_t)*4-1,6))
#define RME_KMEM_FLAG_LOW(FLAGS,EFLAGS)             (RME_KMEM_FLAG_LOW_F(FLAGS)|RME_KMEM_FLAG_LOW_E(EFLAGS))
#define RME_KMEM_FLAG_FLAGS(EFLAGS)                 ((EFLAGS)&RME_MASK(5,0))

/* Process and Thread ********************************************************/
/* Thread states */
/* The thread is currently running */
#define RME_THD_RUNNING                             (0)
/* The thread is currently ready for scheduling */
#define RME_THD_READY                               (1)
/* The thread is currently blocked on a asynchronous send endpoint */
#define RME_THD_BLOCKED                             (2)
/* The thread just ran out of time */
#define RME_THD_TIMEOUT                             (3)
/* The thread is blocked on the sched rcv endpoint */
#define RME_THD_SCHED_BLOCKED                       (4)
/* The thread is stopped due to a fault */
#define RME_THD_FAULT                               (5)

/* Priority level bitmap */
#define RME_PRIO_WORD_NUM                           (RME_MAX_PREEMPT_PRIO>>RME_WORD_ORDER)

/* Thread binding state */
#define RME_THD_UNBINDED                            ((struct RME_CPU_Local*)RME_ALLBITS)
/* Thread sched rcv faulty state */
#define RME_THD_FAULT_FLAG                          (1U<<(sizeof(rme_ptr_t)*8-2))
/* Init thread infinite time marker */
#define RME_THD_INIT_TIME                           (RME_ALLBITS>>1)
/* Other thread infinite time marker */
#define RME_THD_INF_TIME                            (RME_THD_INIT_TIME-1)
/* Thread time upper limit - always ths infinite time */
#define RME_THD_MAX_TIME                            (RME_THD_INF_TIME)
#define RME_THD_SIZE                                sizeof(struct RME_Thd_Struct)
    
/* Time checking macro */
#define RME_TIME_CHECK(DST,AMOUNT) \
do \
{ \
    /* Check if exceeded maximum time or overflowed */ \
    if(RME_UNLIKELY((((DST)+(AMOUNT))>=RME_THD_MAX_TIME)||(((DST)+(AMOUNT))<(DST)))) \
        return RME_ERR_PTH_OVERFLOW; \
} \
while(0)

/* Signal and Invocation *****************************************************/
/* The maximum number of signals on an endpoint */
#define RME_MAX_SIG_NUM                             (RME_ALLBITS>>1)

/* The kernel object sizes */
#define RME_INV_SIZE                                sizeof(struct RME_Inv_Struct)
#define RME_SIG_SIZE                                sizeof(struct RME_Sig_Struct)

/* Get the top of invocation stack */
#define RME_INVSTK_TOP(THD)                         ((struct RME_Inv_Struct*)((((THD)->Inv_Stack.Next)==&((THD)->Inv_Stack))? \
                                                                              (0): \
                                                                              ((THD)->Inv_Stack.Next)))

/* Kernel Function ***********************************************************/
/* Driver layer error reporting macro */
#define RME_ERR_KERN_OPFAIL                         (-1)
/* Kernel function capability flag arrangement
* 32-bit systems: Maximum kernel function number 2^16
* [31        High Limit        16] [15        Low Limit        0]
* 64-bit systems: Maximum kernel function number 2^32
* [63        High Limit        32] [31        Low Limit        0] */
#define RME_KERN_FLAG_HIGH(X)                       ((X)>>(sizeof(rme_ptr_t)*4))
#define RME_KERN_FLAG_LOW(X)                        ((X)&RME_MASK_END((sizeof(rme_ptr_t)*4)-1))
#define RME_KERN_FLAG_FULL_RANGE                    RME_MASK_START(sizeof(rme_ptr_t)*4)

/* __RME_KERNEL_H_DEFS__ */
#endif
/* __HDR_DEFS__ */
#endif
/* End Defines ***************************************************************/

/* Structs *******************************************************************/
#ifdef __HDR_STRUCTS__
#ifndef __RME_KERNEL_H_STRUCTS__
#define __RME_KERNEL_H_STRUCTS__

/* Use defines in these headers */
#define __HDR_DEFS__
#undef __HDR_DEFS__

/*****************************************************************************/
/* Generic *******************************************************************/
/* Capability header structure */
struct RME_Cap_Head
{
    /* The type, status */
    rme_ptr_t Type_Stat;
    /* The root capability (for non-root caps), or the reference count from everything else (for root caps) */
    rme_ptr_t Root_Ref;
    /* The suboperation capability flags */
    rme_ptr_t Flags;
    /* The object address */
    rme_ptr_t Object;
    /* The freeze timestamp */
    rme_ptr_t Timestamp;
};

/* Generic capability structure */
struct RME_Cap_Struct
{
    struct RME_Cap_Head Head;
    rme_ptr_t Info[3];
};

/* Capability Table **********************************************************/
/* Capability table capability structure */
struct RME_Cap_Captbl
{
    struct RME_Cap_Head Head;
    /* The number of entries in this captbl */
    rme_ptr_t Entry_Num;
    
    rme_ptr_t Info[2];
};

/* Page Table ****************************************************************/
/* Page table capability structure */
struct RME_Cap_Pgtbl
{
    struct RME_Cap_Head Head;
    /* The entry size/number order */
    rme_ptr_t Size_Num_Order;
    /* The base address of this page table */
    rme_ptr_t Base_Addr;
    /* Address space ID, if applicable */
    rme_ptr_t ASID;
};

/* Kernel Memory *************************************************************/
/* Kernel memory capability structure */
struct RME_Cap_Kmem
{
    struct RME_Cap_Head Head;
    /* The start address of the allowed kernel memory */
    rme_ptr_t Start;
    /* The end address of the allowed kernel memory */
    rme_ptr_t End;
    rme_ptr_t Info[1];
};

/* Process and Thread ********************************************************/
/* List head structure */
struct RME_List
{
    volatile struct RME_List* Prev;
    volatile struct RME_List* Next;
};

/* Per-CPU run queue structure */
struct RME_Run_Struct
{
    /* The bitmap marking to show if there are active threads at a run level */
    rme_ptr_t Bitmap[RME_PRIO_WORD_NUM];
    /* The actual RME running list */
    struct RME_List List[RME_MAX_PREEMPT_PRIO];
};

/* Process capability structure - does not have an object */
struct RME_Cap_Proc
{
    struct RME_Cap_Head Head;
    /* The capability table struct */
    struct RME_Cap_Captbl* Captbl;
    /* The page table struct */
    struct RME_Cap_Pgtbl* Pgtbl;
    rme_ptr_t Info[1];
};

/* Thread scheduling state structure */
struct RME_Thd_Sched
{
    /* The runnable thread header - This will be inserted into the per-core
     * run queue */
    struct RME_List Run;
    /* The list head for notifications - This will be inserted into scheduler
     * threads' event list */
    struct RME_List Notif; 
    /* What's the TID of the thread? */
    rme_ptr_t TID;
    /* What is the CPU-local data structure that this thread is on? If this is
     * 0xFF....FF, then this is not binded to any core. "struct RME_CPU_Local" is
     * not yet defined here but compilation will still pass - it is a pointer */
    struct RME_CPU_Local* CPU_Local;
    /* How much time slices is left for this thread? */
    rme_ptr_t Slices;
    /* What is the current state of the thread? */
    rme_ptr_t State;
    /* What is the reason for the fault that killed the thread? */
    rme_ptr_t Fault;
    /* What's the priority of the thread? */
    rme_ptr_t Prio;
    /* What's the maximum priority allowed for this thread? */
    rme_ptr_t Max_Prio;
    /* What signal endpoint does this thread block on? */
    struct RME_Cap_Sig* Signal;
    /* Which process is it created in? */
    struct RME_Cap_Proc* Proc;
    /* Am I referenced by someone as a scheduler? */
    rme_ptr_t Sched_Ref;
    /* What is its scheduler thread? */
    struct RME_Thd_Struct* Sched_Thd;
    /* What is the signal endpoint to send to if we have scheduler notifications? (optional) */
    struct RME_Cap_Sig* Sched_Sig;
    /* The event list for the thread */
    struct RME_List Event;
};

/* Thread register set structure */
struct RME_Thd_Regs 
{
    /* The register set - architecture specific */
    struct RME_Reg_Struct Reg;
    /* The co-processor/peripheral context - architecture specific.
     * This usually contains the FPU data */
    struct RME_Cop_Struct Cop_Reg;
};

/* Thread object structure */
struct RME_Thd_Struct
{
    /* The thread scheduling structure */
    struct RME_Thd_Sched Sched;
    /* The pointer to current register set */
    struct RME_Thd_Regs* Cur_Reg;
    /* The default register storage area - may be used or not */
    struct RME_Thd_Regs Def_Reg;
    /* The thread synchronous invocation stack */
    struct RME_List Inv_Stack;
};

/* Thread capability structure */
struct RME_Cap_Thd
{
    struct RME_Cap_Head Head;
    rme_ptr_t Info[3];
};

/* Signal and Invocation *****************************************************/
/* Signal endpoint capability structure - does not have an object */
struct RME_Cap_Sig
{
    struct RME_Cap_Head Head;
    /* The number of signals sent to here */
    rme_ptr_t Sig_Num;
    /* What thread blocked on this one */
    struct RME_Thd_Struct* Thd;
    rme_ptr_t Info[1];
};

/* Invocation object structure */
struct RME_Inv_Struct
{
    /* This will be inserted into a thread structure */
    struct RME_List Head;
    /* The process pointer */
    struct RME_Cap_Proc* Proc;
    /* Is the invocation currently active? If yes, we cannot delete */
    struct RME_Thd_Struct* Active;
    /* The entry and stack of the invocation */
    rme_ptr_t Entry;
    rme_ptr_t Stack;
    /* Do we return immediately on fault? */
    rme_ptr_t Fault_Ret_Flag;
    /* The registers to be saved in the invocation */
    struct RME_Iret_Struct Ret;
};

/* Invocation capability structure */
struct RME_Cap_Inv
{
    struct RME_Cap_Head Head;
    rme_ptr_t Info[3];
};

/* CPU-local data structure */
struct RME_CPU_Local
{
    /* The CPUID of the CPU */
    rme_ptr_t CPUID;
    /* The current thread on the CPU */
    struct RME_Thd_Struct* Cur_Thd;
    /* The tick timer signal endpoint */
    struct RME_Cap_Sig* Tick_Sig;
    /* The vector signal endpoint */
    struct RME_Cap_Sig* Vect_Sig;
    /* The runqueue and bitmap */
    struct RME_Run_Struct Run;
};

/* Kernel Function ***********************************************************/
/* Kernel function capability structure */
struct RME_Cap_Kern
{
    struct RME_Cap_Head Head;
    rme_ptr_t Info[3];
};

/*****************************************************************************/
/* __RME_KERNEL_H_STRUCTS__ */
#endif
/* __HDR_STRUCTS__ */
#endif
/* End Structs ***************************************************************/

/* Private Global Variables **************************************************/
#if(!(defined __HDR_DEFS__||defined __HDR_STRUCTS__))
#ifndef __RME_KERNEL_MEMBERS__
#define __RME_KERNEL_MEMBERS__

/* In this way we can use the data structures and definitions in the headers */
#define __HDR_DEFS__

#undef __HDR_DEFS__

#define __HDR_STRUCTS__

#undef __HDR_STRUCTS__

/* If the header is not used in the public mode */
#ifndef __HDR_PUBLIC_MEMBERS__
/*****************************************************************************/
/* Kernel object table */
static rme_ptr_t RME_Kotbl[RME_KOTBL_WORD_NUM];
/*****************************************************************************/
/* End Private Global Variables **********************************************/

/* Private C Function Prototypes *********************************************/
/* Generic *******************************************************************/
static rme_ret_t __RME_Low_Level_Check(void);
static rme_ret_t _RME_Syscall_Init(void);

/* Capability Table **********************************************************/
/* Capability system calls */
static rme_ret_t _RME_Captbl_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt, 
                                 rme_cid_t Cap_Kmem, rme_cid_t Cap_Crt, rme_ptr_t Raddr, rme_ptr_t Entry_Num);
static rme_ret_t _RME_Captbl_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Del, rme_cid_t Cap_Del);
static rme_ret_t _RME_Captbl_Frz(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Frz, rme_cid_t Cap_Frz);
static rme_ret_t _RME_Captbl_Add(struct RME_Cap_Captbl* Captbl,
                                 rme_cid_t Cap_Captbl_Dst, rme_cid_t Cap_Dst, 
                                 rme_cid_t Cap_Captbl_Src, rme_cid_t Cap_Src,
                                 rme_ptr_t Flags, rme_ptr_t Ext_Flags);
static rme_ret_t _RME_Captbl_Rem(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Rem, rme_cid_t Cap_Rem);

/* Page Table ****************************************************************/
/* Page table system calls */
static rme_ret_t _RME_Pgtbl_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                                rme_cid_t Cap_Kmem, rme_cid_t Cap_Pgtbl, rme_ptr_t Raddr,
                                rme_ptr_t Base_Addr, rme_ptr_t Top_Flag, rme_ptr_t Size_Order, rme_ptr_t Num_Order);
static rme_ret_t _RME_Pgtbl_Del(struct RME_Cap_Captbl* Captbl,  rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl);
static rme_ret_t _RME_Pgtbl_Add(struct RME_Cap_Captbl* Captbl, 
                                rme_cid_t Cap_Pgtbl_Dst, rme_ptr_t Pos_Dst, rme_ptr_t Flags_Dst,
                                rme_cid_t Cap_Pgtbl_Src, rme_ptr_t Pos_Src, rme_ptr_t Index);
static rme_ret_t _RME_Pgtbl_Rem(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Pgtbl, rme_ptr_t Pos);
static rme_ret_t _RME_Pgtbl_Con(struct RME_Cap_Captbl* Captbl,
                                rme_cid_t Cap_Pgtbl_Parent, rme_ptr_t Pos,
                                rme_cid_t Cap_Pgtbl_Child, rme_ptr_t Flags_Child);
static rme_ret_t _RME_Pgtbl_Des(struct RME_Cap_Captbl* Captbl, 
                                rme_cid_t Cap_Pgtbl_Parent, rme_ptr_t Pos, rme_cid_t Cap_Pgtbl_Child);

/* Process and Thread ********************************************************/
/* In-kernel ready-queue primitives */
static rme_ret_t _RME_Run_Ins(struct RME_Thd_Struct* Thd);
static rme_ret_t _RME_Run_Del(struct RME_Thd_Struct* Thd);
static struct RME_Thd_Struct* _RME_Run_High(struct RME_CPU_Local* CPU_Local);
static rme_ret_t _RME_Run_Notif(struct RME_Thd_Struct* Thd);
static rme_ret_t _RME_Run_Swt(struct RME_Reg_Struct* Reg,
                              struct RME_Thd_Struct* Curr_Thd, 
                              struct RME_Thd_Struct* Next_Thd);
/* Process system calls */
static rme_ret_t _RME_Proc_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                               rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl);
static rme_ret_t _RME_Proc_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Proc);
static rme_ret_t _RME_Proc_Cpt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl);
static rme_ret_t _RME_Proc_Pgt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Proc, rme_cid_t Cap_Pgtbl);
/* Thread system calls */
static rme_ret_t _RME_Thd_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Kmem,
                              rme_cid_t Cap_Thd, rme_cid_t Cap_Proc, rme_ptr_t Max_Prio, rme_ptr_t Raddr);
static rme_ret_t _RME_Thd_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Thd);
static rme_ret_t _RME_Thd_Exec_Set(struct RME_Cap_Captbl* Captbl,
                                   rme_cid_t Cap_Thd, rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Param);
static rme_ret_t _RME_Thd_Hyp_Set(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Thd, rme_ptr_t Kaddr);
static rme_ret_t _RME_Thd_Sched_Bind(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Thd,
                                     rme_cid_t Cap_Thd_Sched, rme_cid_t Cap_Sig, rme_tid_t TID, rme_ptr_t Prio);
static rme_ret_t _RME_Thd_Sched_Prio(struct RME_Cap_Captbl* Captbl,
                                     struct RME_Reg_Struct* Reg, rme_cid_t Cap_Thd, rme_ptr_t Prio);
static rme_ret_t _RME_Thd_Sched_Free(struct RME_Cap_Captbl* Captbl, 
                                     struct RME_Reg_Struct* Reg, rme_cid_t Cap_Thd);
static rme_ret_t _RME_Thd_Sched_Rcv(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg, rme_cid_t Cap_Thd);
static rme_ret_t _RME_Thd_Time_Xfer(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                                    rme_cid_t Cap_Thd_Dst, rme_cid_t Cap_Thd_Src, rme_ptr_t Time);
static rme_ret_t _RME_Thd_Swt(struct RME_Cap_Captbl* Captbl,
                              struct RME_Reg_Struct* Reg,
                              rme_cid_t Cap_Thd, rme_ptr_t Yield);
                              
/* Signal and Invocation *****************************************************/
/* Signal system calls */
static rme_ret_t _RME_Sig_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Sig);
static rme_ret_t _RME_Sig_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Sig);
static rme_ret_t _RME_Sig_Snd(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg, rme_cid_t Cap_Sig);
static rme_ret_t _RME_Sig_Rcv(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                              rme_cid_t Cap_Sig, rme_ptr_t Option);
/* Invocation system calls */
static rme_ret_t _RME_Inv_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                              rme_cid_t Cap_Kmem, rme_cid_t Cap_Inv, rme_cid_t Cap_Proc, rme_ptr_t Raddr);
static rme_ret_t _RME_Inv_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Inv);
static rme_ret_t _RME_Inv_Set(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Inv,
                              rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Fault_Ret_Flag);
static rme_ret_t _RME_Inv_Act(struct RME_Cap_Captbl* Captbl, 
                              struct RME_Reg_Struct* Reg, rme_cid_t Cap_Inv, rme_ptr_t Param);
static rme_ret_t _RME_Inv_Ret(struct RME_Reg_Struct* Reg, rme_ptr_t Retval, rme_ptr_t Fault_Flag);

/* Kernel Function ***********************************************************/
static rme_ret_t _RME_Kern_Act(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                               rme_cid_t Cap_Kern, rme_ptr_t Func_ID, rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2);

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
/* Current timestamp counter */
__EXTERN__ rme_ptr_t RME_Timestamp;
/*****************************************************************************/

/* End Public Global Variables ***********************************************/

/* Public C Function Prototypes **********************************************/
/* Generic *******************************************************************/
/* Kernel entry */
__EXTERN__ rme_ret_t RME_Kmain(void);
/* System call handler */
__EXTERN__ void _RME_Svc_Handler(struct RME_Reg_Struct* Reg);
/* Increase counter */
__EXTERN__ rme_ptr_t _RME_Timestamp_Inc(rme_cnt_t Value);
/* Timer interrupt handler */
__EXTERN__ void _RME_Tick_SMP_Handler(struct RME_Reg_Struct* Reg);
__EXTERN__ void _RME_Tick_Handler(struct RME_Reg_Struct* Reg);
/* Memory helpers */
__EXTERN__ void _RME_Clear(void* Addr, rme_ptr_t Size);
__EXTERN__ rme_ret_t _RME_Memcmp(const void* Ptr1, const void* Ptr2, rme_ptr_t Num);
__EXTERN__ void _RME_Memcpy(void* Dst, void* Src, rme_ptr_t Num);
/* Debugging helpers */
__EXTERN__ rme_cnt_t RME_Print_Uint(rme_ptr_t Uint);
__EXTERN__ rme_cnt_t RME_Print_Int(rme_cnt_t Int);
__EXTERN__ rme_cnt_t RME_Print_String(rme_s8_t* String);

/* Capability Table **********************************************************/
/* Boot-time calls */
__EXTERN__ rme_ret_t _RME_Captbl_Boot_Init(rme_cid_t Cap_Captbl, rme_ptr_t Vaddr, rme_ptr_t Entry_Num);
__EXTERN__ rme_ret_t _RME_Captbl_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                                          rme_cid_t Cap_Crt, rme_ptr_t Vaddr, rme_ptr_t Entry_Num);

/* Page Table ****************************************************************/
/* Boot-time calls */
__EXTERN__ rme_ret_t _RME_Pgtbl_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                                         rme_cid_t Cap_Pgtbl, rme_ptr_t Vaddr, rme_ptr_t Base_Addr,
                                         rme_ptr_t Top_Flag, rme_ptr_t Size_Order, rme_ptr_t Num_Order);
__EXTERN__ rme_ret_t _RME_Pgtbl_Boot_Con(struct RME_Cap_Captbl* Captbl,
                                         rme_cid_t Cap_Pgtbl_Parent, rme_ptr_t Pos,
                                         rme_cid_t Cap_Pgtbl_Child, rme_ptr_t Flags_Child);
__EXTERN__ rme_ret_t _RME_Pgtbl_Boot_Add(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Pgtbl, 
                                         rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags);

/* Kernel Memory *************************************************************/
__EXTERN__ rme_ret_t _RME_Kotbl_Init(rme_ptr_t Words);
/* Kernel memory operations (in case HAL needs to allocate kernel memory) */
__EXTERN__ rme_ret_t _RME_Kotbl_Mark(rme_ptr_t Kaddr, rme_ptr_t Size);
__EXTERN__ rme_ret_t _RME_Kotbl_Erase(rme_ptr_t Kaddr, rme_ptr_t Size);
/* Boot-time calls */
__EXTERN__ rme_ret_t _RME_Kmem_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                                        rme_cid_t Cap_Kmem, rme_ptr_t Start, rme_ptr_t End, rme_ptr_t Flags);

/* Process and Thread ********************************************************/
/* Linked list operations */
__EXTERN__ void __RME_List_Crt(volatile struct RME_List* Head);
__EXTERN__ void __RME_List_Del(volatile struct RME_List* Prev,
                               volatile struct RME_List* Next);
__EXTERN__ void __RME_List_Ins(volatile struct RME_List* New,
                               volatile struct RME_List* Prev,
                               volatile struct RME_List* Next);
/* Initialize per-CPU data structures */
__EXTERN__ void _RME_CPU_Local_Init(struct RME_CPU_Local* CPU_Local, rme_ptr_t CPUID);
/* Thread fatal killer */
__EXTERN__ rme_ret_t __RME_Thd_Fatal(struct RME_Reg_Struct* Regs, rme_ptr_t Fault);                              
/* Boot-time calls */
__EXTERN__ rme_ret_t _RME_Proc_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                                        rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl);
__EXTERN__ rme_ret_t _RME_Thd_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                                       rme_cid_t Cap_Thd, rme_cid_t Cap_Proc, rme_ptr_t Vaddr,
                                       rme_ptr_t Prio, struct RME_CPU_Local* CPU_Local);

/* Signal and Invocation *****************************************************/
/* Kernel send facilities */
__EXTERN__ rme_ret_t _RME_Kern_Snd(struct RME_Cap_Sig* Sig);
__EXTERN__ void _RME_Kern_High(struct RME_Reg_Struct* Reg, struct RME_CPU_Local* CPU_Local);
/* Boot-time calls */
__EXTERN__ rme_ret_t _RME_Sig_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Sig);

/* Kernel Function ***********************************************************/
__EXTERN__ rme_ret_t _RME_Kern_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Kern);

/*****************************************************************************/
/* Undefine "__EXTERN__" to avoid redefinition */
#undef __EXTERN__
/* __RME_KERNEL_MEMBERS__ */
#endif
/* !(defined __HDR_DEFS__||defined __HDR_STRUCTS__) */
#endif
/* End Public C Function Prototypes ******************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
