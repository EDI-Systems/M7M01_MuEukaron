;/*****************************************************************************
;Filename    : platform_c66x.s
;Author      : pry
;Date        : 19/01/2017
;License     : The Unlicense; see LICENSE for details.
;Description : The C66X assembly support of the RME RTOS.
;*****************************************************************************/

;/* The TMS320C66X Structure **************************************************
;A0-A31:General purpose register file A. A0-A9 are caller save.
;B0-B31:General purpose register file B. B0-B9 are caller save.
;E1PC:READ-ONLY PC pointer.
;*****************************************************************************/

;/* Begin Stacks *************************************************************/
    .data
    ;The kernel stack segment is 64k-aligned as always - performed by linker script
    ;.align              65536
__RME_C66X_Stack:
    .space              16384
__TI_STACK_END:
    .space              16384*7
    ;Kernel SP storage array also 64k-aligned, 32 bytes. This is not the actual stack.
    .sect               ".stack"
__RME_C66X_Stack_Addr:
    .space              4*8
;/* End Stacks ***************************************************************/
            
;/* Begin Header *************************************************************/
    .text
    .align              2
;/* End Header ***************************************************************/

;/* Begin Exports ************************************************************/
    ;Disable all interrupts
    .global             __RME_Disable_Int
    ;Enable all interrupts
    .global             __RME_Enable_Int
    ;Wait until interrupts happen
    .global             __RME_C66X_Idle
    ;Memory fencing
    .global             __RME_C66X_Read_Acquire
    .global             __RME_C66X_Write_Release
    ;Get the MSB in a word
    .global             __RME_C66X_MSB_Get
    ;Get the core ID
    .global             __RME_C66X_CPUID_Get
    ;Kernel main function wrapper
    .global             _RME_Kmain
    ;The entry point for all other CPUs than the booting one.
    .global             _RME_C66X_SMP_Kmain
    ;Entering of the user mode
    .global             __RME_Enter_User_Mode
    ;The MPU setup routine
    .global             ___RME_C66X_MPU_Set
    ;Save coprocessor context
    .global             ___RME_C66X_Thd_Cop_Save
    ;Restore coprocessor context
    .global             ___RME_C66X_Thd_Cop_Restore
    ;Core-local data and stack
    .global             __RME_C66X_Stack
    .global             __RME_C66X_Stack_Addr
    ;TI's stack definition - their CRT assumes this
    .global             __TI_STACK_END
    ;Vector table start address
    .global             __RME_C66X_Vector_Table
    ;Write ISTP
    .global             __RME_C66X_Set_ISTP
    ;Get EFR
    .global             __RME_C66X_Get_EFR
    ;Set ECR
    .global             __RME_C66X_Set_ECR
    ;Set IERR
    .global             __RME_C66X_Get_IERR
;/* End Exports **************************************************************/

;/* Begin Imports ************************************************************/
    ;The kernel entry of RME. This will be defined in C language.
    .global             RME_Kmain
    ;The kernel entry of non-booting processors. This will be defined in C language.
    .global             __RME_SMP_Kmain
    ;The generic interrupt handler for all other vectors. System calls will also
    ;be called through this. This is a C66X idiosyncrasy.
    .global             __RME_C66X_Generic_Handler
;/* End Imports **************************************************************/

;/* Begin Vector Table *******************************************************/
    ;TI assembler alignment is in bytes
    .align              1024
    ;Vector table for the main core only
__RME_C66X_Vector_Table:
    ;Reset vector - not actually used
    BNOP                __RME_C66X_Vector_Table,5
    ;NMI vector - used for all interrupts and exceptions
    .align              32
    BNOP                SVC_Handler,5
    ;2 reserved vectors
    .align              32
    BNOP                __RME_C66X_Vector_Table,5
    .align              32
    BNOP                __RME_C66X_Vector_Table,5    
    ;Interrupt vector 4-15 - all of them will directly return
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
    .align              32
    B                   IRP
;/* End Vector Table *********************************************************/

;/* Begin Memory Init ********************************************************/

;/* End Memory Init **********************************************************/

;/* Begin Handlers ***********************************************************/

;/* End Handlers *************************************************************/

;/* Begin Function:_RME_C66X_SMP_Kmain ****************************************
;Description    : The entry address of the kernel for non-booting SMP processors.
;                 Never returns. This function must be aligned at at least 2^9,
;                 as this is required by the architecture.
;Input          : None.
;Output         : None.
;Return         : None.
;*****************************************************************************/
    .align              1024
_RME_C66X_SMP_Kmain:
    MVC                 DNUM,B15
    SHL                 B15,2,B15
    ;This means that the kernel per-core SP storage area must be 64k-aligned
    MVKH                __RME_C66X_Stack_Addr,B15
    LDW                 *B15,B15
    CALLP.S2            __RME_SMP_Kmain,B3
