/******************************************************************************
Filename    : rme_kernel.c
Author      : pry
Date        : 23/03/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The system call processing path, debugging primitives and kernel
              capability implementation for the RME system.
              
* Generic Code Section ********************************************************
Provides some utilities and handlers that every operating system have.

* Capability Table Code Section ***********************************************
This section of code might be confusing if the underlying principles
of capability-based systems is not well understood. 

1> Owning a capability means you have the power to use the function
   represented by that capability.
2> Capabilities have an field called flags, which denotes what operations
   is allowed on that captbl.
3> Owning a capability to a capability table means that you have the
   power to modify the capability table's contents.
4> Creation and deletion of kernel objects is an operation on capability
   table, thus requiring the capability to the capability table.

Remember we do not check our master table to see if it is frozen, or if it is
captbl, or something, because if the master table's cap is not explicitly passed
in, we do not operate on it at all; If it is explicitly passed in, it will be checked.
              
Hazard Table: (Operation 2 follows operation 1)
Operation 1    Operation 2    Reason why it is safe
-------------------------------------------------------------------------------------------
Create         Create         Only one creation will be successful, because OCCUPY slot is done by CAS.
Create         Delete         OCCUPY only set the FROZEN. Delete will require a TYPE data, which will
                              only be set after the creation completes. If it is set, then the FROZEN
                              will be cleared, and the deletion CAS will fail. ABA problem cannot occur
                              because of create-freeze quiescence.
                              If there is no quiescence at Create-Freeze, the following may occur:
                              Thread1: Check ---------------------------------------------- Delete(CAS)
                              Thread2: Check - Delete - Create - Freeze -------------------------------
                              In this case, thread 1 will perform a wrong deletion on the new capability
                              (the CAS will be successful), but this cap is actually a new cap created
                              by the thread 2 at the same location, not the old cap, and its quiescence
                              may not be satisfied.
Create         Freeze         OCCUPY only set the FROZEN. FROZEN will require that bit not set.
Create         Add-Src        Add-Src will require a TYPE data, which will only be set after the 
                              creation completes.
Create         Add-Dst        Only one creation will be successful, because OCCUPY slot is done by CAS.
Create         Remove         OCCUPY only set the FROZEN. Remove will require a TYPE data, which will
                              only be set after the creation completes. See Create-Delete for details.
Create         Use            OCCUPY only set the FROZEN. Use the cap will require a TYPE data, which
                              will only be set after the creation completes.
-------------------------------------------------------------------------------------------
Delete         Delete         Actual deletion done by CAS so only one deletion will be successful.
Delete         Freeze         If the deletion fails and clears the freeze flag, nothing will be done;
                              If it does not fail, then the cap will be erased, and the FREEZE CAS
                              will not succeed.
Delete         Remove         Only one will be successful because only the root can be DELETED.
Delete         Others         Banned by the FROZEN flag before deletion.
-------------------------------------------------------------------------------------------
Freeze         Create         Create will fail because something is still in the slot.
Freeze         Delete         Delete will fail if not FROZEN; Even if FROZEN, QUIESCENCE will ban it. 
Freeze         Remove         Remove will fail if not FROZEN; Even if FROZEN, QUIESCENCE will ban it. 
Freeze         Freeze         Freeze done by CAS, and only one will be successful.
Freeze         Others         Freeze will ban them if they do attempt after FROZEN set.
-------------------------------------------------------------------------------------------
Add-Src        Create         Impossible because something in that slot.
Add-Src        Freeze         Cannot freeze if already increased refcnt. If they increase REFCNT just
                              after FROZEN set, let it be. The cap cannot be removed or deleted because
                              they will check refcnt.
Add-Src        Delete         Impossible because cap not frozen.
Add-Src        Remove         Impossible because cap not frozen.
Add-Src        Others         These operations can be done in parallel, so no worry.
-------------------------------------------------------------------------------------------
Add-Dst         ...           Conclusion same as Create operation.
-------------------------------------------------------------------------------------------
Remove          ...           Conclusion same as Delete operation.
-------------------------------------------------------------------------------------------
Use            Create         Impossible because something in that slot.
Use            Delete         Impossible because not FROZEN.
Use            Freeze         It is OK.
Use            Add-Src        It is OK.
Use            Add-Dst        Impossible because something in that slot.
Use            Remove         Impossible because not FROZEN.
Use            Use            It is OK.

* Page Table Code Section *****************************************************
Different from most large-scale operating systems, RME requires the page tables
to be constructed by the user-level rather than kernel logic. Yet, RME provided
sufficient utilities for the user to conduct the necessary paging operations:

1> Creating page directories;
2> Deletiing page directories;
3> Adding(mapping) pages into page directories;
4> Deleting(unmapping) pages from page directories.
5> Constructing hierachical page tables;
6> Destructing hierachical page tables.

* Kernel Memory Code Section **************************************************
Different seL4 and Composite, RME applys a principle that resembles Fiasco.OC's
kernel object factory. However, this is simplified very much in RME - just a table
for registering kernel memory usage! Some may think that this prohibits retyping;
this is not true because we can implement it with a trusted user-level proxy.

* Process and Thread Code Section *********************************************
RME provided process as a light-weight virtual machine/container abstraction, 
and a versatile thread abstraction. Processes enforce isolation, while threads
carry out the task.

* Signal and Invocation Code Section ******************************************
RME employs simple signal endpoints for interrupt passing, inter-core interrupt
and asynchronous communication. Different from most operating systems, it employs
thread migration model for cross-boundary synchronous communication rather than
simple blockpoints. This invocation design have many benefits in many facets.

* Kernel Function Code Section ************************************************
There's no perfect operating system for a particular hardware. A hardware may
have its own idiosyncrasies that needs extra hacks. RME's kernel function utility
provides a disciplined way of making such hacks, in case you need to add new 
system calls or directly manipulate hardware.

******************************************************************************/

/* Includes ******************************************************************/
#define __HDR_DEFS__
#include "Platform/rme_platform.h"
#include "Kernel/rme_kernel.h"
#undef __HDR_DEFS__

#define __HDR_STRUCTS__
#include "Platform/rme_platform.h"
#include "Kernel/rme_kernel.h"
#undef __HDR_STRUCTS__

/* Private include */
#include "Kernel/rme_kernel.h"

#define __HDR_PUBLIC_MEMBERS__
#include "Platform/rme_platform.h"
#undef __HDR_PUBLIC_MEMBERS__
/* End Includes **************************************************************/

/* Begin Function:RME_Kmain ***************************************************
Description : The entry of the operating system.
Input       : None.
Output      : None.
Return      : rme_ret_t - This function never returns.
******************************************************************************/
rme_ret_t RME_Kmain(void)
{
    /* Disable all interrupts first */
    __RME_Disable_Int();
    /* Some low-level checks to make sure the correctness of the core */
    __RME_Low_Level_Check();
    /* Hardware low-level init */
    __RME_Low_Level_Init();
    /* Initialize the kernel page tables */
    __RME_Pgtbl_Kmem_Init();
    
    /* Initialize the kernel object allocation table - default init */
    _RME_Kotbl_Init(RME_KOTBL_WORD_NUM);
    /* Initialize system calls, and kernel timestamp counter */
    _RME_Syscall_Init();
    
    /* Boot into the first process, and handle it all the other cases&enable the interrupt */
    __RME_Boot();
    
    /* Should never reach here */
    return 0;
}
/* End Function:RME_Kmain ****************************************************/

/* Begin Function:__RME_Low_Level_Check ***************************************
Description : Do some low-level checking for the operating system.
Input       : None.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t __RME_Low_Level_Check(void)
{
    /* Make sure the machine is more than 32-bit */
    RME_ASSERT(RME_WORD_ORDER>=5);
    /* Check if the word order setting is correct */
    RME_ASSERT(RME_WORD_BITS==RME_POW2(RME_WORD_ORDER));
    /* Check if the struct sizes are correct */
    RME_ASSERT(sizeof(struct RME_Cap_Struct)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Captbl)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Pgtbl)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Proc)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Thd)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Sig)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Inv)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Kern)==RME_CAP_SIZE);
    RME_ASSERT(sizeof(struct RME_Cap_Kmem)==RME_CAP_SIZE);
    /* Check if the other configurations are correct */
    /* Kernel memory allocation minimal size aligned to word boundary */
    RME_ASSERT(RME_KMEM_SLOT_ORDER>=RME_WORD_ORDER-3);
    /* Make sure the number of priorities does not exceed half-word boundary */
    RME_ASSERT(RME_MAX_PREEMPT_PRIO<=RME_POW2(RME_WORD_BITS>>1));
    return 0;
}
/* End Function:__RME_Low_Level_Check ****************************************/

/* Begin Function:_RME_Syscall_Init *******************************************
Description : The initialization function of system calls. This actually does
              nothing except initializing the timestamp value.
Input       : None.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t _RME_Syscall_Init(void)
{
    /* Set it to 0x00..FF.. */
    RME_Timestamp=(~((rme_ptr_t)(0)))>>(sizeof(rme_ptr_t)*4);
    
    return 0;
}
/* End Function:_RME_Syscall_Init ********************************************/

/* Begin Function:_RME_Svc_Handler ********************************************
Description : The system call handler of the operating system. The register set 
              of the current thread shall be passed in as a parameter.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void _RME_Svc_Handler(struct RME_Reg_Struct* Reg)
{
    /* What's the system call number and major capability ID? */
    rme_ptr_t Svc;
    rme_ptr_t Capid;
    rme_ptr_t Param[3];
    rme_ret_t Retval;
    rme_ptr_t Svc_Num;
    struct RME_CPU_Local* CPU_Local;
    struct RME_Inv_Struct* Inv_Top;
    struct RME_Cap_Captbl* Captbl;

    /* Get the system call parameters from the system call */
    __RME_Get_Syscall_Param(Reg, &Svc, &Capid, Param);
    Svc_Num=Svc&0x3F;
    
    /* Fast path - synchronous invocation returning */
    if(Svc_Num==RME_SVC_INV_RET)
    {
        RME_COVERAGE_MARKER();
        
        Retval=_RME_Inv_Ret(Reg      /* struct RME_Reg_Struct* Reg */,
                            Param[0] /* rme_ptr_t Retval */,
                            0        /* rme_ptr_t Fault_Flag */);
        RME_SWITCH_RETURN(Reg,Retval);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Get our current capability table. No need to check whether it is frozen
     * because it can't be deleted anyway */
    CPU_Local=RME_CPU_LOCAL();
    Inv_Top=RME_INVSTK_TOP(CPU_Local->Cur_Thd);
    if(Inv_Top==0)
    {
        RME_COVERAGE_MARKER();
        
        Captbl=(CPU_Local->Cur_Thd)->Sched.Proc->Captbl;
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        Captbl=Inv_Top->Proc->Captbl;
    }

    /* Fast path - synchronous invocation activation */
    if(Svc_Num==RME_SVC_INV_ACT)
    {
        RME_COVERAGE_MARKER();
        
        Retval=_RME_Inv_Act(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                    Param[0] /* rme_cid_t Cap_Inv */,
                                    Param[1] /* rme_ptr_t Param */);
        RME_SWITCH_RETURN(Reg,Retval);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* See if this operation can potentially cause a register set switch. All the 
     * functions that may cause a register set switch is listed here. The behavior
     * of these functions shall be: If the function is successful, they shall
     * perform the return value saving on proper register stacks by themselves;
     * if the function fails, it should not conduct such return value saving. These
     * paths are less optimized than synchronous invocation, but are still optimized */
    switch(Svc_Num)
    {
        /* Send to a signal endpoint */
        case RME_SVC_SIG_SND:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Sig_Snd(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                        Param[0] /* rme_cid_t Cap_Sig */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Receive from a signal endpoint */
        case RME_SVC_SIG_RCV:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Sig_Rcv(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                        Param[0] /* rme_cid_t Cap_Sig */,
                                        Param[1] /* rme_ptr_t Option */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Call kernel functions */
        case RME_SVC_KERN:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Kern_Act(Captbl, Reg                    /* struct RME_Reg_Struct* Reg */,
                                         Capid                  /* rme_cid_t Cap_Kern */,
                                         RME_PARAM_D0(Param[0]) /* rme_ptr_t Func_ID */,
                                         RME_PARAM_D1(Param[0]) /* rme_ptr_t Sub_ID */,
                                         Param[1]               /* rme_ptr_t Param1 */,
                                         Param[2]               /* rme_ptr_t Param2 */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Changing thread priority */
        case RME_SVC_THD_SCHED_PRIO:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Sched_Prio(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                               Param[0] /* rme_cid_t Cap_Thd */,
                                               Param[1] /* rme_ptr_t Prio */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Free a thread from some core */
        case RME_SVC_THD_SCHED_FREE:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Sched_Free(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                               Param[0] /* rme_cid_t Cap_Thd */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Transfer time to a thread */
        case RME_SVC_THD_TIME_XFER:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Time_Xfer(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                              Param[0] /* rme_cid_t Cap_Thd_Dst */,
                                              Param[1] /* rme_cid_t Cap_Thd_Src */, 
                                              Param[2] /* rme_ptr_t Time */);
            RME_SWITCH_RETURN(Reg,Retval);
        }
        /* Switch to another thread */
        case RME_SVC_THD_SWT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Swt(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                        Param[0] /* rme_cid_t Cap_Thd */,
                                        Param[1] /* rme_ptr_t Full_Yield */);
            RME_SWITCH_RETURN(Reg, Retval);
        }
        default:
        {
            RME_COVERAGE_MARKER();
            break;
        }
    } 

    /* It is guaranteed that these functions will never cause a context switch */
    switch(Svc_Num)
    {
        /* Capability table */
        case RME_SVC_CAPTBL_CRT:
        {
            RME_COVERAGE_MARKER();
            Retval=_RME_Captbl_Crt(Captbl, Capid                  /* rme_cid_t Cap_Captbl_Crt */,
                                           RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Kmem */,
                                           RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Crt */,
                                           Param[1]               /* rme_ptr_t Raddr */,
                                           Param[2]               /* rme_ptr_t Entry_Num */);
            break;
        }
        case RME_SVC_CAPTBL_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Captbl_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl_Del */,
                                           Param[0] /* rme_cid_t Cap_Captbl */);
            break;
        }
        case RME_SVC_CAPTBL_FRZ:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Captbl_Frz(Captbl, Capid    /* rme_cid_t Cap_Captbl_Frz */,
                                           Param[0] /* rme_cid_t Cap_Frz */);
            break;
        }
        case RME_SVC_CAPTBL_ADD:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Captbl_Add(Captbl, RME_PARAM_D1(Param[0])  /* rme_cid_t Cap_Captbl_Dst */,
                                           RME_PARAM_D0(Param[0])  /* rme_cid_t Cap_Dst */,
                                           RME_PARAM_D1(Param[1])  /* rme_cid_t Cap_Captbl_Src */,
                                           RME_PARAM_D0(Param[1])  /* rme_cid_t Cap_Src */,
                                           Param[2]                /* rme_ptr_t Flags */,
                                           RME_PARAM_KM(Svc,Capid) /* rme_ptr_t Ext_Flags */);
            break;
        }
        case RME_SVC_CAPTBL_REM:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Captbl_Rem(Captbl, Capid    /* rme_cid_t Cap_Captbl_Rem */,
                                           Param[0] /* rme_cid_t Cap_Rem */);
            break;
        }
        /* Page table */
        case RME_SVC_PGTBL_CRT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Crt(Captbl, Capid                     /* rme_cid_t Cap_Captbl */,
                                          RME_PARAM_D1(Param[0])    /* rme_cid_t Cap_Kmem */,
                                          RME_PARAM_Q1(Param[0])    /* rme_cid_t Cap_Pgtbl */,
                                          Param[1]                  /* rme_ptr_t Raddr */,
                                          Param[2]&(RME_ALLBITS<<1) /* rme_ptr_t Base_Addr */,
                                          RME_PARAM_PT(Param[2])    /* rme_ptr_t Top_Flag */,
                                          RME_PARAM_Q0(Param[0])    /* rme_ptr_t Size_Order */,
                                          RME_PARAM_PC(Svc)         /* rme_ptr_t Num_Order */);
            break;
        }
        case RME_SVC_PGTBL_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                          Param[0] /* rme_cid_t Cap_Pgtbl */);
            break;
        }
        case RME_SVC_PGTBL_ADD:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Add(Captbl, RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Pgtbl_Dst */,
                                          RME_PARAM_D0(Param[0]) /* rme_ptr_t Pos_Dst */,
                                          Capid                  /* rme_ptr_t Flags_Dst */,
                                          RME_PARAM_D1(Param[1]) /* rme_cid_t Cap_Pgtbl_Src */,
                                          RME_PARAM_D0(Param[1]) /* rme_ptr_t Pos_Src */,
                                          Param[2]               /* rme_ptr_t Index */);
            break;
        }
        case RME_SVC_PGTBL_REM:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Rem(Captbl, Param[0] /* rme_cid_t Cap_Pgtbl */,
                                          Param[1] /* rme_ptr_t Pos */);
            break;
        }
        case RME_SVC_PGTBL_CON:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Con(Captbl, RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Pgtbl_Parent */,
                                          Param[1]               /* rme_ptr_t Pos */,
                                          RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Pgtbl_Child */,
                                          Param[2]               /* rme_ptr_t Flags_Child */);
            break;
        }
        case RME_SVC_PGTBL_DES:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Pgtbl_Des(Captbl, Param[0] /* rme_cid_t Cap_Pgtbl */,
                                          Param[1] /* rme_ptr_t Pos */);
            break;
        }
        /* Process */
        case RME_SVC_PROC_CRT:
        {
            Retval=_RME_Proc_Crt(Captbl, Capid                  /* rme_cid_t Cap_Captbl_Crt */,
                                         RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Kmem */,
                                         RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Proc */,
                                         RME_PARAM_D1(Param[1]) /* rme_cid_t Cap_Captbl */,
                                         RME_PARAM_D0(Param[1]) /* rme_cid_t Cap_Pgtbl */,
                                         Param[2]               /* rme_ptr_t Raddr */);
            break;
        }
        case RME_SVC_PROC_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Proc_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                         Param[0] /* rme_cid_t Cap_Proc */);
            break;
        }
        case RME_SVC_PROC_CPT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Proc_Cpt(Captbl, Param[0] /* rme_cid_t Cap_Proc */,
                                         Param[1] /* rme_cid_t Cap_Captbl */);
            break;
        }
        case RME_SVC_PROC_PGT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Proc_Pgt(Captbl, Param[0] /* rme_cid_t Cap_Proc */,
                                         Param[1] /* rme_cid_t Cap_Pgtbl */);
            break;
        }
        /* Thread */
        case RME_SVC_THD_CRT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Crt(Captbl, Capid                  /* rme_cid_t Cap_Captbl */,
                                        RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Kmem */,
                                        RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Thd */,
                                        RME_PARAM_D1(Param[1]) /* rme_cid_t Cap_Proc */,
                                        RME_PARAM_D0(Param[1]) /* rme_ptr_t Max_Prio */,
                                        Param[2]               /* rme_ptr_t Raddr */);
            break;
        }
        case RME_SVC_THD_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                        Param[0] /* rme_cid_t Cap_Thd */);
            break;
        }
        case RME_SVC_THD_EXEC_SET:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Exec_Set(Captbl, Capid    /* rme_cid_t Cap_Thd */,
                                             Param[0] /* rme_ptr_t Entry */,
                                             Param[1] /* rme_ptr_t Stack */,
                                             Param[2] /* rme_ptr_t Param */);
            break;
        }
        case RME_SVC_THD_HYP_SET:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Hyp_Set(Captbl, Param[0] /* rme_cid_t Cap_Thd */,
                                            Param[1] /* rme_ptr_t Kaddr */);
            break;
        }
        case RME_SVC_THD_SCHED_BIND:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Sched_Bind(Captbl, Capid                  /* rme_cid_t Cap_Thd */,
                                               RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Thd_Sched */,
                                               RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Sig */,
                                               Param[1]               /* rme_tid_t TID */,
                                               Param[2]               /* rme_ptr_t Prio */);
            break;
        }
        case RME_SVC_THD_SCHED_RCV:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Thd_Sched_Rcv(Captbl, Reg      /* struct RME_Reg_Struct* Reg */,
                                              Param[0] /* rme_cid_t Cap_Thd */);
            break;
        }
        /* Signal */
        case RME_SVC_SIG_CRT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Sig_Crt(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                        Param[0] /* rme_cid_t Cap_Kmem */,
                                        Param[1] /* rme_cid_t Cap_Sig */, 
                                        Param[2] /* rme_ptr_t Raddr */);
            break;
        }
        case RME_SVC_SIG_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Sig_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                        Param[0] /* rme_cid_t Cap_Sig */);
            break;
        }
        /* Invocation */
        case RME_SVC_INV_CRT:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Inv_Crt(Captbl, Capid                  /* rme_cid_t Cap_Captbl */,
                                        RME_PARAM_D1(Param[0]) /* rme_cid_t Cap_Kmem */,
                                        RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Inv */,
                                        Param[1]               /* rme_cid_t Cap_Proc */,
                                        Param[2]               /* rme_ptr_t Raddr */);
            break;
        }
        case RME_SVC_INV_DEL:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Inv_Del(Captbl, Capid    /* rme_cid_t Cap_Captbl */,
                                        Param[0] /* rme_cid_t Cap_Inv */);
            break;
        }
        case RME_SVC_INV_SET:
        {
            RME_COVERAGE_MARKER();
            
            Retval=_RME_Inv_Set(Captbl, RME_PARAM_D0(Param[0]) /* rme_cid_t Cap_Inv */,
                                        Param[1]               /* rme_ptr_t Entry */,
                                        Param[2]               /* rme_ptr_t Stack */,
                                        RME_PARAM_D1(Param[0]) /* rme_ptr_t Fault_Ret_Flag */);
            break;
        }
        /* This is an error */
        default: 
        {
            RME_COVERAGE_MARKER();
            
            Retval=RME_ERR_CAP_NULL;
            break;
        }
    }
    
    /* We set the registers and return */
    __RME_Set_Syscall_Retval(Reg, Retval);
}
/* End Function:_RME_Svc_Handler *********************************************/

/* Begin Function:_RME_Timestamp_Inc ******************************************
Description : This function is used in the drivers to update the timestamp value.
Input       : rme_cnt_t Value - The value to increase.
Output      : None.
Return      : rme_ptr_t - The timestamp value before the increment.
******************************************************************************/
rme_ptr_t _RME_Timestamp_Inc(rme_cnt_t Value)
{
    /* The incremental value cannot be smaller than zero or equal to zero */
    RME_ASSERT(Value>0);
    return RME_FETCH_ADD(&RME_Timestamp,Value);
}
/* End Function:_RME_Timestamp_Inc *******************************************/

