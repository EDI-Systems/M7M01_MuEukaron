/******************************************************************************
Filename    : platform_cav7.c
Author      : pry
Date        : 01/04/2017
Licence     : LGPL v3+; see COPYING for details.
Description : The hardware abstraction layer for ARMv7-A machines.
              1. Interrupt controller
              2. Timer
              3. Pgtbl setup
******************************************************************************/

/* Includes ******************************************************************/
#define __HDR_DEFS__
#include "Kernel/rme_kernel.h"
#include "Kernel/rme_kotbl.h"
#include "Kernel/rme_captbl.h"
#include "Kernel/rme_pgtbl.h"
#include "Kernel/rme_prcthd.h"
#include "Kernel/rme_siginv.h"
#include "Platform/CortexAv7/rme_platform_cav7.h"
#undef __HDR_DEFS__

#define __HDR_STRUCTS__
#include "Platform/CortexAv7/rme_platform_cav7.h"
#include "Kernel/rme_captbl.h"
#include "Kernel/rme_kernel.h"
#include "Kernel/rme_prcthd.h"
#include "Kernel/rme_pgtbl.h"
#include "Kernel/rme_siginv.h"
#undef __HDR_STRUCTS__

/* Private include */
#include "Platform/CortexAv7/rme_platform_cav7.h"

#define __HDR_PUBLIC_MEMBERS__
#include "Kernel/rme_kernel.h"
#include "Kernel/rme_captbl.h"
#include "Kernel/rme_pgtbl.h"
#include "Kernel/rme_kotbl.h"
#include "Kernel/rme_prcthd.h"
#include "Kernel/rme_siginv.h"
#undef __HDR_PUBLIC_MEMBERS__
/* End Includes **************************************************************/

/* Begin Function:main ********************************************************
Description : The entrance of the operating system.
Input       : None.
Output      : None.
Return      : int - This function never returns.
******************************************************************************/
int main(void)
{
    /* The main function of the kernel - we will start our kernel boot here */
    _RME_Kmain(RME_KMEM_STACK_ADDR);

    return 0;
}
/* End Function:main *********************************************************/

/* Begin Function:__RME_Putchar ***********************************************
Description : Output a character to console. In Cortex-M, under most circumstances, 
              we should use the ITM for such outputs.
Input       : char Char - The character to print.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Putchar(char Char)
{
    RME_CAV7_PUTCHAR(Char);
    return 0;
}
/* End Function:__RME_Putchar ************************************************/

/* Begin Function:__RME_CAV7_CPU_Local_Get ************************************
Description : Get the CPU-local data structures. This is to identify where we are
              executing.
Input       : None.
Output      : None.
Return      : struct RME_CPU_Local* - The CPU-local data structures.
******************************************************************************/
struct RME_CPU_Local* __RME_CAV7_CPU_Local_Get(void)
{
	return &RME_CAV7_Local[__RME_CAV7_MPIDR_Get()&0x03];
}
/* End Function:__RME_CAV7_CPU_Local_Get *************************************/

/* Begin Function:__RME_CAV7_Int_Init *****************************************
Description : Initialize the interrupt controller of the Cortex-A ARMv7 platform.
              This will only initialize the distributor and will only be run by
              the first CPU. This will initialize the timer as well.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_Int_Init(void)
{
    rme_ptr_t Temp;
    rme_ptr_t Lines;

    /* Who implemented the GIC, what variant, how many interrupts? */
    Temp=RME_CAV7_GICD_IIDR;
    RME_PRINTK_S("\r\nCAV7-GIC: ProductID: ");
    RME_PRINTK_I(Temp>>24);
    RME_PRINTK_S("\r\nCAV7-GIC: Variant: ");
    RME_PRINTK_I((Temp>>20)&0xF);
    RME_PRINTK_S("\r\nCAV7-GIC: Revision: ");
    RME_PRINTK_I((Temp>>12)&0xF);
    RME_PRINTK_S("\r\nCAV7-GIC: Implementer: 0x");
    RME_PRINTK_U(Temp&0xFFF);

    /* How many locked SPIs, security extension enabled or not, number of
     * actual CPUs and interrupt lines */
    Temp=RME_CAV7_GICD_TYPER;
    RME_PRINTK_S("\r\nCAV7-GIC: SPI number: ");
    RME_PRINTK_I(Temp>>16);
    RME_PRINTK_S("\r\nCAV7-GIC: Security extension: ");
    RME_PRINTK_I((Temp>>10)&0x1);
    RME_PRINTK_S("\r\nCAV7-GIC: CPU number: ");
    RME_PRINTK_I(((Temp>>5)&0x7)+1);
    RME_PRINTK_S("\r\nCAV7-GIC: Interrupt line number: ");
    Lines=((Temp&0x1F)+1)*32;
    RME_PRINTK_I(Lines);

