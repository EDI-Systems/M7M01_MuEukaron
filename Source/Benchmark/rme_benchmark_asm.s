;/*****************************************************************************
;Filename    : benchmark_asm.s
;Author      : pry
;Date        : 19/04/2017
;Description : The Cortex-M user-level assembly scheduling support of the RME RTOS.
;*****************************************************************************/
                
;/* Begin Header *************************************************************/
                ;The align is "(2^3)/8=1(Byte)." In fact it does not take effect.            
                AREA            ARCH,CODE,READONLY,ALIGN=3                     
                
                THUMB
                REQUIRE8
                PRESERVE8
;/* End Header ***************************************************************/

;/* Begin Exports ************************************************************/
                ;User entry stub
                EXPORT          RME_Entry
                ;System call gate
                EXPORT          RME_Svc
                ;User level stub for thread creation
                EXPORT          RME_Thd_Stub
                ;User level stub for synchronous invocation
                EXPORT          RME_Inv_Stub
                ;Shut the semihosting up
                EXPORT          __user_setup_stackheap
;/* End Exports **************************************************************/

;/* Begin Imports ************************************************************/
                ;The ARM C library entrance. This will do all the dirty init jobs for us.
                IMPORT          __main
                ;The C routine for 
;/* End Imports **************************************************************/

;/* Begin Function:RME_Entry **************************************************
;Description : The entry of the process.
;Input       : None.
;Output      : None.
;*****************************************************************************/
RME_Entry
                 LDR     R0, =__main
                 BX      R0
;/* End Function:RME_Entry ***************************************************/

;/* Begin Function:RME_Thd_Stub ***********************************************
;Description : The user level stub for thread creation.
;Input       : R4 - The entry address.
;              R5 - The stack address that we are using now.
;Output      : None.
;*****************************************************************************/
RME_Thd_Stub
                BLX      R4                 ; Branch to the actual entry address
                ;B        RME_Thd_Finish     ; Jump to exiting code, should never return.
                B        .                  ; Capture faults.
;/* End Function:RME_Thd_Stub ************************************************/

;/* Begin Function:RME_Inv_Stub ***********************************************
;Description : The user level stub for synchronous invocation.
;Input       : R4 - The entry address.
;              R5 - The stack address that we are using now.
;Output      : None.
;*****************************************************************************/
RME_Inv_Stub
                BLX      R4                 ; Branch to the actual entry address
                ;BX       RME_Inv_Finish     ; Jump to exiting code, should never return.
                B        .                  ; Capture faults.
;/* End Function:RME_Inv_Stub ************************************************/

;/* Begin Function:RME_Svc ****************************************************
;Description : Trigger a system call.
;Input       : R4 - The system call number/other information.
;              R5 - Argument 1.
;              R6 - Argument 2.
;              R7 - Argument 3.
;Output      : None.                              
;*****************************************************************************/
RME_Svc
                PUSH       {R4-R7}  ; Manual clobbering
                MOV        R4,R0    ; Manually pass the parameters according to ARM calling convention
                MOV        R5,R1
                MOV        R6,R2
                MOV        R7,R3
                SVC        #0x00   
                MOV        R0,R4    ; This is the return value
                POP        {R4-R7}  ; Manual recovering
                BX         LR
                B          .        ; Shouldn't reach here.       
;/* End Function:RME_Svc *****************************************************/

;/* Begin Function:RME_Inv ****************************************************
;Description : Do an invocation, and get the return value for that invocation as well.
;Input       : R4 - The system call number/other information.
;              R5 - The invocation capability.
;              R6 - The first argument for the invocation.
;              R7 - Argument 3.
;Output      : None.                              
;*****************************************************************************/
RME_Inv
                PUSH       {R4-R7}  ; Manual clobbering
                MOV        R4,R0    ; Manually pass the parameters according to ARM calling convention
                MOV        R5,R1
                MOV        R6,R2
                MOV        R7,R3
                SVC        #0x00   
                MOV        R0,R4    ; This is the return value
                POP        {R4-R7}  ; Manual recovering
                BX         LR
                B          .        ; Shouldn't reach here.       
;/* End Function:RME_Svc *****************************************************/

;/* Begin Function:__user_setup_stackheap *************************************
;Description : We place the function here to shut the SEMIHOSTING up.
;Input       : None.
;Output      : None.
;*****************************************************************************/
__user_setup_stackheap
                MOV      R0,R1     ; Stack as where we came in, and definitely no heap
                MOV      R2,R1
                BX       LR
                B        .         ; Capture faults
;/* End Function:__user_setup_stackheap **************************************/

                END
;/* End Of File **************************************************************/

;/* Copyright (C) Evo-Devo Instrum. All rights reserved **********************/