/* Begin Function:_RME_Tick_SMP_Handler ***************************************
Description : The system tick timer handler of RME, on all processors except for
              the main processor.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void _RME_Tick_SMP_Handler(struct RME_Reg_Struct* Reg)
{
    struct RME_CPU_Local* CPU_Local;

    CPU_Local=RME_CPU_LOCAL();
    if((CPU_Local->Cur_Thd)->Sched.Slices<RME_THD_INF_TIME)
    {
        RME_COVERAGE_MARKER();
        
        /* Decrease timeslice count */
        (CPU_Local->Cur_Thd)->Sched.Slices--;
        /* See if the current thread's timeslice is used up */
        if((CPU_Local->Cur_Thd)->Sched.Slices==0)
        {
            RME_COVERAGE_MARKER();
            
            /* Running out of time. Kick this guy out and pick someone else */
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_TIMEOUT;
            /* Delete it from runqueue */
            _RME_Run_Del(CPU_Local->Cur_Thd);
            /* Send a scheduler notification to its parent */
            _RME_Run_Notif(CPU_Local->Cur_Thd);
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Send to the system ticker receive endpoint. This endpoint is per-core */
    _RME_Kern_Snd(CPU_Local->Tick_Sig);

    /* All kernel send complete, now pick the highest priority thread to run */
    _RME_Kern_High(Reg, CPU_Local);
}
/* End Function:_RME_Tick_SMP_Handler ****************************************/

/* Begin Function:_RME_Tick_Handler *******************************************
Description : The system tick timer handler of RME.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void _RME_Tick_Handler(struct RME_Reg_Struct* Reg)
{
    /* Increase the tick count */
    RME_Timestamp++;
    /* Call generic handler */
    _RME_Tick_SMP_Handler(Reg);
}
/* End Function:_RME_Tick_Handler ********************************************/

/* Begin Function:_RME_Clear **************************************************
Description : Memset a memory area to zero. This is not fast due to byte operations;
              this is not meant for large memory.
Input       : void* Addr - The address to clear.
              rme_ptr_t Size - The size to clear.
Output      : None.
Return      : None.
******************************************************************************/
void _RME_Clear(void* Addr, rme_ptr_t Size)
{
    rme_cnt_t Count;

    for(Count=0;Count<Size;Count++)
        ((rme_u8_t*)Addr)[Count]=0;
}
/* End Function:_RME_Clear ***************************************************/

/* Begin Function:_RME_Memcmp *************************************************
Description : Compare two memory segments to see if they are equal. This is not
              fast due to byte operations; this is not meant for large memory.
Input       : const void* Ptr1 - The first memory region.
              const void* Ptr2 - The second memory region.
              rme_ptr_t Num - The number of bytes to compare.
Output      : None.
Return      : rme_ret_t - If Ptr1>Ptr2, then return a positive value; else a negative
                          value. If Ptr1==Ptr2, then return 0;
******************************************************************************/
rme_ret_t _RME_Memcmp(const void* Ptr1, const void* Ptr2, rme_ptr_t Num)
{
    rme_u8_t* Dst;
    rme_u8_t* Src;
    rme_cnt_t Count;

    Dst=(rme_u8_t*)Ptr1;
    Src=(rme_u8_t*)Ptr2;

    for(Count=0;Count<Num;Count++)
    {
        if(Dst[Count]!=Src[Count])
        {
            RME_COVERAGE_MARKER();
            
            return Dst[Count]-Src[Count];
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }

    return 0;
}
/* End Function:_RME_Memcmp **************************************************/

/* Begin Function:_RME_Memcpy *************************************************
Description : Copy one segment of memory to another segment. This is not fast
              due to byte operations; this is not meant for large memory.
Input       : void* Dst - The first memory region.
              void* Src - The second memory region.
              rme_ptr_t Num - The number of bytes to compare.
              rme_ptr_t Size - The size to clear.
Output      : None.
Return      : None.
******************************************************************************/
void _RME_Memcpy(void* Dst, void* Src, rme_ptr_t Num)
{
    rme_cnt_t Count;

    for(Count=0;Count<Num;Count++)
        ((rme_u8_t*)Dst)[Count]=((rme_u8_t*)Src)[Count];
}
/* End Function:_RME_Memcpy **************************************************/

/* Begin Function:RME_Print_Int ***********************************************
Description : Print a signed integer on the debugging console. This integer is
              printed as decimal with sign.
Input       : rme_cnt_t Int - The integer to print.
Output      : None.
Return      : rme_cnt_t - The length of the string printed.
******************************************************************************/
rme_cnt_t RME_Print_Int(rme_cnt_t Int)
{
    rme_ptr_t Iter;
    rme_cnt_t Count;
    rme_cnt_t Num;
    rme_ptr_t Div;
    
    /* how many digits are there? */
    if(Int==0)
    {
        RME_COVERAGE_MARKER();
        
        __RME_Putchar('0');
        return 1;
    }
    else if(Int<0)
    {
        RME_COVERAGE_MARKER();
        
        /* How many digits are there? */
        Count=0;
        Div=1;
        Iter=-Int;
        while(Iter!=0)
        {
            Iter/=10;
            Count++;
            Div*=10;
        }
        Div/=10;
        
        __RME_Putchar('-');
        Iter=-Int;
        Num=Count+1;
        
        while(Count>0)
        {
            Count--;
            __RME_Putchar(Iter/Div+'0');
            Iter=Iter%Div;
            Div/=10;
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* How many digits are there? */
        Count=0;
        Div=1;
        Iter=Int;
        while(Iter!=0)
        {
            Iter/=10;
            Count++;
            Div*=10;
        }
        Div/=10;
        
        Iter=Int;
        Num=Count;
        
        while(Count>0)
        {
            Count--;
            __RME_Putchar(Iter/Div+'0');
            Iter=Iter%Div;
            Div/=10;
        }
    }
    
    return Num;
}
/* End Function:RME_Print_Int ************************************************/

/* Begin Function:RME_Print_Uint **********************************************
Description : Print a unsigned integer on the debugging console. This integer is
              printed as hexadecimal.
Input       : rme_ptr_t Uint - The unsigned integer to print.
Output      : None.
Return      : rme_cnt_t - The length of the string printed.
******************************************************************************/
rme_cnt_t RME_Print_Uint(rme_ptr_t Uint)
{
    rme_ptr_t Iter;
    rme_cnt_t Count;
    rme_cnt_t Num;
    
    /* how many digits are there? */
    if(Uint==0)
    {
        RME_COVERAGE_MARKER();
        
        __RME_Putchar('0');
        return 1;
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* Filter out all the zeroes */
        Count=0;
        Iter=Uint;
        while((Iter>>((sizeof(rme_ptr_t)*8)-4))==0)
        {
            Iter<<=4;
            Count++;
        }
        
        /* Count is the number of pts to print */
        Count=sizeof(rme_ptr_t)*2-Count;
        Num=Count;
        while(Count>0)
        {
            Count--;
            Iter=(Uint>>(Count*4))&0x0F;
            if(Iter<10)
                __RME_Putchar('0'+Iter);
            else
                __RME_Putchar('A'+Iter-10);
        }
    }
    
    return Num;
}
/* End Function:RME_Print_Uint ***********************************************/

/* Begin Function:RME_Print_String ********************************************
Description : Print a string the kernel console.
              This is only used for kernel-level debugging.
Input       : rme_s8_t* String - The string to print
Output      : None.
Return      : rme_cnt_t - The length of the string printed, the '\0' is not included.
******************************************************************************/
rme_cnt_t RME_Print_String(rme_s8_t* String)
{
    rme_cnt_t Count;
    
    Count=0;
    while(Count<RME_KERNEL_DEBUG_MAX_STR)
    {
        if(String[Count]=='\0')
        {
            RME_COVERAGE_MARKER();
            
            break;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        __RME_Putchar(String[Count++]);
    }
    
    return Count;
}
/* End Function:RME_Print_String *********************************************/

/* Begin Function:_RME_Captbl_Boot_Init ***************************************
Description : Create the first boot-time capability table. This will be the first
              capability table created in the system. This function must be called
              at system startup first before setting up any other kernel objects.
              This function does not require a kernel memory capability.
Input       : rme_cid_t Cap_Captbl - The capability slot that you want this newly created
                                     capability table capability to be in. 1-Level.
              rme_ptr_t Vaddr - The virtual address to store the capability table kernel object.
              rme_ptr_t Entry_Num - The number of capabilities in the capability table.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Boot_Init(rme_cid_t Cap_Captbl, rme_ptr_t Vaddr, rme_ptr_t Entry_Num)
{
    rme_cnt_t Count;
    struct RME_Cap_Captbl* Captbl;

    /* See if the entry number is too big */
    if((Entry_Num==0)||(Entry_Num>RME_CAPID_2L))
    {
        RME_COVERAGE_MARKER();
        
    	return RME_ERR_CAP_RANGE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_CAPTBL_SIZE(Entry_Num))!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Done. We start creation of the capability table. Clear header as well */
    for(Count=0;Count<Entry_Num;Count++)
        RME_CAP_CLEAR(&(((struct RME_Cap_Struct*)Vaddr)[Count]));

    Captbl=&(((struct RME_Cap_Captbl*)Vaddr)[Cap_Captbl]);
    /* Set the cap's parameters according to what we have just created */
    RME_CAP_CLEAR(Captbl);
    Captbl->Head.Parent=0;
    Captbl->Head.Object=Vaddr;
    /* New cap allows all operations */
    Captbl->Head.Flags=RME_CAPTBL_FLAG_CRT|RME_CAPTBL_FLAG_DEL|RME_CAPTBL_FLAG_FRZ|
                       RME_CAPTBL_FLAG_ADD_SRC|RME_CAPTBL_FLAG_ADD_DST|RME_CAPTBL_FLAG_REM|
                       RME_CAPTBL_FLAG_PROC_CRT|RME_CAPTBL_FLAG_PROC_CPT;
    Captbl->Entry_Num=Entry_Num;

    /* At last, write into slot the correct information, and clear the frozen bit */
    RME_WRITE_RELEASE(&(Captbl->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_CAPTBL,0));
    return Cap_Captbl;
}
/* End Function:_RME_Captbl_Boot_Init ****************************************/

/* Begin Function:_RME_Captbl_Boot_Crt ****************************************
Description : Create a boot-time capability table at the memory segment designated.
              This function does not ask require a kernel memory capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Crt - The capability to the captbl that may contain
                                         the cap to new captbl. 2-Level.
              rme_cid_t Cap_Crt - The cap position to hold the new cap. 1-Level.
              rme_ptr_t Vaddr - The virtual address to store the capability table kernel object.
              rme_ptr_t Entry_Num - The number of capabilities in the capability table.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                               rme_cid_t Cap_Crt, rme_ptr_t Vaddr, rme_ptr_t Entry_Num)
{
    rme_cnt_t Count;
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Captbl* Captbl_Crt;
    rme_ptr_t Type_Ref;
    
    /* See if the entry number is too big - this is not restricted by RME_CAPTBL_LIMIT */
    if((Entry_Num==0)||(Entry_Num>RME_CAPID_2L))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_RANGE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Crt,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);

    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Crt,struct RME_Cap_Captbl*,Captbl_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Captbl_Crt,Type_Ref);
    /* Try to mark this area as populated */
    if(_RME_Kotbl_Mark(Vaddr, RME_CAPTBL_SIZE(Entry_Num))!=0)
    {
        /* Failure. Set the Type_Ref back to 0 and abort the creation process */
        RME_WRITE_RELEASE(&(Captbl_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }

    /* Done. We start creation of the capability table. Clear header as well */
    for(Count=0;Count<Entry_Num;Count++)
        RME_CAP_CLEAR(&(((struct RME_Cap_Struct*)Vaddr)[Count]));

    /* Set the cap's parameters according to what we have just created */
    Captbl_Crt->Head.Parent=0;
    Captbl_Crt->Head.Object=Vaddr;
    Captbl_Crt->Head.Flags=RME_CAPTBL_FLAG_CRT|RME_CAPTBL_FLAG_DEL|RME_CAPTBL_FLAG_FRZ|
                           RME_CAPTBL_FLAG_ADD_SRC|RME_CAPTBL_FLAG_ADD_DST|RME_CAPTBL_FLAG_REM|
                           RME_CAPTBL_FLAG_PROC_CRT|RME_CAPTBL_FLAG_PROC_CPT;
    Captbl_Crt->Entry_Num=Entry_Num;

    /* At last, write into slot the correct information, and clear the frozen bit */
    RME_WRITE_RELEASE(&(Captbl_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_CAPTBL,0));
    return 0;
}
/* End Function:_RME_Captbl_Boot_Crt *****************************************/

/* Begin Function:_RME_Captbl_Crt *********************************************
Description : Create a capability table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Crt - The capability to the captbl that may contain
                                         the cap to new captbl. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Crt - The cap position to hold the new cap. 1-Level.
              rme_ptr_t Raddr - The relative virtual address to store the capability table.
              rme_ptr_t Entry_Num - The number of capabilities in the capability table.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                          rme_cid_t Cap_Kmem, rme_cid_t Cap_Crt, rme_ptr_t Raddr, rme_ptr_t Entry_Num)
{
    rme_cnt_t Count;
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Captbl* Captbl_Crt;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;

    /* See if the entry number is too big */
    if((Entry_Num==0)||(Entry_Num>RME_CAPID_2L))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_RANGE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Are we overrunning the size limit? */
#if(RME_CAPTBL_LIMIT!=0)
    if(Entry_Num>RME_CAPTBL_LIMIT)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_RANGE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#endif

    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Crt,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    /* See if the creation is valid for this kmem range */
    RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_CAPTBL,Raddr,Vaddr,RME_CAPTBL_SIZE(Entry_Num));

    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Crt,struct RME_Cap_Captbl*,Captbl_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Captbl_Crt,Type_Ref);
    /* Try to mark this area as populated */
    if(_RME_Kotbl_Mark(Vaddr, RME_CAPTBL_SIZE(Entry_Num))!=0)
    {
        RME_COVERAGE_MARKER();
        
        /* Failure. Set the Type_Ref back to 0 and abort the creation process */
        RME_WRITE_RELEASE(&(Captbl_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Done. We start creation of the capability table. Clear header as well */
    for(Count=0;Count<Entry_Num;Count++)
        RME_CAP_CLEAR(&(((struct RME_Cap_Struct*)Vaddr)[Count]));

    /* Set the cap's parameters according to what we have just created */
    Captbl_Crt->Head.Parent=0;
    Captbl_Crt->Head.Object=Vaddr;
    Captbl_Crt->Head.Flags=RME_CAPTBL_FLAG_CRT|RME_CAPTBL_FLAG_DEL|RME_CAPTBL_FLAG_FRZ|
                           RME_CAPTBL_FLAG_ADD_SRC|RME_CAPTBL_FLAG_ADD_DST|RME_CAPTBL_FLAG_REM|
                           RME_CAPTBL_FLAG_PROC_CRT|RME_CAPTBL_FLAG_PROC_CPT;
    Captbl_Crt->Entry_Num=Entry_Num;

    /* At last, write into slot the correct information, and clear the frozen bit */
    RME_WRITE_RELEASE(&(Captbl_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_CAPTBL,0));
    return 0;
}
/* End Function:_RME_Captbl_Crt **********************************************/

/* Begin Function:_RME_Captbl_Del *********************************************
Description : Delete a layer of capability table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Del - The capability table containing the cap to
                                         captbl for deletion. 2-Level.
              rme_cid_t Cap_Del - The capability to the captbl being deleted. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Del, rme_cid_t Cap_Del)
{
    rme_cnt_t Count;
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Captbl* Captbl_Del;
    rme_ptr_t Type_Ref;
    /* These are used for deletion */
    rme_ptr_t Object;
    rme_ptr_t Size;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Del,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Del,struct RME_Cap_Captbl*,Captbl_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Captbl_Del,Type_Ref,RME_CAP_CAPTBL);
    
    /* Is there any capability in this capability table? If yes, we cannot destroy it.
     * We will check every slot to make sure nothing is there. This is surely,
     * predictable but not so perfect. So, if the time of such operations is to be 
     * bounded, the user must control the number of entries in the table */
    for(Count=0;Count<Captbl_Del->Entry_Num;Count++)
    {
        if(RME_CAP_TYPE(RME_CAP_GETOBJ(Captbl_Del,struct RME_Cap_Struct*)[Count].Head.Type_Ref)!=RME_CAP_NOP)
        {
            RME_COVERAGE_MARKER();
            
            RME_CAP_DEFROST(Captbl_Del,Type_Ref);
            return RME_ERR_CAP_EXIST;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    
    /* Remember these two variables for deletion */
    Object=RME_CAP_GETOBJ(Captbl_Del,rme_ptr_t);
    Size=RME_CAPTBL_SIZE(Captbl_Del->Entry_Num);

    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Captbl_Del,Type_Ref);
    /* Try to depopulate the area - this must be successful */
    RME_ASSERT(_RME_Kotbl_Erase(Object,Size)!=0);
    
    return 0;
}
/* End Function:_RME_Captbl_Del **********************************************/

/* Begin Function:_RME_Captbl_Frz *********************************************
Description : Freeze a capability in the capability table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Frz  - The capability table containing the cap to
                                          captbl for this operation. 2-Level.
              rme_cid_t Cap_Frz - The cap to the kernel object being freezed. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Frz(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Frz, rme_cid_t Cap_Frz)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Struct* Captbl_Frz;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Frz,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_FRZ);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Frz,struct RME_Cap_Struct*,Captbl_Frz);
    
    /* Check if anything is there. If nothing there, the Type_Ref must be 0. 
     * Need a read acquire barrier here to avoid stale reads below. */
    Type_Ref=RME_READ_ACQUIRE(&(Captbl_Frz->Head.Type_Ref));
    /* See if there is a cap */
    if(RME_CAP_TYPE(Type_Ref)==RME_CAP_NOP)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_NULL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    /* The reference count does not allow freezing */
    if(RME_CAP_REF(Type_Ref)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    /* The capability is already frozen - why do it again? */
    if((Type_Ref&RME_CAP_FROZEN)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_FROZEN;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the slot is quiescent */
    if(RME_UNLIKELY(RME_CAP_QUIE(Captbl_Frz->Head.Timestamp)==0))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_QUIE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Update the timestamp */
    Captbl_Frz->Head.Timestamp=RME_Timestamp;
    
    /* Finally, freeze it */
    if(RME_COMP_SWAP(&(Captbl_Frz->Head.Type_Ref),Type_Ref,Type_Ref|RME_CAPTBL_FLAG_FRZ)==0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_EXIST;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Captbl_Frz **********************************************/

/* Begin Function:_RME_Captbl_Add *********************************************
Description : Add one capability into the capability table. This is doing capability delegation.
Input       : struct RME_Cap_Captbl* Captbl - The capability to the master capability table.
              rme_cid_t Cap_Captbl_Dst - The capability to the destination capability table. 2-Level.
              rme_cid_t Cap_Dst - The capability slot you want to add to. 1-Level.
              rme_cid_t Cap_Captbl_Src - The capability to the source capability table. 2-Level.
              rme_cid_t Cap_Src - The capability in the source capability table to delegate. 1-Level.
              rme_ptr_t Flags - The flags to delegate. The flags can restrict which operations
                                are possible on the cap. If the cap delegated is a page table, we also
                                pass the range information in this field.
              rme_ptr_t Ext_Flags - The extended flags, only effective for kernel memory capability.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Add(struct RME_Cap_Captbl* Captbl,
                          rme_cid_t Cap_Captbl_Dst, rme_cid_t Cap_Dst, 
                          rme_cid_t Cap_Captbl_Src, rme_cid_t Cap_Src,
                          rme_ptr_t Flags, rme_ptr_t Ext_Flags)
{
    struct RME_Cap_Captbl* Captbl_Dst;
    struct RME_Cap_Captbl* Captbl_Src;
    struct RME_Cap_Struct* Cap_Dst_Struct;
    struct RME_Cap_Struct* Cap_Src_Struct;
    rme_ptr_t Type_Ref;
    
    /* These variables are only used for kernel memory checks */
    rme_ptr_t Kmem_End;
    rme_ptr_t Kmem_Start;
    rme_ptr_t Kmem_Flags;

    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Dst,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Dst,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Src,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Src,Type_Ref);
    /* Check if both captbls are not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Dst,RME_CAPTBL_FLAG_ADD_DST);
    RME_CAP_CHECK(Captbl_Src,RME_CAPTBL_FLAG_ADD_SRC);
    
    /* Get the cap slots */
    RME_CAPTBL_GETSLOT(Captbl_Dst,Cap_Dst,struct RME_Cap_Struct*,Cap_Dst_Struct);
    RME_CAPTBL_GETSLOT(Captbl_Src,Cap_Src,struct RME_Cap_Struct*,Cap_Src_Struct);
    
    /* Atomic read - Read barrier to avoid premature checking of the rest */
    Type_Ref=RME_READ_ACQUIRE(&(Cap_Src_Struct->Head.Type_Ref));
    /* Is the source cap freezed? */
    if((Type_Ref&RME_CAP_FROZEN)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_FROZEN;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    /* Does the source cap exist? */
    if(Type_Ref==0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_CAP_NULL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Dewarn some compilers that complain about uninitialized variables */
    Kmem_End=0;
    Kmem_Start=0;
    Kmem_Flags=0;
    
    /* Is there a flag conflict? - For page tables, we have different checking mechanisms */
    if(RME_CAP_TYPE(Cap_Src_Struct->Head.Type_Ref)==RME_CAP_PGTBL)
    {
        RME_COVERAGE_MARKER();
        
        /* Check the delegation range */
        if(RME_PGTBL_FLAG_HIGH(Flags)>RME_PGTBL_FLAG_HIGH(Cap_Src_Struct->Head.Flags))
        {
            RME_COVERAGE_MARKER();
        
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if(RME_PGTBL_FLAG_LOW(Flags)<RME_PGTBL_FLAG_LOW(Cap_Src_Struct->Head.Flags))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if(RME_PGTBL_FLAG_HIGH(Flags)<RME_PGTBL_FLAG_LOW(Flags))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        /* Check the flags - if there are extra ones, or all zero */
        if(RME_PGTBL_FLAG_FLAGS(Flags)==0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if((RME_PGTBL_FLAG_FLAGS(Flags)&(~RME_PGTBL_FLAG_FLAGS(Cap_Src_Struct->Head.Flags)))!=0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else if(RME_CAP_TYPE(Cap_Src_Struct->Head.Type_Ref)==RME_CAP_KERN)
    {
        RME_COVERAGE_MARKER();
        
        /* Kernel capabilities only have ranges, no flags - check the delegation range */
        /* Check the delegation range */
        if(RME_KERN_FLAG_HIGH(Flags)>RME_KERN_FLAG_HIGH(Cap_Src_Struct->Head.Flags))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if(RME_KERN_FLAG_LOW(Flags)<RME_KERN_FLAG_LOW(Cap_Src_Struct->Head.Flags))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if(RME_KERN_FLAG_HIGH(Flags)<RME_KERN_FLAG_LOW(Flags))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else if(RME_CAP_TYPE(Cap_Src_Struct->Head.Type_Ref)==RME_CAP_KMEM)
    {
        RME_COVERAGE_MARKER();
        
        Kmem_End=RME_KMEM_FLAG_HIGH(Flags,Ext_Flags);
        Kmem_Start=RME_KMEM_FLAG_LOW(Flags,Ext_Flags);
        Kmem_Flags=RME_KMEM_FLAG_FLAGS(Ext_Flags);
        
        /* Round start and end to the slot boundary, if we are using slots bigger than 64 bytes */
#if(RME_KMEM_SLOT_ORDER>6)
        Kmem_End=RME_ROUND_DOWN(Kmem_End,RME_KMEM_SLOT_ORDER);
        Kmem_Start=RME_ROUND_UP(Kmem_Start,RME_KMEM_SLOT_ORDER);
#endif
        if(Kmem_End<=Kmem_Start)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }

        /* Convert relative addresses to absolute addresses and check for overflow */
        Kmem_Start+=((struct RME_Cap_Kmem*)Cap_Src_Struct)->Start;
        if(Kmem_Start<((struct RME_Cap_Kmem*)Cap_Src_Struct)->Start)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        Kmem_End+=((struct RME_Cap_Kmem*)Cap_Src_Struct)->Start;
        if(Kmem_End<((struct RME_Cap_Kmem*)Cap_Src_Struct)->Start)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }

        /* Check the ranges of kernel memory */
        if(((struct RME_Cap_Kmem*)Cap_Src_Struct)->Start>Kmem_Start)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if(((struct RME_Cap_Kmem*)Cap_Src_Struct)->End<(Kmem_End-1))
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Check the flags - if there are extra ones, or all zero */
        if(Kmem_Flags==0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if((Kmem_Flags&(~(Cap_Src_Struct->Head.Flags)))!=0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* Check the flags - if there are extra ones, or all zero */
        if(Flags==0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        if((Flags&(~(Cap_Src_Struct->Head.Flags)))!=0)
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_CAP_FLAG;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    
    /* Is the destination slot unoccupied? */
    if(Cap_Dst_Struct->Head.Type_Ref!=0)
    {
        RME_COVERAGE_MARKER();
            
        return RME_ERR_CAP_EXIST;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Try to take the empty slot */
    RME_CAPTBL_OCCUPY(Cap_Dst_Struct,Type_Ref);
    
    /* All done, we replicate the cap with flags */
    if(RME_CAP_TYPE(Cap_Src_Struct->Head.Type_Ref)==RME_CAP_KMEM)
    {
        RME_COVERAGE_MARKER();
            
        RME_CAP_COPY(Cap_Dst_Struct,Cap_Src_Struct,Kmem_Flags);
        /* If this is a kernel memory cap, we need to write the range information as well.
         * This range information is absolute address */
        ((struct RME_Cap_Kmem*)Cap_Dst_Struct)->Start=Kmem_Start;
        /* Internally, the end is stored in a full inclusive encoding for Kmem_End */
        ((struct RME_Cap_Kmem*)Cap_Dst_Struct)->End=Kmem_End-1;
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        RME_CAP_COPY(Cap_Dst_Struct,Cap_Src_Struct,Flags);
    }
    
    /* Set the parent */
    Cap_Dst_Struct->Head.Parent=(rme_ptr_t)Cap_Src_Struct;
    /* Set the parent's reference count */
    Type_Ref=RME_FETCH_ADD(&(Cap_Src_Struct->Head.Type_Ref), 1);
    /* Is it overflowed? */
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();
            
        /* Refcnt overflowed(very unlikely to happen) */
        RME_FETCH_ADD(&(Cap_Src_Struct->Head.Type_Ref), -1);
        /* Clear the taken slot as well */
        RME_WRITE_RELEASE(&(Cap_Dst_Struct->Head.Type_Ref),0);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Write in the correct information at last */
    RME_WRITE_RELEASE(&(Cap_Dst_Struct->Head.Type_Ref),
                      RME_CAP_TYPEREF(RME_CAP_TYPE(Cap_Src_Struct->Head.Type_Ref),0));
    return 0;
}
/* End Function:_RME_Captbl_Add **********************************************/

/* Begin Function:_RME_Captbl_Rem *********************************************
Description : Remove one capability from the capability table. This function reverts
              the delegation.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Rem - The capability to the capability table to 
                                         remove from. 2-Level.
              rme_cid_t Cap_Rem - The capability slot you want to remove. 1-Level.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Captbl_Rem(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Rem, rme_cid_t Cap_Rem)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Struct* Captbl_Rem;
    rme_ptr_t Type_Ref;
    /* This is used for removal */
    struct RME_Cap_Struct* Parent;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Rem,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_REM);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Rem,struct RME_Cap_Struct*,Captbl_Rem);
    /* Removal check */
    RME_CAP_REM_CHECK(Captbl_Rem,Type_Ref);
    /* Remember this for refcnt operations */
    Parent=(struct RME_Cap_Struct*)(Captbl_Rem->Head.Parent);

    /* Remove the cap at last */
    RME_CAP_REMDEL(Captbl_Rem,Type_Ref);
    
    /* Check done, decrease its parent's refcnt */
    RME_FETCH_ADD(&(Parent->Head.Type_Ref), -1);
    
    return 0;
}
/* End Function:_RME_Captbl_Rem **********************************************/

/* Begin Function:_RME_Pgtbl_Boot_Crt *****************************************
Description : Create a boot-time page table, and put that capability into a designated
              capability table. The function will check if the memory region is large
              enough, and how it has been typed; if the function found out that it is
              impossible to create a page table due to some error, it will return an
              error.
              This function does not require a kernel memory capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the captbl that may contain the cap
                                     to new captbl. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability slot that you want this newly created
                                    page table capability to be in. 1-Level.
              rme_ptr_t Vaddr - The virtual address to store the page table kernel object.
              rme_ptr_t Base_Addr - The virtual address to start mapping for this page table.  
                                    This address must be aligned to the total size of the table.
              rme_ptr_t Top_Flag - Whether this page table is the top-level. If it is, we will
                                   map all the kernel page directories into this one.
              rme_ptr_t Size_Order - The size order of the page table. The size refers to
                                     the size of each page in the page directory.
              rme_ptr_t Num_Order - The number order of entries in the page table.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                              rme_cid_t Cap_Pgtbl, rme_ptr_t Vaddr, rme_ptr_t Base_Addr,
                              rme_ptr_t Top_Flag, rme_ptr_t Size_Order, rme_ptr_t Num_Order)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Pgtbl* Pgtbl_Crt;
    rme_ptr_t Type_Ref;
    
    /* Check if the total representable memory exceeds our maximum possible
     * addressible memory under the machine word length */
    if((Size_Order+Num_Order)>RME_POW2(RME_WORD_ORDER))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    
    /* Check if these parameters are feasible */
    if(__RME_Pgtbl_Check(Base_Addr, Top_Flag, Size_Order, Num_Order, Vaddr)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Check if the base address is properly aligned to the total order of the page table */
    if((Base_Addr&RME_MASK_END(Size_Order+Num_Order-1))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Pgtbl,struct RME_Cap_Pgtbl*,Pgtbl_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Pgtbl_Crt,Type_Ref);

    /* Try to populate the area - Are we creating the top level? */
    if(Top_Flag!=0)
    {
        RME_COVERAGE_MARKER();
        
        if(_RME_Kotbl_Mark(Vaddr, RME_PGTBL_SIZE_TOP(Num_Order))!=0)
        {
            RME_COVERAGE_MARKER();
        
            RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
            return RME_ERR_CAP_KOTBL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        if(_RME_Kotbl_Mark(Vaddr, RME_PGTBL_SIZE_NOM(Num_Order))!=0)
        {
            RME_COVERAGE_MARKER();
        
            RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
            return RME_ERR_CAP_KOTBL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    
    Pgtbl_Crt->Head.Parent=0;
    Pgtbl_Crt->Head.Object=Vaddr;
    /* Set the property of the page table to only act as source and creating process */
    Pgtbl_Crt->Head.Flags=RME_PGTBL_FLAG_FULL_RANGE|
                          RME_PGTBL_FLAG_ADD_SRC|
                          RME_PGTBL_FLAG_PROC_CRT;
    Pgtbl_Crt->Base_Addr=Base_Addr|Top_Flag;
    /* These two variables are directly placed here. Checks will be done by the driver */
    Pgtbl_Crt->Size_Num_Order=RME_PGTBL_ORDER(Size_Order,Num_Order);
    /* We start initialization of the page table, and we also add all kernel pages
     * to them if they are top-level. If unsuccessful, we revert operations. */
    if(__RME_Pgtbl_Init(Pgtbl_Crt)!=0)
    {
        RME_COVERAGE_MARKER();
        
        /* This must be successful */
        if(Top_Flag!=0)
        {
            RME_COVERAGE_MARKER();
        
            RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PGTBL_SIZE_TOP(Num_Order))==0);
        }
        else
        {
            RME_COVERAGE_MARKER();
        
            RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PGTBL_SIZE_NOM(Num_Order))==0);
        }
        
        /* Unsuccessful. Revert operations */
        RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Creation complete */
    RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_PGTBL,0));
    return 0;
}
/* End Function:_RME_Pgtbl_Boot_Crt ******************************************/

/* Begin Function:_RME_Pgtbl_Boot_Con *****************************************
Description : At boot-time, map a child page table from the parent page table. 
              In other words, we are doing the construction of a page table tree.
              This operation is only used at boot-time and does not check for flags.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl_Parent - The capability to the parent page table. 2-Level.
              rme_ptr_t Pos - The virtual address to position map the child page table to.
              rme_cid_t Cap_Pgtbl_Child - The capability to the child page table. 2-Level.
              rme_ptr_t Flags_Child - The flags for the child page table mapping. This restricts
                                      the access permissions of all the memory under this mapping.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Boot_Con(struct RME_Cap_Captbl* Captbl,
                              rme_cid_t Cap_Pgtbl_Parent, rme_ptr_t Pos,
                              rme_cid_t Cap_Pgtbl_Child, rme_ptr_t Flags_Child)
{
    struct RME_Cap_Pgtbl* Pgtbl_Parent;
    struct RME_Cap_Pgtbl* Pgtbl_Child;
    rme_ptr_t Type_Ref;

    /* The total size order of the child table */
    rme_ptr_t Child_Size_Ord;
#if(RME_VA_EQU_PA==RME_TRUE)
    /* The start and end mapping address in the parent */
    rme_ptr_t Parent_Map_Addr;
    rme_ptr_t Parend_End_Addr;
#endif
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Parent,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Parent,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Child,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Child,Type_Ref);
    /* Check if both page table caps are not frozen but don't check flags */
    RME_CAP_CHECK(Pgtbl_Parent, 0);
    RME_CAP_CHECK(Pgtbl_Child, 0);
    
    /* See if the mapping range is allowed */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Parent->Size_Num_Order))!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the child table falls within one slot of the parent table */
    Child_Size_Ord=RME_PGTBL_NUMORD(Pgtbl_Child->Size_Num_Order)+
                   RME_PGTBL_SIZEORD(Pgtbl_Child->Size_Num_Order);
    if(RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order)<Child_Size_Ord)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
#if(RME_VA_EQU_PA==RME_TRUE)
    /* Check if the virtual address mapping is correct */
    Parent_Map_Addr=(Pos<<RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order))+
                    RME_PGTBL_START(Pgtbl_Parent->Base_Addr);
    if(Pgtbl_Child->Base_Addr<Parent_Map_Addr)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    Parend_End_Addr=Parent_Map_Addr+RME_POW2(RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order));
    
    /* If this is zero, then we are sure that overflow won't happen because start
     * address is always aligned to the total order of the child page table */
    if(Parend_End_Addr!=0)
    {
        if((Pgtbl_Child->Base_Addr+RME_POW2(Child_Size_Ord))>Parend_End_Addr)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PGT_ADDR;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#endif

    /* Actually do the mapping - This work is passed down to the driver layer. 
     * Successful or not will be determined by the driver layer. */
    if(__RME_Pgtbl_Pgdir_Map(Pgtbl_Parent, Pos, Pgtbl_Child, Flags_Child)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Pgtbl_Boot_Con ******************************************/

/* Begin Function:_RME_Pgtbl_Boot_Add *****************************************
Description : This function is used to initialize the initial user memory mappings.
              This function is exclusively used to set up the Init process's memory
              mappings in the booting process. After the system boots, it is no longer
              possible to fabricate pages like this.
              Additionally, this function will set the cap to page table's property
              as unremovable. This means that it is not allowed to remove any pages
              in the directory. Additionally, it will set the reference count of the
              capability as 1, thus making the capability to the initial page table
              virtually undeletable.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl - The capability to the page table. 2-Level.
              rme_ptr_t Paddr - The physical address to map from.
              rme_ptr_t Pos - The virtual address position to map to. This position is
                              a index in the user memory.
              rme_ptr_t Flags - The flags for the user page.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Boot_Add(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Pgtbl, 
                              rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags)
{
    struct RME_Cap_Pgtbl* Pgtbl_Op;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen, but don't check their properties */
    RME_CAP_CHECK(Pgtbl_Op,0);

#if(RME_VA_EQU_PA==RME_TRUE)
    /* Check if we force identical mapping */
    if(Paddr!=((Pos<<RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order))+RME_PGTBL_START(Pgtbl_Op->Base_Addr)))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#endif

    /* See if the mapping range and the granularity is allowed */
    if(((Pos>>RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order))!=0)||
       ((Paddr&RME_MASK_END(RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)-1))!=0))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Actually do the mapping - This work is passed down to the driver layer. 
     * Successful or not will be determined by the driver layer. */
    if(__RME_Pgtbl_Page_Map(Pgtbl_Op, Paddr, Pos, Flags)!=0)
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Pgtbl_Boot_Add ******************************************/