#if(RME_CAV7_GIC_TYPE==RME_CAV7_GIC_V1)
    /* Initialize all vectors to group 0, and disable all */
	for(Temp=0;Temp<Lines/32;Temp++)
	{
		RME_CAV7_GICD_ICPENDR(Temp)=0xFFFFFFFFU;
		RME_CAV7_GICD_IGROUPR(Temp)=0x00000000U;
		RME_CAV7_GICD_ICENABLER(Temp)=0xFFFFFFFFU;
	}

	/* Set the priority of all such interrupts to the lowest level */
	for(Temp=0;Temp<Lines/4;Temp++)
		RME_CAV7_GICD_IPRIORITYR(Temp)=0xA0A0A0A0U;

	/* All interrupts target CPU0 */
	for(Temp=8;Temp<Lines/4;Temp++)
		RME_CAV7_GICD_ITARGETSR(Temp)=0x01010101U;

	/* All interrupts are edge triggered, and use 1-N model */
	for(Temp=0;Temp<Lines/16;Temp++)
		RME_CAV7_GICD_ICFGR(Temp)=0x55555555U;

	/* Enable the interrupt controller */
	RME_CAV7_GICD_CTLR=RME_CAV7_GICD_CTLR_GRP1EN|RME_CAV7_GICD_CTLR_GRP0EN;
#else
    /* Initialize all vectors to group 1, unless otherwise noted, and disable all */
    for(Temp=0;Temp<Lines/32;Temp++)
    {
    	RME_CAV7_GICD_ICPENDR(Temp)=0xFFFFFFFFU;
    	RME_CAV7_GICD_IGROUPR(Temp)=0xFFFFFFFFU;
    	RME_CAV7_GICD_ICENABLER(Temp)=0xFFFFFFFFU;
    }

    /* Set the priority of all such interrupts to the lowest level*/
    for(Temp=0;Temp<Lines/4;Temp++)
		RME_CAV7_GICD_IPRIORITYR(Temp)=0xE0E0E0E0U;

    /* All interrupts target CPU0 */
    for(Temp=8;Temp<Lines/4;Temp++)
    	RME_CAV7_GICD_ITARGETSR(Temp)=0x01010101U;

    /* All interrupts are edge triggered, and use 1-N model */
    for(Temp=0;Temp<Lines/16;Temp++)
    	RME_CAV7_GICD_ICFGR(Temp)=0xFFFFFFFFU;

    /* Enable the interrupt controller */
    RME_CAV7_GICD_CTLR=RME_CAV7_GICD_CTLR_GRP1EN|RME_CAV7_GICD_CTLR_GRP0EN;
#endif
}
/* End Function:__RME_CAV7_Int_Init ******************************************/

/* Begin Function:__RME_CAV7_Int_Local_Init ***********************************
Description : Initialize the local CPU interface of the processor.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_Int_Local_Init(void)
{
	/* Priority grouping */
	RME_CAV7_GICC_BPR=RME_CAV7_GIC_GROUPING;

#if(RME_CAV7_GIC_TYPE==RME_CAV7_GIC_V1)
	/* Enable all interrupts to this interface - FIQ is bypassed, and all
	 * interrupts go through the IRQ. The FIQ feature is only available on
	 * the stabdalone FIQ interrupt line */
	RME_CAV7_GICC_CTLR=RME_CAV7_GICC_ENABLEGRP0;
#else
	RME_CAV7_GICC_CTLR=RME_CAV7_GICC_CBPR|RME_CAV7_GICC_FIQEN|
			           RME_CAV7_GICC_ENABLEGRP1|RME_CAV7_GICC_ENABLEGRP0;
#endif

	/* No interrupts are masked - This must be set at last because enabling
	 * will trash the contents of this register if previously set. To maintain
	 * compatibility across all possible implementations, no priority level
	 * lower than 0xF0 will be considered valid */
	RME_CAV7_GICC_PMR=0xF0U;
}
/* End Function:__RME_CAV7_Int_Local_Init ************************************/

/* Begin Function:__RME_CAV7_Timer_Init ***************************************
Description : Initialize the interrupt controller of the Cortex-A ARMv7 platform.
              This will only initialize the distributor and will only be run by
              the first CPU. This will initialize the timer as well.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_Timer_Init(void)
{
#if((RME_CAV7_CPU_TYPE==RME_CAV7_CPU_CORTEX_A5)|| \
	(RME_CAV7_CPU_TYPE==RME_CAV7_CPU_CORTEX_A9))
    /* Writing this will also write the counter register as well */
    RME_CAV7_PTWD_PTLR=RME_CAV7_SYSTICK_VAL;
    /* Clear the interrupt flag */
    RME_CAV7_PTWD_PTISR=0;
    /* Start the timer */
    RME_CAV7_PTWD_PTCTLR=RME_CAV7_PTWD_PTCTLR_PRESC(0)|
                         RME_CAV7_PTWD_PTCTLR_IRQEN|
						 RME_CAV7_PTWD_PTCTLR_AUTOREL|
						 RME_CAV7_PTWD_PTCTLR_TIMEN;

    /* Enable the timer interrupt in the GIC */
    RME_CAV7_GICD_ISENABLER(0)|=1<<29;
