/******************************************************************************
Filename    : rme_platform_a7m.c
Author      : pry
Date        : 01/04/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The hardware abstraction layer for Cortex-M microcontrollers.

* Generic Code Section *******************************************************
Small utility functions that can be either implemented with C or assembly, and 
the entry of the kernel. Also responsible for debug printing and CPUID getting.

* Handler Code Section *******************************************************
Contains fault handlers, generic interrupt handlers and kernel function handlers.

* Initialization Code Section ************************************************
Low-level initialization and booting. If we have multiple processors, the 
booting of all processors are dealt with here.

* Register Manipulation Section **********************************************
Low-level register manipulations and parameter extractions.

* Page Table Section *********************************************************
Page table related operations are all here.
The page table conforms to page table implementation style 1, which
states that the 

******************************************************************************/

/* Includes ******************************************************************/
#define __HDR_DEFS__
#include "Platform/A7M/rme_platform_a7m.h"
#include "Kernel/rme_kernel.h"
#undef __HDR_DEFS__

#define __HDR_STRUCTS__
#include "Platform/A7M/rme_platform_a7m.h"
#include "Kernel/rme_kernel.h"
#undef __HDR_STRUCTS__

/* Private include */
#include "Platform/A7M/rme_platform_a7m.h"

#define __HDR_PUBLIC_MEMBERS__
#include "Kernel/rme_kernel.h"
#undef __HDR_PUBLIC_MEMBERS__
/* End Includes **************************************************************/

/* Begin Function:main ********************************************************
Description : The entry of the operating system. This function is for compatibility
              with the toolchains.
Input       : None.
Output      : None.
Return      : int - Dummy value, this function never returns.
******************************************************************************/
int main(void)
{
    /* The main function of the kernel - we will start our kernel boot here */
    _RME_Kmain(RME_KMEM_STACK_ADDR);
    return 0;
}
/* End Function:main *********************************************************/

/* Begin Function:__RME_A7M_Comp_Swap *****************************************
Description : The compare-and-swap atomic instruction. If the Old value is equal to
              *Ptr, then set the *Ptr as New and return 1; else return 0.
              On Cortex-M there is only one core. There's basically no need to do
              anything special, and because we are already in kernel, relevant
              interrupts are already masked by default.
Input       : rme_ptr_t* Ptr - The pointer to the data.
              rme_ptr_t Old - The old value.
              rme_ptr_t New - The new value.
Output      : rme_ptr_t* Ptr - The pointer to the data.
Return      : rme_ptr_t - If successful, 1; else 0.
******************************************************************************/
rme_ptr_t __RME_A7M_Comp_Swap(rme_ptr_t* Ptr, rme_ptr_t Old, rme_ptr_t New)
{
    if(*Ptr==Old)
    {
        *Ptr=New;
        return 1;
    }
    
    return 0;
}
/* End Function:__RME_A7M_Comp_Swap ******************************************/

/* Begin Function:__RME_A7M_Fetch_Add *****************************************
Description : The fetch-and-add atomic instruction. Increase the value that is 
              pointed to by the pointer, and return the value before addition.
Input       : rme_ptr_t* Ptr - The pointer to the data.
              rme_cnt_t Addend - The number to add.
Output      : rme_ptr_t* Ptr - The pointer to the data.
Return      : rme_ptr_t - The value before the addition.
******************************************************************************/
rme_ptr_t __RME_A7M_Fetch_Add(rme_ptr_t* Ptr, rme_cnt_t Addend)
{
    rme_ptr_t Old;
    
    Old=*Ptr;
    *Ptr=Old+Addend;
    
    return Old;
}
/* End Function:__RME_A7M_Fetch_Add ******************************************/

/* Begin Function:__RME_A7M_Fetch_And *****************************************
Description : The fetch-and-logic-and atomic instruction. Logic AND the pointer
              value with the operand, and return the value before logic AND.
Input       : rme_ptr_t* Ptr - The pointer to the data.
              rme_cnt_t Operand - The number to logic AND with the destination.
Output      : rme_ptr_t* Ptr - The pointer to the data.
Return      : rme_ptr_t - The value before the AND operation.
******************************************************************************/
rme_ptr_t __RME_A7M_Fetch_And(rme_ptr_t* Ptr, rme_ptr_t Operand)
{
    rme_ptr_t Old;
    
    Old=*Ptr;
    *Ptr=Old&Operand;
    
    return Old;
}
/* End Function:__RME_A7M_Fetch_And ******************************************/

/* Begin Function:__RME_A7M_Enable_Cache **************************************
Description : Enable the I-Cache and D-Cache for the core. This will only be called
              for processors that actually have cache.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_Enable_Cache(void)
{
    rme_ptr_t Sets;
    rme_ptr_t Ways;
    rme_ptr_t Set_Cnt;
    rme_ptr_t Way_Cnt;
    
    /* I-Cache */
    __RME_A7M_Barrier();
    RME_A7M_SCB_ICALLU=0;
    RME_A7M_SCB_CCR|=RME_A7M_SCB_CCR_IC;
    __RME_A7M_Barrier();
    
    /* D-Cache */
    RME_A7M_SCB_CSSELR=0;
    __RME_A7M_Barrier();

    Sets=RME_A7M_SCB_CCSIDR_SETS(RME_A7M_SCB_CCSIDR);
    Ways=RME_A7M_SCB_CCSIDR_WAYS(RME_A7M_SCB_CCSIDR);
    
    for(Set_Cnt=0;Set_Cnt<=Sets;Set_Cnt++)
    {
        for(Way_Cnt=0;Way_Cnt<=Ways;Way_Cnt++)
        {
            RME_A7M_SCB_DCISW=RME_A7M_SCB_DCISW_INV(Set_Cnt,Way_Cnt);
            __RME_A7M_Barrier();
        }
    }
    
    RME_A7M_SCB_CCR|=RME_A7M_SCB_CCR_DC;
    __RME_A7M_Barrier();
}
/* End Function:__RME_A7M_Enable_Cache ***************************************/

/* Begin Function:__RME_A7M_ITM_Putchar ***************************************
Description : Output a character through ITM.
Input       : char Char - The character to print.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_ITM_Putchar(char Char)
{
    /* ITM enabled & ITM port 0 enabled */
    if(((RME_A7M_ITM_TCR&RME_A7M_ITM_TCR_ITMENA)!=0)&&((RME_A7M_ITM_TER&0x01)!=0UL))
    {
        while(RME_A7M_ITM_PORT(0)==0)
            __RME_A7M_Barrier();
        
        *((volatile char*)&RME_A7M_ITM_PORT(0))=Char;
    }
}
/* End Function:__RME_A7M_ITM_Putchar ****************************************/

/* Begin Function:__RME_Putchar ***********************************************
Description : Output a character to console. In Cortex-M, under most circumstances, 
              we should use the ITM for such outputs.
Input       : char Char - The character to print.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Putchar(char Char)
{
    RME_A7M_PUTCHAR(Char);
    return 0;
}
/* End Function:__RME_Putchar ************************************************/

/* Begin Function:__RME_CPUID_Get *********************************************
Description : Get the CPUID. This is to identify where we are executing.
Input       : None.
Output      : None.
Return      : rme_ptr_t - The CPUID. On Cortex-M, this is certainly always 0.
******************************************************************************/
rme_ptr_t __RME_CPUID_Get(void)
{
    return 0;
}
/* End Function:__RME_CPUID_Get **********************************************/