;/* End Function:_RME_C66X_SMP_Kmain *****************************************/

;/* Begin Function:__RME_Disable_Int ******************************************
;Description : The function for disabling all interrupts.
;Input       : None.
;Output      : None.    
;Return      : None.                                  
;*****************************************************************************/    
__RME_Disable_Int:
    DINT
    BNOP                B3,5
;/* End Function:__RME_Disable_Int *******************************************/

;/* Begin Function:__RME_Enable_Int *******************************************
;Description : The function for enabling all interrupts.
;Input       : None.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_Enable_Int:
    RINT
    BNOP                B3,5
;/* End Function:__RME_Enable_Int ********************************************/

;/* Begin Function:__RME_C66X_Idle ********************************************
;Description : Wait until a new interrupt comes, to save power.
;Input       : None.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_C66X_Idle:
    IDLE
    BNOP                B3,5
;/* End Function:__RME_C66X_Idle *********************************************/

;/* Begin Function:__RME_C66X_Set_ISTP ****************************************
;Description : Load the interrupt vector table.
;Input       : rme_ptr_t A4 - The address to load into the register.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_C66X_Set_ISTP:
    MVC                 A4,ISTP
    BNOP                B3,5
;/* End Function:__RME_C66X_Set_ISTP *****************************************/

;/* Begin Function:__RME_C66X_Get_EFR *****************************************
;Description : Get the EFR and attempt to identify the exception cause.
;Input       : None.
;Output      : None.
;Return      : A4 - The returned EFR content.
;*****************************************************************************/
__RME_C66X_Get_EFR:
    MVC                 EFR,B4
    MV                  B4,A4
    BNOP                B3,5
;/* End Function:__RME_C66X_Get_EFR ******************************************/

;/* Begin Function:__RME_C66X_Set_ECR *****************************************
;Description : Set the ECR and clear the cause of the exception.
;Input       : rme_ptr_t A4 - The ECR content to set.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_C66X_Set_ECR:
    MVC                 A4,ECR
    BNOP                B3,5
;/* End Function:__RME_C66X_Set_ECR ******************************************/

;/* Begin Function:__RME_C66X_Get_IERR *****************************************
;Description : Get the IERR and attempt to identify the error cause.
;Input       : None.
;Output      : None.
;Return      : A4 - The returned IERR content.
;*****************************************************************************/
__RME_C66X_Get_IERR:
    MVC                 IERR,B4
    MV                  B4,A4
    BNOP                B3,5
;/* End Function:__RME_C66X_Get_IERR *****************************************/

;/* Begin Function:__RME_C66X_Read_Acquire ************************************
;Description : Load acquire - no operation will begin until this load finishes.
;Input       : rme_ptr_t* A4 - The address to load from.
;Output      : None.
;Return      : rme_ptr_t A4 - The value loaded.
;*****************************************************************************/
__RME_C66X_Read_Acquire:
    LDW                 *A4,A4
    MFENCE
    BNOP                B3,5
;/* End Function:__RME_C66X_Read_Acquire *************************************/

;/* Begin Function:__RME_C66X_Write_Release ***********************************
;Description : Write release - all operations will finish before this write begins.
;Input       : rme_ptr_t* A4 - The address to write to.
;              rme_ptr_t B4 - The value to write.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_C66X_Write_Release:
    MFENCE
    STW                 B4,*A4
    BNOP                B3,5
;/* End Function:__RME_C66X_Write_Release ************************************/

;/* Begin Function:_RME_Kmain *************************************************
;Description    : The entry address of the kernel. Never returns.
;Input          : rme_ptr_t Stack - The stack address to set SP to.
;Output         : None.
;Return         : None.
;*****************************************************************************/
_RME_Kmain:
    MV                  A4,B15
    CALLP.S2            RME_Kmain,B3
;/* End Function:_RME_Kmain **************************************************/

;/* Begin Function:__RME_C66X_MSB_Get *****************************************
;Description    : Get the MSB of the word.
;Input          : rme_ptr_t Val - The value.
;Output         : None.
;Return         : rme_ptr_t - The MSB position.
;*****************************************************************************/
__RME_C66X_MSB_Get:
    MVK                 1,B4
    LMBD                B4,A4,B4
    MVK                 31,A4
    SUB                 A4,B4,A4
    B                   B3
;/* End Function:__RME_C66X_MSB_Get ******************************************/

;/* Begin Function:__RME_C66X_CPUID_Get ***************************************
;Description    : Get the CPUID of the processor.
;Input          : None.
;Output         : None.
;Return         : rme_ptr_t - The MSB position.
;*****************************************************************************/
__RME_C66X_CPUID_Get:
    MVC                 DNUM,B4
    MV                  B4,A4