#else
	#error Cortex-A7/8/15/17 is not supported at the moment.
    Cortex-A7/15/17 use the new generic timer, and Cortex-A8 does not
	have a processor timer due to very early release dates.
#endif
}
/* End Function:__RME_CAV7_Timer_Init ****************************************/

/* Begin Function:__RME_CAV7_Feature_Get **************************************
Description : Use the CPUID instruction extensively to get all the processor
              information. We assume that all processors installed have the same
              features.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_Feature_Get(void)
{

    /* TODO: Check these flags. If not satisfied, we hang immediately. */
}
/* End Function:__RME_CAV7_Feature_Get ***************************************/

/* Begin Function:__RME_CAV7_Mem_Init *****************************************
Description : Initialize the memory map, and get the size of kernel object
              allocation registration table(Kotbl) and page table reference
              count registration table(Pgreg).
Input       : rme_ptr_t MMap_Addr - The GRUB multiboot memory map data address.
              rme_ptr_t MMap_Length - The GRUB multiboot memory map data length.
Output      : None.
Return      : None.
******************************************************************************/
const rme_ptr_t RME_CAV7_Mem_Info[RME_CAV7_MEM_ENTRIES]={RME_CAV7_MEM_CONTENTS};
void __RME_CAV7_Mem_Init(rme_ptr_t MMap_Addr, rme_ptr_t MMap_Length)
{

}
/* End Function:__RME_CAV7_Mem_Init ******************************************/

/* Begin Function:__RME_CAV7_SMP_Init *****************************************
Description : Start all other processors, one by one.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_SMP_Init(void)
{

}
/* End Function:__RME_CAV7_SMP_Init ******************************************/

/* Begin Function:__RME_CAV7_SMP_Tick *****************************************
Description : Send IPI to all other cores,to run their handler on the time.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_CAV7_SMP_Tick(void)
{

}
/* End Function:__RME_CAV7_SMP_Tick *******************************************/

/* Begin Function:__RME_Low_Level_Init ****************************************
Description : Initialize the low-level hardware. This assumes that we are already
              in Supervisor (SVC) mode.
Input       : None.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Low_Level_Init(void)
{
    /* Initialize hardware */
    RME_CAV7_LOW_LEVEL_INIT();

    /* Initialize our own CPU-local data structure */
    _RME_CPU_Local_Init(&RME_CAV7_Local[0],0);

    /* Initialize the interrupt controller */
    __RME_CAV7_Int_Init();

    /* Initialize CPU-local interrupt resources */
	__RME_CAV7_Int_Local_Init();

    /* Initialize the vector table */
    RME_PRINTK_S("\r\nCAV7-Vector: 0x");
    RME_PRINTK_U((rme_ptr_t)&__RME_CAV7_Vector_Table);
    __RME_CAV7_VBAR_Set((rme_ptr_t)&__RME_CAV7_Vector_Table);

    RME_PRINTK_S("\r\nCAV7-Non-Secure: ");
    RME_PRINTK_U(__RME_CAV7_SCR_Get());

    return 0;
}
/* End Function:__RME_Low_Level_Init *****************************************/

/* Begin Function:__RME_Pgtbl_Kmem_Init ***************************************
Description : Initialize the kernel mapping tables, so it can be added to all the
              top-level page tables. Currently this have no consideration for >1TB
              RAM, and is not NUMA-aware.
Input       : None.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Kmem_Init(void)
{

    return 0;
}
/* End Function:__RME_Pgtbl_Kmem_Init ****************************************/

/* Begin Function:__RME_SMP_Low_Level_Init ************************************
Description : Low-level initialization for all other cores.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
rme_ptr_t __RME_SMP_Low_Level_Init(void)
{
    /* Initialize hardware */
    RME_CAV7_SMP_LOW_LEVEL_INIT();
    /* Initialize our own CPU-local variables */
    _RME_CPU_Local_Init(&RME_CAV7_Local[RME_CAV7_CPU_Cnt],RME_CAV7_CPU_Cnt);
    return 0;
}
/* End Function:__RME_SMP_Low_Level_Init *************************************/

/* Begin Function:__RME_Boot **************************************************
Description : Boot the first process in the system.
Input       : None.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Boot(void)
{
    /* Initialize timer */
	__RME_CAV7_Timer_Init();
    __RME_Enable_Int();
	while(1);
    return 0;
}
/* End Function:__RME_Boot ***************************************************/