/* Begin Function:__RME_A7M_Fault_Handler *************************************
Description : The fault handler of RME. In Cortex-M, this is used to handle multiple
              faults.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_A7M_Fault_Handler(struct RME_Reg_Struct* Reg)
{
    rme_ptr_t Cur_HFSR;
    rme_ptr_t Cur_CFSR;
    rme_ptr_t Cur_MMFAR;
    rme_ptr_t Flags;
    rme_ptr_t* Stack;
    struct RME_Cap_Proc* Proc;
    struct RME_Inv_Struct* Inv_Top;
    struct __RME_A7M_Pgtbl_Meta* Meta;
    
    /* Is it a kernel-level fault? If yes, panic */
    RME_ASSERT((Reg->LR&RME_A7M_EXC_RET_RET_USER)!=0);
    
    /* Get the address of this faulty address, and what caused this fault */
    Cur_HFSR=RME_A7M_SCB_HFSR;
    Cur_CFSR=RME_A7M_SCB_CFSR;
    Cur_MMFAR=RME_A7M_SCB_MMFAR;
    
    /* Are we activating the NMI? If yes, we directly soft lockup */
    RME_ASSERT((RME_A7M_SCB_ICSR&RME_A7M_ICSR_NMIPENDSET)==0);
    /* If this is a hardfault, make sure that it is not the vector table problem, or we lockup */
    RME_ASSERT((Cur_HFSR&RME_A7M_HFSR_VECTTBL)==0);
    /* Is this a escalated hard fault? If yes, and this is not a trivial debug fault, we lockup */
    if((Cur_HFSR&RME_A7M_HFSR_FORCED)!=0)
        RME_ASSERT((Cur_HFSR&RME_A7M_HFSR_DEBUGEVT)!=0);
    
    /* We cannot recover from the following: */
    if((Cur_CFSR&
        (RME_A7M_UFSR_DIVBYZERO|        /* Division by zero errors */
         RME_A7M_UFSR_UNALIGNED|        /* Unaligned access errors */
         RME_A7M_UFSR_NOCP|             /* Unpresenting coprocessor errors */
         RME_A7M_UFSR_INVPC|            /* Invalid PC from return load errors */
         RME_A7M_UFSR_INVSTATE|         /* Invalid EPSR state errors */
         RME_A7M_UFSR_UNDEFINSTR|       /* Undefined instruction errors */
         RME_A7M_BFSR_UNSTKERR|         /* Bus unstacking errors */ 
         RME_A7M_BFSR_PRECISERR|        /* Precise bus data errors */
         RME_A7M_BFSR_IBUSERR|          /* Bus instruction errors */
         RME_A7M_MFSR_MUNSTKERR))!=0)   /* MPU unstacking errors */
    {
        
        __RME_Thd_Fatal(Reg, Cur_CFSR);
    }
    /* Attempt recovery from memory management fault by MPU region swapping */
    else if((Cur_CFSR&RME_A7M_MFSR_MMARVALID)!=0)
    {
        /* This must be a data violation. This is the only case where MMAR will be loaded */
        RME_ASSERT((Cur_CFSR&RME_A7M_MFSR_DACCVIOL)!=0);
        /* There is a valid MMAR, so possibly this is a benigh MPU miss. See if the fault address
         * can be found in our current page table, and if it is there, we only care about the flags */
        Inv_Top=RME_INVSTK_TOP(RME_A7M_Local.Cur_Thd);
        if(Inv_Top==0)
            Proc=(RME_A7M_Local.Cur_Thd)->Sched.Proc;
        else
            Proc=Inv_Top->Proc;
        
        if(__RME_Pgtbl_Walk(Proc->Pgtbl, Cur_MMFAR, (rme_ptr_t*)(&Meta), 0, 0, 0, 0, &Flags)!=0)
            __RME_Thd_Fatal(Reg, RME_A7M_MFSR_DACCVIOL);
        else
        {
            /* This must be a dynamic page. Or there must be something wrong in the kernel, we lockup */
            RME_ASSERT((Flags&RME_PGTBL_STATIC)==0);
            /* Try to update the dynamic page */
            if(___RME_Pgtbl_MPU_Update(Meta, 1)!=0)
                __RME_Thd_Fatal(Reg, RME_A7M_MFSR_DACCVIOL);
        }
    }
    /* This is an instruction access violation. We need to know where that instruction is.
     * This requires access of an user-level page, due to the fact that the fault address
     * is stored on the fault stack. */ 
    else if((Cur_CFSR&RME_A7M_MFSR_IACCVIOL)!=0)
    {
        Inv_Top=RME_INVSTK_TOP(RME_A7M_Local.Cur_Thd);
        if(Inv_Top==0)
            Proc=(RME_A7M_Local.Cur_Thd)->Sched.Proc;
        else
            Proc=Inv_Top->Proc;
        
        Stack=(rme_ptr_t*)(Reg->SP);
        
        /* Stack[6] is where the PC is before the fault */
        if(__RME_Pgtbl_Walk(Proc->Pgtbl, (rme_ptr_t)(&Stack[6]), (rme_ptr_t*)(&Meta), 0, 0, 0, 0, &Flags)!=0)
            __RME_Thd_Fatal(Reg, RME_A7M_MFSR_IACCVIOL);
        else
        {
            /* The SP address is actually accessible. Find the actual instruction address then */
            if(__RME_Pgtbl_Walk(Proc->Pgtbl, Stack[6], (rme_ptr_t*)(&Meta), 0, 0, 0, 0, &Flags)!=0)
                __RME_Thd_Fatal(Reg, RME_A7M_MFSR_IACCVIOL);
            else
            {
                /* This must be a dynamic page */
                RME_ASSERT((Flags&RME_PGTBL_STATIC)==0);
                
                /* This page does not allow execution */
                if((Flags&RME_PGTBL_EXECUTE)==0)
                    __RME_Thd_Fatal(Reg, RME_A7M_MFSR_IACCVIOL);
                else
                {
                    /* Try to update the dynamic page */
                    if(___RME_Pgtbl_MPU_Update(Meta, 1)!=0)
                        __RME_Thd_Fatal(Reg, RME_A7M_MFSR_IACCVIOL);
                }
            }
        }
    }
    /* Drop anything else. Imprecise bus faults, stacking errors, because they cannot be attributed
     * correctly (The unstacking errors are attributable). Imprecise bus faults can happen over context
     * boundaries, while the stacking errors may happen with the old thread but be attributed to the new.
     * If we incorrectly handle and attribute these, a malicious thread can trigger these spurious
     * asynchronous bus faults and stacking errors in the hope that they might be attributed to some other
     * thread. This compromises integrity and availability of other threads. Still, even if they go
     * unhandled, these faults may still comsume the timeslices of other threads (because they do happen
     * in other threads' context), and may lead to slight service quality degredation. This is an inherent
     * issue in the Cortex-M architecture and we will never be able to address this perfectly. */  
    else
    {
        /* Additional remarks: the LSPERR, STKERR, MLSPERR and MSTKERR will soon develop into larger
         * other faults that can be handled because the stacks are corrupted. Thus, they are less harmful.
         * The IMPRECISERR either go undetected (the bus data fault may be transient), or keeps spinning. */
        /* RME_A7M_BFSR_LSPERR - Bus FP lazy stacking errors */
        /* RME_A7M_BFSR_STKERR - Bus stacking errors */
        /* RME_A7M_BFSR_IMPRECISERR - Imprecise bus data errors */
        /* RME_A7M_MFSR_MLSPERR - MPU FP lazy stacking errors */
        /* RME_A7M_MFSR_MSTKERR - MPU stacking errors */
    }
    
    /* Clear all bits in these status registers - they are sticky */
    RME_A7M_SCB_HFSR=RME_ALLBITS>>1;
    RME_A7M_SCB_CFSR=RME_ALLBITS;
}
/* End Function:__RME_A7M_Fault_Handler **************************************/

/* Begin Function:__RME_A7M_Set_Flag ******************************************
Description : Set a generic flag in a flag set. Works for both vectors and events
              for ARMv7-M.
Input       : rme_ptr_t Flagset - The address of the flagset.
              rme_ptr_t Pos - The position in the flagset to set.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_Set_Flag(rme_ptr_t Flagset, rme_ptr_t Pos)
{
    struct __RME_A7M_Flag_Set* Flags;
    
    /* Choose a data structure that is not locked at the moment */
    if(((struct __RME_A7M_Phys_Flags*)Flagset)->Set0.Lock==0)
        Flags=&(((struct __RME_A7M_Phys_Flags*)Flagset)->Set0);
    else
        Flags=&(((struct __RME_A7M_Phys_Flags*)Flagset)->Set1);
    
    /* Set the flags for this interrupt source */
    Flags->Group|=RME_POW2(Pos>>RME_WORD_ORDER);
    Flags->Flags[Pos>>RME_WORD_ORDER]|=RME_POW2(Pos&RME_MASK_END(RME_WORD_ORDER-1));
}
/* End Function:__RME_A7M_Set_Flag *******************************************/

/* Begin Function:__RME_A7M_Vect_Handler **************************************
Description : The generic interrupt handler of RME for ARMv7-M.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t Vect_Num - The vector number. For ARMv7-M, this is in accordance
                                   with the ARMv7-M architecture reference manual.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_A7M_Vect_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Vect_Num)
{
#if(RME_GEN_ENABLE==RME_TRUE)
    /* Do in-kernel processing first */
    extern rme_ptr_t RME_Boot_Vect_Handler(rme_ptr_t Vect_Num);
    /* If the user decided to send to the generic interrupt endpoint (or hoped to bypass
     * this due to some reason), we skip the flag marshalling & sending process */
    if(RME_Boot_Vect_Handler(Vect_Num)!=0)
        return;
#endif
    
    __RME_A7M_Set_Flag(RME_A7M_VECT_FLAG_ADDR, Vect_Num);
    
    _RME_Kern_Snd(RME_A7M_Local.Vect_Sig);
    /* Remember to pick the guy with the highest priority after we did all sends */
    _RME_Kern_High(Reg, &RME_A7M_Local);
}
/* End Function:__RME_A7M_Vect_Handler ***************************************/

