/******************************************************************************
Filename    : benchmark.c
Author      : pry
Date        : 04/09/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The benchmark file for RME.
******************************************************************************/

/* Defines *******************************************************************/
/* Types */
typedef signed int  s32;
typedef signed short s16;
typedef signed char  s8;
typedef unsigned int  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef s32 tid_t;
typedef u32 ptr_t;
typedef s32 cnt_t;
typedef s32 cid_t;
typedef s32 ret_t;

#define BENCHMARK_STACK_SIZE 4096
/* System service stub */
#define RME_CAP_OP(OP,CAPID,ARG1,ARG2,ARG3) RME_Svc(((OP)<<(sizeof(ptr_t)*4)|(CAPID)),ARG1,ARG2,ARG3)
#define RME_PARAM_D_MASK                    (((ptr_t)(-1))>>(sizeof(ptr_t)*4))
#define RME_PARAM_Q_MASK                    (((ptr_t)(-1))>>(sizeof(ptr_t)*6))
#define RME_PARAM_O_MASK                    (((ptr_t)(-1))>>(sizeof(ptr_t)*7))
/* The parameter passing - not to be confused with kernel macros. These macros just place the parameters */
#define RME_PARAM_D1(X)                     (((X)&RME_PARAM_D_MASK)<<(sizeof(ptr_t)*4))
#define RME_PARAM_D0(X)                     ((X)&RME_PARAM_D_MASK)

#define RME_PARAM_Q3(X)                     (((X)&RME_PARAM_Q_MASK)<<(sizeof(ptr_t)*6))
#define RME_PARAM_Q2(X)                     (((X)&RME_PARAM_Q_MASK)<<(sizeof(ptr_t)*4))
#define RME_PARAM_Q1(X)                     (((X)&RME_PARAM_Q_MASK)<<(sizeof(ptr_t)*2))
#define RME_PARAM_Q0(X)                     ((X)&RME_PARAM_Q_MASK)

#define RME_PARAM_O7(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*7))
#define RME_PARAM_O6(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*6))
#define RME_PARAM_O5(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*5))
#define RME_PARAM_O4(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*4))
#define RME_PARAM_O3(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*3))
#define RME_PARAM_O2(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*2))
#define RME_PARAM_O1(X)                     (((X)&RME_PARAM_O_MASK)<<(sizeof(ptr_t)*1))
#define RME_PARAM_O0(X)                     ((X)&RME_PARAM_O_MASK)

/* Initial boot capabilities - This should be in accordnace with the kernel settings */
/* The capability table of the init process */
#define RME_BOOT_CAPTBL          0
/* The top-level page table of the init process - always 4GB full range split into 8 pages */
#define RME_BOOT_PGTBL           1
/* The top-level page table of the init process's SRAM */
#define RME_BOOT_PGTBL_SRAM      2
/* The init process */
#define RME_BOOT_INIT_PROC       3
/* The init thread */
#define RME_BOOT_INIT_THD        4
/* The initial kernel function capability */
#define RME_BOOT_INIT_KERN       5
/* The initial kernel memory capability */
#define RME_BOOT_INIT_KMEM       6
/* The initial timer endpoint */
#define RME_BOOT_INIT_TIMER      7

/* The test objects */
#define RME_BOOT_BENCH_THD       8
#define RME_BOOT_BENCH_PGTBL_TOP 9
#define RME_BOOT_BENCH_PGTBL_SRAM 10

/* Need to export the memory frontier! */
/* Need to export the flags as well ! */
/* Export the errno too */
#define RME_BOOT_BENCH_KMEM_FRONTIER 0x10005000

/* The stack safe size */
#define RME_STACK_SAFE_SIZE 16

#define RME_TSC() TIM2->CNT

/* Need to export the system priority limit! */
struct RME_CMX_Ret_Stack
{
    /* Normal registers */
    ptr_t R0;
    ptr_t R1;
    ptr_t R2;
    ptr_t R3;
    ptr_t R12;
    ptr_t LR;
    ptr_t PC;
    ptr_t XPSR;
    /* FPU registers follow - no matter they are used or not, we reserve the space
     * in stack creation */
    ptr_t S0;
    ptr_t S1;
    ptr_t S2;
    ptr_t S3;
    ptr_t S4;
    ptr_t S5;
    ptr_t S6;
    ptr_t S7;
    ptr_t S8;
    ptr_t S9;
    ptr_t S10;
    ptr_t S11;
    ptr_t S12;
    ptr_t S13;
    ptr_t S14;
    ptr_t S15;
};
/* End Defines ***************************************************************/