/* Begin Function:__RME_Reboot ************************************************
Description : Reboot the machine, abandon all operating system states.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_Reboot(void)
{
    RME_ASSERT(RME_WORD_BITS!=RME_POW2(RME_WORD_ORDER));
}
/* End Function:__RME_Reboot *************************************************/

/* Begin Function:__RME_Shutdown **********************************************
Description : Shutdown the machine, abandon all operating system states.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_Shutdown(void)
{
    RME_ASSERT(RME_WORD_BITS!=RME_POW2(RME_WORD_ORDER));
}
/* End Function:__RME_Shutdown ***********************************************/

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
    *Svc=(Reg->R0)>>16;
    *Capid=(Reg->R0)&0xFFFF;
    Param[0]=Reg->R1;
    Param[1]=Reg->R2;
    Param[2]=Reg->R3;
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
    Reg->R0=(rme_ptr_t)Retval;
}
/* End Function:__RME_Set_Syscall_Retval *************************************/

/* Begin Function:__RME_Thd_Reg_Init ******************************************
Description : Initialize the register set for the thread.
Input       : rme_ptr_t Entry - The thread entry address.
              rme_ptr_t Stack - The thread stack address.
              rme_ptr_t Param - The parameter to pass to it.
Output      : struct RME_Reg_Struct* Reg - The register set content generated.
Return      : None.
******************************************************************************/
void __RME_Thd_Reg_Init(rme_ptr_t Entry, rme_ptr_t Stack, rme_ptr_t Param, struct RME_Reg_Struct* Reg)
{
    /* Set the entry and stack */
    Reg->PC=Entry;
    Reg->SP=Stack;
    /* Set the parameter */
    Reg->R0=Param;
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
    Dst->CPSR=Src->CPSR;
    Dst->R0=Src->R0;
    Dst->R1=Src->R1;
    Dst->R2=Src->R2;
    Dst->R3=Src->R3;
    Dst->R4=Src->R4;
    Dst->R5=Src->R5;
    Dst->R6=Src->R6;
    Dst->R7=Src->R7;
    Dst->R8=Src->R8;
    Dst->R9=Src->R9;
    Dst->R10=Src->R10;
    Dst->R11=Src->R11;
    Dst->SP=Src->SP;
    Dst->LR=Src->LR;
    Dst->PC=Src->PC;
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
    /* Not used for now */
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
    /* Not used for now */
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
    Ret->PC=Reg->PC;
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
    Reg->PC=Ret->PC;
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
    Reg->R1=(rme_ptr_t)Retval;
}
/* End Function:__RME_Set_Inv_Retval *****************************************/

/* Begin Function:__RME_Kern_Func_Handler *************************************
Description : Handle kernel function calls.
Input       : struct RME_Reg_Struct* Reg - The current register set.
              rme_ptr_t Func_ID - The function ID.
              rme_ptr_t Sub_ID - The sub function ID.
              rme_ptr_t Param1 - The first parameter.
              rme_ptr_t Param2 - The second parameter.
Output      : None.
Return      : rme_ptr_t - The value that the function returned.
******************************************************************************/
rme_ptr_t __RME_Kern_Func_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Func_ID,
                                  rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2)
{
	/* Currently no kernel function implemented */
    return 0;
}
/* End Function:__RME_Kern_Func_Handler **************************************/

/* Begin Function:__RME_CAV7_Undefined_Handler ********************************
Description : The undefined instruction vector handler of RME.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_CAV7_Undefined_Handler(struct RME_Reg_Struct* Reg)
{
	/* We don't handle undefined instructions now */
	while(1);
}
/* End Function:__RME_CAV7_Undefined_Handler *********************************/

/* Begin Function:__RME_CAV7_Prefetch_Abort_Handler ***************************
Description : The prefetch abort vector handler of RME.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_CAV7_Prefetch_Abort_Handler(struct RME_Reg_Struct* Reg)
{
	/* We don't handle prefetch aborts now */
	while(1);
}
/* End Function:__RME_CAV7_Prefetch_Abort_Handler ****************************/

/* Begin Function:__RME_CAV7_Data_Abort_Handler *******************************
Description : The data abort vector handler of RME.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_CAV7_Data_Abort_Handler(struct RME_Reg_Struct* Reg)
{
	/* We don't handle data aborts now */
	while(1);
}
/* End Function:__RME_CAV7_Data_Abort_Handler ********************************/

/* Begin Function:__RME_CAV7_IRQ_Handler **************************************
Description : The regular interrupt vector handler of RME.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_CAV7_IRQ_Handler(struct RME_Reg_Struct* Reg)
{
	rme_ptr_t Int_ID;
	rme_ptr_t CPUID;

	/* What interrupt is this? */
	Int_ID=RME_CAV7_GICC_IAR;
	CPUID=Int_ID>>10;
	Int_ID&=0x3FFU;