/* Begin Function:__RME_A7M_Debug_Reg_Mod *************************************
Description : Debug register modification implementation for ARMv7-M.
Input       : struct RME_Cap_Captbl* Captbl - The current capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_cid_t Cap_Thd - The capability to the thread to consult.
              rme_ptr_t Operation - The operation, e.g. which register to read or write.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : rme_ret_t - If successful, 0; if a negative value, failed.
******************************************************************************/
rme_ret_t __RME_A7M_Debug_Reg_Mod(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg, 
                                  rme_cid_t Cap_Thd, rme_ptr_t Operation)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    /* These are used to free the thread */
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_TYPE_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    
    /* See if the target thread is already binded. If no or binded to other cores, we just quit */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Struct=(struct RME_Thd_Struct*)Thd_Op->Head.Object;
    if(Thd_Struct->Sched.CPU_Local!=CPU_Local)
        return RME_ERR_PTH_INVSTATE;
    
    switch(Operation)
    {
        /* Register read/write */
        case RME_KERN_DEBUG_REG_MOD_SP_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.SP;break;
        case RME_KERN_DEBUG_REG_MOD_SP_WRITE:Thd_Struct->Cur_Reg->Reg.SP=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R4_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R4;break;
        case RME_KERN_DEBUG_REG_MOD_R4_WRITE:Thd_Struct->Cur_Reg->Reg.R4=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R5_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R5;break;
        case RME_KERN_DEBUG_REG_MOD_R5_WRITE:Thd_Struct->Cur_Reg->Reg.R5=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R6_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R6;break;
        case RME_KERN_DEBUG_REG_MOD_R6_WRITE:Thd_Struct->Cur_Reg->Reg.R6=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R7_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R7;break;
        case RME_KERN_DEBUG_REG_MOD_R7_WRITE:Thd_Struct->Cur_Reg->Reg.R7=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R8_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R8;break;
        case RME_KERN_DEBUG_REG_MOD_R8_WRITE:Thd_Struct->Cur_Reg->Reg.R8=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R9_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R9;break;
        case RME_KERN_DEBUG_REG_MOD_R9_WRITE:Thd_Struct->Cur_Reg->Reg.R9=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R10_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R10;break;
        case RME_KERN_DEBUG_REG_MOD_R10_WRITE:Thd_Struct->Cur_Reg->Reg.R10=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_R11_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.R11;break;
        case RME_KERN_DEBUG_REG_MOD_R11_WRITE:Thd_Struct->Cur_Reg->Reg.R11=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_LR_READ:Reg->R6=Thd_Struct->Cur_Reg->Reg.LR;break;
        /* case RME_KERN_DEBUG_REG_MOD_LR_WRITE: LR write is not allowed, may cause arbitrary kernel execution */
        /* FPU register read/write */
        case RME_KERN_DEBUG_REG_MOD_S16_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S16;break;
        case RME_KERN_DEBUG_REG_MOD_S16_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S16=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S17_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S17;break;
        case RME_KERN_DEBUG_REG_MOD_S17_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S17=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S18_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S18;break;
        case RME_KERN_DEBUG_REG_MOD_S18_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S18=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S19_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S19;break;
        case RME_KERN_DEBUG_REG_MOD_S19_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S19=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S20_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S20;break;
        case RME_KERN_DEBUG_REG_MOD_S20_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S20=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S21_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S21;break;
        case RME_KERN_DEBUG_REG_MOD_S21_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S21=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S22_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S22;break;
        case RME_KERN_DEBUG_REG_MOD_S22_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S22=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S23_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S23;break;
        case RME_KERN_DEBUG_REG_MOD_S23_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S23=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S24_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S24;break;
        case RME_KERN_DEBUG_REG_MOD_S24_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S24=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S25_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S25;break;
        case RME_KERN_DEBUG_REG_MOD_S25_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S25=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S26_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S26;break;
        case RME_KERN_DEBUG_REG_MOD_S26_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S26=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S27_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S27;break;
        case RME_KERN_DEBUG_REG_MOD_S27_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S27=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S28_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S28;break;
        case RME_KERN_DEBUG_REG_MOD_S28_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S28=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S29_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S29;break;
        case RME_KERN_DEBUG_REG_MOD_S29_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S29=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S30_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S30;break;
        case RME_KERN_DEBUG_REG_MOD_S30_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S30=Reg->R6;break;
        case RME_KERN_DEBUG_REG_MOD_S31_READ:Reg->R6=Thd_Struct->Cur_Reg->Cop_Reg.S31;break;
        case RME_KERN_DEBUG_REG_MOD_S31_WRITE:Thd_Struct->Cur_Reg->Cop_Reg.S31=Reg->R6;break;
        default:return RME_ERR_KERN_OPFAIL;
    }
    
    __RME_Set_Syscall_Retval(Reg, 0);
    return 0;
}
/* End Function:__RME_A7M_Debug_Reg_Mod **************************************/

/* Begin Function:__RME_Kern_Func_Handler *************************************
Description : Handle kernel function calls.
Input       : struct RME_Cap_Captbl* Captbl - The current capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_ptr_t Func_ID - The function ID.
              rme_ptr_t Sub_ID - The subfunction ID.
              rme_ptr_t Param1 - The first parameter.
              rme_ptr_t Param2 - The second parameter.
Output      : None.
Return      : rme_ret_t - The value that the function returned.
******************************************************************************/
rme_ret_t __RME_Kern_Func_Handler(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                                  rme_ptr_t Func_ID, rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2)
{
#if(RME_GEN_ENABLE==RME_TRUE)
    rme_ret_t Retval;
#endif

    /* Currently we only implement sends */
    switch(Func_ID)
    {
        case RME_KERN_IDLE_SLEEP:
        {
            __RME_A7M_Wait_Int();
            
            __RME_Set_Syscall_Retval(Reg,0);
            
            return 0;
        }
        case RME_KERN_EVT_LOCAL_TRIG:
        {
            if(Sub_ID!=0)
                return RME_ERR_KERN_OPFAIL;
            
            if(Param1>=RME_A7M_MAX_EVTS)
                return RME_ERR_KERN_OPFAIL;
            
            __RME_A7M_Set_Flag(RME_A7M_EVT_FLAG_ADDR, 0);
            
            if(_RME_Kern_Snd(RME_A7M_Local.Vect_Sig)!=0)
                return RME_ERR_KERN_OPFAIL;
            
            __RME_Set_Syscall_Retval(Reg,0);
            
            _RME_Kern_High(Reg, &RME_A7M_Local);
            
            return 0;
        }
        case RME_KERN_DEBUG_REG_MOD:
        {
            return __RME_A7M_Debug_Reg_Mod(Captbl, Reg, Sub_ID, Param1);
        }
        default:
        {
#if(RME_GEN_ENABLE==RME_TRUE)
            extern rme_ret_t RME_User_Kern_Func_Handler(rme_ptr_t Func_ID, rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2);
            Retval=RME_User_Kern_Func_Handler(Func_ID, Sub_ID, Param1, Param2);
            
            if(Retval>=0)
                __RME_Set_Syscall_Retval(Reg,0);
            
            return Retval;
#else
            break;
#endif
        }
    }

#if(RME_GEN_ENABLE!=RME_TRUE)
    /* If it gets here, we must have failed */
    return RME_ERR_KERN_OPFAIL;
#endif
}
/* End Function:__RME_Kern_Func_Handler **************************************/

/* Begin Function:__RME_A7M_NVIC_Enable_IRQ ***********************************
Description : Enable an ARMv7-M non-core interrupt.
Input       : rme_ptr_t IRQ - The non-core IRQ number.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_NVIC_Enable_IRQ(rme_ptr_t IRQ)
{
  RME_A7M_NVIC_ISE(IRQ>>5)=RME_POW2(IRQ&0x1F);
}
/* End Function:__RME_A7M_NVIC_Enable_IRQ ************************************/

/* Begin Function:__RME_A7M_NVIC_Disable_IRQ **********************************
Description : Disable an ARMv7-M non-core interrupt.
Input       : rme_ptr_t IRQ - The non-core IRQ number.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_NVIC_Disable_IRQ(rme_ptr_t IRQ)
{
    RME_A7M_NVIC_ICE(IRQ>>5)=RME_POW2(IRQ&0x1F);
}
/* End Function:__RME_A7M_NVIC_Disable_IRQ ***********************************/

/* Begin Function:__RME_A7M_NVIC_Set_Prio *************************************
Description : Set the interrupt priority in ARMv7-M architecture.
Input       : rme_ret_t IRQ - The IRQ number.
              rme_ptr_t Prio - The priority to directly write into the priority
                               register.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_NVIC_Set_Prio(rme_ret_t IRQ, rme_ptr_t Prio)
{
    if(IRQ<0)
        RME_A7M_SCB_SHPR((((rme_ptr_t)IRQ)&0xFU)-4U)=Prio;
    else
        RME_A7M_NVIC_IP(IRQ)=Prio;
}
/* End Function:__RME_A7M_NVIC_Set_Prio **************************************/

/* Begin Function:__RME_A7M_Low_Level_Preinit *********************************
Description : Initialize the low-level hardware, before the loading of the kernel
              even takes place.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_A7M_Low_Level_Preinit(void)
{
    RME_A7M_LOW_LEVEL_PREINIT();

#if(RME_GEN_ENABLE==RME_TRUE)
    extern void RME_Boot_Pre_Init(void);
    RME_Boot_Pre_Init();
#endif
}
/* End Function:__RME_A7M_Low_Level_Preinit **********************************/

/* Begin Function:__RME_Low_Level_Init ****************************************
Description : Initialize the low-level hardware.
Input       : None.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Low_Level_Init(void)
{
    rme_ptr_t Temp;
    
    RME_A7M_LOW_LEVEL_INIT();

    /* Enable the MPU */
    RME_A7M_MPU_CTRL&=~RME_A7M_MPU_CTRL_ENABLE;
    RME_A7M_SCB_SHCSR&=~RME_A7M_SCB_SHCSR_MEMFAULTENA;
    RME_A7M_MPU_CTRL=RME_A7M_MPU_CTRL_PRIVDEF|RME_A7M_MPU_CTRL_ENABLE;
    
    /* Enable all fault handlers */
    RME_A7M_SCB_SHCSR|=RME_A7M_SCB_SHCSR_USGFAULTENA|
                       RME_A7M_SCB_SHCSR_BUSFAULTENA|
                       RME_A7M_SCB_SHCSR_MEMFAULTENA;
    
    /* Set priority grouping */
    Temp=RME_A7M_SCB_AIRCR;
    Temp&=~0xFFFF0700U;
    Temp|=(0x5FAU<<16)|(RME_A7M_NVIC_GROUPING<<8);
    RME_A7M_SCB_AIRCR=Temp;
  
    /* Set the priority of timer, svc and faults to the lowest */
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_SVCALL, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_PENDSV, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_SYSTICK, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_BUSFAULT, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_USAGEFAULT, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_DEBUGMONITOR, 0xFF);
    __RME_A7M_NVIC_Set_Prio(RME_A7M_IRQN_MEMORYMANAGEMENT, 0xFF);

    /* Initialize CPU-local data structures */
    _RME_CPU_Local_Init(&RME_A7M_Local, 0);
    
    /* Configure and turn on the systick */
    RME_A7M_SYSTICK_LOAD=RME_A7M_SYSTICK_VAL-1;
    RME_A7M_SYSTICK_VALREG=0;
    RME_A7M_SYSTICK_CTRL=RME_A7M_SYSTICK_CTRL_CLKSOURCE|
                         RME_A7M_SYSTICK_CTRL_TICKINT|
                         RME_A7M_SYSTICK_CTRL_ENABLE;
                 
    /* We do not need to turn off lazy stacking, because even if a fault occurs,
     * it will get dropped by our handler deliberately and will not cause wrong
     * attribution. They can be alternatively disabled as well if you wish */
		 