/* Begin Function:_RME_Pgtbl_Crt **********************************************
Description : Create a layer of page table, and put that capability into a designated
              capability table. The function will check if the memory region is large
              enough, and how it has been typed; if the function found out that it is
              impossible to create a page table due to some error, it will return an
              error.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the captbl that may contain the cap
                                     to new captbl. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability slot that you want this newly created
                                    page table capability to be in. 1-Level.
              rme_ptr_t Raddr - The relative virtual address to store the page table kernel object.
              rme_ptr_t Base_Addr - The virtual address to start mapping for this page table.  
                                    This address must be aligned to the total size of the table.
              rme_ptr_t Top_Flag - Whether this page table is the top-level. If it is, we will
                                   map all the kernel page directories into this one.
              rme_ptr_t Size_Order - The size order of the page table. The size refers to
                                     the size of each page in the page directory.
              rme_ptr_t Num_Order - The number order of entries in the page table.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                         rme_cid_t Cap_Kmem, rme_cid_t Cap_Pgtbl, rme_ptr_t Raddr,
                         rme_ptr_t Base_Addr, rme_ptr_t Top_Flag, rme_ptr_t Size_Order, rme_ptr_t Num_Order)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Pgtbl* Pgtbl_Crt;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;
    
    /* Check if the total representable memory exceeds our maximum possible
     * addressible memory under the machine word length */
    if((Size_Order+Num_Order)>RME_POW2(RME_WORD_ORDER))
    {
        RME_COVERAGE_MARKER();
        
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    /* See if the creation is valid for this kmem range */
    if(Top_Flag!=0)
    {
        RME_COVERAGE_MARKER();

        RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_PGTBL,Raddr,Vaddr,RME_PGTBL_SIZE_TOP(Num_Order));
    }
    else
    {
        RME_COVERAGE_MARKER();

        RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_PGTBL,Raddr,Vaddr,RME_PGTBL_SIZE_NOM(Num_Order));
    }

    /* Check if these parameters are feasible */
    if(__RME_Pgtbl_Check(Base_Addr, Top_Flag, Size_Order, Num_Order, Vaddr)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Check if the start address is properly aligned to the total order of the page table */
    if((Base_Addr&RME_MASK_END(Size_Order+Num_Order-1))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Pgtbl,struct RME_Cap_Pgtbl*,Pgtbl_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Pgtbl_Crt,Type_Ref);

    /* Try to populate the area - Are we creating the top level? */
    if(Top_Flag!=0)
    {
        RME_COVERAGE_MARKER();

        if(_RME_Kotbl_Mark(Vaddr, RME_PGTBL_SIZE_TOP(Num_Order))!=0)
        {
            RME_COVERAGE_MARKER();

            RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
            return RME_ERR_CAP_KOTBL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();

        if(_RME_Kotbl_Mark(Vaddr, RME_PGTBL_SIZE_NOM(Num_Order))!=0)
        {
            RME_COVERAGE_MARKER();

            RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
            return RME_ERR_CAP_KOTBL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    
    Pgtbl_Crt->Head.Parent=0;
    Pgtbl_Crt->Head.Object=Vaddr;
    Pgtbl_Crt->Head.Flags=RME_PGTBL_FLAG_FULL_RANGE|
                          RME_PGTBL_FLAG_ADD_SRC|RME_PGTBL_FLAG_ADD_DST|RME_PGTBL_FLAG_REM|
                          RME_PGTBL_FLAG_CON_CHILD|RME_PGTBL_FLAG_CON_PARENT|RME_PGTBL_FLAG_DES|
                          RME_PGTBL_FLAG_PROC_CRT|RME_PGTBL_FLAG_PROC_PGT;
    Pgtbl_Crt->Base_Addr=Base_Addr|Top_Flag;
    /* These two variables are directly placed here. Checks will be done by the driver */
    Pgtbl_Crt->Size_Num_Order=RME_PGTBL_ORDER(Size_Order,Num_Order);
    /* We start initialization of the page table, and we also add all kernel pages
     * to them if they are top-level. If unsuccessful, we revert operations. */
    if(__RME_Pgtbl_Init(Pgtbl_Crt)!=0)
    {
        RME_COVERAGE_MARKER();

        /* This must be successful */
        if(Top_Flag!=0)
        {
            RME_COVERAGE_MARKER();

            RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PGTBL_SIZE_TOP(Num_Order))==0);
        }
        else
        {
            RME_COVERAGE_MARKER();

            RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PGTBL_SIZE_NOM(Num_Order))==0);
        }
        
        /* Unsuccessful. Revert operations */
        RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),0);
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Creation complete */
    RME_WRITE_RELEASE(&(Pgtbl_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_PGTBL,0));
    return 0;
}
/* End Function:_RME_Pgtbl_Crt ***********************************************/

/* Begin Function:_RME_Pgtbl_Del **********************************************
Description : Delete a layer of page table. We do not care if the childs are all
              deleted. For MPU based environments, it is required that all the 
              mapped child page tables are deconstructed from the master table
              before we can destroy the master table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the captbl that may contain the cap
                                     to new captbl. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability slot that you want this newly created
                                    page table capability to be in. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Pgtbl* Pgtbl_Del;
    rme_ptr_t Type_Ref;
    /* These are used for deletion */
    rme_ptr_t Object;
    rme_ptr_t Size;
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Pgtbl,struct RME_Cap_Pgtbl*,Pgtbl_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Pgtbl_Del,Type_Ref,RME_CAP_PGTBL);
    
    /* Hardware related deletion check - can we delete this now? This work is passed
     * down to the hardware level. The consistency check should make sure that, when 
     * a page table can be directly deleted:
     * 1> It is not referenced by any higher-level page tables.
     * 2> It does not reference any lower-level page tables.
     * If the driver layer does not conform to this, the deletion of page table is
     * not guaranteed to main kernel consistency, and such consistency must be maintained
     * by the user-level. */
    if(__RME_Pgtbl_Del_Check(Pgtbl_Del)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Pgtbl_Del,Type_Ref);
        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Remember these two variables for deletion */
    Object=RME_CAP_GETOBJ(Pgtbl_Del,rme_ptr_t);
    if(((Pgtbl_Del->Base_Addr)&RME_PGTBL_TOP)!=0)
    {
        RME_COVERAGE_MARKER();

        Size=RME_PGTBL_SIZE_TOP(RME_PGTBL_NUMORD(Pgtbl_Del->Size_Num_Order));
    }
    else
    {
        RME_COVERAGE_MARKER();

        Size=RME_PGTBL_SIZE_NOM(RME_PGTBL_NUMORD(Pgtbl_Del->Size_Num_Order));
    }
    
    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Pgtbl_Del,Type_Ref);
    /* Try to erase the area - This must be successful */
    RME_ASSERT(_RME_Kotbl_Erase(Object, Size));
    
    return 0;
}
/* End Function:_RME_Pgtbl_Del ***********************************************/