#if(RME_CAV7_GIC_TYPE==RME_CAV7_GIC_V1)
	/* Is this a spurious interrupt? (Can't be 1022 because GICv1 don't have group1) */
	RME_ASSERT(Int_ID!=1022);
	if(Int_ID==1023)
		return;
	/* Only the booting processor will receive timer interrupts */
#if((RME_CAV7_CPU_TYPE==RME_CAV7_CPU_CORTEX_A5)|| \
	(RME_CAV7_CPU_TYPE==RME_CAV7_CPU_CORTEX_A9))
	/* Is is an timer interrupt? (we know that it is at 29) */
	if(Int_ID==29)
	{
		/* Clear the interrupt flag */
	    RME_CAV7_PTWD_PTISR=0;
		_RME_Tick_Handler(Reg);
		/* Send interrupt to all other processors to notify them about this */
		/* EOI the interrupt */
		RME_CAV7_GICC_EOIR=Int_ID;
		return;
	}
#else

#endif

#else

#endif
	/* Is this a coprocessor timer interrupt? (We use interrupt number 0 for these) */
	if(Int_ID==0)
	{
		/* This must have originated from interface 0 */
		RME_ASSERT(CPUID==0);
		_RME_Tick_SMP_Handler(Reg);
		/* EOI the interrupt */
		RME_CAV7_GICC_EOIR=Int_ID;
		return;
	}

	/* Is this an other IPI? (All the rest of the SGIs are these) */
	if(Int_ID<16)
	{
		_RME_CAV7_SGI_Handler(Reg,CPUID,Int_ID);
		/* EOI the interrupt */
		RME_CAV7_GICC_EOIR=(CPUID<<10U)|Int_ID;
		return;
	}

	/* Is this an casual interrupt? */
	RME_ASSERT(CPUID==0);

}
/* End Function:__RME_CAV7_IRQ_Handler ***************************************/

/* Begin Function:_RME_CAV7_SGI_Handler ***************************************
Description : The generic interrupt handler of RME for x64.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t CPUID - The ID of the sender CPU.
              rme_ptr_t Int_ID - The ID of the SGI.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void _RME_CAV7_SGI_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t CPUID, rme_ptr_t Int_ID)
{
	/* Not handling SGIs */
	return;
}
/* End Function:_RME_CAV7_SGI_Handler ****************************************/

/* Begin Function:__RME_CAV7_Generic_Handler **********************************
Description : The generic interrupt handler of RME for Cortex-A (ARMv7).
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t Int_Num - The interrupt number.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_CAV7_Generic_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Int_Num)
{
    /* Not handling interrupts */
    RME_PRINTK_S("\r\nGeneral int:");
    RME_PRINTK_I(Int_Num);

    /* Not handling anything */
    switch(Int_Num)
    {
        default:break;
    }
    /* Remember to perform context switch after any kernel sends */
}
/* End Function:__RME_CAV7_Generic_Handler ***********************************/

/* Begin Function:__RME_Pgtbl_Set *********************************************
Description : Set the processor's page table.
Input       : rme_ptr_t Pgtbl - The virtual address of the page table.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_Pgtbl_Set(rme_ptr_t Pgtbl)
{

}
/* End Function:__RME_Pgtbl_Set **********************************************/

/* Begin Function:__RME_Pgtbl_Check *******************************************
Description : Check if the page table parameters are feasible, according to the
              parameters. This is only used in page table creation. The following
              restrictions apply:
1. PAE is not supported by this port: if you have that much memory, use the Cortex-A
   64-bit port instead. The translation scheme here is always:
   4GB VA -> {Supersection, Section} -> {Large Page, Small Page}.
   The supersection is described in ARMv7-AR ARM as optional, but is implemented on
   all known ARMv7-A processors.
   We only user TTBR0 here and we set value N to 0 as always. Using TTBR1 is nonsense
   in our context, as it gives little benefit.
2. The ARM page tables have idiosyncrasy that allow two page size mappings to exist
   in the same page table.
3. ARM page tables use repetition when it comes to supersections and large pages.
   This breaks our numorder-sizeorder assumption, and for this reason, we do not
   support these two page sizes explicitly. These two sizes are supported by kernel
   functions instead, and the kernel function always maps/unmaps the pages as needed
   without checking for any permissions; this can be viewed as a hack, and require
   some caution when we are mapping pages in.
Input       : rme_ptr_t Start_Addr - The start mapping address.
              rme_ptr_t Top_Flag - The top-level flag,
              rme_ptr_t Size_Order - The size order of the page directory.
              rme_ptr_t Num_Order - The number order of the page directory.
              rme_ptr_t Vaddr - The virtual address of the page directory.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Check(rme_ptr_t Start_Addr, rme_ptr_t Top_Flag,
                            rme_ptr_t Size_Order, rme_ptr_t Num_Order, rme_ptr_t Vaddr)
{
    /* Top-level - 1MB pages, 4096 entries, 16kB alignment */
    if(Top_Flag!=0)
    {
        if(Size_Order!=RME_PGTBL_SIZE_1M)
            return RME_ERR_PGT_OPFAIL;
        if(Num_Order!=RME_PGTBL_NUM_4K)
            return RME_ERR_PGT_OPFAIL;
        if((Vaddr&0x3FFF)!=0)
            return RME_ERR_PGT_OPFAIL;
    }
    /* Second-level - 1MB pages, 4096 entries, 16kB alignment */
    else
    {
        if(Size_Order!=RME_PGTBL_SIZE_64K)
            return RME_ERR_PGT_OPFAIL;
        if(Num_Order!=RME_PGTBL_NUM_1K)
            return RME_ERR_PGT_OPFAIL;
        if((Vaddr&0x3FF)!=0)
            return RME_ERR_PGT_OPFAIL;
    }

    return 0;
}
/* End Function:__RME_Pgtbl_Check ********************************************/

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

    /* Is this a first-level or a second-level? */
    if((Pgtbl_Op->Start_Addr&RME_PGTBL_TOP)!=0)
    {
    	/* First-level - clean up the first half and map in the second half as kernel entries */
		for(Count=0;Count<2048;Count++)
			Ptr[Count]=0;

		for(;Count<4096;Count++)
			Ptr[Count]=(&__RME_CAV7_Kern_Pgtbl)[Count];
    }
    else
    {
    	/* Second-level - just clean it up to all zeros */
		for(Count=0;Count<256;Count++)
			Ptr[Count]=0;
    }

    RME_CAV7_PGREG_POS(Ptr).Parent_Cnt=0;
    RME_CAV7_PGREG_POS(Ptr).ASID_Child_Cnt=0;
    return 0;
}
/* End Function:__RME_Pgtbl_Init *********************************************/