#if(RME_GEN_ENABLE==RME_TRUE)
	extern void RME_Boot_Post_Init(void);
    RME_Boot_Post_Init();
#endif
    
    return 0;
}
/* End Function:__RME_Low_Level_Init *****************************************/

/* Begin Function:__RME_Boot **************************************************
Description : Boot the first process in the system.
Input       : None.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Boot(void)
{
    rme_ptr_t Cur_Addr;
    volatile rme_ptr_t Size;
    
    Cur_Addr=RME_KMEM_VA_START;
    
    /* Create the capability table for the init process */
    RME_ASSERT(_RME_Captbl_Boot_Init(RME_BOOT_CAPTBL,Cur_Addr,RME_A7M_BOOT_CAPTBL_SIZE)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(RME_A7M_BOOT_CAPTBL_SIZE));
    
#if(RME_GEN_ENABLE==RME_TRUE)
    /* Create the page table for the init process, and map in the page alloted for it */
    /* The top-level page table - covers 4G address range */
    RME_ASSERT(_RME_Pgtbl_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_PGTBL, 
               Cur_Addr, 0x00000000, RME_PGTBL_TOP, RME_PGTBL_SIZE_4G, RME_PGTBL_NUM_1)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_PGTBL_SIZE_TOP(RME_PGTBL_NUM_1));
    /* Other memory regions will be directly added, because we do not protect them in the init process */
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x00000000, 0, RME_PGTBL_ALL_PERM)==0);
#else
    /* Create the page table for the init process, and map in the page alloted for it */
    /* The top-level page table - covers 4G address range */
    RME_ASSERT(_RME_Pgtbl_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_PGTBL, 
               Cur_Addr, 0x00000000, RME_PGTBL_TOP, RME_PGTBL_SIZE_512M, RME_PGTBL_NUM_8)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_PGTBL_SIZE_TOP(RME_PGTBL_NUM_8));
    /* Other memory regions will be directly added, because we do not protect them in the init process */
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x00000000, 0, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x20000000, 1, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x40000000, 2, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x60000000, 3, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0x80000000, 4, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0xA0000000, 5, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0xC0000000, 6, RME_PGTBL_ALL_PERM)==0);
    RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_A7M_CPT, RME_BOOT_PGTBL, 0xE0000000, 7, RME_PGTBL_ALL_PERM)==0);
#endif

    /* Activate the first process - This process cannot be deleted */
    RME_ASSERT(_RME_Proc_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_PROC, 
                                  RME_BOOT_CAPTBL, RME_BOOT_PGTBL)==0);
    
    /* Create the initial kernel function capability, and kernel memory capability */
    RME_ASSERT(_RME_Kern_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_KERN)==0);
    RME_ASSERT(_RME_Kmem_Boot_Crt(RME_A7M_CPT, 
                                  RME_BOOT_CAPTBL, 
                                  RME_BOOT_INIT_KMEM,
                                  RME_KMEM_VA_START,
                                  RME_KMEM_VA_START+RME_KMEM_SIZE-1,
                                  RME_KMEM_FLAG_ALL)==0);
    
    /* Create the initial kernel endpoint for timer ticks */
    RME_A7M_Local.Tick_Sig=(struct RME_Cap_Sig*)&(RME_A7M_CPT[RME_BOOT_INIT_TIMER]);
    RME_ASSERT(_RME_Sig_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_TIMER)==0);
    
    /* Create the initial kernel endpoint for all other interrupts */
    RME_A7M_Local.Vect_Sig=(struct RME_Cap_Sig*)&(RME_A7M_CPT[RME_BOOT_INIT_VECT]);
    RME_ASSERT(_RME_Sig_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_VECT)==0);
    
    /* Clean up the region for vectors and events */
    RME_ASSERT(sizeof(struct __RME_A7M_Phys_Flags)<=512);
    _RME_Clear((void*)RME_A7M_VECT_FLAG_ADDR,sizeof(struct __RME_A7M_Phys_Flags));
    _RME_Clear((void*)RME_A7M_EVT_FLAG_ADDR,sizeof(struct __RME_A7M_Phys_Flags));
    
    /* Activate the first thread, and set its priority */
    RME_ASSERT(_RME_Thd_Boot_Crt(RME_A7M_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_THD,
                                 RME_BOOT_INIT_PROC, Cur_Addr, 0, &RME_A7M_Local)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_THD_SIZE);
    
    /* Print the size of some kernel objects, only used in debugging */
    Size=RME_CAPTBL_SIZE(1);
    Size=RME_PGTBL_SIZE_TOP(0)-sizeof(rme_ptr_t);
    Size=RME_PGTBL_SIZE_NOM(0)-sizeof(rme_ptr_t);
    Size=RME_INV_SIZE;
    Size=RME_THD_SIZE;
    
    /* If generator is enabled for this project, generate what is required by the generator */
#if(RME_GEN_ENABLE==RME_TRUE)
    extern rme_ptr_t RME_Boot_Vect_Init(struct RME_Cap_Captbl* Captbl, rme_ptr_t Cap_Front, rme_ptr_t Kmem_Front);
    Cur_Addr=RME_Boot_Vect_Init(RME_A7M_CPT, RME_BOOT_INIT_VECT+1, Cur_Addr);
#endif

    /* Before we go into user level, make sure that the kernel object allocation is within the limits */
#if(RME_GEN_ENABLE==RME_TRUE)
    RME_ASSERT(Cur_Addr==RME_A7M_KMEM_BOOT_FRONTIER);
#else
    RME_ASSERT(Cur_Addr<RME_A7M_KMEM_BOOT_FRONTIER);
#endif

    /* Enable the MPU & interrupt */
    __RME_Pgtbl_Set(RME_CAP_GETOBJ((RME_A7M_Local.Cur_Thd)->Sched.Proc->Pgtbl,rme_ptr_t));
    __RME_Enable_Int();
    
    /* Boot into the init thread */
    __RME_Enter_User_Mode(RME_A7M_INIT_ENTRY, RME_A7M_INIT_STACK, 0);
    
    /* Dummy return, never reaches here */
    return 0;
}
/* End Function:__RME_Boot ***************************************************/

/* Begin Function:__RME_Get_Syscall_Param *************************************
Description : Get the system call parameters from the stack frame.
Input       : struct RME_Reg_Struct* Reg - The register set.
Output      : rme_ptr_t* Svc - The system service number.
              rme_ptr_t* Capid - The capability ID number.
              rme_ptr_t* Param - The parameters.
Return      : None.
******************************************************************************/
void __RME_Get_Syscall_Param(struct RME_Reg_Struct* Reg, rme_ptr_t* Svc, rme_ptr_t* Capid, rme_ptr_t* Param)
{
    *Svc=(Reg->R4)>>16;
    *Capid=(Reg->R4)&0xFFFF;
    Param[0]=Reg->R5;
    Param[1]=Reg->R6;
    Param[2]=Reg->R7;
}
/* End Function:__RME_Get_Syscall_Param **************************************/

/* Begin Function:__RME_Set_Syscall_Retval ************************************
Description : Set the system call return value to the stack frame. This function 
              may carry up to 4 return values. If the last 3 is not needed, just set
              them to zero.
Input       : rme_ret_t Retval - The return value.
Output      : struct RME_Reg_Struct* Reg - The register set.
Return      : None.
******************************************************************************/
void __RME_Set_Syscall_Retval(struct RME_Reg_Struct* Reg, rme_ret_t Retval)
{
    Reg->R4=(rme_ptr_t)Retval;
}
/* End Function:__RME_Set_Syscall_Retval *************************************/

/* Begin Function:__RME_Thd_Reg_Init ******************************************
Description : Initialize the register set for the thread.
Input       : rme_ptr_t Entry - The thread entry address.
              rme_ptr_t Stack - The thread stack address.
              rme_ptr_t Param - The parameter to pass.
Output      : struct RME_Reg_Struct* Reg - The register set content generated.
Return      : None.
******************************************************************************/
void __RME_Thd_Reg_Init(rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Param, struct RME_Reg_Struct* Reg)
{
    /* Set the LR to a value indicating that we have never used FPU in this new task */
    Reg->LR=RME_A7M_EXC_RET_INIT;
    /* The entry point needs to have the last bit set to avoid ARM mode */
    Reg->R4=Entry|0x01;
    /* Put something in the SP later */
    Reg->SP=Stack;
    /* Set the parameter */
    Reg->R5=Param;
}
/* End Function:__RME_Thd_Reg_Init *******************************************/

/* Begin Function:__RME_Thd_Reg_Copy ******************************************
Description : Copy one set of registers into another.
Input       : struct RME_Reg_Struct* Src - The source register set.
Output      : struct RME_Reg_Struct* Dst - The destination register set.
Return      : None.
******************************************************************************/
void __RME_Thd_Reg_Copy(struct RME_Reg_Struct* Dst, struct RME_Reg_Struct* Src)
{
    /* Make sure that the ordering is the same so the compiler can optimize */
    Dst->SP=Src->SP;
    Dst->R4=Src->R4;
    Dst->R5=Src->R5;
    Dst->R6=Src->R6;
    Dst->R7=Src->R7;
    Dst->R8=Src->R8;
    Dst->R9=Src->R9;
    Dst->R10=Src->R10;
    Dst->R11=Src->R11;
    Dst->LR=Src->LR;
}
/* End Function:__RME_Thd_Reg_Copy *******************************************/