/* Begin Function:_RME_Pgtbl_Add **********************************************
Description : Delegate a page from one page table to another. This is the only way
              to add pages to new page tables after the system boots.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl_Dst - The capability to the destination page directory. 2-Level.
              rme_ptr_t Pos_Dst - The position to delegate to in the destination page directory.
              rme_ptr_t Flags_Dst - The page access permission for the destination page. This is
                                    not to be confused with the flags for the capabilities for
                                    page tables!
              rme_cid_t Cap_Pgtbl_Src - The capability to the source page directory. 2-Level.
              rme_ptr_t Pos_Dst - The position to delegate from in the source page directory.
              rme_ptr_t Index - The index of the physical address frame to delegate.
                                For example, if the destination directory's page size is 1/4
                                of that of the source directory, index=0 will delegate the first
                                1/4, index=1 will delegate the second 1/4, index=2 will delegate
                                the third 1/4, and index=3 will delegate the last 1/4.
                                All other index values are illegal.
Output      : None.
Return      : rme_ret_t - If the unmapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Add(struct RME_Cap_Captbl* Captbl, 
                         rme_cid_t Cap_Pgtbl_Dst, rme_ptr_t Pos_Dst, rme_ptr_t Flags_Dst,
                         rme_cid_t Cap_Pgtbl_Src, rme_ptr_t Pos_Src, rme_ptr_t Index)
{
    struct RME_Cap_Pgtbl* Pgtbl_Src;
    struct RME_Cap_Pgtbl* Pgtbl_Dst;
    rme_ptr_t Paddr_Dst;
    rme_ptr_t Paddr_Src;
    rme_ptr_t Flags_Src;
    rme_ptr_t Type_Ref;
    rme_ptr_t Src_Page_Size;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Dst,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Dst,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Src,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Src,Type_Ref);
    /* Check if both page table caps are not frozen and allows such operations */
    RME_CAP_CHECK(Pgtbl_Dst, RME_PGTBL_FLAG_ADD_DST);
    RME_CAP_CHECK(Pgtbl_Src, RME_PGTBL_FLAG_ADD_SRC);
    /* Check the operation range - This is page table specific */
    if((Pos_Dst>RME_PGTBL_FLAG_HIGH(Pgtbl_Dst->Head.Flags))||
       (Pos_Dst<RME_PGTBL_FLAG_LOW(Pgtbl_Dst->Head.Flags))||
       (Pos_Src>RME_PGTBL_FLAG_HIGH(Pgtbl_Src->Head.Flags))||
       (Pos_Src<RME_PGTBL_FLAG_LOW(Pgtbl_Src->Head.Flags)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_CAP_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
            
    /* See if the size order relationship is correct */
    if(RME_PGTBL_SIZEORD(Pgtbl_Dst->Size_Num_Order)>RME_PGTBL_SIZEORD(Pgtbl_Src->Size_Num_Order))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the position indices are out of range */
    if(((Pos_Dst>>RME_PGTBL_NUMORD(Pgtbl_Dst->Size_Num_Order))!=0)||
       ((Pos_Src>>RME_PGTBL_NUMORD(Pgtbl_Src->Size_Num_Order))!=0))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the source subposition index is out of range */
    Src_Page_Size=RME_POW2(RME_PGTBL_SIZEORD(Pgtbl_Src->Size_Num_Order));
    if(Src_Page_Size!=0)
    {
        if(Src_Page_Size<=(Index<<RME_PGTBL_SIZEORD(Pgtbl_Dst->Size_Num_Order)))
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PGT_ADDR;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the physical address and RME standard flags of that source page */
    if(__RME_Pgtbl_Lookup(Pgtbl_Src, Pos_Src, &Paddr_Src, &Flags_Src)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_HW;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Calculate the destination physical address */
    Paddr_Dst=Paddr_Src+(Index<<RME_PGTBL_SIZEORD(Pgtbl_Dst->Size_Num_Order));
#if(RME_VA_EQU_PA==RME_TRUE)
    /* Check if we force identical mapping. No need to check granularity here */
    if(Paddr_Dst!=((Pos_Dst<<RME_PGTBL_SIZEORD(Pgtbl_Dst->Size_Num_Order))+
                   RME_PGTBL_START(Pgtbl_Dst->Base_Addr)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#endif
    /* Analyze the flags - we do not allow expansion of access permissions */
    if(((Flags_Dst)&(~Flags_Src))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_PERM;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Actually do the mapping - This work is passed down to the driver layer. 
     * Successful or not will be determined by the driver layer. Under a multi-core
     * environment, the driver layer need to determine whether two cores are modifying
     * a same page, and do corresponding CAS if such operations are to be avoided. */
    if(__RME_Pgtbl_Page_Map(Pgtbl_Dst, Paddr_Dst, Pos_Dst, Flags_Dst)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    return 0;
}
/* End Function:_RME_Pgtbl_Add ***********************************************/

/* Begin Function:_RME_Pgtbl_Rem **********************************************
Description : Remove a page from the page table. We are doing unmapping of a page.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl - The capability to the page table. 2-Level.
              rme_ptr_t Pos - The virtual address position to unmap from.
Output      : None.
Return      : rme_ret_t - If the unmapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Rem(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Pgtbl, rme_ptr_t Pos)
{
    struct RME_Cap_Pgtbl* Pgtbl_Rem;
    rme_ptr_t Type_Ref;
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_CAPTBL,struct RME_Cap_Pgtbl*,Pgtbl_Rem,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Pgtbl_Rem,RME_PGTBL_FLAG_REM);
    /* Check the operation range - This is page table specific */
    if((Pos>RME_PGTBL_FLAG_HIGH(Pgtbl_Rem->Head.Flags))||
       (Pos<RME_PGTBL_FLAG_LOW(Pgtbl_Rem->Head.Flags)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_CAP_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* See if the unmapping range is allowed */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Rem->Size_Num_Order))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Actually do the unmapping - This work is passed down to the driver layer.
     * Successful or not will be determined by the driver layer. In the multi-core
     * environment, this should be taken care of by the driver to make sure hazard will
     * not happen by using the CAS. */
    if(__RME_Pgtbl_Page_Unmap(Pgtbl_Rem, Pos)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Pgtbl_Rem ***********************************************/

/* Begin Function:_RME_Pgtbl_Con **********************************************
Description : Map a child page table from the parent page table. Basically, we 
              are doing the construction of a page table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl_Parent - The capability to the parent page table. 2-Level.
              rme_ptr_t Pos - The virtual address to position map the child page table to.
              rme_cid_t Cap_Pgtbl_Child - The capability to the child page table. 2-Level.
              rme_ptr_t Flags_Child - The flags for the child page table mapping. This restricts
                                      the access permissions of all the memory under this mapping.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Con(struct RME_Cap_Captbl* Captbl,
                         rme_cid_t Cap_Pgtbl_Parent, rme_ptr_t Pos,
                         rme_cid_t Cap_Pgtbl_Child, rme_ptr_t Flags_Child)
{
    struct RME_Cap_Pgtbl* Pgtbl_Parent;
    struct RME_Cap_Pgtbl* Pgtbl_Child;
    /* The total size order of the child table */
    rme_ptr_t Child_Size_Ord;
#if(RME_VA_EQU_PA==RME_TRUE)
    /* The start and end mapping address in the parent */
    rme_ptr_t Parent_Map_Addr;
    rme_ptr_t Parend_End_Addr;
#endif
    rme_ptr_t Type_Ref;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Parent,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Parent,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl_Child,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Child,Type_Ref);
    /* Check if both page table caps are not frozen and allows such operations */
    RME_CAP_CHECK(Pgtbl_Parent, RME_PGTBL_FLAG_CON_PARENT);
    RME_CAP_CHECK(Pgtbl_Child, RME_PGTBL_FLAG_CON_CHILD);
    /* Check the operation range - This is page table specific */
    if((Pos>RME_PGTBL_FLAG_HIGH(Pgtbl_Parent->Head.Flags))||
       (Pos<RME_PGTBL_FLAG_LOW(Pgtbl_Parent->Head.Flags)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_CAP_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the mapping range is allowed */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Parent->Size_Num_Order))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the child table falls within one slot of the parent table */
    Child_Size_Ord=RME_PGTBL_NUMORD(Pgtbl_Child->Size_Num_Order)+
                   RME_PGTBL_SIZEORD(Pgtbl_Child->Size_Num_Order);

#if(RME_VA_EQU_PA==RME_TRUE)
    /* Path-compression option available */
    if(RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order)<Child_Size_Ord)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Check if the virtual address mapping is correct */
    Parent_Map_Addr=(Pos<<RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order))+
                    RME_PGTBL_START(Pgtbl_Parent->Base_Addr);
    if(Pgtbl_Child->Base_Addr<Parent_Map_Addr)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    Parend_End_Addr=Parent_Map_Addr+RME_POW2(RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order));
    
    /* If this is zero, then we are sure that overflow won't happen because start
     * address is always aligned to the total order of the child page table */
    if(Parend_End_Addr!=0)
    {
        if((Pgtbl_Child->Base_Addr+RME_POW2(Child_Size_Ord))>Parend_End_Addr)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PGT_ADDR;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#else
    /* If this is the case, then we force no path compression */
    if(RME_PGTBL_SIZEORD(Pgtbl_Parent->Size_Num_Order)!=Child_Size_Ord)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
#endif
    /* Actually do the mapping - This work is passed down to the driver layer. 
     * Successful or not will be determined by the driver layer. */
    if(__RME_Pgtbl_Pgdir_Map(Pgtbl_Parent, Pos, Pgtbl_Child, Flags_Child)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Pgtbl_Con ***********************************************/

/* Begin Function:_RME_Pgtbl_Des **********************************************
Description : Unmap a child page table from the parent page table. Basically, we 
              are doing the destruction of a page table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Pgtbl - The capability to the page table. 2-Level.
              rme_ptr_t Pos - The virtual address to position unmap the child page
                              table from.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Pgtbl_Des(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Pgtbl, rme_ptr_t Pos)
{
    struct RME_Cap_Pgtbl* Pgtbl_Des;
    rme_ptr_t Type_Ref;
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_CAPTBL,struct RME_Cap_Pgtbl*,Pgtbl_Des,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Pgtbl_Des,RME_PGTBL_FLAG_DES);
    /* Check the operation range - This is page table specific */
    if((Pos>RME_PGTBL_FLAG_HIGH(Pgtbl_Des->Head.Flags))||
       (Pos<RME_PGTBL_FLAG_LOW(Pgtbl_Des->Head.Flags)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_CAP_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* See if the unmapping range is allowed */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Des->Size_Num_Order))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_ADDR;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Actually do the unmapping - This work is passed down to the driver layer.
     * Successful or not will be determined by the driver layer. */
    if(__RME_Pgtbl_Pgdir_Unmap(Pgtbl_Des, Pos)!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PGT_MAP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Pgtbl_Des ***********************************************/

/* Begin Function:_RME_Kotbl_Init *********************************************
Description : Initialize the kernel object table according to the size of the table.
Input       : rme_ptr_t Words - the number of words in the table.
Output      : None.
Return      : rme_ret_t - If the number of words are is not sufficient to hold all
                          kernel memory, -1; else 0.
******************************************************************************/
rme_ret_t _RME_Kotbl_Init(rme_ptr_t Words)
{
    rme_ptr_t Count;
    
    if(Words<RME_KOTBL_WORD_NUM)
    {
        RME_COVERAGE_MARKER();

        return -1;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Avoid compiler warning about unused variable */
    RME_Kotbl[0]=0;

    /* Zero out the whole table */
    for(Count=0;Count<Words;Count++)
    	RME_KOTBL[Count]=0;
    
    return 0;
}
/* End Function:_RME_Kotbl_Init **********************************************/

/* Begin Function:_RME_Kotbl_Mark *********************************************
Description : Populate the kernel object bitmap contiguously.
Input       : rme_ptr_t Kaddr - The kernel virtual address.
              rme_ptr_t Size - The size of the memory to populate.
Output      : None.
Return      : rme_ret_t - If the operation is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Kotbl_Mark(rme_ptr_t Kaddr, rme_ptr_t Size)
{
    rme_ptr_t Count;
    /* The old value */
    rme_ptr_t Old_Val;
    /* Whether we need to undo our operations because of CAS failure */
    rme_ptr_t Undo;
    /* The actual word to start the marking */
    rme_ptr_t Start;
    /* The actual word to end the marking */
    rme_ptr_t End;
    /* The mask at the start word */
    rme_ptr_t Start_Mask;
    /* The mask at the end word */
    rme_ptr_t End_Mask;

    /* Check if the marking is well aligned */
    if((Kaddr&RME_MASK_END(RME_KMEM_SLOT_ORDER-1))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_KOT_BMP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Check if the marking is within range - unnecessary due to the kmem cap range limits */
    /* if((Kaddr<RME_KMEM_VA_START)||((Kaddr+Size)>(RME_KMEM_VA_START+RME_KMEM_SIZE)))
        return RME_ERR_KOT_BMP; */
    
    /* Round the marking to RME_KMEM_SLOT_ORDER boundary, and rely on compiler for optimization */
    Start=(Kaddr-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER;
    Start_Mask=RME_MASK_START(Start&RME_MASK_END(RME_WORD_ORDER-1));
    Start=Start>>RME_WORD_ORDER;
    
    End=(Kaddr+Size-1-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER;
    End_Mask=RME_MASK_END(End&RME_MASK_END(RME_WORD_ORDER-1));
    End=End>>RME_WORD_ORDER;
    
    /* See if the start and end are in the same word */
    if(Start==End)
    {
        RME_COVERAGE_MARKER();
        
        /* Someone already populated something here */
        Old_Val=RME_KOTBL[Start];
        if((Old_Val&(Start_Mask&End_Mask))!=0)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Check done, do the marking with CAS */
        if(RME_COMP_SWAP(&RME_KOTBL[Start],Old_Val,Old_Val|(Start_Mask&End_Mask))==0)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        Undo=0;
        /* Check&Mark the start */
        Old_Val=RME_KOTBL[Start];
        if((Old_Val&Start_Mask)!=0)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        if(RME_COMP_SWAP(&RME_KOTBL[Start],Old_Val,Old_Val|Start_Mask)==0)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Check&Mark the middle */
        for(Count=Start+1;Count<End;Count++)
        {
            Old_Val=RME_KOTBL[Count];
            if(Old_Val!=0)
            {
                RME_COVERAGE_MARKER();

                Undo=1;
                break;
            }
            else
            {
                RME_COVERAGE_MARKER();
                
                if(RME_COMP_SWAP(&RME_KOTBL[Count],Old_Val,RME_ALLBITS)==0)
                {
                    RME_COVERAGE_MARKER();
                    
                    Undo=1;
                    break;
                }
                else
                {
                    RME_COVERAGE_MARKER();
                }
            }
        }
        
        /* See if the middle part failed. If yes, we skip the end marking */
        if(Undo==0)
        {
            RME_COVERAGE_MARKER();

            /* Check&Mark the end */
            Old_Val=RME_KOTBL[End];
            if((Old_Val&End_Mask)!=0)
            {
                RME_COVERAGE_MARKER();

                Undo=1;
            }
            else
            {
                RME_COVERAGE_MARKER();

                if(RME_COMP_SWAP(&RME_KOTBL[End],Old_Val,Old_Val|End_Mask)==0)
                {
                    RME_COVERAGE_MARKER();

                    Undo=1;
                }
                else
                {
                    RME_COVERAGE_MARKER();
                }
            }
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* See if we need to undo. If yes, proceed to unroll and return error */
        if(Undo!=0)
        {
            RME_COVERAGE_MARKER();

            /* Undo the middle part - we do not need CAS here, because write back is always atomic */
            for(Count--;Count>Start;Count--)
                RME_KOTBL[Count]=0;
            /* Undo the first word - need atomic instructions */
            RME_FETCH_AND(&(RME_KOTBL[Start]),~Start_Mask);
            /* Return failure */
            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    
    return 0;
}
/* End Function:_RME_Kotbl_Mark **********************************************/

/* Begin Function:_RME_Kotbl_Erase ********************************************
Description : Depopulate the kernel object bitmap contiguously. We do not need 
              CAS on erasure operations.
Input       : rme_ptr_t Kaddr - The kernel virtual address.
              rme_ptr_t Size - The size of the memory to depopulate.
Output      : None.
Return      : rme_ret_t - If the operation is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Kotbl_Erase(rme_ptr_t Kaddr, rme_ptr_t Size)
{
    /* The actual word to start the marking */
    rme_ptr_t Start;
    /* The actual word to end the marking */
    rme_ptr_t End;
    /* The mask at the start word */
    rme_ptr_t Start_Mask;
    /* The mask at the end word */
    rme_ptr_t End_Mask;
    rme_ptr_t Count;

    /* Check if the marking is well aligned */
    if((Kaddr&RME_MASK_END(RME_KMEM_SLOT_ORDER-1))!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_KOT_BMP;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Check if the marking is within range - unnecessary due to the kmem cap range limits */
    /* if((Kaddr<RME_KMEM_VA_START)||((Kaddr+Size)>(RME_KMEM_VA_START+RME_KMEM_SIZE)))
        return RME_ERR_KOT_BMP; */
    
    /* Round the marking to RME_KMEM_SLOT_ORDER boundary, and rely on compiler for optimization */
    Start=(Kaddr-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER;
    Start_Mask=RME_MASK_START(Start&RME_MASK_END(RME_WORD_ORDER-1));
    Start=Start>>RME_WORD_ORDER;
    
    End=(Kaddr+Size-1-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER;
    End_Mask=RME_MASK_END(End&RME_MASK_END(RME_WORD_ORDER-1));
    End=End>>RME_WORD_ORDER;
    
    /* See if the start and end are in the same word */
    if(Start==End)
    {
        RME_COVERAGE_MARKER();

        /* This address range is not fully populated */
        if((RME_KOTBL[Start]&(Start_Mask&End_Mask))!=(Start_Mask&End_Mask))
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }

        /* Check done, do the unmarking - need atomic operations */
        RME_FETCH_AND(&(RME_KOTBL[Start]),~(Start_Mask&End_Mask));
    }
    else
    {
        RME_COVERAGE_MARKER();

        /* Check the start */
        if((RME_KOTBL[Start]&Start_Mask)!=Start_Mask)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Check the middle */
        for(Count=Start+1;Count<End-1;Count++)
        {
            if(RME_KOTBL[Count]!=RME_ALLBITS)
            {
                RME_COVERAGE_MARKER();

                return RME_ERR_KOT_BMP;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
        }

        /* Check the end */
        if((RME_KOTBL[End]&End_Mask)!=End_Mask)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_KOT_BMP;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Erase the start - make it atomic */
        RME_FETCH_AND(&(RME_KOTBL[Start]),~Start_Mask);
        /* Erase the middle - do not need atomics here */
        for(Count=Start+1;Count<End-1;Count++)
            RME_KOTBL[Count]=0;
        /* Erase the end - make it atomic */
        RME_FETCH_AND(&(RME_KOTBL[End]),~End_Mask);
    }
    
    return 0;
}
/* End Function:_RME_Kotbl_Erase *********************************************/

/* Begin Function:_RME_Kmem_Boot_Crt ******************************************
Description : This function is used to create boot-time kernel memory capability.
              This kind of capability that does not have a kernel object. Kernel
              memory capabilities are the capabilities that allow you to create
              specific types of kernel objects in a specific kernel memory range.
              The initial kernel memory capability's content is supplied by the
              kernel according to the initial memory setup macros.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the captbl that may contain the cap
                                     to kernel function. 2-Level.
              rme_cid_t Cap_Kmem - The capability to the kernel memory. 1-Level.
              rme_ptr_t Start - The start address of the kernel memory, aligned to kotbl granularity.
              rme_ptr_t End - The end address of the kernel memory, aligned to kotbl granularity, then -1.
              rme_ptr_t Flags - The operation flags for this kernel memory.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Kmem_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                             rme_cid_t Cap_Kmem, rme_ptr_t Start, rme_ptr_t End, rme_ptr_t Flags)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Kmem* Kmem_Crt;
    rme_ptr_t Kmem_Start;
    rme_ptr_t Kmem_End;
    rme_ptr_t Type_Ref;
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Kmem,struct RME_Cap_Kmem*,Kmem_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Kmem_Crt,Type_Ref);
    
    /* Align addresses */
#if(RME_KMEM_SLOT_ORDER>6)
    Kmem_End=RME_ROUND_DOWN(End+1,RME_KMEM_SLOT_ORDER);
    Kmem_Start=RME_ROUND_UP(Start,RME_KMEM_SLOT_ORDER);
#else
    Kmem_End=RME_ROUND_DOWN(End+1,6);
    Kmem_Start=RME_ROUND_UP(Start,6);
#endif
    /* Must at least allow creation of something */
    RME_ASSERT(Flags!=0);

    Kmem_Crt->Head.Parent=0;
    /* The kernel memory capability does not have an object */
    Kmem_Crt->Head.Object=0;
    /* Fill in the flags, start and end */
    Kmem_Crt->Head.Flags=Flags;
    /* Extra flags */
    Kmem_Crt->Start=Kmem_Start;
    Kmem_Crt->End=Kmem_End-1;

    /* Creation complete */
    RME_WRITE_RELEASE(&(Kmem_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_KMEM,1));
    return 0;
}
/* End Function:_RME_Kmem_Boot_Crt *******************************************/

/* Begin Function:_RME_CPU_Local_Init *****************************************
Description : Initialize the CPU-local data structure.
Input       : struct RME_CPU_Local* CPU_Local - The pointer to the data structure.
              rme_ptr_t CPUID - The CPUID of the CPU.
Output      : None.
Return      : None.
******************************************************************************/
void _RME_CPU_Local_Init(struct RME_CPU_Local* CPU_Local, rme_ptr_t CPUID)
{
    rme_cnt_t Prio_Cnt;
    
    CPU_Local->CPUID=CPUID;
    CPU_Local->Cur_Thd=0;
    CPU_Local->Vect_Sig=0;
    CPU_Local->Tick_Sig=0;
    
    /* Initialize the run-queue and bitmap */
    for(Prio_Cnt=0;Prio_Cnt<RME_MAX_PREEMPT_PRIO;Prio_Cnt++)
    {
        (CPU_Local->Run).Bitmap[Prio_Cnt>>RME_WORD_ORDER]=0;
        __RME_List_Crt(&((CPU_Local->Run).List[Prio_Cnt]));
    }
}
/* End Function:_RME_CPU_Local_Init ******************************************/

/* Begin Function:__RME_List_Crt **********************************************
Description : Create a doubly linked list.
Input       : volatile struct RME_List* Head - The pointer to the list head.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_List_Crt(volatile struct RME_List* Head)
{
    Head->Prev=(struct RME_List*)Head;
    Head->Next=(struct RME_List*)Head;
}
/* End Function:__RME_List_Crt ***********************************************/

/* Begin Function:__RME_List_Del **********************************************
Description : Delete a node from the doubly-linked list.
Input       : volatile struct RME_List* Prev - The prevoius node of the target node.
              volatile struct RME_List* Next - The next node of the target node.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_List_Del(volatile struct RME_List* Prev,volatile struct RME_List* Next)
{
    Next->Prev=(struct RME_List*)Prev;
    Prev->Next=(struct RME_List*)Next;
}
/* End Function:__RME_List_Del ***********************************************/

/* Begin Function:__RME_List_Ins **********************************************
Description : Insert a node to the doubly-linked list.
Input       : volatile struct RME_List* New - The new node to insert.
              volatile struct RME_List* Prev - The previous node.
              volatile struct RME_List* Next - The next node.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_List_Ins(volatile struct RME_List* New,
                    volatile struct RME_List* Prev,
                    volatile struct RME_List* Next)
{
    Next->Prev=(struct RME_List*)New;
    New->Next=(struct RME_List*)Next;
    New->Prev=(struct RME_List*)Prev;
    Prev->Next=(struct RME_List*)New;
}
/* End Function:__RME_List_Ins ***********************************************/

/* Begin Function:__RME_Thd_Fatal *********************************************
Description : The fatal fault handler of RME. This handler will be called by the 
              ISR that handles the faults. This indicates that a fatal fault
              has happened and we need to see if this thread is in a synchronous
              invocation. If yes, we stop the synchronous invocation immediately,
              and return a fault value to the old register set. If not, we just
              kill the thread. If the thread is killed, a timeout notification will
              be sent to its parent, and if we try to delegate time to it, the time
              delegation will just fail. A thread execution set is required to clear
              the fatal fault status of the thread.
              Some processors may raise some faults that are difficult to attribute to
              a particular thread, either due to the fact that they are asynchronous, or
              they are derived from exception entry. A good example is Cortex-M: its
              autostacking feature derives fault from exception entry, and some of its
              bus faults are asynchronous and can cross context boundaries. RME requires
              that all these exceptions be dropped rather than handled; or there will be
              integrity and availability compromises.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t Fault - The reason of this fault.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t __RME_Thd_Fatal(struct RME_Reg_Struct* Reg, rme_ptr_t Fault)
{
    struct RME_CPU_Local* CPU_Local;
    
    /* Attempt to return from the invocation, from fault */
    if(_RME_Inv_Ret(Reg, 0, 1)!=0)
    {
        RME_COVERAGE_MARKER();

        /* Return failure, we are not in an invocation. Killing the thread now */
        CPU_Local=RME_CPU_LOCAL();
        /* Are we attempting to kill the init threads? If yes, panic */
        RME_ASSERT((CPU_Local->Cur_Thd)->Sched.Slices!=RME_THD_INIT_TIME);
        /* Deprive it of all its timeslices */
        (CPU_Local->Cur_Thd)->Sched.Slices=0;
        /* Set the fault flag and reason of the fault */
        (CPU_Local->Cur_Thd)->Sched.State=RME_THD_FAULT;
        (CPU_Local->Cur_Thd)->Sched.Fault=Fault;
        _RME_Run_Del(CPU_Local->Cur_Thd);
        /* Send a scheduler notification to its parent */
        _RME_Run_Notif(CPU_Local->Cur_Thd);
    	/* All kernel send complete, now pick the highest priority thread to run */
    	_RME_Kern_High(Reg,CPU_Local);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
        
    return 0;
}
/* End Function:__RME_Thd_Fatal **********************************************/

/* Begin Function:_RME_Run_Ins ************************************************
Description : Insert a thread into the runqueue. In this function we do not check
              if the thread is on the current core, or is runnable, because it 
              should have been checked by someone else.
Input       : struct RME_Thd_Struct* Thd - The thread to insert.
              rme_ptr_t CPUID - The cpu to consult.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t _RME_Run_Ins(struct RME_Thd_Struct* Thd)
{
    rme_ptr_t Prio;
    struct RME_CPU_Local* CPU_Local;
    
    Prio=Thd->Sched.Prio;
    CPU_Local=Thd->Sched.CPU_Local;
    /* It can't be unbinded or there must be an error */
    RME_ASSERT(CPU_Local!=RME_THD_UNBINDED);
    
    /* Insert this thread into the runqueue */
    __RME_List_Ins(&(Thd->Sched.Run),(CPU_Local->Run).List[Prio].Prev,&((CPU_Local->Run).List[Prio]));
    /* Set the bit in the bitmap */
    (CPU_Local->Run).Bitmap[Prio>>RME_WORD_ORDER]|=RME_POW2(Prio&RME_MASK_END(RME_WORD_ORDER-1));
    
    return 0;
}
/* End Function:_RME_Run_Ins *************************************************/

/* Begin Function:_RME_Run_Del ************************************************
Description : Delete a thread from the runqueue.
Input       : struct RME_Thd_Struct* Thd - The thread to delete.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t _RME_Run_Del(struct RME_Thd_Struct* Thd)
{
    rme_ptr_t Prio;
    struct RME_CPU_Local* CPU_Local;
    
    Prio=Thd->Sched.Prio;
    CPU_Local=Thd->Sched.CPU_Local;
    /* It can't be unbinded or there must be an error */
    RME_ASSERT(CPU_Local!=RME_THD_UNBINDED);
    
    /* Delete this thread from the runqueue */
    __RME_List_Del(Thd->Sched.Run.Prev,Thd->Sched.Run.Next);
    /* __RME_List_Crt(&(Thd->Sched.Run)); */
    
    /* See if there are any thread on this priority level. If no, clear the bit */
    if((CPU_Local->Run).List[Prio].Next==&((CPU_Local->Run).List[Prio]))
    {
        RME_COVERAGE_MARKER();

        (CPU_Local->Run).Bitmap[Prio>>RME_WORD_ORDER]&=~(RME_POW2(Prio&RME_MASK_END(RME_WORD_ORDER-1)));
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Run_Del *************************************************/

/* Begin Function:_RME_Run_High ***********************************************
Description : Find the thread with the highest priority on the core.
Input       : struct RME_CPU_Local* CPU_Local - The CPU-local data structure.
Output      : None.
Return      : struct RME_Thd_Struct* - The thread returned.
******************************************************************************/
struct RME_Thd_Struct* _RME_Run_High(struct RME_CPU_Local* CPU_Local)
{
    rme_cnt_t Count;
    rme_ptr_t Prio;
    
    /* We start looking for preemption priority levels from the highest */
    for(Count=RME_PRIO_WORD_NUM-1;Count>=0;Count--)
    {
        if((CPU_Local->Run).Bitmap[Count]!=0)
        {
            RME_COVERAGE_MARKER();

            break;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    /* It must be possible to find one thread per core */
    RME_ASSERT(Count>=0);
    /* Get the first "1"'s position in the word */
    Prio=RME_MSB_GET((CPU_Local->Run).Bitmap[Count]);
    Prio+=Count<<RME_WORD_ORDER;
    /* Now there is something at this priority level. Get it and start to run */
    return (struct RME_Thd_Struct*)((CPU_Local->Run).List[Prio].Next);
}
/* End Function:_RME_Run_High ************************************************/

/* Begin Function:_RME_Run_Notif **********************************************
Description : Send a notification to the thread's parent, to notify that this 
              thread is currently out of time, or have a fault.
              This function includes kernel send, so we need to call _RME_Kern_High
              after this. The only exception being the _RME_Thd_Swt system call, in
              which we use a more optimized routine.
Input       : struct RME_Thd_Struct* Thd - The thread to send notification for.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t _RME_Run_Notif(struct RME_Thd_Struct* Thd)
{
    /* See if there is already a notification. If yes, do not do the send again */
    if(Thd->Sched.Notif.Next==&(Thd->Sched.Notif))
    {
        RME_COVERAGE_MARKER();

        __RME_List_Ins(&(Thd->Sched.Notif),
                       Thd->Sched.Parent->Sched.Event.Prev,
                       &(Thd->Sched.Parent->Sched.Event));
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* If this guy have an endpoint, send to it */
    if(Thd->Sched.Sched_Sig!=0)
    {
        RME_COVERAGE_MARKER();
    	_RME_Kern_Snd(Thd->Sched.Sched_Sig);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    return 0;
}
/* End Function:_RME_Run_Notif ***********************************************/

/* Begin Function:_RME_Run_Swt ************************************************
Description : Switch the register set and page table to another thread. 
Input       : struct RME_Reg_Struct* Reg - The current register set.
              struct RME_Thd_Struct* Curr_Thd - The current thread.
              struct RME_Thd_Struct* Next_Thd - The next thread.
Output      : None.
Return      : rme_ret_t - Always 0.
******************************************************************************/
rme_ret_t _RME_Run_Swt(struct RME_Reg_Struct* Reg,
                       struct RME_Thd_Struct* Curr_Thd, 
                       struct RME_Thd_Struct* Next_Thd)
{
    struct RME_Inv_Struct* Curr_Inv_Top;
    struct RME_Cap_Pgtbl* Curr_Pgtbl;
    struct RME_Inv_Struct* Next_Inv_Top;
    struct RME_Cap_Pgtbl* Next_Pgtbl;
    
    /* Save current context */
    __RME_Thd_Reg_Copy(&(Curr_Thd->Cur_Reg->Reg), Reg);
    __RME_Thd_Cop_Save(Reg, &(Curr_Thd->Cur_Reg->Cop_Reg));
    /* Restore next context */
    __RME_Thd_Reg_Copy(Reg, &(Next_Thd->Cur_Reg->Reg));
    __RME_Thd_Cop_Restore(Reg, &(Next_Thd->Cur_Reg->Cop_Reg));

    /* Are we going to switch page tables? If yes, we change it now */
    Curr_Inv_Top=RME_INVSTK_TOP(Curr_Thd);
    Next_Inv_Top=RME_INVSTK_TOP(Next_Thd);
    
    if(Curr_Inv_Top==0)
    {
        RME_COVERAGE_MARKER();

        Curr_Pgtbl=Curr_Thd->Sched.Proc->Pgtbl;
    }
    else
    {
        RME_COVERAGE_MARKER();

        Curr_Pgtbl=Curr_Inv_Top->Proc->Pgtbl;
    }
    
    if(Next_Inv_Top==0)
    {
        RME_COVERAGE_MARKER();

        Next_Pgtbl=Next_Thd->Sched.Proc->Pgtbl;
    }
    else
    {
        RME_COVERAGE_MARKER();

        Next_Pgtbl=Next_Inv_Top->Proc->Pgtbl;
    }
    
    if(RME_CAP_GETOBJ(Curr_Pgtbl,rme_ptr_t)!=RME_CAP_GETOBJ(Next_Pgtbl,rme_ptr_t))
    {
        RME_COVERAGE_MARKER();

        __RME_Pgtbl_Set(RME_CAP_GETOBJ(Next_Pgtbl,rme_ptr_t));
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Run_Swt *************************************************/

/* Begin Function:_RME_Proc_Boot_Crt ******************************************
Description : Create a process. A process is in fact a protection domain associated
              with a set of capabilities.
              This function does not require a kernel memory capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Crt - The capability to the capability table to use
                                         for this process. 2-Level.
              rme_cid_t Cap_Proc - The capability slot that you want this newly created
                                   process capability to be in. 1-Level.
              rme_cid_t Cap_Captbl - The capability to the capability table to use for
                                     this process. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability to the page table to use for this process.
                                    2-Level.
              rme_ptr_t Vaddr - The virtual address to store the process kernel object.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Proc_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt,
                             rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl, rme_ptr_t Vaddr)
{
    struct RME_Cap_Captbl* Captbl_Crt;
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Pgtbl* Pgtbl_Op;
    struct RME_Cap_Proc* Proc_Crt;
    struct RME_Proc_Struct* Proc_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Crt,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Crt,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Op,Type_Ref);
    /* Check if the captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Crt,RME_CAPTBL_FLAG_CRT);
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_PROC_CRT);
    RME_CAP_CHECK(Pgtbl_Op,RME_PGTBL_FLAG_PROC_CRT);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Crt,Cap_Proc,struct RME_Cap_Proc*,Proc_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Proc_Crt,Type_Ref);
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_PROC_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    Proc_Crt->Head.Parent=0;
    Proc_Crt->Head.Object=Vaddr;
    /* Does not allow changing page tables and capability tables for it */
    Proc_Crt->Head.Flags=RME_PROC_FLAG_INV|RME_PROC_FLAG_THD;
    Proc_Struct=((struct RME_Proc_Struct*)Vaddr);
    /* Reference it to make the process undeletable */
    Proc_Struct->Refcnt=1;
    
    /* Set the capability table, reference it and check for overflow */
    Proc_Struct->Captbl=Captbl_Op;
    Type_Ref=RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), -1);
        RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PROC_SIZE)==0);
        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Set the page table, reference it and check for overflow */
    Proc_Struct->Pgtbl=Pgtbl_Op;
    Type_Ref=RME_FETCH_ADD(&(Pgtbl_Op->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), -1);
        RME_FETCH_ADD(&(Pgtbl_Op->Head.Type_Ref), -1);
        RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PROC_SIZE)==0);
        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_PROC,0));
    return 0;
}
/* End Function:_RME_Proc_Boot_Crt *******************************************/

/* Begin Function:_RME_Proc_Crt ***********************************************
Description : Create a process. A process is in fact a protection domain associated
              with a set of capabilities.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl_Crt - The capability to the capability table to place
                                         this process capability in. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Proc - The capability slot that you want this newly created
                                   process capability to be in. 1-Level.
              rme_cid_t Cap_Captbl - The capability to the capability table to use for
                                     this process. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability to the page table to use for this process.
                                    2-Level.
              rme_ptr_t Raddr - The relative virtual address to store the process kernel object.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Proc_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl_Crt, rme_cid_t Cap_Kmem,
                        rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl, rme_cid_t Cap_Pgtbl, rme_ptr_t Raddr)
{
    struct RME_Cap_Captbl* Captbl_Crt;
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Pgtbl* Pgtbl_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Proc* Proc_Crt;
    struct RME_Proc_Struct* Proc_Struct;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl_Crt,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Crt,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Crt,RME_CAPTBL_FLAG_CRT);
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_PROC_CRT);
    RME_CAP_CHECK(Pgtbl_Op,RME_PGTBL_FLAG_PROC_CRT);
    /* See if the creation is valid for this kmem range */
    RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_PROC,Raddr,Vaddr,RME_PROC_SIZE);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Crt,Cap_Proc,struct RME_Cap_Proc*,Proc_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Proc_Crt,Type_Ref);
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_PROC_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    Proc_Crt->Head.Parent=0;
    Proc_Crt->Head.Object=Vaddr;
    Proc_Crt->Head.Flags=RME_PROC_FLAG_INV|RME_PROC_FLAG_THD|
                         RME_PROC_FLAG_CPT|RME_PROC_FLAG_PGT;
    Proc_Struct=((struct RME_Proc_Struct*)Vaddr);
    
    /* Set the capability table, reference it and check for overflow */
    Proc_Struct->Captbl=Captbl_Op;
    Proc_Struct->Refcnt=0;
    Type_Ref=RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), -1);
        RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PROC_SIZE)==0);
        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Set the page table, reference it and check for overflow */
    Proc_Struct->Pgtbl=Pgtbl_Op;
    Type_Ref=RME_FETCH_ADD(&(Pgtbl_Op->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_Op->Head.Type_Ref), -1);
        RME_FETCH_ADD(&(Pgtbl_Op->Head.Type_Ref), -1);
        RME_ASSERT(_RME_Kotbl_Erase(Vaddr, RME_PROC_SIZE)==0);
        RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Proc_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_PROC,0));
    return 0;
}
/* End Function:_RME_Proc_Crt ************************************************/

/* Begin Function:_RME_Proc_Del ***********************************************
Description : Delete a process.
Input       : struct RME_Cap_Captbl* Captbl - The capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table. 2-Level.
              rme_cid_t Cap_Proc - The capability to the process. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Proc_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Proc)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Proc* Proc_Del;
    rme_ptr_t Type_Ref;

    /* Used for deletion */
    struct RME_Proc_Struct* Object;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Proc,struct RME_Cap_Proc*,Proc_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Proc_Del,Type_Ref,RME_CAP_PROC);
    
    /* Remember the object location for deletion */
    Object=RME_CAP_GETOBJ(Proc_Del,struct RME_Proc_Struct*);
    
    /* See if the object is referenced by another thread or invocation kernel
     * object. If yes, cannot delete */
    if(Object->Refcnt!=0)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Proc_Del,Type_Ref);
        return RME_ERR_PTH_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Proc_Del,Type_Ref);
    
    /* Decrease the refcnt for the two caps */
    RME_FETCH_ADD(&(Object->Captbl->Head.Type_Ref), -1);
    RME_FETCH_ADD(&(Object->Pgtbl->Head.Type_Ref), -1);
        
    /* Try to depopulate the area - this must be successful */
    RME_ASSERT(_RME_Kotbl_Erase((rme_ptr_t)Object, RME_PROC_SIZE)!=0);
    
    return 0;
}
/* End Function:_RME_Proc_Del ************************************************/