/* Begin Function:__RME_Pgtbl_Del_Check ***************************************
Description : Check if the page table can be deleted.
Input       : struct RME_Cap_Pgtbl Pgtbl_Op* - The capability to the page table to operate on.
Output      : None.
Return      : rme_ptr_t - If can be deleted, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Del_Check(struct RME_Cap_Pgtbl* Pgtbl_Op)
{
    rme_ptr_t* Table;

    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* See if this is a top-level. If yes, we only check child count */
    if((Pgtbl_Op->Start_Addr&RME_PGTBL_TOP)!=0)
    {
    	if(((RME_CAV7_PGREG_POS(Table).ASID_Child_Cnt)&0xFFFF)==0)
    		return 0;
    }
    else
    {
    	/* Must be the second-level. We only check parent count because it can't have child anymore */
    	if(RME_CAV7_PGREG_POS(Table).Parent_Cnt==0)
    	        return 0;
    }

    return RME_ERR_PGT_OPFAIL;
}
/* End Function:__RME_Pgtbl_Del_Check ****************************************/

/* Begin Function:__RME_Pgtbl_Page_Map ****************************************
Description : Map a page into the page table. This architecture requires that the
              mapping is always at least readable. The Cortex-A's super pages are
              not supported by this. Also, RME dfoesn't use the domain model; all
              page tables' domain is always client, which means that the permission
              flags will always be checked.
Input       : struct RME_Cap_Pgtbl* - The cap ability to the page table to operate on.
              rme_ptr_t Paddr - The physical address to map to. If we are unmapping,
                                this have no effect.
              rme_ptr_t Pos - The position in the page table.
              rme_ptr_t Flags - The RME standard page attributes. Need to translate
                                them into architecture specific page table's settings.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Page_Map(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags)
{
    rme_ptr_t* Table;
    rme_ptr_t CAV7_Flags;

    /* It should at least be readable */
    if((Flags&RME_PGTBL_READ)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we trying to map into the kernel space on the top level? */
    if(((Pgtbl_Op->Start_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=2048))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Generate flags */
    if(RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)==RME_PGTBL_SIZE_4K)
        CAV7_Flags=RME_CAV7_MMU_1M_PAGE_ADDR(Paddr)|RME_CAV7_PGFLG_1M_RME2NAT(Flags);
    else
        CAV7_Flags=RME_CAV7_MMU_4K_PAGE_ADDR(Paddr)|RME_CAV7_PGFLG_4K_RME2NAT(Flags);

    /* Try to map it in */
    if(RME_COMP_SWAP(&(Table[Pos]),0,CAV7_Flags)==0)
        return RME_ERR_PGT_OPFAIL;

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

    /* Are we trying to unmap the kernel space on the top level? */
    if(((Pgtbl_Op->Start_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=2048))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Make sure that there is something */
    Temp=Table[Pos];
    if(Temp==0)
        return RME_ERR_PGT_OPFAIL;

    /* Is this a page directory? We cannot unmap page directories like this */
    if((RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)!=RME_PGTBL_SIZE_4K)&&((Temp&RME_CAV7_MMU_1M_PGDIR_PRESENT)!=0))
        return RME_ERR_PGT_OPFAIL;

    /* Try to unmap it. Use CAS just in case */
    if(RME_COMP_SWAP(&(Table[Pos]),Temp,0)==0)
        return RME_ERR_PGT_OPFAIL;

    return 0;
}
/* End Function:__RME_Pgtbl_Page_Unmap ***************************************/