/* Includes ******************************************************************/
#include "RME.h"
#include "stm32f4xx.h"
/* Need to export error codes, and size of each object, in words! */
/* End Includes **************************************************************/

/* Private Variables *********************************************************/
/* The stack of the threads - enough for 4 threads */
ptr_t RME_Stack[2048];
u16 Time[10000];
s8 RME_Bench_Buf[1024];
/* End Private Variables *****************************************************/

/* Function Prototypes *******************************************************/
extern ret_t RME_Svc(ptr_t Svc_Capid,ptr_t Param1, ptr_t Param2, ptr_t Param3);
extern cnt_t RME_Sprint_Uint(s8* Buffer,u32 Arg_Int);
extern void RME_Thd_Stub(void);
extern void RME_Inv_Stub(void);
ptr_t _RME_Stack_Init(ptr_t Stack, ptr_t Stub, ptr_t Param1, ptr_t Param2, ptr_t Param3, ptr_t Param4);
void RME_Benchmark(void);
void RME_Same_Proc_Thd_Switch_Test_Thd(ptr_t Param1, ptr_t Param2, ptr_t Param3, ptr_t Param4);
void RME_Same_Proc_Thd_Switch_Test(void);
/* End Function Prototypes ***************************************************/

/* Begin Function:_RME_Tsc_Init ***********************************************
Description : The initialization of timestamp counter. 19 secs before overflowing.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _RME_Tsc_Init(void)
{
//    TIM_HandleTypeDef TIM2_Handle;
//    
//    /* Initialize timer 2 to run at the same speed as the CPU */
//    TIM2_Handle.Instance=TIM2;
//    TIM2_Handle.Init.Prescaler=0;
//    TIM2_Handle.Init.CounterMode=TIM_COUNTERMODE_UP;
//    TIM2_Handle.Init.Period=(ptr_t)(-1);
//    TIM2_Handle.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
//    HAL_TIM_Base_Init(&TIM2_Handle);
//    __HAL_RCC_TIM2_CLK_ENABLE();
//    __HAL_TIM_ENABLE(&TIM2_Handle);
}
/* End Function:_RME_Tsc_Init ************************************************/

/* Begin Function:_RME_Stack_Init *********************************************
Description : The thread's stack initializer, initializes the thread's stack.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
ptr_t _RME_Stack_Init(ptr_t Stack, ptr_t Stub, ptr_t Param1, ptr_t Param2, ptr_t Param3, ptr_t Param4)
{
    struct RME_CMX_Ret_Stack* Stack_Ptr;
    
    Stack_Ptr=(struct RME_CMX_Ret_Stack*)(Stack-RME_STACK_SAFE_SIZE-sizeof(struct RME_CMX_Ret_Stack));
    Stack_Ptr->R0=Param1;
    Stack_Ptr->R1=Param2;
    Stack_Ptr->R2=Param3;
    Stack_Ptr->R3=Param4;
    Stack_Ptr->R12=0;
    Stack_Ptr->LR=0;
    Stack_Ptr->PC=Stub;
    /* Initialize the xPSR to avoid a transition to ARM state */
    Stack_Ptr->XPSR=0x01000200;
    
    return (ptr_t)Stack_Ptr;
}
/* End Function:_RME_Stack_Init **********************************************/

/* Begin Function:RME_Same_Proc_Thd_Switch_Test_Thd ***************************
Description : The thread for testing same-process thread switching performance.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void RME_Same_Proc_Thd_Switch_Test_Thd(ptr_t Param1, ptr_t Param2, ptr_t Param3, ptr_t Param4)
{
    ret_t Retval;
    /* Now we switch back to the init thread, immediately */
    while(1)
    {
        Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                          RME_BOOT_INIT_THD,
                          0,
                          0);
    }
}
/* End Function:RME_Same_Proc_Thd_Switch_Test_Thd ****************************/