/* Begin Function:_RME_Proc_Cpt ***********************************************
Description : Change a process's capability table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Proc - The capability to the process that have been created
                                   already. 2-Level.
              rme_cid_t Cap_Captbl - The capability to the capability table to use for
                                     this process. 2-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Proc_Cpt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Proc, rme_cid_t Cap_Captbl)
{
    struct RME_Cap_Proc* Proc_Op;
    struct RME_Cap_Captbl* Captbl_New;
    struct RME_Cap_Captbl* Captbl_Old;
    struct RME_Proc_Struct* Proc_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Proc,RME_CAP_PROC,struct RME_Cap_Proc*,Proc_Op,Type_Ref); 
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_New,Type_Ref);     
    /* Check if the target caps is not frozen and allows such operations */
    RME_CAP_CHECK(Proc_Op,RME_PROC_FLAG_CPT);
    RME_CAP_CHECK(Captbl_New,RME_CAPTBL_FLAG_PROC_CPT);
    
    /* Increase the reference count of the new cap first - If that fails, we can revert easily */
    Type_Ref=RME_FETCH_ADD(&(Captbl_New->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_New->Head.Type_Ref), -1);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Read the old captbl, and do CAS here. If we fail, revert the refcnt */
    Proc_Struct=RME_CAP_GETOBJ(Proc_Op,struct RME_Proc_Struct*);
    Captbl_Old=Proc_Struct->Captbl;
    /* Actually commit the change */
    if(RME_COMP_SWAP((rme_ptr_t*)(&(Proc_Struct->Captbl)),
                     (rme_ptr_t)Captbl_Old,
                     (rme_ptr_t)Captbl_New)==0)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Captbl_New->Head.Type_Ref), -1);
        return RME_ERR_PTH_CONFLICT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Release the old table */
    RME_FETCH_ADD(&(Captbl_Old->Head.Type_Ref), -1);
    
    return 0;
}
/* End Function:_RME_Proc_Cpt ************************************************/

/* Begin Function:_RME_Proc_Pgt ***********************************************
Description : Change a process's page table.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Proc - The capability slot that you want this newly created
                                   process capability to be in. 2-Level.
              rme_cid_t Cap_Pgtbl - The capability to the page table to use for this
                                    process. 2-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Proc_Pgt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Proc, rme_cid_t Cap_Pgtbl)
{
    struct RME_Cap_Proc* Proc_Op;
    struct RME_Cap_Pgtbl* Pgtbl_New;
    struct RME_Cap_Pgtbl* Pgtbl_Old;
    struct RME_Proc_Struct* Proc_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Proc,RME_CAP_PROC,struct RME_Cap_Proc*,Proc_Op,Type_Ref); 
    RME_CAPTBL_GETCAP(Captbl,Cap_Pgtbl,RME_CAP_PGTBL,struct RME_Cap_Pgtbl*,Pgtbl_New,Type_Ref);     
    /* Check if the target caps is not frozen and allows such operations */
    RME_CAP_CHECK(Proc_Op,RME_PROC_FLAG_PGT);
    RME_CAP_CHECK(Pgtbl_New,RME_PGTBL_FLAG_PROC_PGT);
    
    /* Increase the reference count of the new cap first - If that fails, we can revert easily */
    Type_Ref=RME_FETCH_ADD(&(Pgtbl_New->Head.Type_Ref), 1);
    if(RME_CAP_REF(Type_Ref)>=RME_CAP_MAXREF)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Pgtbl_New->Head.Type_Ref), -1);
        return RME_ERR_CAP_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Read the old captbl, and do CAS here. If we fail, revert the refcnt */
    Proc_Struct=RME_CAP_GETOBJ(Proc_Op,struct RME_Proc_Struct*);
    Pgtbl_Old=Proc_Struct->Pgtbl;
    /* Actually commit the change */
    if(RME_COMP_SWAP((rme_ptr_t*)(&(Proc_Struct->Captbl)),
                       (rme_ptr_t)Pgtbl_Old,
                       (rme_ptr_t)Pgtbl_New)==0)
    {
        RME_COVERAGE_MARKER();

        RME_FETCH_ADD(&(Pgtbl_New->Head.Type_Ref), -1);
        return RME_ERR_PTH_CONFLICT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Release the old table */
    RME_FETCH_ADD(&(Pgtbl_Old->Head.Type_Ref), -1);
    
    return 0;
}
/* End Function:_RME_Proc_Pgt ************************************************/

/* Begin Function:_RME_Thd_Boot_Crt *******************************************
Description : Create a boot-time thread. The boot-time thread is per-core, and
              will have infinite budget, and has no parent. This function
              allows creation of a thread on behalf of other processors,
              by passing a CPUID parameter. Therefore, it is recommended to
              create the threads sequentially during boot-up; if you create threads
              in parallel, be sure to only create the thread on your local core.
              This function does not require a kernel memory capability, and 
              the initial threads' maximum priority will be set by the system.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table. 2-Level.
              rme_cid_t Cap_Thd - The capability slot that you want this newly created
                                  thread capability to be in. 1-Level.
              rme_cid_t Cap_Proc - The capability to the process that it is in. 2-Level.
              rme_ptr_t Vaddr - The virtual address to store the thread kernel object.
              rme_ptr_t Prio - The priority level of the thread.
              struct RME_CPU_Local* CPU_Local - The CPU-local data structure of the CPU
                                                to bind this thread to.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                            rme_cid_t Cap_Thd, rme_cid_t Cap_Proc, rme_ptr_t Vaddr,
                            rme_ptr_t Prio, struct RME_CPU_Local* CPU_Local)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Proc* Proc_Op;
    struct RME_Cap_Thd* Thd_Crt;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Type_Ref;
    
    /* Check whether the priority level is allowed */
    if(Prio>=RME_MAX_PREEMPT_PRIO)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_PRIO;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref); 
    RME_CAPTBL_GETCAP(Captbl,Cap_Proc,RME_CAP_PROC,struct RME_Cap_Proc*,Proc_Op,Type_Ref);   
    /* Check if the target caps is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    RME_CAP_CHECK(Proc_Op,RME_PROC_FLAG_THD);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Thd,struct RME_Cap_Thd*,Thd_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Thd_Crt,Type_Ref);
     
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_THD_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Thd_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Get the thread, and start creation */
    Thd_Struct=(struct RME_Thd_Struct*)Vaddr;
    /* The TID of these threads are by default taken care of by the kernel */
    Thd_Struct->Sched.TID=0;
    /* Set this initially to 1 to make it virtually unfreeable & undeletable */
    Thd_Struct->Sched.Refcnt=1;
    Thd_Struct->Sched.Slices=RME_THD_INIT_TIME;
    Thd_Struct->Sched.State=RME_THD_RUNNING;
    Thd_Struct->Sched.Signal=0;
    Thd_Struct->Sched.Prio=Prio;
    Thd_Struct->Sched.Max_Prio=RME_MAX_PREEMPT_PRIO-1;
    Thd_Struct->Sched.Sched_Sig=0;
    /* Bind the thread to the current CPU */
    Thd_Struct->Sched.CPU_Local=CPU_Local;
    /* This is a marking that this thread haven't sent any notifications */
    __RME_List_Crt(&(Thd_Struct->Sched.Notif));
    __RME_List_Crt(&(Thd_Struct->Sched.Event));
    /* RME_List_Crt(&(Thd_Struct->Sched.Run)); */
    Thd_Struct->Sched.Proc=RME_CAP_GETOBJ(Proc_Op,struct RME_Proc_Struct*);
    /* Point its pointer to itself - this will never be a hypervisor thread */
    Thd_Struct->Cur_Reg=&(Thd_Struct->Def_Reg);
    /* Initialize the invocation stack */
    __RME_List_Crt(&(Thd_Struct->Inv_Stack));
    
    /* Increase the reference count of the process structure(Not the process capability) */
    RME_FETCH_ADD(&(RME_CAP_GETOBJ(Proc_Op, struct RME_Proc_Struct*)->Refcnt), 1);
    
    /* Set the cap's parameters according to what we have just created */
    Thd_Crt->Head.Parent=0;
    Thd_Crt->Head.Object=Vaddr;
    /* This can only be a parent, and not a child, and cannot be freed. Additionally,
     * this should not be blocked on any endpoint. Any attempt to block this thread will fail.
     * Setting execution information for this is also prohibited. */
    Thd_Crt->Head.Flags=RME_THD_FLAG_SCHED_PRIO|RME_THD_FLAG_SCHED_PARENT|
                        RME_THD_FLAG_XFER_DST|RME_THD_FLAG_XFER_SRC|
                        RME_THD_FLAG_SCHED_RCV|RME_THD_FLAG_SWT;
    Thd_Crt->TID=0;
    
    /* Insert this into the runqueue, and set current thread to it */
    _RME_Run_Ins(Thd_Struct);
    CPU_Local->Cur_Thd=Thd_Struct;
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Thd_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_THD,0));
    return 0;
}
/* End Function:_RME_Thd_Boot_Crt ********************************************/