||  B                   B3
;/* End Function:__RME_C66X_CPUID_Get ****************************************/

;/* Begin Function:__RME_Enter_User_Mode **************************************
;Description : Entering of the user mode, after the system finish its preliminary
;              booting. The function shall never return. This function should only
;              be used to boot the first process in the system. C66X allows user-level
;              to disable interrupts; this is too bad. Luckily, we can redirect all
;              interrupts to the EXCEP vector, which is not influenced by user-level 
;              GIE setting.
;Input       : A4 - The user execution startpoint.
;              B4 - The user stack.
;              A6 - The CPUID.
;Output      : None.
;Return      : None.
;*****************************************************************************/
__RME_Enter_User_Mode:
    ;Fill stack pointer
    MV                  B4,B15
    ;Fill return address
||  MVC                 A4,NRP
    ;Fill in TSR
||  MVK                 2,B3
    MVC                 B3,NTSR
||  MV                  A6,A4
    B                   NRP
;/* End Function:__RME_Enter_User_Mode ***************************************/

;/* Begin Function:___RME_C66X_Thd_Cop_Save ***********************************
;Description : Save coprocessor context.
;Input       : None.
;Output      : A4 - The pointer to the structure.
;Return      : None.
;*****************************************************************************/
___RME_C66X_Thd_Cop_Save:
    MVC                 AMR,B4
    MVC                 GFPGFR,B5
||  STW                 B4,*+A4[0]
    MVC                 GPLYA,B4
||  STW                 B5,*+A4[1]
    MVC                 GPLYB,B5
||  STW                 B4,*+A4[2]
    MVC                 FADCR,B4
||  STW                 B5,*+A4[3]
    MVC                 FAUCR,B5
||  STW                 B4,*+A4[4]
    MVC                 FMCR,B4
||  STW                 B5,*+A4[5]
    STW                 B4,*+A4[6]
||  B                   B3
;/* End Function:___RME_C66X_Thd_Cop_Save ************************************/

;/* Begin Function:___RME_C66X_Thd_Cop_Restore ********************************
;Description : Save coprocessor context.
;Input       : A4 - The pointer to the structure.
;Output      : None.
;Return      : None.
;*****************************************************************************/
___RME_C66X_Thd_Cop_Restore:
    LDW                 *+A4[0],B4
    LDW                 *+A4[1],B5
||  MVC                 B4,AMR
    LDW                 *+A4[2],B4
||  MVC                 B5,GFPGFR
    LDW                 *+A4[3],B5
||  MVC                 B4,GPLYA
    LDW                 *+A4[4],B4
||  MVC                 B5,GPLYB
    LDW                 *+A4[5],B5
||  MVC                 B4,FADCR
    LDW                 *+A4[6],B4
||  MVC                 B5,FAUCR
    MVC                 B4,FMCR
    B                   B3
;/* End Function:___RME_C66X_Thd_Cop_Restore *********************************/

;/* Begin Function:SVC_Handler ************************************************
;Description : The SVC handler routine. This will in fact call a C function to
;              resolve the system service routines. This will be the entry of all
;              stuff: system calls, exceptions, and NMI. We need to distinguish what
;              had happened when we enter the handler. C6000 does not even provide
;              us with a way to automatically switch kernel pointers on entry. 
;              Luckily, we have another register - REP available, and we are not
;              using it at all in RME: we will use this to store the SP. C6678 uses
;              an empty descending stack.
;Input       : None.
;Output      : None.
;Return      : None.
;*****************************************************************************/
SVC_Handler:
    MVC                 B15,REP
    MVC                 DNUM,B15
    SHL                 B15,2,B15
    ;This means that the kernel per-core SP storage area must be 64k-aligned
    MVKH                __RME_C66X_Stack_Addr,B15
    LDW                 *B15,B15
    ;Push A15,A14,B15,B14 to stack, which we later use as a pushing pair. No parallelism
    STDW                A15:A14,*B15--
    MV                  B15,A15
    MVC                 REP,B15
    STDW                B15:B14,*A15--
    MV                  A15,B15
    ;Leave the space for pushing registers
    SUBAW               A15,30,A15
    ;We are now on kernel stack. Each pair is parallel
    STDW                A31:A30,*A15--
||  STDW                B31:B30,*B15--
    STDW                A29:A28,*A15--
||  STDW                B29:B28,*B15--
    ;Return PC pointer
||  MVC                 NRP,B31
    STDW                A27:A26,*A15--
||  STDW                B27:B26,*B15--
    ;Saturation flags
||  MVC                 SSR,B30
    STDW                A25:A24,*A15--