/* Begin Function:__RME_Pgtbl_Pgdir_Map ***************************************
Description : Map a page directory into the page table. It is surprising that
              Cortex-A (ARMv7) does not support any kind of page directory flags.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Parent - The parent page table.
              struct RME_Cap_Pgtbl* Pgtbl_Child - The child page table.
              rme_ptr_t Pos - The position in the destination page table.
              rme_ptr_t Flags - The RME standard flags for the child page table.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Pgdir_Map(struct RME_Cap_Pgtbl* Pgtbl_Parent, rme_ptr_t Pos,
                                struct RME_Cap_Pgtbl* Pgtbl_Child, rme_ptr_t Flags)
{
    rme_ptr_t* Parent_Table;
    rme_ptr_t* Child_Table;
    rme_ptr_t CAV7_Flags;

    /* It should at least be readable */
    if((Flags&RME_PGTBL_READ)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we trying to map into the kernel space on the top level? */
    if(((Pgtbl_Parent->Start_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=2048))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Parent_Table=RME_CAP_GETOBJ(Pgtbl_Parent,rme_ptr_t*);
    Child_Table=RME_CAP_GETOBJ(Pgtbl_Child,rme_ptr_t*);

    /* Generate the content */
    CAV7_Flags=RME_CAV7_MMU_1M_PGTBL_ADDR(RME_CAV7_VA2PA(Child_Table))|RME_CAV7_MMU_1M_PGDIR_PRESENT;

    /* Try to map it in - may need to increase some count */
    if(RME_COMP_SWAP(&(Parent_Table[Pos]),0,CAV7_Flags)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Map complete, increase reference count for both page tables */
    RME_FETCH_ADD((rme_ptr_t*)&(RME_CAV7_PGREG_POS(Child_Table).Parent_Cnt),1);
    RME_FETCH_ADD((rme_ptr_t*)&(RME_CAV7_PGREG_POS(Parent_Table).ASID_Child_Cnt),1);

    return 0;
}
/* End Function:__RME_Pgtbl_Pgdir_Map ****************************************/

/* Begin Function:__RME_Pgtbl_Pgdir_Unmap *************************************
Description : Unmap a page directory from the page table.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Op - The page table to operate on.
              rme_ptr_t Pos - The position in the page table.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Pgdir_Unmap(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos)
{
    rme_ptr_t* Parent_Table;
    rme_ptr_t* Child_Table;
    rme_ptr_t Temp;

    /* Are we trying to unmap the kernel space on the top level? */
    if(((Pgtbl_Op->Start_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=2048))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Parent_Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Make sure that there is something */
    Temp=Parent_Table[Pos];
    if(Temp==0)
        return RME_ERR_PGT_OPFAIL;

    /* Is this a page? We cannot unmap pages like this */
    if((RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)==RME_PGTBL_SIZE_4K)||((Temp&RME_CAV7_MMU_1M_PAGE_PRESENT)!=0))
        return RME_ERR_PGT_OPFAIL;

    Child_Table=(rme_ptr_t*)Temp;
    /* Try to unmap it. Use CAS just in case */
    if(RME_COMP_SWAP(&(Parent_Table[Pos]),Temp,0)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Decrease reference count */
    RME_FETCH_ADD((rme_ptr_t*)&(RME_CAV7_PGREG_POS(Child_Table).Parent_Cnt),-1);
    RME_FETCH_ADD((rme_ptr_t*)&(RME_CAV7_PGREG_POS(Parent_Table).ASID_Child_Cnt),-1);

    return 0;
}
/* End Function:__RME_Pgtbl_Pgdir_Unmap **************************************/

/* Begin Function:__RME_Pgtbl_Lookup ********************************************
Description : Lookup a page entry in a page directory. This function cannot deal
              with large pages and super sections - it is the user's responsibility
              to avoid this.
Input       : struct RME_Cap_Pgtbl* Pgtbl_Op - The page directory to lookup.
              rme_ptr_t Pos - The position to look up.
Output      : rme_ptr_t* Paddr - The physical address of the page.
              rme_ptr_t* Flags - The RME standard flags of the page.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Lookup(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Pos, rme_ptr_t* Paddr, rme_ptr_t* Flags)
{
    rme_ptr_t* Table;
    rme_ptr_t Temp;

    /* Check if the position is within the range of this page table */
    if((Pos>>RME_PGTBL_NUMORD(Pgtbl_Op->Size_Num_Order))!=0)
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);
    /* Get the position requested - atomic read */
    Temp=Table[Pos];

    /* Start lookup - is this a terminal page, or? */
    if(RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)==RME_PGTBL_SIZE_4K)
    {
        if((Temp&RME_CAV7_MMU_4K_PAGE_PRESENT)==0)
            return RME_ERR_PGT_OPFAIL;

        /* This is a small page. Return the physical address and flags */
        if(Paddr!=0)
        	*Paddr=RME_CAV7_MMU_4K_PAGE_ADDR(Temp);
        if(Flags!=0)
            *Flags=RME_CAV7_PGFLG_4K_NAT2RME(Temp);
    }
    else
    {
        if((Temp&RME_CAV7_MMU_1M_PAGE_PRESENT)==0)
            return RME_ERR_PGT_OPFAIL;

        /* This is a section. Return the physical address and flags */
        if(Paddr!=0)
        	*Paddr=RME_CAV7_MMU_1M_PAGE_ADDR(Temp);
        if(Flags!=0)
            *Flags=RME_CAV7_PGFLG_1M_NAT2RME(Temp);
    }

    return 0;
}
/* End Function:__RME_Pgtbl_Lookup *******************************************/