/* Begin Function:_RME_Thd_Crt ************************************************
Description : Create a thread. A thread is the minimal kernel-aware execution unit.
              When the thread is created, there are no time associated with it.
              This can be called on any core.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Thd - The capability slot that you want this newly created
                                  thread capability to be in. 1-Level.
              rme_cid_t Cap_Proc - The capability to the process that it is in. 2-Level.
              rme_ptr_t Max_Prio - The maximum priority allowed for this thread. Once set,
                                   this cannot be changed.
              rme_ptr_t Raddr - The relative virtual address to store the thread kernel object.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Kmem,
                       rme_cid_t Cap_Thd, rme_cid_t Cap_Proc, rme_ptr_t Max_Prio, rme_ptr_t Raddr)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Proc* Proc_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Thd* Thd_Crt;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;
    
    /* See if the maximum priority relationship is correct - a thread can never create
     * a thread with higher maximum priority */
    if((RME_CPU_LOCAL()->Cur_Thd)->Sched.Max_Prio<Max_Prio)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_PRIO;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref); 
    RME_CAPTBL_GETCAP(Captbl,Cap_Proc,RME_CAP_PROC,struct RME_Cap_Proc*,Proc_Op,Type_Ref);   
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the target caps is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    RME_CAP_CHECK(Proc_Op,RME_PROC_FLAG_THD);
    /* See if the creation is valid for this kmem range */
    RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_THD,Raddr,Vaddr,RME_THD_SIZE);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Thd,struct RME_Cap_Thd*,Thd_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Thd_Crt,Type_Ref);
     
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_THD_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Thd_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Get the thread, and start creation */
    Thd_Struct=(struct RME_Thd_Struct*)Vaddr;
    /* These thread's TID default to 0 */
    Thd_Struct->Sched.TID=0;
    Thd_Struct->Sched.Refcnt=0;
    Thd_Struct->Sched.Slices=0;
    Thd_Struct->Sched.State=RME_THD_TIMEOUT;
    Thd_Struct->Sched.Signal=0;
    Thd_Struct->Sched.Max_Prio=Max_Prio;
    Thd_Struct->Sched.Sched_Sig=0;
    /* Currently the thread is not binded to any particular CPU */
    Thd_Struct->Sched.CPU_Local=RME_THD_UNBINDED;
    /* This is a marking that this thread haven't sent any notifications */
    __RME_List_Crt(&(Thd_Struct->Sched.Notif));
    __RME_List_Crt(&(Thd_Struct->Sched.Event));
    /* RME_List_Crt(&(Thd_Struct->Sched.Run)); */
    Thd_Struct->Sched.Proc=RME_CAP_GETOBJ(Proc_Op,struct RME_Proc_Struct*);
    /* Point its pointer to itself - this is not a hypervisor thread yet */
    Thd_Struct->Cur_Reg=&(Thd_Struct->Def_Reg);
    /* Initialize the invocation stack */
    __RME_List_Crt(&(Thd_Struct->Inv_Stack));
    
    /* Increase the reference count of the process structure(Not the process capability) */
    RME_FETCH_ADD(&(RME_CAP_GETOBJ(Proc_Op, struct RME_Proc_Struct*)->Refcnt), 1);
    
    /* Set the cap's parameters according to what we have just created */
    Thd_Crt->Head.Parent=0;
    Thd_Crt->Head.Object=Vaddr;
    Thd_Crt->Head.Flags=RME_THD_FLAG_EXEC_SET|RME_THD_FLAG_HYP_SET|
                        RME_THD_FLAG_SCHED_CHILD|RME_THD_FLAG_SCHED_PARENT|
                        RME_THD_FLAG_SCHED_PRIO|RME_THD_FLAG_SCHED_FREE|
                        RME_THD_FLAG_SCHED_RCV|RME_THD_FLAG_SWT|
                        RME_THD_FLAG_XFER_SRC|RME_THD_FLAG_XFER_DST;
    Thd_Crt->TID=0;
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Thd_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_THD,0));
    return 0;
}
/* End Function:_RME_Thd_Crt *************************************************/

/* Begin Function:_RME_Thd_Del ************************************************
Description : Delete a thread. This can be called on any core.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table. 
              rme_cid_t Cap_Captbl - The capability to the capability table. 2-Level.
              rme_cid_t Cap_Thd - The capability to the thread in the captbl. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Thd)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Thd* Thd_Del;
    rme_ptr_t Type_Ref;
    /* These are for deletion */
    struct RME_Thd_Struct* Thd_Struct;
    struct RME_Inv_Struct* Inv_Struct;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Thd,struct RME_Cap_Thd*,Thd_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Thd_Del,Type_Ref,RME_CAP_THD);
    
    /* Get the thread */
    Thd_Struct=RME_CAP_GETOBJ(Thd_Del,struct RME_Thd_Struct*);
    
    /* See if the thread is unbinded. If not, we cannot proceed to deletion */
    if(Thd_Struct->Sched.CPU_Local!=RME_THD_UNBINDED)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Thd_Del,Type_Ref);
        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Thd_Del,Type_Ref);
    
    /* Is the thread using any invocation? If yes, just pop the invocation
     * stack to empty, and free all the invocation stubs. This can be virtually
     * unbounded if the invocation stack is just too deep. This is left to the
     * user; if this is what he or she wants, be our guest. */
    while(Thd_Struct->Inv_Stack.Next!=&(Thd_Struct->Inv_Stack))
    {
        Inv_Struct=(struct RME_Inv_Struct*)(Thd_Struct->Inv_Stack.Next);
        __RME_List_Del(Inv_Struct->Head.Prev,Inv_Struct->Head.Next);
        Inv_Struct->Active=0;
    }
    
    /* Dereference the process */
    RME_FETCH_ADD(&(Thd_Struct->Sched.Proc->Refcnt), -1);
    
    /* Try to depopulate the area - this must be successful */
    RME_ASSERT(_RME_Kotbl_Erase((rme_ptr_t)Thd_Struct,RME_THD_SIZE)!=0);
    
    return 0;
}
/* End Function:_RME_Thd_Del *************************************************/

/* Begin Function:_RME_Thd_Exec_Set *******************************************
Description : Set a thread's entry point and stack. The registers will be initialized
              with these contents. Only when the thread has exited, or just after
              created should we change these values.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level.
              rme_ptr_t Entry - The entry of the thread. An address.
              rme_ptr_t Stack - The stack address to use for execution. An address.
              rme_ptr_t Param - The parameter to pass to the thread.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Exec_Set(struct RME_Cap_Captbl* Captbl,
                            rme_cid_t Cap_Thd, rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Param)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_EXEC_SET);
    
    /* See if the target thread is already binded. If no or incorrect, we just quit */
    Thd_Struct=RME_CAP_GETOBJ(Thd_Op,struct RME_Thd_Struct*);
    if(Thd_Struct->Sched.CPU_Local!=RME_CPU_LOCAL())
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if there is a fault pending. If yes, we clear it */
    if(Thd_Struct->Sched.State==RME_THD_FAULT)
    {
        RME_COVERAGE_MARKER();

        Thd_Struct->Sched.State=RME_THD_TIMEOUT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Commit the change if both values are non-zero. If both are zero we are just
     * clearing the error flag and continue execution from where it faulted */
    if((Entry!=0)&&(Stack!=0))
    {
        RME_COVERAGE_MARKER();

        __RME_Thd_Reg_Init(Entry, Stack, Param, &(Thd_Struct->Cur_Reg->Reg));
        __RME_Thd_Cop_Init(&(Thd_Struct->Cur_Reg->Reg), &(Thd_Struct->Cur_Reg->Cop_Reg));
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    return 0;
}
/* End Function:_RME_Thd_Exec_Set ********************************************/

/* Begin Function:_RME_Thd_Hyp_Set ********************************************
Description : Set the thread as hypervisor-managed. This means that the thread's
              register set will be saved to somewhere that is indicated by the user,
              instead of in the kernel data structures.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level.
              rme_ptr_t Kaddr - The kernel-accessible virtual address to save the register set to.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Hyp_Set(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Thd, rme_ptr_t Kaddr)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_HYP_SET);
    
    /* See if the target thread is already binded. If no or incorrect, we just quit */
    Thd_Struct=RME_CAP_GETOBJ(Thd_Op,struct RME_Thd_Struct*);
    if(Thd_Struct->Sched.CPU_Local!=RME_CPU_LOCAL())
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Set the thread's register storage back to default if the address passed in is null */
    if(Kaddr==0)
    {
        RME_COVERAGE_MARKER();

        Thd_Struct->Cur_Reg=&(Thd_Struct->Def_Reg);
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* Register external save area must be aligned to word boundary and accessible to the kernel */
        if(RME_IS_ALIGNED(Kaddr)&&(Kaddr>=RME_HYP_VA_START)&&
           ((Kaddr+sizeof(struct RME_Thd_Regs))<(RME_HYP_VA_START+RME_HYP_SIZE)))
        {
            RME_COVERAGE_MARKER();

            Thd_Struct->Cur_Reg=(struct RME_Thd_Regs*)Kaddr;
        }
        else
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PTH_PGTBL;
        }
    }
    
    return 0;
}
/* End Function:_RME_Thd_Hyp_Set *********************************************/