/* Begin Function:__RME_Thd_Cop_Init ******************************************
Description : Initialize the coprocessor register set for the thread.
Input       : struct RME_Reg_Struct* Reg - The register struct to help initialize the coprocessor.
Output      : struct RME_Reg_Cop_Struct* Cop_Reg - The register set content generated.
Return      : None.
******************************************************************************/
void __RME_Thd_Cop_Init(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg)
{
    /* Empty function, return immediately. The FPU contents is not predictable */
}
/* End Function:__RME_Thd_Cop_Reg_Init ***************************************/

/* Begin Function:__RME_Thd_Cop_Save ******************************************
Description : Save the co-op register sets. This operation is flexible - If the
              program does not use the FPU, we do not save its context.
Input       : struct RME_Reg_Struct* Reg - The context, used to decide whether
                                           to save the context of the coprocessor.
Output      : struct RME_Cop_Struct* Cop_Reg - The pointer to the coprocessor contents.
Return      : None.
******************************************************************************/
void __RME_Thd_Cop_Save(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg)
{
    /* If we do not have a FPU, return 0 directly */
#ifdef RME_A7M_FPU_TYPE
#if(RME_A7M_FPU_TYPE!=RME_A7M_FPU_NONE)
    /* If this is a standard frame which does not contain FPU usage&context */
    if(((Reg->LR)&RME_A7M_EXC_RET_STD_FRAME)!=0)
        return;
    /* Not. We save the context of FPU */
    ___RME_A7M_Thd_Cop_Save(Cop_Reg);
#endif
#endif
}
/* End Function:__RME_Thd_Cop_Save *******************************************/

/* Begin Function:__RME_Thd_Cop_Restore ***************************************
Description : Restore the co-op register sets. This operation is flexible - If the
              FPU is not used, we do not restore its context.
Input       : struct RME_Reg_Struct* Reg - The context, used to decide whether
                                           to save the context of the coprocessor.
Output      : struct RME_Cop_Struct* Cop_Reg - The pointer to the coprocessor contents.
Return      : None.
******************************************************************************/
void __RME_Thd_Cop_Restore(struct RME_Reg_Struct* Reg, struct RME_Cop_Struct* Cop_Reg)
{    
/* If we do not have a FPU, return 0 directly */
#ifdef RME_A7M_FPU_TYPE
#if(RME_A7M_FPU_TYPE!=RME_A7M_FPU_NONE)
    /* If this is a standard frame which does not contain FPU usage&context */
    if(((Reg->LR)&RME_A7M_EXC_RET_STD_FRAME)!=0)
        return;
    /* Not. We restore the context of FPU */
    ___RME_A7M_Thd_Cop_Restore(Cop_Reg);
#endif
#endif
}
/* End Function:__RME_Thd_Cop_Restore ****************************************/

/* Begin Function:__RME_Inv_Reg_Save ******************************************
Description : Save the necessary registers on invocation for returning. Only the
              registers that will influence program control flow will be saved.
Input       : struct RME_Reg_Struct* Reg - The register set.
Output      : struct RME_Iret_Struct* Ret - The invocation return register context.
Return      : None.
******************************************************************************/
void __RME_Inv_Reg_Save(struct RME_Iret_Struct* Ret, struct RME_Reg_Struct* Reg)
{
    Ret->LR=Reg->LR;
    Ret->SP=Reg->SP;
}
/* End Function:__RME_Inv_Reg_Save *******************************************/

/* Begin Function:__RME_Inv_Reg_Restore ***************************************
Description : Restore the necessary registers for returning from an invocation.
Input       : struct RME_Iret_Struct* Ret - The invocation return register context.
Output      : struct RME_Reg_Struct* Reg - The register set.
Return      : None.
******************************************************************************/
void __RME_Inv_Reg_Restore(struct RME_Reg_Struct* Reg, struct RME_Iret_Struct* Ret)
{
    Reg->LR=Ret->LR;
    Reg->SP=Ret->SP;
}
/* End Function:__RME_Inv_Reg_Restore ****************************************/

/* Begin Function:__RME_Set_Inv_Retval ****************************************
Description : Set the invocation return value to the stack frame.
Input       : rme_ret_t Retval - The return value.
Output      : struct RME_Reg_Struct* Reg - The register set.
Return      : None.
******************************************************************************/
void __RME_Set_Inv_Retval(struct RME_Reg_Struct* Reg, rme_ret_t Retval)
{
    Reg->R5=(rme_ptr_t)Retval;
}
/* End Function:__RME_Set_Inv_Retval *****************************************/

/* Begin Function:__RME_Pgtbl_Kmem_Init ***************************************
Description : Initialize the kernel mapping tables, so it can be added to all the
              top-level page tables. In Cortex-M, we do not need to add such pages.
Input       : None.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Kmem_Init(void)
{
    /* Empty function, always immediately successful */
    return 0;
}
/* End Function:__RME_Pgtbl_Kmem_Init ****************************************/

/* Begin Function:__RME_A7M_Rand **********************************************
Description : The random number generator used for random replacement policy.
              ARMv7-M have only one core, thus we make the LFSR local.
Input       : None.
Output      : None.
Return      : rme_ptr_t - The random number returned.
******************************************************************************/
rme_ptr_t __RME_A7M_Rand(void)
{   
    static rme_ptr_t LFSR=0xACE1ACE1U;
    
    if((LFSR&0x01)!=0)
    {
        LFSR>>=1;
        LFSR^=0xB400B400U;
    }
    else
        LFSR>>=1;
    
    return LFSR;
}
/* End Function:__RME_A7M_Rand ***********************************************/

/* Begin Function:__RME_Pgtbl_Init ********************************************
Description : Initialize the page table data structure, according to the capability.
Input       : struct RME_Cap_Pgtbl* - The capability to the page table to operate on.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Init(struct RME_Cap_Pgtbl* Pgtbl_Op)
{
    rme_cnt_t Count;
    rme_ptr_t* Ptr;
    
    /* Get the actual table */
    Ptr=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);
    
    /* Initialize the causal metadata */
    ((struct __RME_A7M_Pgtbl_Meta*)Ptr)->Base_Addr=Pgtbl_Op->Base_Addr;
    ((struct __RME_A7M_Pgtbl_Meta*)Ptr)->Toplevel=0;
    ((struct __RME_A7M_Pgtbl_Meta*)Ptr)->Size_Num_Order=Pgtbl_Op->Size_Num_Order;
    ((struct __RME_A7M_Pgtbl_Meta*)Ptr)->Dir_Page_Count=0;
    Ptr+=sizeof(struct __RME_A7M_Pgtbl_Meta)/sizeof(rme_ptr_t);
    
    /* Is this a top-level? If it is, we need to clean up the MPU data. In MMU
     * environments, if it is top-level, we need to add kernel pages as well */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0)
    {
        ((struct __RME_A7M_MPU_Data*)Ptr)->Static=0;
        
        for(Count=0;Count<RME_A7M_MPU_REGIONS;Count++)
        {
            ((struct __RME_A7M_MPU_Data*)Ptr)->Data[Count].MPU_RBAR=RME_A7M_MPU_VALID|Count;
            ((struct __RME_A7M_MPU_Data*)Ptr)->Data[Count].MPU_RASR=0;
        }
        
        Ptr+=sizeof(struct __RME_A7M_MPU_Data)/sizeof(rme_ptr_t);
    }
    
    /* Clean up the table itself - This is could be virtually unbounded if the user
     * pass in some very large length value */
    for(Count=0;Count<RME_POW2(RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order));Count++)
        Ptr[Count]=0;
    
    return 0;
}
/* End Function:__RME_Pgtbl_Init *********************************************/

/* Begin Function:__RME_Pgtbl_Check *******************************************
Description : Check if the page table parameters are feasible, according to the
              parameters. This is only used in page table creation.
Input       : rme_ptr_t Base_Addr - The start mapping address.
              rme_ptr_t Top_Flag - The top-level flag,
              rme_ptr_t Size_Order - The size order of the page directory.
              rme_ptr_t Num_Order - The number order of the page directory.
              rme_ptr_t Vaddr - The virtual address of the page directory.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Check(rme_ptr_t Base_Addr, rme_ptr_t Top_Flag, 
                            rme_ptr_t Size_Order, rme_ptr_t Num_Order, rme_ptr_t Vaddr)
{
    if(Num_Order>RME_PGTBL_NUM_256)
        return RME_ERR_PGT_OPFAIL;
    if(Size_Order<RME_PGTBL_SIZE_32B)
        return RME_ERR_PGT_OPFAIL;
    if(Size_Order>RME_PGTBL_SIZE_4G)
        return RME_ERR_PGT_OPFAIL;
    if((Vaddr&0x03)!=0)
        return RME_ERR_PGT_OPFAIL;
    
    return 0;
}
/* End Function:__RME_Pgtbl_Check ********************************************/

/* Begin Function:__RME_Pgtbl_Del_Check ***************************************
Description : Check if the page table can be deleted.
Input       : struct RME_Cap_Pgtbl Pgtbl_Op* - The capability to the page table to operate on.
Output      : None.
Return      : rme_ptr_t - If can be deleted, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Del_Check(struct RME_Cap_Pgtbl* Pgtbl_Op)
{
    /* Check if we are standalone */
    if(((RME_CAP_GETOBJ(Pgtbl_Op,struct __RME_A7M_Pgtbl_Meta*)->Dir_Page_Count)>>16)!=0)
        return RME_ERR_PGT_OPFAIL;
    
    /* Check if we still have a top-level */
    if(RME_CAP_GETOBJ(Pgtbl_Op,struct __RME_A7M_Pgtbl_Meta*)->Toplevel!=0)
        return RME_ERR_PGT_OPFAIL;

    return 0;
}
/* End Function:__RME_Pgtbl_Del_Check ****************************************/