/* Begin Function:__RME_Pgtbl_Walk ********************************************
Description : Walking function for the page table. This function just does page
              table lookups. The page table that is being walked must be the top-
              level page table. The output values are optional; only pass in pointers
              when you need that value.
              Walking kernel page tables is prohibited.
              This function also does not support large pages and super sections.
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
                           rme_ptr_t* Map_Vaddr, rme_ptr_t* Paddr, rme_ptr_t* Size_Order, rme_ptr_t* Num_Order, rme_ptr_t* Flags)
{
    rme_ptr_t* Table;
    rme_ptr_t Pos;
    rme_ptr_t Temp;

    /* Check if this is the top-level page table */
    if(((Pgtbl_Op->Start_Addr)&RME_PGTBL_TOP)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we attempting a kernel or non-canonical lookup? If yes, stop immediately */
    if(Vaddr>=0x7FFFFFFFU)
        return RME_ERR_PGT_OPFAIL;

    /* Get the table and start lookup - We know that there are only 2 levels */
    Table=RME_CAP_GETOBJ(Pgtbl_Op, rme_ptr_t*);

	/* Calculate where is the entry - always 0 to 4096 */
	Pos=(Vaddr>>RME_PGTBL_SIZE_1M)&0xFFFU;
	/* Atomic read */
	Temp=Table[Pos];

	/* Find the position of the entry - Is there a page, a directory, or nothing? */
	if((Temp&RME_CAV7_MMU_1M_PAGE_PRESENT)!=0)
	{
		/* There is a 1M page */
		if(Pgtbl!=0)
			*Pgtbl=(rme_ptr_t)Table;
		if(Map_Vaddr!=0)
			*Map_Vaddr=RME_ROUND_DOWN(Vaddr,RME_PGTBL_SIZE_1M);
		if(Paddr!=0)
			*Paddr=RME_CAV7_MMU_1M_PAGE_ADDR(Temp);
		if(Size_Order!=0)
			*Size_Order=RME_PGTBL_SIZE_1M;
		if(Num_Order!=0)
			*Num_Order=RME_PGTBL_NUM_4K;
		if(Flags!=0)
			*Flags=RME_CAV7_PGFLG_1M_NAT2RME(Temp);

	}
	else if((Temp&RME_CAV7_MMU_1M_PGDIR_PRESENT)!=0)
	{
		Table=(rme_ptr_t*)RME_CAV7_PA2VA(RME_CAV7_MMU_1M_PGTBL_ADDR(Temp));
		/* Calculate where is the entry - always 0 to 256 */
		Pos=(Vaddr>>RME_PGTBL_SIZE_4K)&0xFFU;
		/* Atomic read */
		Temp=Table[Pos];

		if((Temp&RME_CAV7_MMU_4K_PAGE_PRESENT)!=0)
		{
			/* There is a 4k page */
			if(Pgtbl!=0)
				*Pgtbl=(rme_ptr_t)Table;
			if(Map_Vaddr!=0)
				*Map_Vaddr=RME_ROUND_DOWN(Vaddr,RME_PGTBL_SIZE_4K);
			if(Paddr!=0)
				*Paddr=RME_CAV7_MMU_4K_PAGE_ADDR(Temp);
			if(Size_Order!=0)
				*Size_Order=RME_PGTBL_SIZE_4K;
			if(Num_Order!=0)
				*Num_Order=RME_PGTBL_NUM_256;
			if(Flags!=0)
				*Flags=RME_CAV7_PGFLG_4K_NAT2RME(Temp);
		}
		else
			return RME_ERR_PGT_OPFAIL;
	}
	else
		return RME_ERR_PGT_OPFAIL;

    return 0;
}
/* End Function:__RME_Pgtbl_Walk *********************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