/* Begin Function:RME_Same_Proc_Thd_Switch_Test *******************************
Description : The same-process thread switch test code.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void RME_Same_Proc_Thd_Switch_Test(void)
{
    /* Intra-process thread switching time */
    ret_t Retval;
    cnt_t Count;
    ptr_t Stack_Addr;
    ptr_t Temp;
    /* Initialize the thread's stack before entering it */
    Stack_Addr=_RME_Stack_Init((ptr_t)(&RME_Stack[2047]),
                               (ptr_t)RME_Thd_Stub,
                               1, 2, 3, 4);
    /* There are still many bugs in the kernel. Need a white-box test to guarantee
     * that it is free of bugs. Find a scheme to do that */
    Retval=RME_CAP_OP(RME_SVC_THD_CRT,RME_BOOT_CAPTBL,
                      RME_PARAM_D1(RME_BOOT_INIT_KMEM)|RME_PARAM_D0(RME_BOOT_BENCH_THD),
                      RME_PARAM_D1(RME_BOOT_INIT_PROC)|RME_PARAM_D0(31),
                      RME_BOOT_BENCH_KMEM_FRONTIER);
    
    /* Bind the thread to the processor */
    Retval=RME_CAP_OP(RME_SVC_THD_SCHED_BIND,0,
                      RME_BOOT_BENCH_THD,
                      RME_BOOT_INIT_THD,
                      0);
    
    /* Set the execution information */
    Retval=RME_CAP_OP(RME_SVC_THD_EXEC_SET,0,
                      RME_BOOT_BENCH_THD,
                      (ptr_t)RME_Same_Proc_Thd_Switch_Test_Thd,
                      Stack_Addr);
                      
    /* Delegate some timeslice to it */
    Retval=RME_CAP_OP(RME_SVC_THD_TIME_XFER,0,
                      RME_BOOT_BENCH_THD,
                      RME_BOOT_INIT_THD,
                      10000000);
    
    /* Try to switch to that thread - should fail */
    Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                      RME_BOOT_BENCH_THD,
                      0,
                      0);
    /* Test result: intra-process ctxsw 358cycles/1.657us, frt w/mpu 163cycles/0.754us,
    * composite 324. opted max:323
    * all:33.0
    * empty: 4.09 - 0.409us, most time spent on internals
    * w/selections: 7.15 - maybe no need to check frozen cap from the proc.
    * w/checkings:10.926 - 317us.
    * total:16.57
    * 16.2us now, after cleaning up two bad things
    * 14.7us after CPUID optimizations. The quiescence hardly worked.
    * no cache - 3 times slower, mainly due to the flash. ART does not really help.
    * Performance cannot be further optimized anymore without compiler intrinsics.
    * Something terribly wrong with systick. 38 second wrapwround
    * This configuration, CPU works at 216MHz, correct, but the 
    * The TSC is always 8 cycles between reads.
    */
    _RME_Tsc_Init();
    for(Count=0;Count<10000;Count++)
    {
        Temp=RME_TSC();
        Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                          RME_BOOT_BENCH_THD,
                          0,
                          0);
        Temp=RME_TSC()-Temp;
        Time[Count]=Temp-8;
    }
    
    while(1);
}
/* End Function:RME_Same_Proc_Thd_Switch_Test ********************************/

/* Begin Function:RME_Diff_Proc_Thd_Switch_Test_Thd ***************************
Description : The thread for testing same-process thread switching performance.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void RME_Diff_Proc_Thd_Switch_Test_Thd(ptr_t Param1, ptr_t Param2, ptr_t Param3, ptr_t Param4)
{
    ret_t Retval;
    /* Now we switch back to the init thread, immediately */
    while(1)
    {
        Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                          RME_BOOT_INIT_THD,
                          0,
                          0);
    }
}
/* End Function:RME_Diff_Proc_Thd_Switch_Test_Thd ****************************/

/* Begin Function:RME_Diff_Proc_Thd_Switch_Test *******************************
Description : The same-process thread switch test code.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void RME_Diff_Proc_Thd_Switch_Test(void)
{
    /* Intra-process thread switching time */
    ret_t Retval;
    cnt_t Count;
    ptr_t Stack_Addr;
    ptr_t Temp;
    ptr_t Frontier;
    
    Frontier=RME_BOOT_BENCH_KMEM_FRONTIER;
    /* Initialize the thread's stack before entering it */
    Stack_Addr=_RME_Stack_Init((ptr_t)(&RME_Stack[2047]),
                               (ptr_t)RME_Thd_Stub,
                               1, 2, 3, 4);
    
    /* Create the page table for the whole address space range */
    Retval=RME_CAP_OP(RME_SVC_PGTBL_CRT,RME_BOOT_CAPTBL,
                      RME_PARAM_D1(RME_BOOT_INIT_KMEM)|RME_PARAM_Q1(RME_BOOT_BENCH_PGTBL_TOP)|
                      RME_PARAM_O1(29)|RME_PARAM_O0(3),
                      Frontier,
                      1);