/* Begin Function:___RME_Pgtbl_MPU_Gen_RASR ***********************************
Description : Generate the RASR metadata for this level of page table.
Input       : rme_ptr_t* Table - The table to generate data for. This is directly the
                                 raw page table itself, without accounting for metadata.
              rme_ptr_t Flags - The flags for each entry.
              rme_ptr_t Size_Order - The size order of the page directory.
              rme_ptr_t Num_Order - The number order of the page directory.
Output      : struct __RME_A7M_MPU_Entry* Entry - The data generated.
Return      : rme_ptr_t - The RASR value returned.
******************************************************************************/
rme_ptr_t ___RME_Pgtbl_MPU_Gen_RASR(rme_ptr_t* Table, rme_ptr_t Flags, 
                                    rme_ptr_t Size_Order, rme_ptr_t Num_Order)
{
    rme_ptr_t RASR;
    rme_ptr_t Count;
    rme_ptr_t Flag;
    
    /* Get the SRD part first */
    RASR=0;
    
    switch(Num_Order)
    {
        case RME_PGTBL_NUM_1:Flag=0xFFU;break;
        case RME_PGTBL_NUM_2:Flag=0x0FU;break;
        case RME_PGTBL_NUM_4:Flag=0x03U;break;
        case RME_PGTBL_NUM_8:Flag=0x01U;break;
        default:RME_ASSERT(0);
    }
    
    for(Count=0;Count<RME_POW2(Num_Order);Count++)
    {
        if(((Table[Count]&RME_A7M_PGTBL_PRESENT)!=0)&&((Table[Count]&RME_A7M_PGTBL_TERMINAL)!=0))
            RASR|=Flag<<Count*RME_POW2(RME_PGTBL_NUM_8-Num_Order);
    }
    
    if(RASR==0)
        return 0;
    
    RASR<<=8;
    
    RASR=RME_A7M_MPU_SRDCLR&(~RASR);
    RASR|=RME_A7M_MPU_SZENABLE;
    /* Is it read-only? - we do not care if the read bit is set, because it is always readable anyway */
    if((Flags&RME_PGTBL_WRITE)!=0)
        RASR|=RME_A7M_MPU_RW;
    else
        RASR|=RME_A7M_MPU_RO;
    /* Can we fetch instructions from there? */
    if((Flags&RME_PGTBL_EXECUTE)==0)
        RASR|=RME_A7M_MPU_XN;
    /* Is the area cacheable? */
    if((Flags&RME_PGTBL_CACHEABLE)!=0)
        RASR|=RME_A7M_MPU_CACHEABLE;
    /* Is the area bufferable? */
    if((Flags&RME_PGTBL_BUFFERABLE)!=0)
        RASR|=RME_A7M_MPU_BUFFERABLE;
    /* What is the region size? */
    RASR|=RME_A7M_MPU_REGIONSIZE(Size_Order+Num_Order);
    
    return RASR;
}
/* End Function:___RME_Pgtbl_MPU_Gen_RASR ************************************/

/* Begin Function:___RME_Pgtbl_MPU_Clear **************************************
Description : Clear the MPU setting of this directory. If it exists, clear it;
              If it does not exist, don't do anything.
Input       : struct __RME_A7M_MPU_Data* Top_MPU - The top-level MPU metadata
              rme_ptr_t Base_Addr - The start mapping address of the directory.
              rme_ptr_t Size_Order - The size order of the page directory.
              rme_ptr_t Num_Order - The number order of the page directory.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t ___RME_Pgtbl_MPU_Clear(struct __RME_A7M_MPU_Data* Top_MPU, 
                                 rme_ptr_t Base_Addr, rme_ptr_t Size_Order, rme_ptr_t Num_Order)
{
    rme_ptr_t Count;
    
    for(Count=0;Count<RME_A7M_MPU_REGIONS;Count++)
    {
        if((Top_MPU->Data[Count].MPU_RASR&RME_A7M_MPU_SZENABLE)!=0)
        {
            /* We got one MPU region valid here */
            if((RME_A7M_MPU_ADDR(Top_MPU->Data[Count].MPU_RBAR)==Base_Addr)&&
               (RME_A7M_MPU_SZORD(Top_MPU->Data[Count].MPU_RASR)==(Size_Order+Num_Order)))
            {
                /* Clean it up and return */
                Top_MPU->Data[Count].MPU_RBAR=RME_A7M_MPU_VALID|Count;
                Top_MPU->Data[Count].MPU_RASR=0;
                /* Clean the static flag as well */
                Top_MPU->Static&=~RME_POW2(Count);
                return 0;
            }
        }
    }
    
    return 0;
}
/* End Function:___RME_Pgtbl_MPU_Clear ***************************************/

/* Begin Function:___RME_Pgtbl_MPU_Add ****************************************
Description : Add or update the MPU entry in the top-level MPU table. We guarantee
              that at any time at least two regions are dedicated to dynamic entries.
              This is due to the fact that ARM have LDRD and STRD, which can require
              two regions to function correctly.
Input       : struct __RME_A7M_MPU_Data* Top_MPU - The top-level MPU metadata.
              rme_ptr_t Base_Addr - The start mapping address of the directory.
              rme_ptr_t Size_Order - The size order of the page directory.
              rme_ptr_t Num_Order - The number order of the page directory.
              rme_ptr_t MPU_RASR - The RASR register content, if set.
              rme_ptr_t Static - The flag denoting if this entry is static.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t ___RME_Pgtbl_MPU_Add(struct __RME_A7M_MPU_Data* Top_MPU, 
                               rme_ptr_t Base_Addr, rme_ptr_t Size_Order, rme_ptr_t Num_Order,
                               rme_ptr_t MPU_RASR, rme_ptr_t Static)
{
    rme_ptr_t Count;
    /* The number of empty slots available */
    rme_ptr_t Empty_Cnt;
    /* The empty slots */
    rme_u8_t Empty[RME_A7M_MPU_REGIONS];
    /* The number of dynamic slots available */
    rme_ptr_t Dynamic_Cnt;
    /* The dynamic slots */
    rme_u8_t Dynamic[RME_A7M_MPU_REGIONS];
    
    /* Set these values to some overrange value */
    Empty_Cnt=0;
    Dynamic_Cnt=0;
    for(Count=0;Count<RME_A7M_MPU_REGIONS;Count++)
    {
        if((Top_MPU->Data[Count].MPU_RASR&RME_A7M_MPU_SZENABLE)!=0)
        {
            if((Top_MPU->Static&RME_POW2(Count))==0)
            {
                Dynamic[Dynamic_Cnt]=Count;
                Dynamic_Cnt++;
            }
            /* We got one MPU region valid here */
            if((RME_A7M_MPU_ADDR(Top_MPU->Data[Count].MPU_RBAR)==Base_Addr)&&
               (RME_A7M_MPU_SZORD(Top_MPU->Data[Count].MPU_RASR)==(Size_Order+Num_Order)))
            {
                /* Update the RASR - all flag changes except static are reflected here */
                Top_MPU->Data[Count].MPU_RASR=MPU_RASR;
                /* STATIC or not is reflected in the MPU state; instead it is
                 * maintained by using another standalone word */
                if(Static!=0)
                    Top_MPU->Static|=RME_POW2(Count);
                else
                    Top_MPU->Static&=~RME_POW2(Count);
                return 0;
            }
        }
        else
        {
            Empty[Empty_Cnt]=Count;
            Empty_Cnt++;
        }
    }
    
    /* Update unsuccessful, we didn't find any match. We will need a new slot. 
     * If this is a static page:
     * 1. See if the number of regions left (dynamic+empty) is larger than 3. If not, we cannot map.
     *    (At least two slots reserved for dynamic regions.)
     * 2. If there is an empty slot, use that slot.
     * 3. If there is no such slot, use random replacement policy to evict a dynamic page in use.
     * If this is a dynamic page:
     * 1. See if there are empty slots, if there is, use it.
     * 2. See if there are dynamic regions in use, if there is, evict one. If none is present, throw an error. */
    if(Static!=0)
    {
        if((Empty_Cnt+Dynamic_Cnt)<3)
            return RME_ERR_PGT_OPFAIL;
    }
    else
    {
        if((Empty_Cnt+Dynamic_Cnt)==0)
            return RME_ERR_PGT_OPFAIL;
    }
    
    /* We may map in using an empty slot */
    if(Empty_Cnt!=0)
        Count=Empty[0];
    /* We must evict an dynamic entry */
    else
        Count=Dynamic[__RME_A7M_Rand()%Dynamic_Cnt];
    
    /* Put the data to this slot */
    Top_MPU->Data[Count].MPU_RBAR=RME_A7M_MPU_ADDR(Base_Addr)|RME_A7M_MPU_VALID|Count;
    Top_MPU->Data[Count].MPU_RASR=MPU_RASR;
    /* STATIC or not is reflected in the state */
    if(Static!=0)
        Top_MPU->Static|=RME_POW2(Count);
    else
        Top_MPU->Static&=!RME_POW2(Count);

    return 0;
}
/* End Function:___RME_Pgtbl_MPU_Add *****************************************/