/* Begin Function:_RME_Thd_Sched_Bind *****************************************
Description : Set a thread's priority level, and its scheduler thread. When there
              is any state change on this thread, a notification will be sent to
              scheduler thread. If the state of the thread changes for multiple
              times, then only the most recent state will be in the scheduler's 
              receive queue.
              The scheduler and the threads that it schedule must be on the same
              core. When a thread wants to go from one core to another, its notification
              to the scheduler must all be processed, and it must have no scheduler
              notifications in itself (That is, unbinded). 
              This must be called on the same core with the Cap_Thd_Sched, and the
              Cap_Thd itself must be free.
              It is impossible to set a thread's priority beyond its maximum priority. 
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level.
              rme_cid_t Cap_Thd_Sched - The scheduler thread. 2-Level.
              rme_cid_t Cap_Sig - The signal endpoint for scheduler notifications. This signal
                                  endpoint will be sent to whenever this thread has a fault, or
                                  timeouts. This is purely optional; if it is not needed, pass
                                  in RME_CAPID_NULL which is a number smaller than zero.
              rme_tid_t TID - The thread ID. This is user-supplied, and the kernel will not
                              check whether there are two threads that have the same TID.
              rme_ptr_t Prio - The priority level, higher is more critical.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Sched_Bind(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Thd,
                              rme_cid_t Cap_Thd_Sched, rme_cid_t Cap_Sig, rme_tid_t TID, rme_ptr_t Prio)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Cap_Thd* Thd_Sched;
    struct RME_Cap_Sig* Sig_Op;
    struct RME_Thd_Struct* Thd_Op_Struct;
    struct RME_Thd_Struct* Thd_Sched_Struct;
    struct RME_Sig_Struct* Sig_Op_Struct;
    struct RME_CPU_Local* Old_CPU_Local;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd_Sched,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Sched,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_SCHED_CHILD);
    RME_CAP_CHECK(Thd_Sched,RME_THD_FLAG_SCHED_PARENT);
    
    /* See if we need the signal endpoint for this operation */
    if(Cap_Sig<RME_CAPID_NULL)
    {
        RME_COVERAGE_MARKER();

        RME_CAPTBL_GETCAP(Captbl,Cap_Sig,RME_CAP_SIG,struct RME_Cap_Sig*,Sig_Op,Type_Ref);
        RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_SCHED);
    }
    else
    {
        RME_COVERAGE_MARKER();

    	Sig_Op=0;
    }

    /* Check the TID passed in to see whether it is good */
    if((TID>=RME_THD_FAULT_FLAG)||(TID<0))
    {
        RME_COVERAGE_MARKER();

    	return RME_ERR_PTH_TID;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* See if the target thread is already binded. If yes, we just quit */
    Thd_Op_Struct=RME_CAP_GETOBJ(Thd_Op,struct RME_Thd_Struct*);
    Old_CPU_Local=Thd_Op_Struct->Sched.CPU_Local;
    if(Old_CPU_Local!=RME_THD_UNBINDED)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the parent thread is on the same core with the current processor */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Sched_Struct=RME_CAP_GETOBJ(Thd_Sched,struct RME_Thd_Struct*);
    if(Thd_Sched_Struct->Sched.CPU_Local!=CPU_Local)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* See if we are trying to bind to ourself. This is prohibited */
    if(Thd_Op_Struct==Thd_Sched_Struct)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_NOTIF;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the priority relationship is correct */
    if(Thd_Sched_Struct->Sched.Max_Prio<Prio)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_PRIO;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Yes, it is on the current processor. Try to bind the thread */
    if(RME_COMP_SWAP((rme_ptr_t*)&(Thd_Op_Struct->Sched.CPU_Local),
                     (rme_ptr_t)Old_CPU_Local,
                     (rme_ptr_t)CPU_Local)==0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_CONFLICT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Binding successful. Do operations to finish this. There's no need to worry about
     * other cores' operations on this thread because this thread is already binded
     * to this core */
    Thd_Op_Struct->Sched.Parent=Thd_Sched_Struct;
    Thd_Op_Struct->Sched.Prio=Prio;
    Thd_Op_Struct->Sched.TID=TID;

    /* Tie the signal endpoint to it if not zero */
    if(Sig_Op==0)
    {
        RME_COVERAGE_MARKER();

    	Thd_Op_Struct->Sched.Sched_Sig=0;
    }
    else
    {
        RME_COVERAGE_MARKER();

        Sig_Op_Struct=RME_CAP_GETOBJ(Sig_Op,struct RME_Sig_Struct*);
    	Thd_Op_Struct->Sched.Sched_Sig=Sig_Op_Struct;
        /* Increase the reference count of the signal endpoint(not the capability!) */
        RME_FETCH_ADD(&(Sig_Op_Struct->Refcnt), 1);
    }

    /* We can use this because it is core-local */
    Thd_Sched_Struct->Sched.Refcnt++;
    
    return 0;
}
/* End Function:_RME_Thd_Sched_Bind ******************************************/

/* Begin Function:_RME_Thd_Sched_Prio *****************************************
Description : Change a thread's priority level. This can only be called from the
              core that have the thread binded.
              This system call can cause a potential context switch.
              It is impossible to set a thread's priority beyond its maximum priority. 
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level.
              rme_ptr_t Prio - The priority level, higher is more critical.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Sched_Prio(struct RME_Cap_Captbl* Captbl,
                              struct RME_Reg_Struct* Reg,
                              rme_cid_t Cap_Thd, rme_ptr_t Prio)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_SCHED_PRIO);
    
    /* See if the target thread is already binded to this core. If no, we just quit */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Struct=(struct RME_Thd_Struct*)Thd_Op->Head.Object;
    if(Thd_Struct->Sched.CPU_Local!=CPU_Local)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the priority relationship is correct */
    if(Thd_Struct->Sched.Max_Prio<Prio)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_PRIO;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Now save the system call return value to the caller stack */
    __RME_Set_Syscall_Retval(Reg,0);
    
    /* See if this thread is currently running, or is runnable. If yes, it must be
     * in the run queue. Remove it from there and change priority, after changing
     * priority, put it back, and see if we need a reschedule. */
    if((Thd_Struct->Sched.State==RME_THD_RUNNING)||(Thd_Struct->Sched.State==RME_THD_READY))
    {
        RME_COVERAGE_MARKER();

        _RME_Run_Del(Thd_Struct);
        Thd_Struct->Sched.Prio=Prio;
        _RME_Run_Ins(Thd_Struct);
        
        /* Get the current highest-priority running thread */
        Thd_Struct=_RME_Run_High(CPU_Local);
        RME_ASSERT(Thd_Struct->Sched.Prio>=(CPU_Local->Cur_Thd)->Sched.Prio);
        
        /* See if we need a context switch */
        if(Thd_Struct->Sched.Prio>(CPU_Local->Cur_Thd)->Sched.Prio)
        {
            RME_COVERAGE_MARKER();

            /* This will cause a solid context switch - The current thread will be set
             * to ready, and we will set the thread that we switch to to be running. */
            _RME_Run_Swt(Reg,CPU_Local->Cur_Thd,Thd_Struct);
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_READY;
            Thd_Struct->Sched.State=RME_THD_RUNNING;
            CPU_Local->Cur_Thd=Thd_Struct;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();

        Thd_Struct->Sched.Prio=Prio;
    }
    
    return 0;
}
/* End Function:_RME_Thd_Sched_Prio ******************************************/

/* Begin Function:_RME_Thd_Sched_Free *****************************************
Description : Free a thread from its current binding. This function can only be
              executed from the same core on with the thread.
              This system call can cause a potential context switch.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Sched_Free(struct RME_Cap_Captbl* Captbl, 
                              struct RME_Reg_Struct* Reg, rme_cid_t Cap_Thd)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    /* These are used to free the thread */
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_SCHED_FREE);
    
    /* See if the target thread is already binded. If no or binded to other cores, we just quit */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Struct=(struct RME_Thd_Struct*)Thd_Op->Head.Object;
    if(Thd_Struct->Sched.CPU_Local!=CPU_Local)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Am I referenced by someone as a scheduler? If yes, we cannot unbind. Because
     * boot-time thread's refcnt will never be 0, thus they will never pass this checking */
    if(Thd_Struct->Sched.Refcnt!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_REFCNT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Decrease the parent's reference count */
    Thd_Struct->Sched.Parent->Sched.Refcnt--;
    
    /* See if we have any events sent to the parent. If yes, remove that event */
    if(Thd_Struct->Sched.Notif.Next!=&(Thd_Struct->Sched.Notif))
    {
        RME_COVERAGE_MARKER();

        __RME_List_Del(Thd_Struct->Sched.Notif.Prev,Thd_Struct->Sched.Notif.Next);
        __RME_List_Crt(&(Thd_Struct->Sched.Notif));
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* If we have an scheduler event endpoint, release it */
    if(Thd_Struct->Sched.Sched_Sig!=0)
    {
        RME_COVERAGE_MARKER();

    	RME_FETCH_ADD(&(Thd_Struct->Sched.Sched_Sig->Refcnt), -1);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Now save the system call return value to the caller stack */
    __RME_Set_Syscall_Retval(Reg,0);  
    
    /* If the thread is running, or ready to run, kick it out of the run queue.
     * If it is blocked on some endpoint, end the blocking and set the return
     * value to RME_ERR_SIV_FREE. If the thread is killed due to a fault, we will
     * not clear the fault here, and we will wait for the Exec_Set to clear it. */
    if(Thd_Struct->Sched.State!=RME_THD_BLOCKED)
    {
        RME_COVERAGE_MARKER();

        if((Thd_Struct->Sched.State==RME_THD_RUNNING)||(Thd_Struct->Sched.State==RME_THD_READY))
        {
            RME_COVERAGE_MARKER();

            _RME_Run_Del(Thd_Struct);
            Thd_Struct->Sched.State=RME_THD_TIMEOUT;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* If it got here, the thread that is operated on cannot be the current thread, so
         * we are not overwriting the return value of the caller thread */
        __RME_Set_Syscall_Retval(&(Thd_Struct->Cur_Reg->Reg),RME_ERR_SIV_FREE);
        Thd_Struct->Sched.Signal->Thd=0;
        Thd_Struct->Sched.Signal=0;
        Thd_Struct->Sched.State=RME_THD_TIMEOUT;
    }
    /* Delete all slices on it */
    Thd_Struct->Sched.Slices=0;
    
    /* See if this thread is the current thread. If yes, then there will be a context switch */
    if(CPU_Local->Cur_Thd==Thd_Struct)
    {
        RME_COVERAGE_MARKER();

        CPU_Local->Cur_Thd=_RME_Run_High(CPU_Local);
        _RME_Run_Ins(CPU_Local->Cur_Thd);
        (CPU_Local->Cur_Thd)->Sched.State=RME_THD_RUNNING;
        _RME_Run_Swt(Reg,Thd_Struct,CPU_Local->Cur_Thd);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Set the state to unbinded so other cores can bind */
    RME_WRITE_RELEASE((rme_ptr_t*)&(Thd_Struct->Sched.CPU_Local),(rme_ptr_t)RME_THD_UNBINDED);
    return 0;
}
/* End Function:_RME_Thd_Sched_Free ******************************************/

/* Begin Function:_RME_Thd_Sched_Rcv ******************************************
Description : Try to receive a notification from the scheduler queue. This
              can only be called from the same core the thread is on.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The current thread's register set.
              rme_cid_t Cap_Thd - The capability to the scheduler thread. We are going
                                  to get timeout or fault notifications for the threads
                                  that it is responsible for scheduling. This capability
                                  must point to a thread on the same core. 2-Level.
Output      : struct RME_Reg_Struct* Reg - The current thread's register set, 
                                           with the architecture-specific reason of the
                                           fault populated to the invocation return value
                                           position of it.
Return      : rme_ret_t - If successful, the thread ID; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Sched_Rcv(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg, rme_cid_t Cap_Thd)
{
    struct RME_Cap_Thd* Thd_Op;
    struct RME_Thd_Struct* Thd_Struct;
    struct RME_Thd_Struct* Thd_Child;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Op,RME_THD_FLAG_SCHED_RCV);
    
    /* Check if the CPUID is correct. Only if yes can we proceed */
    Thd_Struct=(struct RME_Thd_Struct*)Thd_Op->Head.Object;
    if(Thd_Struct->Sched.CPU_Local!=RME_CPU_LOCAL())
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Are there any notifications? */
    if(Thd_Struct->Sched.Event.Next==&(Thd_Struct->Sched.Event))
    {
        RME_COVERAGE_MARKER();

        /* Check the blocking flag to see whether we need to block the thread */
        return RME_ERR_PTH_NOTIF;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Return one notification and delete it from the notification list */
    Thd_Child=(struct RME_Thd_Struct*)(Thd_Struct->Sched.Event.Next-1);
    __RME_List_Del(Thd_Child->Sched.Notif.Prev,Thd_Child->Sched.Notif.Next);
    /* We need to do this because we are using this to detect whether the notification is sent */
    __RME_List_Crt(&(Thd_Child->Sched.Notif));
    
    /* See if the child is in a faulty state. If yes, we return a fault notification with that TID */
    if(Thd_Child->Sched.State==RME_THD_FAULT)
    {
        RME_COVERAGE_MARKER();
        /* Set the reason of that fault to the invocation return value register */
        __RME_Set_Inv_Retval(Reg,Thd_Child->Sched.Fault);
        /* Return the TID with the fault flag set */
        return Thd_Child->Sched.TID|RME_THD_FAULT_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Return the notification TID, which means that it is just a timeout */
    return Thd_Child->Sched.TID;
}
/* End Function:_RME_Thd_Sched_Rcv *******************************************/

/* Begin Function:_RME_Thd_Time_Xfer ******************************************
Description : Transfer time from one thread to another. This can only be called
              from the core that the thread is on, and the the two threads involved
              in the time transfer must be on the same core.
              If the time transfered is more than or equal to what the source have,
              the source will be out of time or blocked. If the source is both out of
              time and blocked, we do not send the notification; Instead, we send the
              notification when the receive endpoint actually receive something.
              It is possible to transfer time to threads have a lower priority, and it
              is also possible to transfer time to threads that have a higher priority.
              In the latter case, and if the source is currently running, a preemption
              will directly occur.
              There are 3 kinds of threads in the system:
              1. Init threads - They are created at boot-time and have infinite budget.
              2. Infinite threads - They are created later but have infinite budget.
              3. Normal threads - They are created later and have a finite budget.
              There are 3 kinds of transfer in the system:
              1. Normal transfers - They transfer a finite budget.
              2. Infinite transfers - They attempt to transfer an infinite budget but will
                 not revoke the timeslices of the source if the source have infinite budget.
              3. Revoking transfers - They attempt to transfer an infinite budget but will
                 revoke the timeslices of the source if the source is an infinite thread.
              Normal budget transferring rules:
              -----------------------------------------------------------------
                Nom   |   From   |     Init     |   Infinite   |    Normal
              -----------------------------------------------------------------
                 To   |   Init   |      --      |      --      |      T-
              -----------------------------------------------------------------
                      | Infinite |      --      |      --      |      T-
              -----------------------------------------------------------------
                      |  Normal  |      -A      |      TA      |      TA
              -----------------------------------------------------------------
              Infinite budget transferring rules:
              -----------------------------------------------------------------
                Inf   |   From   |     Init     |   Infinite   |    Normal
              -----------------------------------------------------------------
                 To   |   Init   |      --      |      --      |      S-
              -----------------------------------------------------------------
                      | Infinite |      --      |      --      |      S-
              -----------------------------------------------------------------
                      |  Normal  |      -I      |      -I      |      TA
              -----------------------------------------------------------------
              Revoking budget transferring rules:
              -----------------------------------------------------------------
                Rev   |   From   |     Init     |   Infinite   |    Normal
              -----------------------------------------------------------------
                 To   |   Init   |      --      |      S-      |      S-
              -----------------------------------------------------------------
                      | Infinite |      --      |      S-      |      S-
              -----------------------------------------------------------------
                      |  Normal  |      -I      |      SI      |      TA
              -----------------------------------------------------------------
              Notations:
              -:Nothing will happen on source/destination.
              T:Source timeout if all transferred.
              S:Source will definitely timeout.
              A:Destination accept if not overflow.
              I:Destination will definitely become an infinite thread.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_cid_t Cap_Thd_Dst - The destination thread. 2-Level.
              rme_cid_t Cap_Thd_Src - The source thread. 2-Level.
              rme_ptr_t Time - The time to transfer, in slices, for normal transfers.
                               A slice is the minimal amount of time transfered in the
                               system usually on the order of 100us or 1ms.
                               Use RME_THD_INIT_TIME for revoking transfer.
                               Use RME_THD_INF_TIME for infinite trasnfer.
Output      : None.
Return      : rme_ret_t - If successful, the destination time amount; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Time_Xfer(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                             rme_cid_t Cap_Thd_Dst, rme_cid_t Cap_Thd_Src, rme_ptr_t Time)
{
    struct RME_Cap_Thd* Thd_Dst;
    struct RME_Cap_Thd* Thd_Src;
    struct RME_Thd_Struct* Thd_Dst_Struct;
    struct RME_Thd_Struct* Thd_Src_Struct;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Time_Xfer;
    rme_ptr_t Type_Ref;
    
    /* We may allow transferring infinite time here */
    if(Time==0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd_Dst,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Dst,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Thd_Src,RME_CAP_THD,struct RME_Cap_Thd*,Thd_Src,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Thd_Dst,RME_THD_FLAG_XFER_DST);
    RME_CAP_CHECK(Thd_Src,RME_THD_FLAG_XFER_SRC);

    /* Check if the two threads are on the core that is accordance with what we are on */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Src_Struct=RME_CAP_GETOBJ(Thd_Src,struct RME_Thd_Struct*);
    if(Thd_Src_Struct->Sched.CPU_Local!=CPU_Local)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Do we have slices to transfer? - slices == 0 implies TIMEOUT, or BLOCKED, or even FAULT */
    if(Thd_Src_Struct->Sched.Slices==0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    Thd_Dst_Struct=RME_CAP_GETOBJ(Thd_Dst,struct RME_Thd_Struct*);
    
    if(Thd_Dst_Struct->Sched.CPU_Local!=CPU_Local)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_INVSTATE;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if the destination is in a fault. If yes, cancel the transfer */
    if(Thd_Dst_Struct->Sched.State==RME_THD_FAULT)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_PTH_FAULT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Delegating from a normal thread */
    if(Thd_Src_Struct->Sched.Slices<RME_THD_INF_TIME)
    {
        RME_COVERAGE_MARKER();

        /* Delegate all our time */
        if(Time>=RME_THD_INF_TIME)
        {
            RME_COVERAGE_MARKER();

            Time_Xfer=Thd_Src_Struct->Sched.Slices;
        }
        /* Delegate some time, if not sufficient, clean up the source time */
        else
        {
            RME_COVERAGE_MARKER();
            
            if(Thd_Src_Struct->Sched.Slices>Time)
            {
                RME_COVERAGE_MARKER();

                Time_Xfer=Time;
            }
            else
            {
                RME_COVERAGE_MARKER();

                Time_Xfer=Thd_Src_Struct->Sched.Slices;
            }
        }
        
        /* See if we are transferring to an infinite budget thread. If yes, we
         * are revoking timeslices; If not, this is a finite transfer */
        if(Thd_Dst_Struct->Sched.Slices<RME_THD_INF_TIME)
        {
            RME_COVERAGE_MARKER();
            
            RME_TIME_CHECK(Thd_Dst_Struct->Sched.Slices,Time_Xfer);
            Thd_Dst_Struct->Sched.Slices+=Time_Xfer;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        Thd_Src_Struct->Sched.Slices-=Time_Xfer;
    }
    /* Delegating from init or infinite thread */
    else
    {
        RME_COVERAGE_MARKER();

        /* Infinite transfer to the destination */
        if(Time>=RME_THD_INF_TIME)
        {
            RME_COVERAGE_MARKER();

            /* This transfer will revoke the infinite budget */
            if(Time==RME_THD_INIT_TIME)
            {
                RME_COVERAGE_MARKER();
                
                /* Will not revoke, source is an init thread */
                if(Thd_Src_Struct->Sched.Slices!=RME_THD_INIT_TIME)
                {
                    RME_COVERAGE_MARKER();
                    
                    Thd_Src_Struct->Sched.Slices=0;
                }
                else
                {
                    RME_COVERAGE_MARKER();
                }
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
            
            /* Set destination to infinite if it is not an init thread */
            if(Thd_Dst_Struct->Sched.Slices<RME_THD_INF_TIME)
            {
                RME_COVERAGE_MARKER();
                
                Thd_Dst_Struct->Sched.Slices=RME_THD_INF_TIME;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
        }
        else
        {
            RME_COVERAGE_MARKER();

            /* Just increase the budget of the other thread - check first */
            RME_TIME_CHECK(Thd_Dst_Struct->Sched.Slices,Time);
            Thd_Dst_Struct->Sched.Slices+=Time;
        }
    }

    /* Is the source time used up? If yes, delete it from the run queue, and notify its 
     * parent. If it is not in the run queue, The state of the source must be BLOCKED. */
    if(Thd_Src_Struct->Sched.Slices==0)
    {
        RME_COVERAGE_MARKER();
        
    	/* If it is blocked, we do not change its state, and only sends the scheduler notification */
        if((Thd_Src_Struct->Sched.State==RME_THD_RUNNING)||(Thd_Src_Struct->Sched.State==RME_THD_READY))
        {
            RME_COVERAGE_MARKER();
            
            _RME_Run_Del(Thd_Src_Struct);
            Thd_Src_Struct->Sched.State=RME_THD_TIMEOUT;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }

        /* Notify the parent about this */
        _RME_Run_Notif(Thd_Src_Struct);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Now save the system call return value to the caller stack - how much time the destination have now */
    __RME_Set_Syscall_Retval(Reg,Thd_Dst_Struct->Sched.Slices);

    /* See what was the state of the destination thread. If it is timeout, then
     * activate it. If it is other state, then leave it alone */
    if(Thd_Dst_Struct->Sched.State==RME_THD_TIMEOUT)
    {
        RME_COVERAGE_MARKER();
        
        Thd_Dst_Struct->Sched.State=RME_THD_READY;
        _RME_Run_Ins(Thd_Dst_Struct);
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* All possible kernel send (scheduler notifications) done, now pick the highest
     * priority thread to run */
    _RME_Kern_High(Reg, CPU_Local);
    
    return 0;
}
/* End Function:_RME_Thd_Time_Xfer *******************************************/

/* Begin Function:_RME_Thd_Swt ************************************************
Description : Switch to another thread. The thread to switch to must have the same
              preemptive priority as this thread, and have time, and not blocked.
              If trying to switch to a higher priority thread, this is impossible
              because whenever a thread with higher priority exists in the system,
              the kernel wiull let it preempt the current thread. 
              If trying to switch to a lower priority thread, this is impossible
              because the current thread just preempts it after the thread switch.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table. 
              struct RME_Reg_Struct* Reg - The register set structure.
              rme_cid_t Cap_Thd - The capability to the thread. 2-Level. If this is
                                  smaller than zero, the kernel will pickup whatever
                                  thread that have the highest priority and have time
                                  to run. 
              rme_ptr_t Full_Yield - This is a flag to indicate whether this is a 
                                     full yield. If it is, the kernel will kill all
                                     the time allocated for this thread. Full yield
                                     only works for threads that have non-infinite
                                     timeslices.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Thd_Swt(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                       rme_cid_t Cap_Thd, rme_ptr_t Full_Yield)
{
    struct RME_Cap_Thd* Next_Thd_Cap;
    struct RME_Thd_Struct* Next_Thd;
    struct RME_Thd_Struct* High_Thd;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;

    /* See if the scheduler is given the right to pick a thread to run */
    CPU_Local=RME_CPU_LOCAL();                                                   
    if(Cap_Thd<RME_CAPID_NULL)
    {
        RME_COVERAGE_MARKER();
        
        RME_CAPTBL_GETCAP(Captbl,Cap_Thd,RME_CAP_THD,struct RME_Cap_Thd*,Next_Thd_Cap,Type_Ref);
        /* Check if the target cap is not frozen and allows such operations */
        RME_CAP_CHECK(Next_Thd_Cap,RME_THD_FLAG_SWT);
        /* See if we can do operation on this core */
        Next_Thd=RME_CAP_GETOBJ(Next_Thd_Cap, struct RME_Thd_Struct*);
        if(Next_Thd->Sched.CPU_Local!=CPU_Local)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PTH_INVSTATE;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
            
        /* See if we can yield to the thread */
        if((CPU_Local->Cur_Thd)->Sched.Prio!=Next_Thd->Sched.Prio)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PTH_PRIO;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
            
        /* See if the state will allow us to do this */
        if((Next_Thd->Sched.State==RME_THD_BLOCKED)||(Next_Thd->Sched.State==RME_THD_TIMEOUT))
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PTH_INVSTATE;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
            
        /* See if the target is in a fault state */
        if(Next_Thd->Sched.State==RME_THD_FAULT)
        {
            RME_COVERAGE_MARKER();

            return RME_ERR_PTH_FAULT;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* See if we need to give up all our timeslices in this yield */
        if((Full_Yield!=0)&&((CPU_Local->Cur_Thd)->Sched.Slices<RME_THD_INF_TIME))
        {
            RME_COVERAGE_MARKER();
            
            _RME_Run_Del(CPU_Local->Cur_Thd);
            (CPU_Local->Cur_Thd)->Sched.Slices=0;
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_TIMEOUT;
            /* Notify the parent about this. This function includes kernel send as well, but
             * we don't need to call _RME_Kern_High after this because we are using optimized
             * logic for this context switch. For all other cases, _RME_Kern_High must be called. */
            _RME_Run_Notif(CPU_Local->Cur_Thd);
            /* See if the next thread is on the same priority level with the designated thread.
             * because we have sent a notification, we are not sure about this now. Additionally,
             * if the next thread is the current thread, we are forced to switch to someone else,
             * because our timeslice have certainly exhausted. */
            High_Thd=_RME_Run_High(CPU_Local);
            if((High_Thd->Sched.Prio>Next_Thd->Sched.Prio)||(CPU_Local->Cur_Thd==Next_Thd))
            {
                RME_COVERAGE_MARKER();
                
            	Next_Thd=High_Thd;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
        }
        else
        {
            RME_COVERAGE_MARKER();
            
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_READY;
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
        
        /* See if we need to give up all our timeslices in this yield */
        if((Full_Yield!=0)&&((CPU_Local->Cur_Thd)->Sched.Slices<RME_THD_INF_TIME))
        {
            RME_COVERAGE_MARKER();
            
            _RME_Run_Del(CPU_Local->Cur_Thd);
            (CPU_Local->Cur_Thd)->Sched.Slices=0;
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_TIMEOUT;
            /* Notify the parent about this */
            _RME_Run_Notif(CPU_Local->Cur_Thd);
        }
        else
        {
            RME_COVERAGE_MARKER();
            
            /* This operation is just to make sure that there are any other thread
             * at the same priority level, we're not switching to ourself */
            _RME_Run_Del(CPU_Local->Cur_Thd);
            _RME_Run_Ins(CPU_Local->Cur_Thd);
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_READY;
        }
        
        Next_Thd=_RME_Run_High(CPU_Local);
    }
    
    /* Now that we are successful, save the system call return value to the caller stack */
    __RME_Set_Syscall_Retval(Reg,0);

    /* Set the next thread's state first */
    Next_Thd->Sched.State=RME_THD_RUNNING;
    /* Here we do not need to call _RME_Kern_High because we have picked the
     * highest priority thread according to the logic above. We just check if
     * it happens to be ourself so we can return from the fast path. */
    if(CPU_Local->Cur_Thd==Next_Thd)
    {
        RME_COVERAGE_MARKER();
        
        return 0;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
            
    /* We have a solid context switch */
    _RME_Run_Swt(Reg, CPU_Local->Cur_Thd, Next_Thd);
    CPU_Local->Cur_Thd=Next_Thd;

    return 0;
}
/* End Function:_RME_Thd_Swt *************************************************/

/* Begin Function:_RME_Sig_Boot_Crt *******************************************
Description : Create a boot-time kernel signal capability. This is not a system 
              call, and is only used at boot-time to create endpoints that are
              related directly to hardware interrupts.
              This function does not require a kernel memory capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table to use
                                     for this signal. 2-Level.
              rme_cid_t Cap_Inv - The capability slot that you want this newly created
                                  signal capability to be in. 1-Level.
              rme_ptr_t Vaddr - The virtual address to store the signal endpoint kernel object.

Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Sig_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                            rme_cid_t Cap_Sig, rme_ptr_t Vaddr)
{
    struct RME_Cap_Captbl* Captbl_Crt;
    struct RME_Cap_Sig* Sig_Crt;
    struct RME_Sig_Struct* Sig_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Crt,Type_Ref);
    /* Check if the captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Crt,RME_CAPTBL_FLAG_CRT);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Crt,Cap_Sig,struct RME_Cap_Sig*,Sig_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Sig_Crt,Type_Ref);
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_SIG_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Sig_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Fill in the structure */
    Sig_Struct=(struct RME_Sig_Struct*)Vaddr;
    /* This is a kernel endpoint */
    Sig_Struct->Refcnt=1;
    Sig_Struct->Signal_Num=0;
    Sig_Struct->Thd=0;
    
    /* Fill in the header part */
    Sig_Crt->Head.Parent=0;
    Sig_Crt->Head.Object=Vaddr;
    /* Receive only because this is from kernel. Kernel send does not check flags anyway */
    Sig_Crt->Head.Flags=RME_SIG_FLAG_RCV_BS|RME_SIG_FLAG_RCV_BM|
                        RME_SIG_FLAG_RCV_NS|RME_SIG_FLAG_RCV_NM;

    /* Creation complete */
    RME_WRITE_RELEASE(&(Sig_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_SIG,0));
    return 0;
}
/* End Function:_RME_Sig_Boot_Crt ********************************************/

/* Begin Function:_RME_Sig_Crt ************************************************
Description : Create a signal capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table to use
                                     for this signal. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Inv - The capability slot that you want this newly created
                                  signal capability to be in. 1-Level.
              rme_ptr_t Raddr - The relative virtual address to store the signal endpoint
                                kernel object.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Sig_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                       rme_cid_t Cap_Kmem, rme_cid_t Cap_Sig, rme_ptr_t Raddr)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Sig* Sig_Crt;
    struct RME_Sig_Struct* Sig_Struct;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    /* See if the creation is valid for this kmem range */
    RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_SIG,Raddr,Vaddr,RME_SIG_SIZE);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Sig,struct RME_Cap_Sig*,Sig_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Sig_Crt,Type_Ref);
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_SIG_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Sig_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Fill in the structure */
    Sig_Struct=(struct RME_Sig_Struct*)Vaddr;
    Sig_Struct->Refcnt=0;
    Sig_Struct->Signal_Num=0;
    Sig_Struct->Thd=0;
    
    /* Fill in the header part */
    Sig_Crt->Head.Parent=0;
    Sig_Crt->Head.Object=Vaddr;
    Sig_Crt->Head.Flags=RME_SIG_FLAG_SND|RME_SIG_FLAG_RCV_BS|RME_SIG_FLAG_RCV_BM|
                        RME_SIG_FLAG_RCV_NS|RME_SIG_FLAG_RCV_NM|RME_SIG_FLAG_SCHED;
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Sig_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_SIG,0));
    return 0;
}
/* End Function:_RME_Sig_Crt *************************************************/

/* Begin Function:_RME_Sig_Del ************************************************
Description : Delete a signal capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table to delete from.
                                     2-Level.
              rme_cid_t Cap_Sig - The capability to the signal. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Sig_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Sig)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Sig* Sig_Del;
    rme_ptr_t Type_Ref;
    /* These are for deletion */
    struct RME_Sig_Struct* Sig_Struct;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Sig,struct RME_Cap_Sig*,Sig_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Sig_Del,Type_Ref,RME_CAP_SIG);
    
    /* Get the thread */
    Sig_Struct=RME_CAP_GETOBJ(Sig_Del,struct RME_Sig_Struct*);
    
    /* See if the signal endpoint is currently used. If yes, we cannot delete it */
    if(Sig_Struct->Thd!=0)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Sig_Del,Type_Ref);
        return RME_ERR_SIV_ACT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* See if this is a kernel endpoint, or a currently referened endpoint. If yes,
     * we cannot delete it */
    if(Sig_Struct->Refcnt!=0)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Sig_Del,Type_Ref);
        return RME_ERR_SIV_CONFLICT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Sig_Del,Type_Ref);
    /* Try to depopulate the area - this must be successful */
    RME_ASSERT(_RME_Kotbl_Erase((rme_ptr_t)Sig_Struct,RME_SIG_SIZE)!=0);
    
    return 0;
}
/* End Function:_RME_Sig_Del *************************************************/

/* Begin Function:_RME_Kern_High **********************************************
Description : Pick the thread with the highest priority to run. Always call this
              after you finish all your kernel sending stuff in the interrupt
              handler, or the kernel send will not be correct.
Input       : struct RME_Reg_Struct* Reg - The register set before the switch.
              struct RME_CPU_Local* CPU_Local - The CPU-local data structure.
Output      : struct RME_Reg_Struct* Reg - The register set after the switch.
Return      : None.
******************************************************************************/
void _RME_Kern_High(struct RME_Reg_Struct* Reg, struct RME_CPU_Local* CPU_Local)
{
    struct RME_Thd_Struct* Thd_Struct;

    Thd_Struct=_RME_Run_High(CPU_Local);
    RME_ASSERT(Thd_Struct!=0);

    /* Are these two threads the same? */
    if(Thd_Struct==(CPU_Local->Cur_Thd))
    {
        RME_COVERAGE_MARKER();

        return;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Is the current thread running or ready? */
    if(((CPU_Local->Cur_Thd)->Sched.State==RME_THD_RUNNING)||
       ((CPU_Local->Cur_Thd)->Sched.State==RME_THD_READY))
    {
        RME_COVERAGE_MARKER();

        /* Yes, compare the priority to see if we need to do it */
        if(Thd_Struct->Sched.Prio<=(CPU_Local->Cur_Thd)->Sched.Prio)
        {
            RME_COVERAGE_MARKER();

            return;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* We will have a solid context switch on this point */
    if((CPU_Local->Cur_Thd)->Sched.State==RME_THD_RUNNING)
    {
        RME_COVERAGE_MARKER();

        (CPU_Local->Cur_Thd)->Sched.State=RME_THD_READY;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    _RME_Run_Swt(Reg,(CPU_Local->Cur_Thd),Thd_Struct);
    Thd_Struct->Sched.State=RME_THD_RUNNING;
    (CPU_Local->Cur_Thd)=Thd_Struct;
}
/* End Function:_RME_Kern_High ***********************************************/

/* Begin Function:_RME_Kern_Snd ***********************************************
Description : Try to send a signal to an endpoint from kernel. This is intended to
              be called in the interrupt routines in the kernel, and this is not a
              system call.
Input       : struct RME_Sig_Struct* Sig - The signal structure.
Output      : None.
Return      : rme_ret_t - If successful, 0, or an error code.
******************************************************************************/
rme_ret_t _RME_Kern_Snd(struct RME_Sig_Struct* Sig_Struct)
{
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Unblock;
    
    /* See if we can receive on that endpoint - if someone blocks, we must
     * wait for it to unblock before we can proceed */
    Thd_Struct=Sig_Struct->Thd;
    /* If and only if we are calling from the same core as the blocked thread do
     * we actually unblock. Use an intermediate variable Unblock to avoid optimizations */
    if(Thd_Struct!=0)
    {
        RME_COVERAGE_MARKER();

        if(Thd_Struct->Sched.CPU_Local==RME_CPU_LOCAL())
        {
            RME_COVERAGE_MARKER();

            Unblock=1;
        }
        else
        {
            RME_COVERAGE_MARKER();

            Unblock=0;
        }
    }
    else
    {
        RME_COVERAGE_MARKER();

        Unblock=0;
    }

    if(Unblock!=0)
    {
        RME_COVERAGE_MARKER();

        /* The thread is blocked, and it is on our core. Unblock it, and
         * set the return value to one as always, Even if we were specifying
         * multi-receive. This is because other cores may reduce the count
         * to zero while we are doing this */
        __RME_Set_Syscall_Retval(&(Thd_Struct->Cur_Reg->Reg), 1);
        /* See if the thread still have time left */
        if(Thd_Struct->Sched.Slices!=0)
        {
            RME_COVERAGE_MARKER();

            /* Put this into the runqueue and just set it to ready. We will not switch to it
             * immediately; this is because we may send to a myriad of endpoints in one
             * interrupt, and we hope to perform the context switch only once when exiting
             * that handler. We can save many register push/pops! */
            _RME_Run_Ins(Thd_Struct);
            Thd_Struct->Sched.State=RME_THD_READY;
        }
        else
        {
            RME_COVERAGE_MARKER();

            /* No slices left. The only possible reason is because we delegated
             * all of its time to someone else. We will not notify its parent again
             * here because we will have notified it when we transferred all the
             * timeslices away. We just silently change the state of this thread
             * to TIMEOUT. Same for the next function. */
            Thd_Struct->Sched.State=RME_THD_TIMEOUT;
        }
        
        /* Clear the blocking status of the endpoint up - we don't need a write release barrier
         * here because even if this is reordered to happen earlier it is still fine. */
        Sig_Struct->Thd=0;
    }
    else
    {
        RME_COVERAGE_MARKER();

        /* The guy who blocked on it is not on our core, or nobody blocked.
         * We just faa the counter value and return */
        if(RME_FETCH_ADD(&(Sig_Struct->Signal_Num),1)>RME_MAX_SIG_NUM)
        {
            RME_COVERAGE_MARKER();

            RME_FETCH_ADD(&(Sig_Struct->Signal_Num),-1);
            return RME_ERR_SIV_FULL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
    }

    return 0;
}
/* End Function:_RME_Kern_Snd ************************************************/

/* Begin Function:_RME_Sig_Snd ************************************************
Description : Try to send a signal from user level. This system call can cause
              a potential context switch.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The register set.
              rme_cid_t Cap_Sig - The capability to the signal. 2-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0, or an error code.
******************************************************************************/
rme_ret_t _RME_Sig_Snd(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg, rme_cid_t Cap_Sig)
{
    struct RME_Cap_Sig* Sig_Op;
    struct RME_Sig_Struct* Sig_Struct;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Unblock;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Sig,RME_CAP_SIG,struct RME_Cap_Sig*,Sig_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_SND);
    
    CPU_Local=RME_CPU_LOCAL();
    Sig_Struct=RME_CAP_GETOBJ(Sig_Op,struct RME_Sig_Struct*);
    Thd_Struct=Sig_Struct->Thd;
    /* If and only if we are calling from the same core as the blocked thread do
     * we actually unblock. Use an intermediate variable Unblock to avoid optimizations */
    if(Thd_Struct!=0)
    {
        RME_COVERAGE_MARKER();

        if(Thd_Struct->Sched.CPU_Local==CPU_Local)
        {
            RME_COVERAGE_MARKER();

            Unblock=1;
        }
        else
        {
            RME_COVERAGE_MARKER();

            Unblock=0;
        }
    }
    else
    {
        RME_COVERAGE_MARKER();

        Unblock=0;
    }
    
    if(Unblock!=0)
    {
        RME_COVERAGE_MARKER();

        /* Now save the system call return value to the caller stack */
        __RME_Set_Syscall_Retval(Reg,0);
        /* The thread is blocked, and it is on our core. Unblock it, and
         * set the return value to one as always, Even if we were specifying
         * multi-receive. This is because other cores may reduce the count
         * to zero while we are doing this */
        __RME_Set_Syscall_Retval(&(Thd_Struct->Cur_Reg->Reg), 1);
        /* See if the thread still have time left */
        if(Thd_Struct->Sched.Slices!=0)
        {
            RME_COVERAGE_MARKER();

            /* Put this into the runqueue */
            _RME_Run_Ins(Thd_Struct);
            /* See if it will preempt us */
            if(Thd_Struct->Sched.Prio>(CPU_Local->Cur_Thd)->Sched.Prio)
            {
                RME_COVERAGE_MARKER();

                /* Yes. Do a context switch */
                _RME_Run_Swt(Reg,CPU_Local->Cur_Thd,Thd_Struct);
                (CPU_Local->Cur_Thd)->Sched.State=RME_THD_READY;
                Thd_Struct->Sched.State=RME_THD_RUNNING;
                CPU_Local->Cur_Thd=Thd_Struct;
            }
            else
            {
                RME_COVERAGE_MARKER();

                Thd_Struct->Sched.State=RME_THD_READY;
            }
        }
        else
        {
            RME_COVERAGE_MARKER();

            /* Silently change state to timeout */
            Thd_Struct->Sched.State=RME_THD_TIMEOUT;
        }
        
        /* Clear the blocking status of the endpoint up - we don't need a write release barrier
         * here because even if this is reordered to happen earlier it is still fine. */
        Sig_Struct->Thd=0;
    }
    else
    {
        RME_COVERAGE_MARKER();

        /* The guy who blocked on it is not on our core, we just faa and return */
        if(RME_FETCH_ADD(&(Sig_Struct->Signal_Num),1)>RME_MAX_SIG_NUM)
        {
            RME_COVERAGE_MARKER();

            RME_FETCH_ADD(&(Sig_Struct->Signal_Num),-1);
            return RME_ERR_SIV_FULL;
        }
        else
        {
            RME_COVERAGE_MARKER();
        }
        
        /* Now save the system call return value to the caller stack */
        __RME_Set_Syscall_Retval(Reg,0);
    }

    return 0;
}
/* End Function:_RME_Sig_Snd *************************************************/

/* Begin Function:_RME_Sig_Rcv ************************************************
Description : Try to receive a signal capability. The rules for the signal capability
              is:
              1.If a receive endpoint have many send endpoints, everyone can send to it,
                and sending to it will increase the signal count by 1.
              2.If some thread blocks on a receive endpoint, the wakeup is only possible
                from the same core that thread is on.
              3.It is not recommended to let 2 cores operate on the rcv endpoint simutaneously.
              This system call can potentially trigger a context switch.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The register set.
              rme_cid_t Cap_Sig - The capability to the signal. 2-Level.
              rme_ptr_t Option - The option to the receive. There are 4 operations
                                 available on one endpoint:
                                 0 - Blocking single receive. This will possibly block
                                     and will receive a single signal.
                                 1 - Blocking multi receive. This will possibly lock
                                     and will receive all signals on that endpoint.
                                 2 - Non-blocking single receive. This will return immediately
                                     on failure and will receive a single signal.
                                 3 - Non-blocking multi receive. This will return immediately
                                     on failure and will receive all signals on that endpoint.
Output      : None.
Return      : rme_ret_t - If successful, a non-negative number containing the number of signals
                          received will be returned; else an error code.
******************************************************************************/
rme_ret_t _RME_Sig_Rcv(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                       rme_cid_t Cap_Sig, rme_ptr_t Option)
{
    struct RME_Cap_Sig* Sig_Op;
    struct RME_Sig_Struct* Sig_Struct;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Old_Value;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Sig,RME_CAP_SIG,struct RME_Cap_Sig*,Sig_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    switch(Option)
    {
        case RME_RCV_BS:
        {
            RME_COVERAGE_MARKER();

            RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_RCV_BS);
            break;
        }
        case RME_RCV_BM:
        {
            RME_COVERAGE_MARKER();
            
            RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_RCV_BM);
            break;
        }
        case RME_RCV_NS:
        {
            RME_COVERAGE_MARKER();
            
            RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_RCV_NS);
            break;
        }
        case RME_RCV_NM:
        {
            RME_COVERAGE_MARKER();
            
            RME_CAP_CHECK(Sig_Op,RME_SIG_FLAG_RCV_NM);
            break;
        }
        default:
        {
            RME_COVERAGE_MARKER();
            
            return RME_ERR_SIV_ACT;
        }
    }
    
    /* See if we can receive on that endpoint - if someone blocks, we must
     * wait for it to unblock before we can proceed */
    Sig_Struct=RME_CAP_GETOBJ(Sig_Op,struct RME_Sig_Struct*);
    Thd_Struct=Sig_Struct->Thd;
    if(Thd_Struct!=0)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_ACT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Are we trying to let a boot-time thread block on a signal? This is NOT allowed.
     * Additionally, if the current thread have no timeslice left (which shouldn't happen
     * under whatever circumstances), we assert and die */
    CPU_Local=RME_CPU_LOCAL();
    Thd_Struct=CPU_Local->Cur_Thd;
    RME_ASSERT(Thd_Struct->Sched.Slices!=0);
    if(Thd_Struct->Sched.Slices==RME_THD_INIT_TIME)
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_BOOT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Are there any counts available? If yes, just take one and return. We cannot
     * use faa here because we don't know if we will get it below zero */
    Old_Value=Sig_Struct->Signal_Num;
    if(Old_Value>0)
    {
        RME_COVERAGE_MARKER();

        /* We can't use fetch-and-add because we don't know if other cores will reduce count to zero */
        if((Option==RME_RCV_BS)||(Option==RME_RCV_NS))
        {
            RME_COVERAGE_MARKER();

            /* Try to take one */
            if(RME_COMP_SWAP(&(Sig_Struct->Signal_Num),Old_Value,Old_Value-1)==0)
            {
                RME_COVERAGE_MARKER();

                return RME_ERR_SIV_CONFLICT;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
            
            /* We have taken it, now return what we have taken */
            __RME_Set_Syscall_Retval(Reg, 1);
        }
        else
        {
            RME_COVERAGE_MARKER();

            /* Try to take all */
            if(RME_COMP_SWAP(&(Sig_Struct->Signal_Num),Old_Value,0)==0)
            {
                RME_COVERAGE_MARKER();

                return RME_ERR_SIV_CONFLICT;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }
            
            /* We have taken all, now return what we have taken */
            __RME_Set_Syscall_Retval(Reg, Old_Value);
        }
        
        return 0;
    }
    else
    {
        RME_COVERAGE_MARKER();

        /* There's no value, Old_Value==0, We use this variable to try to block */
        if((Option==RME_RCV_BS)||(Option==RME_RCV_BM))
        {
            RME_COVERAGE_MARKER();

            if(RME_COMP_SWAP((rme_ptr_t*)(&(Sig_Struct->Thd)),Old_Value,(rme_ptr_t)Thd_Struct)==0)
            {
                RME_COVERAGE_MARKER();

                return RME_ERR_SIV_CONFLICT;
            }
            else
            {
                RME_COVERAGE_MARKER();
            }

            /* Now we block our current thread. No need to set any return value to the register
             * set here, because we do not yet know how many signals will be there when the thread
             * unblocks. The unblocking does not need an option so we don't keep that; we always
             * treat it as single receive when we unblock anyway. */
            Thd_Struct->Sched.State=RME_THD_BLOCKED;
            Thd_Struct->Sched.Signal=Sig_Struct;
            _RME_Run_Del(Thd_Struct);
            CPU_Local->Cur_Thd=_RME_Run_High(CPU_Local);
            _RME_Run_Swt(Reg,Thd_Struct,CPU_Local->Cur_Thd);
            (CPU_Local->Cur_Thd)->Sched.State=RME_THD_RUNNING;
        }
        else
        {
            RME_COVERAGE_MARKER();

            /* We have taken nothing but the system call is successful anyway */
            __RME_Set_Syscall_Retval(Reg, 0);
        }
    }
    
    return 0;
}
/* End Function:_RME_Sig_Rcv *************************************************/

/* Begin Function:_RME_Inv_Crt ************************************************
Description : Create an invocation capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table to use
                                     for this process. 2-Level.
              rme_cid_t Cap_Kmem - The kernel memory capability. 2-Level.
              rme_cid_t Cap_Inv - The capability slot that you want this newly created
                                  invocation capability to be in. 1-Level.
              rme_cid_t Cap_Proc - The capability to the process that it is in. 2-Level.
              rme_ptr_t Raddr - The relative virtual address to store the invocation port
                                kernel object.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Inv_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl,
                       rme_cid_t Cap_Kmem, rme_cid_t Cap_Inv, rme_cid_t Cap_Proc, rme_ptr_t Raddr)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Proc* Proc_Op;
    struct RME_Cap_Kmem* Kmem_Op;
    struct RME_Cap_Inv* Inv_Crt;
    struct RME_Inv_Struct* Inv_Struct;
    rme_ptr_t Type_Ref;
    rme_ptr_t Vaddr;
    
    /* Get the capability slots */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Proc,RME_CAP_PROC,struct RME_Cap_Proc*,Proc_Op,Type_Ref);
    RME_CAPTBL_GETCAP(Captbl,Cap_Kmem,RME_CAP_KMEM,struct RME_Cap_Kmem*,Kmem_Op,Type_Ref);
    /* Check if the captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    RME_CAP_CHECK(Proc_Op,RME_PROC_FLAG_INV);
    /* See if the creation is valid for this kmem range */
    RME_KMEM_CHECK(Kmem_Op,RME_KMEM_FLAG_INV,Raddr,Vaddr,RME_INV_SIZE);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Inv,struct RME_Cap_Inv*,Inv_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Inv_Crt,Type_Ref);
    
    /* Try to populate the area */
    if(_RME_Kotbl_Mark(Vaddr, RME_INV_SIZE)!=0)
    {
        RME_COVERAGE_MARKER();

        RME_WRITE_RELEASE(&(Inv_Crt->Head.Type_Ref),0);
        return RME_ERR_CAP_KOTBL;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Fill in the structure */
    Inv_Struct=(struct RME_Inv_Struct*)Vaddr;
    Inv_Struct->Proc=RME_CAP_GETOBJ(Proc_Op,struct RME_Proc_Struct*);
    Inv_Struct->Active=0;
    /* By default we do not return on fault */
    Inv_Struct->Fault_Ret_Flag=0;
    /* Increase the reference count of the process structure(Not the process capability) */
    RME_FETCH_ADD(&(RME_CAP_GETOBJ(Proc_Op, struct RME_Proc_Struct*)->Refcnt), 1);
    
    /* Fill in the header part */
    Inv_Crt->Head.Parent=0;
    Inv_Crt->Head.Object=Vaddr;
    Inv_Crt->Head.Flags=RME_INV_FLAG_SET|RME_INV_FLAG_ACT;
    
    /* Creation complete */
    RME_WRITE_RELEASE(&(Inv_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_INV,0));
    return 0;
}
/* End Function:_RME_Inv_Crt *************************************************/

/* Begin Function:_RME_Inv_Del ************************************************
Description : Delete an invocation capability.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the capability table to delete from.
                                     2-Level.
              rme_cid_t Cap_Inv - The capability to the invocation stub. 1-Level.
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Inv_Del(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Inv)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Inv* Inv_Del;
    rme_ptr_t Type_Ref;
    /* These are for deletion */
    struct RME_Inv_Struct* Inv_Struct;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);    
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_DEL);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Inv,struct RME_Cap_Inv*,Inv_Del);
    /* Delete check */
    RME_CAP_DEL_CHECK(Inv_Del,Type_Ref,RME_CAP_INV);
    
    /* Get the thread */
    Inv_Struct=RME_CAP_GETOBJ(Inv_Del,struct RME_Inv_Struct*);
    
    /* See if the invocation is currently used. If yes, we cannot delete it */
    if(Inv_Struct->Active!=0)
    {
        RME_COVERAGE_MARKER();

        RME_CAP_DEFROST(Inv_Del,Type_Ref);
        return RME_ERR_SIV_ACT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }
    
    /* Now we can safely delete the cap */
    RME_CAP_REMDEL(Inv_Del,Type_Ref);
    /* Dereference the process */
    RME_FETCH_ADD(&(Inv_Struct->Proc->Refcnt), -1);
    /* Try to clear the area - this must be successful */
    RME_ASSERT(_RME_Kotbl_Erase((rme_ptr_t)Inv_Struct,RME_INV_SIZE)!=0);
    
    return 0;
}
/* End Function:_RME_Inv_Del *************************************************/