//    Frontier+=;
//    /* Create the page table for the SRAM range */
//    Retval=RME_CAP_OP(RME_SVC_PGTBL_CRT,RME_BOOT_CAPTBL,
//                      RME_PARAM_D1(RME_BOOT_INIT_KMEM)|RME_PARAM_Q1(RME_BOOT_BENCH_PGTBL_SRAM)|
//                      RME_PARAM_O1(16)|RME_PARAM_O0(3),
//                      Frontier,
//                      0x20000001);
//    Frontier+=;
//    /* Map the pages into the top-level and the second-level */
//    RME_CAP_OP(RME_SVC_PGTBL_ADD,0,
//               RME_PARAM_Q1(RME_BOOT_BENCH_PGTBL_TOP)|0,
//               RME_PARAM_D1(RME_BOOT_PGTBL)|0,
//               |0)
                      
                  
    
    
    
    Retval=RME_CAP_OP(RME_SVC_THD_CRT,RME_BOOT_CAPTBL,
                      RME_PARAM_D1(RME_BOOT_INIT_KMEM)|RME_PARAM_D0(RME_BOOT_BENCH_THD),
                      RME_PARAM_D1(RME_BOOT_INIT_PROC)|RME_PARAM_D0(31),
                      RME_BOOT_BENCH_KMEM_FRONTIER);
    
    /* Bind the thread to the processor */
    Retval=RME_CAP_OP(RME_SVC_THD_SCHED_BIND,0,
                      RME_BOOT_BENCH_THD,
                      RME_BOOT_INIT_THD,
                      0);
    
    /* Set the execution information */
    Retval=RME_CAP_OP(RME_SVC_THD_EXEC_SET,0,
                      RME_BOOT_BENCH_THD,
                      (ptr_t)RME_Same_Proc_Thd_Switch_Test_Thd,
                      Stack_Addr);
                      
    /* Delegate some timeslice to it */
    Retval=RME_CAP_OP(RME_SVC_THD_TIME_XFER,0,
                      RME_BOOT_BENCH_THD,
                      RME_BOOT_INIT_THD,
                      10000000);
    
    /* Try to switch to that thread - should fail */
    Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                      RME_BOOT_BENCH_THD,
                      0,
                      0);
    /* Test result: intra-process ctxsw 358cycles/1.657us, frt w/mpu 163cycles/0.754us,
    * composite 324. opted max:323
    * all:33.0
    * empty: 4.09 - 0.409us, most time spent on internals
    * w/selections: 7.15 - maybe no need to check frozen cap from the proc.
    * w/checkings:10.926 - 317us.
    * total:16.57
    * 16.2us now, after cleaning up two bad things
    * 14.7us after CPUID optimizations. The quiescence hardly worked.
    * no cache - 3 times slower, mainly due to the flash. ART does not really help.
    * Performance cannot be further optimized anymore without compiler intrinsics.
    * Something terribly wrong with systick. 38 second wrapwround
    * This configuration, CPU works at 216MHz, correct, but the 
    * The TSC is always 8 cycles between reads.
    */
    _RME_Tsc_Init();
    for(Count=0;Count<10000;Count++)
    {
        Temp=RME_TSC();
        Retval=RME_CAP_OP(RME_SVC_THD_SWT,0,
                          RME_BOOT_BENCH_THD,
                          0,
                          0);
        Temp=RME_TSC()-Temp;
        Time[Count]=Temp-8;
    }
    
    while(1);
}
/* End Function:RME_Diff_Proc_Thd_Switch_Test ********************************/

/* Begin Function:RME_Benchmark ***********************************************
Description : The benchmark entry, also the init thread.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void RME_Benchmark(void)
{
    RME_Same_Proc_Thd_Switch_Test();
}
/* End Function:RME_Benchmark ************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