/* Begin Function:___RME_Pgtbl_MPU_Update *************************************
Description : Update the top-level MPU metadata for this level of page table.
Input       : struct __RME_A7M_Pgtbl_Meta* Meta - This page table.
              rme_ptr_t Op_Flag - The operation flag. 1 for add, 0 for clean.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t ___RME_Pgtbl_MPU_Update(struct __RME_A7M_Pgtbl_Meta* Meta, rme_ptr_t Op_Flag)
{
    rme_ptr_t* Table;
    rme_ptr_t MPU_RASR;
    struct __RME_A7M_MPU_Data* Top_MPU;
    
    /* Is it possible for MPU to represent this? */
    if(RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order)>RME_PGTBL_NUM_8)
        return RME_ERR_PGT_OPFAIL;
    
    /* Get the tables */
    if(Meta->Toplevel!=0)
    {
        /* We have a top-level */
        Top_MPU=(struct __RME_A7M_MPU_Data*)(Meta->Toplevel+sizeof(struct __RME_A7M_Pgtbl_Meta));
        Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Meta);
    }
    else if(((Meta->Base_Addr)&RME_PGTBL_TOP)!=0)
    {
        /* We don't have a top-level, but we are the top-level */
        Top_MPU=(struct __RME_A7M_MPU_Data*)(((rme_ptr_t)Meta)+sizeof(struct __RME_A7M_Pgtbl_Meta));
        Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Meta);
    }
    else
        return RME_ERR_PGT_OPFAIL;
    
    if(Op_Flag==RME_A7M_MPU_CLR)
    {
        /* Clear the metadata - this function will never fail */
        ___RME_Pgtbl_MPU_Clear(Top_MPU,
                               RME_A7M_PGTBL_START(Meta->Base_Addr),
                               RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order),
                               RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order));
    }
    else
    {
        /* See if the RASR contains anything */
        MPU_RASR=___RME_Pgtbl_MPU_Gen_RASR(Table, Meta->Page_Flags, 
                                           RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order),
                                           RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order));
        if(MPU_RASR==0)
        {
            /* All pages are unmapped. Clear this from the MPU data */
            ___RME_Pgtbl_MPU_Clear(Top_MPU,
                                   RME_A7M_PGTBL_START(Meta->Base_Addr),
                                   RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order),
                                   RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order));
        }
        else
        {
            /* At least one of the pages are there. Map it */
            if(___RME_Pgtbl_MPU_Add(Top_MPU,
                                    RME_A7M_PGTBL_START(Meta->Base_Addr),
                                    RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order),
                                    RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order),
                                    MPU_RASR,Meta->Page_Flags&RME_PGTBL_STATIC)!=0)
                return RME_ERR_PGT_OPFAIL;
        }
    }
    
    return 0;
}
/* End Function:___RME_Pgtbl_MPU_Update **************************************/

/* Begin Function:__RME_Pgtbl_Set *********************************************
Description : Set the processor's page table.
Input       : rme_ptr_t Pgtbl - The virtual address of the page table.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_Pgtbl_Set(rme_ptr_t Pgtbl)
{
    struct __RME_A7M_MPU_Data* MPU_Data;
    
    MPU_Data=(struct __RME_A7M_MPU_Data*)(Pgtbl+sizeof(struct __RME_A7M_Pgtbl_Meta));
    /* Get the physical address of the page table - here we do not need any conversion,
     * because VA = PA as always. We just need to extract the MPU metadata part
     * and pass it down */
    ___RME_A7M_MPU_Set((rme_ptr_t)(&(MPU_Data->Data[0].MPU_RBAR)));
}
/* End Function:__RME_Pgtbl_Set **********************************************/

/* Begin Function:__RME_Pgtbl_Page_Map ****************************************
Description : Map a page into the page table. If a page is mapped into the slot, the
              flags is actually placed on the metadata place because all pages are
              required to have the same flags. We take advantage of this to increase
              the page granularity. This architecture requires that the mapping is
              always at least readable.
Input       : struct RME_Cap_Pgtbl* - The cap ability to the page table to operate on.
              rme_ptr_t Paddr - The physical address to map to. No effect if we are unmapping.
              rme_ptr_t Pos - The position in the page table.
              rme_ptr_t Flags - The RME standard page attributes. Need to translate them into 
                                architecture specific page table's settings.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Page_Map(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags)
{
    rme_ptr_t* Table;
    struct __RME_A7M_Pgtbl_Meta* Meta;

    /* It should at least be readable */
    if((Flags&RME_PGTBL_READ)==0)
        return RME_ERR_PGT_OPFAIL;
        
    /* We are doing page-based operations on this, so the page directory should
     * be MPU-representable. Only page sizes of 1, 2, 4 & 8 are representable for ARMv7-M */
    if(RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order)>RME_PGTBL_NUM_8)
        return RME_ERR_PGT_OPFAIL;
    
    /* Get the metadata */
    Meta=RME_CAP_GETOBJ(Pgtbl_Op,struct __RME_A7M_Pgtbl_Meta*);
    
    /* Where is the entry slot */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0)
        Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Meta);
    else
        Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Meta);
    
    /* Check if we are trying to make duplicate mappings into the same location */
    if((Table[Pos]&RME_A7M_PGTBL_PRESENT)!=0)
        return RME_ERR_PGT_OPFAIL;

    /* Trying to map something. Check if the pages flags are consistent. MPU
     * subregions shall share the same flags in Cortex-M */
    if(RME_A7M_PGTBL_PAGENUM(Meta->Dir_Page_Count)==0)
        Meta->Page_Flags=Flags;
    else
    {
        if(Meta->Page_Flags!=Flags)
            return RME_ERR_PGT_OPFAIL;
    }

    /* Register into the page table */
    Table[Pos]=RME_A7M_PGTBL_PRESENT|RME_A7M_PGTBL_TERMINAL|
               RME_ROUND_DOWN(Paddr,RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order));
   
    /* If we are the top level or we have a top level, and we have static pages mapped in, do MPU updates */
    if((Meta->Toplevel!=0)||(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0))
    {
        if((Flags&RME_PGTBL_STATIC)!=0)
        {
            /* Mapping static pages, update the MPU representation */
            if(___RME_Pgtbl_MPU_Update(Meta, RME_A7M_MPU_UPD)==RME_ERR_PGT_OPFAIL)
            {
                /* MPU update failed. Revert operations */
                Table[Pos]=0;
                return RME_ERR_PGT_OPFAIL;
            }
        }
    }
    /* Modify count */
    RME_A7M_PGTBL_INC_PAGENUM(Meta->Dir_Page_Count);
    
    return 0;
}
/* End Function:__RME_Pgtbl_Page_Map *****************************************/

/* Begin Function:__RME_Pgtbl_Page_Unmap **************************************
Description : Unmap a page from the page table.
Input       : struct RME_Cap_Pgtbl* - The capability to the page table to operate on.
              rme_ptr_t Pos - The position in the page table.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Page_Unmap(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos)
{
    rme_ptr_t* Table;
    rme_ptr_t Temp;
    struct __RME_A7M_Pgtbl_Meta* Meta;
        
    /* We are doing page-based operations on this, so the page directory should
     * be MPU-representable. Only page sizes of 1, 2, 4 & 8 are representable for ARMv7-M */
    if(RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order)>RME_PGTBL_NUM_8)
        return RME_ERR_PGT_OPFAIL;
    
    /* Get the metadata */
    Meta=RME_CAP_GETOBJ(Pgtbl_Op,struct __RME_A7M_Pgtbl_Meta*);
    
    /* Where is the entry slot */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0)
        Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Meta);
    else
        Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Meta);

    /* Check if we are trying to remove something that does not exist, or trying to
     * remove a page directory */
    if(((Table[Pos]&RME_A7M_PGTBL_PRESENT)==0)||((Table[Pos]&RME_A7M_PGTBL_TERMINAL)==0))
        return RME_ERR_PGT_OPFAIL;

    Temp=Table[Pos];
    Table[Pos]=0;
    /* If we are top-level or we have a top-level, do MPU updates */
    if((Meta->Toplevel!=0)||(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0))
    {
        /* Now we are unmapping the pages - Immediately update MPU representations */
        if(___RME_Pgtbl_MPU_Update(Meta, RME_A7M_MPU_UPD)==RME_ERR_PGT_OPFAIL)
        {
            /* Revert operations */
            Table[Pos]=Temp;
            return RME_ERR_PGT_OPFAIL;
        }
    }
    /* Modify count */
    RME_A7M_PGTBL_DEC_PAGENUM(Meta->Dir_Page_Count);
    
    return 0;
}
/* End Function:__RME_Pgtbl_Page_Unmap ***************************************/