||  STDW                B25:B24,*B15--
||  MVC                 CSR,B29
    STDW                A23:A22,*A15--
||  STDW                B23:B22,*B15--
    ;Hardware loop related
||  MVC                 ILC,B28
    STDW                A21:A20,*A15--
||  STDW                B21:B20,*B15--
    ;Hardware loop related
||  MVC                 RILC,B27
    STDW                A19:A18,*A15--
||  STDW                B19:B18,*B15--
    ;Task state register
||  MVC                 NTSR,B26
    STDW                A17:A16,*A15--
||  STDW                B17:B16,*B15--
||  MV                  B31:B30,A31:A30
    STDW                A13:A12,*A15--
||  STDW                B13:B12,*B15--
||  MV                  B29:B28,A29:A28
    STDW                A11:A10,*A15--
||  STDW                B11:B10,*B15--
||  MV                  B27:B26,A27:A26
    STDW                A9:A8,*A15--
||  STDW                B9:B8,*B15--
    STDW                A7:A6,*A15--
||  STDW                B7:B6,*B15--
    STDW                A5:A4,*A15--
||  STDW                B5:B4,*B15--
    STDW                A3:A2,*A15--
||  STDW                B3:B2,*B15--
    STDW                A1:A0,*A15--
||  STDW                B1:B0,*B15--
    ;Now A31:A30 contains NRP:SSR,A29:A28 contains CSR:ILC, A27:A26 contains NTSR:RILC
    STDW                A31:A30,*A15--
    STDW                A29:A28,*A15--
    STDW                A27:A26,*A15--
    
    ;Assign the correct stack pointer position to SP
    MV                  A15,B15
    ;Pass the pointer to the data structure to the handler 
    SUB                 A15,4,A4
    CALLP.S2            __RME_C66X_Generic_Handler,B3
    MV                  B15,A15
     
    ;Load the important registers back
    LDDW                *++A15,A27:A26
    LDDW                *++A15,A29:A28
    LDDW                *++A15,A31:A30
    ;Pop all general purpose registers
    LDDW                *++A15,A1:A0
||  LDDW                *++B15,B1:B0
    LDDW                *++A15,A3:A2
||  LDDW                *++B15,B3:B2
    LDDW                *++A15,A5:A4
||  LDDW                *++B15,B5:B4
    LDDW                *++A15,A7:A6
||  LDDW                *++B15,B7:B6
    LDDW                *++A15,A9:A8
||  LDDW                *++B15,B9:B8
    LDDW                *++A15,A11:A10
||  LDDW                *++B15,B11:B10
||  MV                  A27:A26,B27:B26
    LDDW                *++A15,A13:A12
||  LDDW                *++B15,B13:B12
||  MV                  A29:A28,B29:B28
    LDDW                *++A15,A17:A16
||  LDDW                *++B15,B17:B16
||  MV                  A31:A30,B31:B30
    ;Restore some registers
    LDDW                *++A15,A19:A18
||  LDDW                *++B15,B19:B18
    ;Task state register
||  MVC                 NTSR,B26
    LDDW                *++A15,A21:A20
||  LDDW                *++B15,B21:B20
    ;Hardware loop related
||  MVC                 RILC,B27
    LDDW                *++A15,A23:A22
||  LDDW                *++B15,B23:B22
    ;Hardware loop related
||  MVC                 ILC,B28
    ;When we restore the CSR, we must detect if the SAT bit is set. If
    ;yes, we need to execute an instruction that is sure to cause saturation
    ;before we restore this stuff
    LDDW                *++A15,A25:A24
||  LDDW                *++B15,B25:B24
||  MVC                 CSR,B29
    LDDW                *++A15,A27:A26
||  LDDW                *++B15,B27:B26
||  MVK                 0x200,B28
    AND                 B28,B29,B29
    MV                  A0,A31
    CMPEQ               B29,0,A0
    [!A0]MVK            0xFFFFFFFF,B29
    [!A0]SADD           B26,B26,B29
    MV                  A31,A0
    ;Saturation flags must be restored after the job
||  MVC                 B30,SSR
    LDDW                *++A15,A29:A28
||  LDDW                *++B15,B29:B28
    ;Return PC pointer
||  MVC                 B31,NRP
    LDDW                *++A15,A31:A30
||  LDDW                *++B15,B31:B30
    ;Now we restore the stack pointers - cannot be parallel
    MV                  B15,A15
    LDDW                *++A15,B15:B14
    LDDW                *++A15,A15:A14
    ;Return to user-level
    B                   NRP
;/* End Function:SVC_Handler *************************************************/
 
;/* End Of File **************************************************************/

;/* Copyright (C) Evo-Devo Instrum. All rights reserved **********************/