/* Begin Function:_RME_Inv_Set ************************************************
Description : Set an invocation stub's entry point and stack. The registers will
              be initialized with these contents.
Input       : struct RME_Cap_Captbl* Captbl - The capability table.
              rme_cid_t Cap_Inv - The capability to the invocation stub. 2-Level.
              rme_ptr_t Entry - The entry of the thread.
              rme_ptr_t Stack - The stack address to use for execution.
              rme_ptr_t Fault_Ret_Flag - If there is an error in this invocation, we return
                                         immediately, or we wait for fault handling?
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Inv_Set(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Inv,
                       rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Fault_Ret_Flag)
{
    struct RME_Cap_Inv* Inv_Op;
    struct RME_Inv_Struct* Inv_Struct;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Inv,RME_CAP_INV,struct RME_Cap_Inv*,Inv_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Inv_Op,RME_INV_FLAG_SET);
    
    /* Commit the change - we do not care if the invocation is in use */
    Inv_Struct=RME_CAP_GETOBJ(Inv_Op,struct RME_Inv_Struct*);
    Inv_Struct->Entry=Entry;
    Inv_Struct->Stack=Stack;
    Inv_Struct->Fault_Ret_Flag=Fault_Ret_Flag;
    
    return 0;
}
/* End Function:_RME_Inv_Set *************************************************/

/* Begin Function:_RME_Inv_Act ************************************************
Description : Activate an invocation capability. That means, do the invocation.
Input       : struct RME_Cap_Captbl* Captbl - The capability table.
              struct RME_Reg_Struct* Reg - The register set for this thread.
              rme_cid_t Cap_Inv - The capability slot to the invocation stub. 2-Level.
              rme_ptr_t Param - The parameter for the call.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Inv_Act(struct RME_Cap_Captbl* Captbl, 
                       struct RME_Reg_Struct* Reg,
                       rme_cid_t Cap_Inv, rme_ptr_t Param)
{
    struct RME_Cap_Inv* Inv_Op;
    struct RME_Inv_Struct* Inv_Struct;
    struct RME_Thd_Struct* Thd_Struct;
    rme_ptr_t Active;
    rme_ptr_t Type_Ref;

    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Inv,RME_CAP_INV,struct RME_Cap_Inv*,Inv_Op,Type_Ref);
    /* Check if the target cap is not frozen and allows such operations */
    RME_CAP_CHECK(Inv_Op,RME_INV_FLAG_ACT);

    /* Get the invocation struct */
    Inv_Struct=RME_CAP_GETOBJ(Inv_Op,struct RME_Inv_Struct*);
    /* See if we are currently active - If yes, we can't activate it again */
    Active=Inv_Struct->Active;
    if(RME_UNLIKELY(Active!=0))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_ACT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Push this invocation stub capability into the current thread's invocation stack */
    Thd_Struct=RME_CPU_LOCAL()->Cur_Thd;
    /* Try to do CAS and activate it */
    if(RME_UNLIKELY(RME_COMP_SWAP(&(Inv_Struct->Active),Active,1)==0))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_ACT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Save whatever is needed to return to the point - normally only SP and IP needed
     * because all other registers, including the coprocessor registers, are saved at
     * user-level. We do not set the return value because it will be set by Inv_Ret.
     * The coprocessor state will be consistent across the call */
    __RME_Inv_Reg_Save(&(Inv_Struct->Ret), Reg);
    /* Push this into the stack: insert after the thread list header */
    __RME_List_Ins(&(Inv_Struct->Head),&(Thd_Struct->Inv_Stack),Thd_Struct->Inv_Stack.Next);
    /* Setup the register contents, and do the invocation */
    __RME_Thd_Reg_Init(Inv_Struct->Entry, Inv_Struct->Stack, Param, Reg);
    
    /* We are assuming that we are always invoking into a new process (why use synchronous
     * invocation if you don't do so?). So we always switch page tables regardless. */
    __RME_Pgtbl_Set(RME_CAP_GETOBJ(Inv_Struct->Proc->Pgtbl,rme_ptr_t));
    
    return 0;
}
/* End Function:_RME_Inv_Act *************************************************/

/* Begin Function:_RME_Inv_Ret ************************************************
Description : Return from the invocation function, and set the return value to
              the old register set. This function does not need a capability
              table to work.
Input       : struct RME_Reg_Struct* Reg - The register set for this thread.
              rme_ptr_t Retval - The return value of this synchronous invocation.
              rme_ptr_t Fault_Flag - Are we attempting a return from fault?
Output      : None.
Return      : rme_ret_t - If successful, 0; or an error code.
******************************************************************************/
rme_ret_t _RME_Inv_Ret(struct RME_Reg_Struct* Reg, rme_ptr_t Retval, rme_ptr_t Fault_Flag)
{
    struct RME_Thd_Struct* Thd_Struct;
    struct RME_Inv_Struct* Inv_Struct;

    /* See if we can return; If we can, get the structure */
    Thd_Struct=RME_CPU_LOCAL()->Cur_Thd;
    Inv_Struct=RME_INVSTK_TOP(Thd_Struct);
    if(RME_UNLIKELY(Inv_Struct==0))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_EMPTY;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Is this return forced by a fault? If yes, check if we allow that */
    if(RME_UNLIKELY((Fault_Flag!=0)&&(Inv_Struct->Fault_Ret_Flag==0)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_SIV_FAULT;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Pop it from the stack */
    __RME_List_Del(Inv_Struct->Head.Prev,Inv_Struct->Head.Next);

    /* Restore the register contents, and set return value. We need to set
     * the return value of the invocation system call itself as well */
    __RME_Inv_Reg_Restore(Reg, &(Inv_Struct->Ret));
    __RME_Set_Inv_Retval(Reg, Retval);

    /* We have successfully returned, set the invocation as inactive. We need
     * a barrier here to avoid potential destruction of the return value. */
    RME_WRITE_RELEASE(&(Inv_Struct->Active),0);

    /* Decide the system call's return value */
    if(RME_UNLIKELY(Fault_Flag!=0))
    {
        RME_COVERAGE_MARKER();

        __RME_Set_Syscall_Retval(Reg, RME_ERR_SIV_FAULT);
    }
    else
    {
        RME_COVERAGE_MARKER();

        __RME_Set_Syscall_Retval(Reg, 0);
    }

    /* Same assumptions as in invocation activation */
    Inv_Struct=RME_INVSTK_TOP(Thd_Struct);
    if(Inv_Struct!=0)
    {
        RME_COVERAGE_MARKER();

        __RME_Pgtbl_Set(RME_CAP_GETOBJ(Inv_Struct->Proc->Pgtbl,rme_ptr_t));
    }
    else
    {
        RME_COVERAGE_MARKER();

        __RME_Pgtbl_Set(RME_CAP_GETOBJ(Thd_Struct->Sched.Proc->Pgtbl,rme_ptr_t));
    }
    
    return 0;
}
/* End Function:_RME_Inv_Ret *************************************************/

/* Begin Function:_RME_Kern_Boot_Crt ******************************************
Description : This function is used to create boot-time kernel call capability.
              This kind of capability that does not have a kernel object. Kernel
              function capabilities are the capabilities that allow you to execute
              functions in kernel mode. These functions must be defined in the 
              CPU driver platform file.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              rme_cid_t Cap_Captbl - The capability to the captbl that may contain the cap
                                     to kernel function. 2-Level.
              rme_cid_t Cap_Kern - The capability to the kernel function. 1-Level.
Output      : None.
Return      : rme_ret_t - If the mapping is successful, it will return 0; else error code.
******************************************************************************/
rme_ret_t _RME_Kern_Boot_Crt(struct RME_Cap_Captbl* Captbl, rme_cid_t Cap_Captbl, rme_cid_t Cap_Kern)
{
    struct RME_Cap_Captbl* Captbl_Op;
    struct RME_Cap_Kern* Kern_Crt;
    rme_ptr_t Type_Ref;
    
    /* Get the cap location that we care about */
    RME_CAPTBL_GETCAP(Captbl,Cap_Captbl,RME_CAP_CAPTBL,struct RME_Cap_Captbl*,Captbl_Op,Type_Ref);
    /* Check if the target captbl is not frozen and allows such operations */
    RME_CAP_CHECK(Captbl_Op,RME_CAPTBL_FLAG_CRT);
    
    /* Get the cap slot */
    RME_CAPTBL_GETSLOT(Captbl_Op,Cap_Kern,struct RME_Cap_Kern*,Kern_Crt);
    /* Take the slot if possible */
    RME_CAPTBL_OCCUPY(Kern_Crt,Type_Ref);
    
    Kern_Crt->Head.Parent=0;
    /* The kernel capability does not have an object */
    Kern_Crt->Head.Object=0;
    Kern_Crt->Head.Flags=RME_KERN_FLAG_FULL_RANGE;
    
    /* Creation complete, and make it undeletable */
    RME_WRITE_RELEASE(&(Kern_Crt->Head.Type_Ref),RME_CAP_TYPEREF(RME_CAP_KERN,1));
    return 0;
}
/* End Function:_RME_Kern_Boot_Crt *******************************************/

/* Begin Function:_RME_Kern_Act ***********************************************
Description : Activate a kernel function.
Input       : struct RME_Cap_Captbl* Captbl - The master capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_cid_t Cap_Kern - The capability to the kernel capability. 2-Level.
              rme_ptr_t Func_ID - The function ID to invoke.
              rme_ptr_t Sub_ID - The subfunction ID to invoke.
              rme_ptr_t Param1 - The first parameter.
              rme_ptr_t Param2 - The second parameter.
Output      : None.
Return      : rme_ret_t - If the call is successful, it will return whatever the 
                          function returned(It is expected that these functions shall
                          never return an negative value); else error code. If the 
                          kernel function ever succeeds, it is responsible for setting
                          the return value. On failure, a context switch shall never
                          happen.
******************************************************************************/
rme_ret_t _RME_Kern_Act(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                        rme_cid_t Cap_Kern, rme_ptr_t Func_ID, rme_ptr_t Sub_ID,
                        rme_ptr_t Param1, rme_ptr_t Param2)
{
    struct RME_Cap_Kern* Kern_Op;
    rme_ptr_t Type_Ref;
    
    /* Get the capability slot */
    RME_CAPTBL_GETCAP(Captbl,Cap_Kern,RME_CAP_KERN,struct RME_Cap_Kern*,Kern_Op,Type_Ref);    

    /* Check if the range of calling is allowed - This is kernel function specific */
    if((Func_ID>RME_KERN_FLAG_HIGH(Kern_Op->Head.Flags))||
       (Func_ID<RME_KERN_FLAG_LOW(Kern_Op->Head.Flags)))
    {
        RME_COVERAGE_MARKER();

        return RME_ERR_CAP_FLAG;
    }
    else
    {
        RME_COVERAGE_MARKER();
    }

    /* Return whatever the function returns */
    return __RME_Kern_Func_Handler(Captbl,Reg,Func_ID,Sub_ID,Param1,Param2);
}
/* End Function:_RME_Kern_Act ************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