/* Begin Function:__RME_Pgtbl_Pgdir_Map ***************************************
Description : Map a page directory into the page table. This architecture does not
              support page directory flags.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Parent - The parent page table.
              struct RME_Cap_Pgtbl* Pgtbl_Child - The child page table.
              rme_ptr_t Pos - The position in the destination page table.
              rme_ptr_t Flags - This have no effect for MPU-based architectures
                            (because page table addresses use up the whole word).
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Pgdir_Map(struct RME_Cap_Pgtbl* Pgtbl_Parent, rme_ptr_t Pos, 
                                struct RME_Cap_Pgtbl* Pgtbl_Child, rme_ptr_t Flags)
{
    rme_ptr_t* Parent_Table;
    struct __RME_A7M_Pgtbl_Meta* Parent_Meta;
    struct __RME_A7M_Pgtbl_Meta* Child_Meta;
    
    /* Is the child a designated top level directory? If it is, we do not allow 
     * constructions. In Cortex-M, we only allow the designated top-level to be
     * the actual top-level. */
    if(((Pgtbl_Child->Base_Addr)&RME_PGTBL_TOP)!=0)
        return RME_ERR_PGT_OPFAIL;
    
    /* Get the metadata */
    Parent_Meta=RME_CAP_GETOBJ(Pgtbl_Parent,struct __RME_A7M_Pgtbl_Meta*);
    Child_Meta=RME_CAP_GETOBJ(Pgtbl_Child,struct __RME_A7M_Pgtbl_Meta*);
    
    /* The parent table must have or be a top-directory */
    if((Parent_Meta->Toplevel==0)&&(((Parent_Meta->Base_Addr)&RME_PGTBL_TOP)==0))
        return RME_ERR_PGT_OPFAIL;
    
    /* Check if the child already mapped somewhere, or have grandchild directories */
    if(((Child_Meta->Toplevel)!=0)||(RME_A7M_PGTBL_DIRNUM(Child_Meta->Dir_Page_Count)!=0))
        return RME_ERR_PGT_OPFAIL;
    
    /* Where is the entry slot? */
    if(((Parent_Meta->Base_Addr)&RME_PGTBL_TOP)!=0)
        Parent_Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Parent_Meta);
    else
        Parent_Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Parent_Meta);
    
    /* Check if anything already mapped in */
    if((Parent_Table[Pos]&RME_A7M_PGTBL_PRESENT)!=0)
        return RME_ERR_PGT_OPFAIL;
    
    /* The address must be aligned to a word */
    Parent_Table[Pos]=RME_A7M_PGTBL_PRESENT|RME_A7M_PGTBL_PGD_ADDR((rme_ptr_t)Child_Meta);
    
    /* Log the entry into the destination - if the parent is a top-level, then the top-level
     * is the parent; if the parent have a top-level, then the top-level is the parent's top-level */
    if(Parent_Meta->Toplevel==0)
        Child_Meta->Toplevel=(rme_ptr_t)Parent_Meta;
    else
        Child_Meta->Toplevel=Parent_Meta->Toplevel;

    RME_A7M_PGTBL_INC_DIRNUM(Parent_Meta->Dir_Page_Count);
    
    /* Update MPU settings if there are static pages mapped into the source. If there
     * are any, update the MPU settings */
    if((RME_A7M_PGTBL_PAGENUM(Child_Meta->Dir_Page_Count)!=0)&&
       (((Child_Meta->Page_Flags)&RME_PGTBL_STATIC)!=0))
    {
        if(___RME_Pgtbl_MPU_Update(Child_Meta, RME_A7M_MPU_UPD)==RME_ERR_PGT_OPFAIL)
        {
            /* Mapping failed. Revert operations */
            Parent_Table[Pos]=0;
            Child_Meta->Toplevel=0;
            RME_A7M_PGTBL_DEC_DIRNUM(Parent_Meta->Dir_Page_Count);
            return RME_ERR_PGT_OPFAIL;
        }
    }

    return 0;
}
/* End Function:__RME_Pgtbl_Pgdir_Map ****************************************/

/* Begin Function:__RME_Pgtbl_Pgdir_Unmap *************************************
Description : Unmap a page directory from the page table.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Parent - The parent page table to unmap from.
              rme_ptr_t Pos - The position in the page table.
              struct RME_Cap_Pgtbl* Pgtbl_Child - The child page table to unmap.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Pgdir_Unmap(struct RME_Cap_Pgtbl* Pgtbl_Parent, rme_ptr_t Pos, 
                                  struct RME_Cap_Pgtbl* Pgtbl_Child)
{
    rme_ptr_t* Table;
    struct __RME_A7M_Pgtbl_Meta* Parent_Meta;
    struct __RME_A7M_Pgtbl_Meta* Child_Meta;
    
    /* Get the metadata */
    Parent_Meta=RME_CAP_GETOBJ(Pgtbl_Parent,struct __RME_A7M_Pgtbl_Meta*);
    
    /* Where is the entry slot */
    if(((Pgtbl_Parent->Base_Addr)&RME_PGTBL_TOP)!=0)
        Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Parent_Meta);
    else
        Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Parent_Meta);

    /* Check if we try to remove something nonexistent, or a page */
    if(((Table[Pos]&RME_A7M_PGTBL_PRESENT)==0)||((Table[Pos]&RME_A7M_PGTBL_TERMINAL)!=0))
        return RME_ERR_PGT_OPFAIL;
    
    /* See if the child page table is actually mapped there */
    Child_Meta=(struct __RME_A7M_Pgtbl_Meta*)RME_A7M_PGTBL_PGD_ADDR(Table[Pos]);
    if(Child_Meta!=RME_CAP_GETOBJ(Pgtbl_Child,struct __RME_A7M_Pgtbl_Meta*))
        return RME_ERR_PGT_OPFAIL;

    /* Check if the directory still have child directories */
    if(RME_A7M_PGTBL_DIRNUM(Parent_Meta->Dir_Page_Count)!=0)
        return RME_ERR_PGT_OPFAIL;
    
    /* We are removing a page directory. Do MPU updates if any page mapped in */
    if(RME_A7M_PGTBL_PAGENUM(Parent_Meta->Dir_Page_Count)!=0)
    {
        if(___RME_Pgtbl_MPU_Update(Parent_Meta, RME_A7M_MPU_CLR)==RME_ERR_PGT_OPFAIL)
            return RME_ERR_PGT_OPFAIL;
    }

    Table[Pos]=0;
    Parent_Meta->Toplevel=0;
    RME_A7M_PGTBL_DEC_DIRNUM(Parent_Meta->Dir_Page_Count);

    return 0;
}
/* End Function:__RME_Pgtbl_Pgdir_Unmap **************************************/

/* Begin Function:__RME_Pgtbl_Lookup ******************************************
Description : Lookup a page entry in a page directory.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Op - The page directory to lookup.
              rme_ptr_t Pos - The position to look up.
Output      : rme_ptr_t* Paddr - The physical address of the page.
              rme_ptr_t* Flags - The RME standard flags of the page.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Lookup(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos, rme_ptr_t* Paddr, rme_ptr_t* Flags)
{
    rme_ptr_t* Table;
    
    /* Check if the position is within the range of this page table */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order))!=0)
        return RME_ERR_PGT_OPFAIL;
    
    /* Check if this is the top-level page table. Get the table */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)!=0)
        Table=RME_A7M_PGTBL_TBL_TOP(RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*));
    else
        Table=RME_A7M_PGTBL_TBL_NOM(RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*));
    
    /* Start lookup */
    if(((Table[Pos]&RME_A7M_PGTBL_PRESENT)==0)||
       ((Table[Pos]&RME_A7M_PGTBL_TERMINAL)==0))
        return RME_ERR_PGT_OPFAIL;
    
    /* This is a page. Return the physical address and flags */
    if(Paddr!=0)
        *Paddr=RME_A7M_PGTBL_PTE_ADDR(Table[Pos]);
    
    if(Flags!=0)
        *Flags=RME_CAP_GETOBJ(Pgtbl_Op,struct __RME_A7M_Pgtbl_Meta*)->Page_Flags;

    return 0;
}
/* End Function:__RME_Pgtbl_Lookup *******************************************/

/* Begin Function:__RME_Pgtbl_Walk ********************************************
Description : Walking function for the page table. This function just does page
              table lookups. The page table that is being walked must be the top-
              level page table. The output values are optional; only pass in pointers
              when you need that value.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Op - The page table to walk.
              rme_ptr_t Vaddr - The virtual address to look up.
Output      : rme_ptr_t* Pgtbl - The pointer to the page table level.
              rme_ptr_t* Map_Vaddr - The virtual address that starts mapping.
              rme_ptr_t* Paddr - The physical address of the page.
              rme_ptr_t* Size_Order - The size order of the page.
              rme_ptr_t* Num_Order - The entry order of the page.
              rme_ptr_t* Flags - The RME standard flags of the page.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Walk(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Vaddr, rme_ptr_t* Pgtbl,
                           rme_ptr_t* Map_Vaddr, rme_ptr_t* Paddr, rme_ptr_t* Size_Order,
                           rme_ptr_t* Num_Order, rme_ptr_t* Flags)
{
    struct __RME_A7M_Pgtbl_Meta* Meta;
    rme_ptr_t* Table;
    rme_ptr_t Pos;
    
    /* Check if this is the top-level page table */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)==0)
        return RME_ERR_PGT_OPFAIL;
    
    /* Get the table and start lookup */
    Meta=RME_CAP_GETOBJ(Pgtbl_Op, struct __RME_A7M_Pgtbl_Meta*);
    Table=RME_A7M_PGTBL_TBL_TOP((rme_ptr_t*)Meta);
    
    /* Do lookup recursively */
    while(1)
    {
        /* Check if the virtual address is in our range */
        if(Vaddr<RME_A7M_PGTBL_START(Meta->Base_Addr))
            return RME_ERR_PGT_OPFAIL;
        /* Calculate where is the entry */
        Pos=(Vaddr-RME_A7M_PGTBL_START(Meta->Base_Addr))>>RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order);
        /* See if the entry is overrange */
        if((Pos>>RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order))!=0)
            return RME_ERR_PGT_OPFAIL;
        /* Find the position of the entry - Is there a page, a directory, or nothing? */
        if((Table[Pos]&RME_A7M_PGTBL_PRESENT)==0)
            return RME_ERR_PGT_OPFAIL;
        if((Table[Pos]&RME_A7M_PGTBL_TERMINAL)!=0)
        {
            /* This is a page - we found it */
            if(Pgtbl!=0)
                *Pgtbl=(rme_ptr_t)Meta;
            if(Map_Vaddr!=0)
                *Map_Vaddr=RME_A7M_PGTBL_START(Meta->Base_Addr)+(Pos<<RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order));
            if(Paddr!=0)
                *Paddr=RME_A7M_PGTBL_START(Meta->Base_Addr)+(Pos<<RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order));
            if(Size_Order!=0)
                *Size_Order=RME_A7M_PGTBL_SIZEORD(Meta->Size_Num_Order);
            if(Num_Order!=0)
                *Num_Order=RME_A7M_PGTBL_NUMORD(Meta->Size_Num_Order);
            if(Flags!=0)
                *Flags=Meta->Page_Flags;
            
            break;
        }
        else
        {
            /* This is a directory, we goto that directory to continue walking */
            Meta=(struct __RME_A7M_Pgtbl_Meta*)RME_A7M_PGTBL_PGD_ADDR(Table[Pos]);
            Table=RME_A7M_PGTBL_TBL_NOM((rme_ptr_t*)Meta);
        }
    }
    return 0;
}
/* End Function:__RME_Pgtbl_Walk *********************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
