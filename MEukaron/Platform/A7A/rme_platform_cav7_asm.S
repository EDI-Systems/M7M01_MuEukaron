/******************************************************************************
Filename    : platform_cav7_asm.s
Author      : pry
Date        : 19/01/2017
Description : The Cortex-A (ARMv7) assembly support of the RME RTOS.
              We don't seek to support AFE or TRE on this architecture for RME.
              This is because some processors does not implement these two features
              correctly and even the ARM errata does not contain these info. There
              are 3rd-party implementations that are not guaranteed to have these 
              features as well. To be safe, we are only using what everyone else
              is using. 
              We also rely on the U-boot for errata fixing.
******************************************************************************/

/* The ARMv7 Cortex-A Architecture ********************************************
 Sys/User     FIQ Supervisor(SVC) Abort    IRQ    Undefined
    R0        R0        R0        R0       R0        R0
    R1        R1        R1        R1       R1        R1
    R2        R2        R2        R2       R2        R2
    R3        R3        R3        R3       R3        R3
    R4        R4        R4        R4       R4        R4
    R5        R5        R5        R5       R5        R5
    R6        R6        R6        R6       R6        R6
    R7        R7        R7        R7       R7        R7
    R8        R8_F      R8        R8       R8        R8
    R9        R9_F      R9        R9       R9        R9
    R10       R10_F     R10       R10      R10       R10
    R11       R11_F     R11       R11      R11       R11
    R12       R12_F     R12       R12      R12       R12
    SP        SP_F      SP_S      SP_A     SP_I      SP_U
    LR        LR_F      LR_S      LR_A     LR_I      LR_U
    PC        PC        PC        PC       PC        PC
---------------------------------------------------------------
    CPSR      CPSR      CPSR      CPSR     CPSR      CPSR
              SPSR_F    SPSR_S    SPSR_A   SPSR_I    SPSR_U
 11111/10000  10001     10011     10111    10010     11011
---------------------------------------------------------------
R0-R7  : General purpose registers that are accessible
R8-R12 : General purpose regsisters that are not accessible by 16-bit thumb
R13    : SP, Stack pointer
R14    : LR, Link register
R15    : PC, Program counter
CPSR   : Program status word
SPSR   : Banked program status word
The ARM Cortex-A also include various FPU implementations.
******************************************************************************/

/* CP15 Macros ***************************************************************/
.macro CP15_GET_INIT CRN OP1 CRM OP2
    MRC                 P15,\OP1,R0,\CRN,\CRM,\OP2
.endm

.macro CP15_GET CRN OP1 CRM OP2
    MRC                 P15,\OP1,R0,\CRN,\CRM,\OP2
    BX                  LR
.endm

.macro CP15_GET_DOUBLE CRM OP
    MRRC                P15,\OP,R2,R3,\CRM
    STR                 R2,[R0]
    STR                 R3,[R1]
    BX                  LR
.endm

.macro CP15_SET_INIT CRN OP1 CRM OP2
    MCR                 P15,\OP1,R0,\CRN,\CRM,\OP2
.endm

.macro CP15_SET CRN OP1 CRM OP2
    MCR                 P15,\OP1,R0,\CRN,\CRM,\OP2
    BX                  LR
.endm

.macro CP15_SET_DOUBLE CRM OP
    MCRR                P15,\OP,R0,R0,\CRM
    BX                  LR
.endm
/* End CP15 Macros ***********************************************************/

/* Begin Exports *************************************************************/
    /* Disable all interrupts */
    .global             __RME_Disable_Int
    /* Enable all interrupts */
    .global             __RME_Enable_Int
    /* Atomic compare and exchange */
    .global             __RME_CAV7_Comp_Swap
    /* Atomic add */
    .global             __RME_CAV7_Fetch_Add
    /* Atomic and */
    .global             __RME_CAV7_Fetch_And
    /* Read acquire/Write release for Cortex-A (ARMv7) */
    .global             __RME_CAV7_Read_Acquire
    .global             __RME_CAV7_Write_Release
    /* Get the MSB in a word */
    .global             __RME_CAV7_MSB_Get
    /* Kernel main function wrapper */
    .global             _RME_Kmain
    /* Entering of the user mode */
    .global             __RME_Enter_User_Mode

    /* CAV7 specific stuff */
    /* HALT processor to wait for interrupt */
    .global             __RME_CAV7_Halt
    /* Load page table */
    .global             __RME_CAV7_Pgtbl_Set
    /* Get register values */
    .global             __RME_CAV7_CPSR_Get
    .global             __RME_CAV7_SPSR_Get
    /* C0 */
    .global             __RME_CAV7_MIDR_Get
    .global             __RME_CAV7_CTR_Get
    .global             __RME_CAV7_TCMTR_Get
    .global             __RME_CAV7_TLBTR_Get
    .global             __RME_CAV7_MPIDR_Get
    .global             __RME_CAV7_REVIDR_Get
    .global             __RME_CAV7_ID_PFR0_Get
    .global             __RME_CAV7_ID_PFR1_Get
    .global             __RME_CAV7_ID_DFR0_Get
    .global             __RME_CAV7_ID_AFR0_Get
    .global             __RME_CAV7_ID_MMFR0_Get
    .global             __RME_CAV7_ID_MMFR1_Get
    .global             __RME_CAV7_ID_MMFR2_Get
    .global             __RME_CAV7_ID_MMFR3_Get
    .global             __RME_CAV7_ID_ISAR0_Get
    .global             __RME_CAV7_ID_ISAR1_Get
    .global             __RME_CAV7_ID_ISAR2_Get
    .global             __RME_CAV7_ID_ISAR3_Get
    .global             __RME_CAV7_ID_ISAR4_Get
    .global             __RME_CAV7_ID_ISAR5_Get
    .global             __RME_CAV7_ID_CCSIDR_Get
    .global             __RME_CAV7_ID_CLIDR_Get
    .global             __RME_CAV7_ID_AIDR_Get
    .global             __RME_CAV7_ID_CSSELR_Get
    .global             __RME_CAV7_ID_VPIDR_Get
    .global             __RME_CAV7_ID_VMPIDR_Get
    /* C1 */
    .global             __RME_CAV7_SCTLR_Get
    .global             __RME_CAV7_ACTLR_Get
    .global             __RME_CAV7_CPACR_Get
    .global             __RME_CAV7_SCR_Get
    .global             __RME_CAV7_SDER_Get
    .global             __RME_CAV7_NSACR_Get
    .global             __RME_CAV7_HSCTLR_Get
    .global             __RME_CAV7_HACTLR_Get
    .global             __RME_CAV7_HCR_Get
    .global             __RME_CAV7_HDCR_Get
    .global             __RME_CAV7_HCPTR_Get
    .global             __RME_CAV7_HSTR_Get
    .global             __RME_CAV7_HACR_Get
    /* C2 */
    .global             __RME_CAV7_TTBR0_Get
    .global             __RME_CAV7_TTBR1_Get
    .global             __RME_CAV7_TTBCR_Get
    .global             __RME_CAV7_HTCR_Get
    .global             __RME_CAV7_VTCR_Get
    .global             __RME_CAV7_DACR_Get
    /* C5 */
    .global             __RME_CAV7_DFSR_Get
    .global             __RME_CAV7_IFSR_Get
    .global             __RME_CAV7_ADFSR_Get
    .global             __RME_CAV7_AIFSR_Get
    .global             __RME_CAV7_HADFSR_Get
    .global             __RME_CAV7_HAIFSR_Get
    .global             __RME_CAV7_HSR_Get
    .global             __RME_CAV7_DFAR_Get
    .global             __RME_CAV7_IFAR_Get
    .global             __RME_CAV7_HDFAR_Get
    .global             __RME_CAV7_HIFAR_Get
    .global             __RME_CAV7_HPFAR_Get
    .global             __RME_CAV7_PAR_Get
    /* C10 */
    .global             __RME_CAV7_TLBLR_Get
    .global             __RME_CAV7_PRRR_Get
    .global             __RME_CAV7_NMRR_Get
    .global             __RME_CAV7_AMAIR0_Get
    .global             __RME_CAV7_AMAIR1_Get
    .global             __RME_CAV7_HMAIR0_Get
    .global             __RME_CAV7_HMAIR1_Get
    .global             __RME_CAV7_HAMAIR0_Get
    .global             __RME_CAV7_HAMAIR1_Get
    /* C12 */
    .global             __RME_CAV7_VBAR_Get
    .global             __RME_CAV7_MVBAR_Get
    .global             __RME_CAV7_ISR_Get
    .global             __RME_CAV7_HVBAR_Get
    /* C13 */
    .global             __RME_CAV7_FCSEIDR_Get
    .global             __RME_CAV7_CONTEXTIDR_Get
    .global             __RME_CAV7_TPIDRURW_Get
    .global             __RME_CAV7_TPIDRURO_Get
    .global             __RME_CAV7_TPIDRPRW_Get
    .global             __RME_CAV7_HTPIDR_Get
    /* C14 */
    .global             __RME_CAV7_CNTFRQ_Get
    .global             __RME_CAV7_CNTKCTL_Get
    .global             __RME_CAV7_CNTP_TVAL_Get
    .global             __RME_CAV7_CNTP_CTL_Get
    .global             __RME_CAV7_CNTV_TVAL_Get
    .global             __RME_CAV7_CNTV_CTL_Get
    .global             __RME_CAV7_CNTHCTL_Get
    .global             __RME_CAV7_CNTHP_TVAL_Get
    .global             __RME_CAV7_CNTHP_CTL_Get
    /* Double words */
    .global             __RME_CAV7_CNTPCT_DW_Get
    .global             __RME_CAV7_CNTVCT_DW_Get
    .global             __RME_CAV7_CNTP_CVAL_DW_Get
    .global             __RME_CAV7_CNTV_CVAL_DW_Get
    .global             __RME_CAV7_CNTVOFF_DW_Get
    .global             __RME_CAV7_CNTHP_CVAL_DW_Get

    /* Set register values */
    .global             __RME_CAV7_CPSR_Set
    .global             __RME_CAV7_SPSR_Set
    /* C0 */
    .global             __RME_CAV7_ID_CSSELR_Set
    .global             __RME_CAV7_ID_VPIDR_Set
    .global             __RME_CAV7_ID_VMPIDR_Set
    /* C1 */
    .global             __RME_CAV7_SCTLR_Set
    .global             __RME_CAV7_ACTLR_Set
    .global             __RME_CAV7_CPACR_Set
    .global             __RME_CAV7_SCR_Set
    .global             __RME_CAV7_SDER_Set
    .global             __RME_CAV7_NSACR_Set
    .global             __RME_CAV7_HSCTLR_Set
    .global             __RME_CAV7_HACTLR_Set
    .global             __RME_CAV7_HCR_Set
    .global             __RME_CAV7_HDCR_Set
    .global             __RME_CAV7_HCPTR_Set
    .global             __RME_CAV7_HSTR_Set
    .global             __RME_CAV7_HACR_Set
    /* C2,C3 */
    .global             __RME_CAV7_TTBR0_Set
    .global             __RME_CAV7_TTBR1_Set
    .global             __RME_CAV7_TTBCR_Set
    .global             __RME_CAV7_HTCR_Set
    .global             __RME_CAV7_VTCR_Set
    .global             __RME_CAV7_DACR_Set
    /* C5 */
    .global             __RME_CAV7_DFSR_Set
    .global             __RME_CAV7_IFSR_Set
    .global             __RME_CAV7_ADFSR_Set
    .global             __RME_CAV7_AIFSR_Set
    .global             __RME_CAV7_HADFSR_Set
    .global             __RME_CAV7_HAIFSR_Set
    .global             __RME_CAV7_HSR_Set
    .global             __RME_CAV7_DFAR_Set
    .global             __RME_CAV7_IFAR_Set
    .global             __RME_CAV7_HDFAR_Set
    .global             __RME_CAV7_HIFAR_Set
    .global             __RME_CAV7_HPFAR_Set
    /* C7 */
    .global             __RME_CAV7_ICIALLUIS_Set
    .global             __RME_CAV7_BPIALLIS_Set
    .global             __RME_CAV7_PAR_Set
    .global             __RME_CAV7_ICIALLU_Set
    .global             __RME_CAV7_ICIMVAU_Set
    .global             __RME_CAV7_CP15ISB_Set
    .global             __RME_CAV7_BPIALL_Set
    .global             __RME_CAV7_BPIMVA_Set
    .global             __RME_CAV7_DCIMVAC_Set
    .global             __RME_CAV7_DCISW_Set
    .global             __RME_CAV7_ATS1CPR_Set
    .global             __RME_CAV7_ATS1CPW_Set
    .global             __RME_CAV7_ATS1CUR_Set
    .global             __RME_CAV7_ATS1CUW_Set
    .global             __RME_CAV7_ATS12NSOPR_Set
    .global             __RME_CAV7_ATS12NSOPW_Set
    .global             __RME_CAV7_ATS12NSOUR_Set
    .global             __RME_CAV7_ATS12NSOUW_Set
    .global             __RME_CAV7_DCCMVAC_Set
    .global             __RME_CAV7_DCCSW_Set
    .global             __RME_CAV7_CP15DSB_Set
    .global             __RME_CAV7_CP15DMB_Set
    .global             __RME_CAV7_DCCMVAU_Set
    .global             __RME_CAV7_DCCIMVAC_Set
    .global             __RME_CAV7_DCCISW_Set
    .global             __RME_CAV7_ATS1HR_Set
    .global             __RME_CAV7_ATS1HW_Set
    /* C8 */
    .global             __RME_CAV7_TLBIALLIS_Set
    .global             __RME_CAV7_TLBIMVAIS_Set
    .global             __RME_CAV7_TLBIASIDIS_Set
    .global             __RME_CAV7_TLBIMVAAIS_Set
    .global             __RME_CAV7_ITLBIALL_Set
    .global             __RME_CAV7_ITLBIMVA_Set
    .global             __RME_CAV7_ITLBIASID_Set
    .global             __RME_CAV7_DTLBIALL_Set
    .global             __RME_CAV7_DTLBIMVA_Set
    .global             __RME_CAV7_DTLBIASID_Set
    .global             __RME_CAV7_TLBIALL_Set
    .global             __RME_CAV7_TLBIMVA_Set
    .global             __RME_CAV7_TLBIASID_Set
    .global             __RME_CAV7_TLBIMVAA_Set
    .global             __RME_CAV7_TLBIALLHIS_Set
    .global             __RME_CAV7_TLBIMVAHIS_Set
    .global             __RME_CAV7_TLBIALLNSNHIS_Set
    .global             __RME_CAV7_TLBIALLH_Set
    .global             __RME_CAV7_TLBIMVAH_Set
    .global             __RME_CAV7_TLBIALLNSNH_Set
    /* C10 */
    .global             __RME_CAV7_TLBLR_Set
    .global             __RME_CAV7_PRRR_Set
    .global             __RME_CAV7_NMRR_Set
    .global             __RME_CAV7_AMAIR0_Set
    .global             __RME_CAV7_AMAIR1_Set
    .global             __RME_CAV7_HMAIR0_Set
    .global             __RME_CAV7_HMAIR1_Set
    .global             __RME_CAV7_HAMAIR0_Set
    .global             __RME_CAV7_HAMAIR1_Set
    /* C12 */
    .global             __RME_CAV7_VBAR_Set
    .global             __RME_CAV7_MVBAR_Set
    .global             __RME_CAV7_HVBAR_Set
    /* C13 */
    .global             __RME_CAV7_CONTEXTIDR_Set
    .global             __RME_CAV7_TPIDRURW_Set
    .global             __RME_CAV7_TPIDRURO_Set
    .global             __RME_CAV7_TPIDRPRW_Set
    .global             __RME_CAV7_HTPIDR_Set
    /* C14 */
    .global             __RME_CAV7_CNTFRQ_Set
    .global             __RME_CAV7_CNTKCTL_Set
    .global             __RME_CAV7_CNTP_TVAL_Set
    .global             __RME_CAV7_CNTP_CTL_Set
    .global             __RME_CAV7_CNTV_TVAL_Set
    .global             __RME_CAV7_CNTV_CTL_Set
    .global             __RME_CAV7_CNTHCTL_Set
    .global             __RME_CAV7_CNTHP_TVAL_Set
    .global             __RME_CAV7_CNTHP_CTL_Set
    /* Double words */
    .global             __RME_CAV7_CNTP_CVAL_DW_Set
    .global             __RME_CAV7_CNTV_CVAL_DW_Set
    .global             __RME_CAV7_CNTVOFF_DW_Set
    .global             __RME_CAV7_CNTHP_CVAL_DW_Set

    /* Booting specific stuff */
    .global             __RME_CAV7_Stack_Start
    /* Initial page table */
    .global             __RME_CAV7_Kern_Pgtbl
    /* Vector table */
    .global             __RME_CAV7_Vector_Table
    /* Fault handlers and user handlers are exported on their spot */
/* End Exports ***************************************************************/

/* Begin Imports *************************************************************/
    /* The kernel entry of RME. This will be defined in C language. */
    .global             RME_Kmain
    /* The fault handlerd of RME. These will be defined in C language. */
    .global             __RME_CAV7_Undefined_Handler
    .global             __RME_CAV7_Prefetch_Abort_Handler
    .global             __RME_CAV7_Data_Abort_Handler
    .global             __RME_CAV7_IRQ_Handler
    .global             __RME_CAV7_FIQ_Handler
    /* The generic interrupt handler of RME. This will be defined in C language. */
    .global             __RME_CAV7_Generic_Handler
    /* The system call handler of RME. This will be defined in C language. */
    .global             _RME_Svc_Handler
    /* The system tick handler of RME. This will be defined in C language. */
    .global             _RME_Tick_Handler
    /* The entry of SMP after they have finished their initialization */
    .global             __RME_SMP_Low_Level_Init
    /* All other processor's timer interrupt handler */
    .global             __RME_CAV7_SMP_Tick
    /* Memory layout information - This is the actual page table mapping */
    .global             RME_CAV7_Mem_Info
/* End Imports ***************************************************************/

/* Begin Memory Init *********************************************************/
/* Das U-Boot header is appended by the u-boot through FIT image generation.
 * We do not temper with this here, as it is unnecessary. For the .data section,
 * we are perfectly fine because it is in RAM do not even need relocation, and
 * it is of course included in the resulting binary from elf. (Actually RME
 * kernel code does not make use of the .data section anyway). As a result,
 * We just need to clean up the. bss section. The linux kernel, when booting,
 * always hacks the CPU to SVC mode, perhaps to allow usage of many different
 * bootloaders; however, we know that u-boot have already set the CPU to SVC
 * mode, which can spare us the ugly self-modifying code. */
    .section            .text
    .syntax             divided
    .code               32
    .align              16
    .global             __bss_start__
    .global             __bss_end__
    .global             __va_offset__
    .global             main
_start:
    .global             _start
    LDR                 R0,=__bss_start__
    LDR                 R1,=__bss_end__
    LDR                 R2,=__va_offset__
    SUB                 R0,R0,R2
    SUB                 R1,R1,R2
    LDR                 R2,=0x00
clear_bss:
    CMP                 R0,R1
    BEQ                 clear_done
    STR                 R2,[R0]
    ADD                 R0,#0x04
    B                   clear_bss
clear_done:
    /* Set stacks for all modes */
    LDR                 R4,=__RME_CAV7_Stack_Start
    ADD                 R4,#0x10000
    /* IRQ mode */
    LDR                 R0,=0x600F00D2
    MSR                 CPSR,R0
    MOV                 SP,R4
    /* ABT mode */
    LDR                 R0,=0x600F00D7
    MSR                 CPSR,R0
    MOV                 SP,R4
    /* FIQ mode */
    LDR                 R0,=0x600F00D1
    MSR                 CPSR,R0
    MOV                 SP,R4
    /* UND mode */
    LDR                 R0,=0x600F00DB
    MSR                 CPSR,R0
    MOV                 SP,R4
    /* SYS mode */
    LDR                 R0,=0x600F00DB
    MSR                 CPSR,R0
    MOV                 SP,R4
    /* SVC mode */
    LDR                 R0,=0x600F00D3
    MSR                 CPSR,R0
    MOV                 SP,R4

    /* Turn off the MMU and all cache if it is already enabled. There's no need
     * to turn cache off because we are not modifying the instruction stream at
     * all; the TLB walker will start walking from L1D if it is enabled */
    CP15_GET_INIT       CRN=C1 OP1=0 CRM=C0 OP2=0
    LDR                 R1,=~(1<<0)
    AND                 R0,R0,R1
    CP15_SET_INIT       CRN=C1 OP1=0 CRM=C0 OP2=0 /* SCTLR.AFE,TRE,I,C,M */
    ISB
    /* Flush TLB */
    LDR                 R0,=0x00
    CP15_SET_INIT       CRN=C8 OP1=0 CRM=C7 OP2=0 /* TLBIALL */
    ISB

    /* Set up the initial page table
     * R0: Read base address
     * R1: Write base address
     * R2: End read address
     * R3: PA address to map from
     * R4: VA address to map into
     * R5: Number of pages
     * R6: Property mask
     * R7: Page counter
     * R8: Write index register
     * R9: Write content register */
    LDR                 R0,=RME_CAV7_Mem_Info
    LDR                 R1,=__RME_CAV7_Kern_Pgtbl
    LDR                 R2,=__va_offset__
    /* Calculate the actual address */
    SUB                 R0,R0,R2
    SUB                 R1,R1,R2
    /* Calculate the configuration end address */
    LDR                 R3,[R0]
    ADD                 R1,R0,R3
    ADD                 R0,R0,#0x04

    /* Load configurations and generate page table layout one by one */
load_config:
    LDMIA               R0!,{R3-R6}
    MOV                 R7,#0x00
    LSR                 R8,R4,#18
    ADD                 R8,R1,R8
    ORR                 R9,R3,R6

fill_pgtbl:
    STR                 R9,[R8]
    ADD                 R8,R8,#4
    ADD                 R7,R7,#1
    ADD                 R9,R9,#0x100000
    CMP                 R7,R5
    BNE                 fill_pgtbl

    CMP                 R0,R2
    BNE                 load_config

    /* Set the registers */
    LDR                 R0,=0x00
    CP15_SET_INIT       CRN=C2 OP1=0 CRM=C0 OP2=2 /* TTBCR */
    LDR                 R0,=0x55555555
    CP15_SET_INIT       CRN=C3 OP1=0 CRM=C0 OP2=0 /* DACR */
    LDR                 R0,=0x000A00A4
    CP15_SET_INIT       CRN=C10 OP1=0 CRM=C2 OP2=0 /* PRRR */
    LDR                 R0,=0x006C006C
    CP15_SET_INIT       CRN=C10 OP1=0 CRM=C2 OP2=1 /* NMRR */
    /* Set base address */
    LDR                 R0,=__RME_CAV7_Kern_Pgtbl
    LDR                 R1,=__va_offset__
    SUB                 R0,R0,R1
    CP15_SET_INIT       CRN=C2 OP1=0 CRM=C0 OP2=0 /* TTBR0 */
    /* Load the main function address to R3 first to prepare for a long jump */
    LDR                 R3,=main
    ISB
    /* Turn on paging and cache */
    CP15_GET_INIT       CRN=C1 OP1=0 CRM=C0 OP2=0
    LDR                 R1,=(1<<29)|(1<<28)|(1<<12)|(1<<2)|(1<<0)
    ORR                 R0,R0,R1
    CP15_SET_INIT       CRN=C1 OP1=0 CRM=C0 OP2=0 /* SCTLR.AFE,TRE,I,C,M */
    ISB
    /* Branch to main function */
    BX                  R3
    .ltorg
/* Initial page table ********************************************************/
/* Das U-Boot will set up us an initial page table with all identical mappings.
 * There's no need to do anything special here. Also, we will not initialize
 * the serial port for similar reasons. But, we do need to set up initial stacks
 * for the CPUs. Because we know that there can be no more than 4 CPUs in an ARMv7
 * chip, we can statically allocate them here. Each core is associated with 64kB
 * stack, which should be more than sufficient. */
    .align              8
__RME_CAV7_Stack_Start:
    .space              4*65536
__RME_CAV7_Stack_End:
    .space              4096
/* The kernel page table - the initialization sequence is totally controlled
 * by the configuration file, because there's no generic way to detect memory on
 * these devices. */
    .align              14
__RME_CAV7_Kern_Pgtbl:
    .space              16384
/* Vectors *******************************************************************/
    .align              8
__RME_CAV7_Vector_Table:
    B                   Reset_Handler
    B                   Undefined_Handler
    B                   SVC_Handler
    B                   Prefetch_Abort_Handler
    B                   Data_Abort_Handler
    B                   Unused_Handler
    B                   IRQ_Handler
    B                   FIQ_Handler
/* End Memory Init ***********************************************************/

/* Begin Function:__RME_CAV7_XXXX_Get *****************************************
Description : Get the XXXX register of the CPU. These registers must be read with
              MRS/MRC instruction.
Input       : None
Output      : None.
Return      : R0 - The XXXX register contents.
******************************************************************************/
/* CPSR & SPSR */
__RME_CAV7_CPSR_Get:
    MRS                 R0,CPSR
    BX                  LR
__RME_CAV7_SPSR_Get:
    MRS                 R0,SPSR
    BX                  LR

/* Main ID register */
__RME_CAV7_MIDR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=0
/* Cache type register */
__RME_CAV7_CTR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=1
/* TCM type register */
__RME_CAV7_TCMTR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=2
/* TLB type register */
__RME_CAV7_TLBTR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=3
/* Multiprocessor affinity register */
__RME_CAV7_MPIDR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=5
/* Revision ID register */
__RME_CAV7_REVIDR_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C0 OP2=6
/* Processor feature register 0 */
__RME_CAV7_ID_PFR0_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=0
/* Processor feature register 1 */
__RME_CAV7_ID_PFR1_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=1
/* Debug feature register 0 */
__RME_CAV7_ID_DFR0_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=2
/* Auxiliary feature register 0 */
__RME_CAV7_ID_AFR0_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=3
/* Memory model feature register 0 */
__RME_CAV7_ID_MMFR0_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=4
/* Memory model feature register 1 */
__RME_CAV7_ID_MMFR1_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=5
/* Memory model feature register 2 */
__RME_CAV7_ID_MMFR2_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=6
/* Memory model feature register 3 */
__RME_CAV7_ID_MMFR3_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C1 OP2=7
/* ISA feature register 0 */
__RME_CAV7_ID_ISAR0_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=0
/* ISA feature register 1 */
__RME_CAV7_ID_ISAR1_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=1
/* ISA feature register 2 */
__RME_CAV7_ID_ISAR2_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=2
/* ISA feature register 3 */
__RME_CAV7_ID_ISAR3_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=3
/* ISA feature register 4 */
__RME_CAV7_ID_ISAR4_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=4
/* ISA feature register 5 */
__RME_CAV7_ID_ISAR5_Get:
    CP15_GET            CRN=C0 OP1=0 CRM=C2 OP2=5
/* Cache size ID registers */
__RME_CAV7_ID_CCSIDR_Get:
    CP15_GET            CRN=C0 OP1=1 CRM=C0 OP2=0
/* Cache level ID register */
__RME_CAV7_ID_CLIDR_Get:
    CP15_GET            CRN=C0 OP1=1 CRM=C0 OP2=1
/* Auxiliary ID register */
__RME_CAV7_ID_AIDR_Get:
    CP15_GET            CRN=C0 OP1=1 CRM=C0 OP2=7
/* Cache size selection register */
__RME_CAV7_ID_CSSELR_Get:
    CP15_GET            CRN=C0 OP1=2 CRM=C0 OP2=0
/* Virtualization processor ID register  */
__RME_CAV7_ID_VPIDR_Get:
    CP15_GET            CRN=C0 OP1=4 CRM=C0 OP2=0
/* Virtualization multiprocessor ID register */
__RME_CAV7_ID_VMPIDR_Get:
    CP15_GET            CRN=C0 OP1=4 CRM=C0 OP2=5

/* System control register */
__RME_CAV7_SCTLR_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C0 OP2=0
/* Auxiliary control register */
__RME_CAV7_ACTLR_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C0 OP2=1
/* Coprocessor auxiliary control register */
__RME_CAV7_CPACR_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C0 OP2=2
/* Secure configuration register */
__RME_CAV7_SCR_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C1 OP2=0
/* Secure debug enable register */
__RME_CAV7_SDER_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C1 OP2=1
/* Non-secure access control register */
__RME_CAV7_NSACR_Get:
    CP15_GET            CRN=C1 OP1=0 CRM=C1 OP2=2
/* Hyp system control register */
__RME_CAV7_HSCTLR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C0 OP2=0
/* Hyp auxiliary control register */
__RME_CAV7_HACTLR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C0 OP2=1
/* Hyp configuration register */
__RME_CAV7_HCR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C1 OP2=0
/* Hyp debug configuration register */
__RME_CAV7_HDCR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C1 OP2=1
/* Hyp coprocessor trap register */
__RME_CAV7_HCPTR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C1 OP2=2
/* Hyp system trap register */
__RME_CAV7_HSTR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C1 OP2=3
/* Hyp auxiliary configuration register */
__RME_CAV7_HACR_Get:
    CP15_GET            CRN=C1 OP1=4 CRM=C1 OP2=7

/* Translation table base register 0 - 32bit. We do not support PAE of any kind */
__RME_CAV7_TTBR0_Get:
    CP15_GET            CRN=C2 OP1=0 CRM=C0 OP2=0
/* Translation table base register 1 - 32bit. We do not support PAE of any kind */
__RME_CAV7_TTBR1_Get:
    CP15_GET            CRN=C2 OP1=0 CRM=C0 OP2=1
/* Translation table base controle register */
__RME_CAV7_TTBCR_Get:
    CP15_GET            CRN=C2 OP1=0 CRM=C0 OP2=2
/* Hyp translation control register */
__RME_CAV7_HTCR_Get:
    CP15_GET            CRN=C2 OP1=4 CRM=C0 OP2=2
/* Virtualization translation control register */
__RME_CAV7_VTCR_Get:
    CP15_GET            CRN=C2 OP1=4 CRM=C1 OP2=2
/* Domain access control register */
__RME_CAV7_DACR_Get:
    CP15_GET            CRN=C3 OP1=0 CRM=C0 OP2=0

/* Data fault status register */
__RME_CAV7_DFSR_Get:
    CP15_GET            CRN=C5 OP1=0 CRM=C0 OP2=0
/* Instruction fault status register */
__RME_CAV7_IFSR_Get:
    CP15_GET            CRN=C5 OP1=0 CRM=C0 OP2=1
/* Auxiliary data fault status register */
__RME_CAV7_ADFSR_Get:
    CP15_GET            CRN=C5 OP1=0 CRM=C1 OP2=0
/* Auxiliary instruction fault status register */
__RME_CAV7_AIFSR_Get:
    CP15_GET            CRN=C5 OP1=0 CRM=C1 OP2=1
/* Hyp auxiliary data fault status register */
__RME_CAV7_HADFSR_Get:
    CP15_GET            CRN=C5 OP1=4 CRM=C1 OP2=0
/* Hyp auxiliary instruction fault status register */
__RME_CAV7_HAIFSR_Get:
    CP15_GET            CRN=C5 OP1=4 CRM=C1 OP2=1
/* Hyp syndrome register */
__RME_CAV7_HSR_Get:
    CP15_GET            CRN=C5 OP1=4 CRM=C2 OP2=0
/* Data fault address register */
__RME_CAV7_DFAR_Get:
    CP15_GET            CRN=C6 OP1=0 CRM=C0 OP2=0
/* Instruction fault address register */
__RME_CAV7_IFAR_Get:
    CP15_GET            CRN=C6 OP1=0 CRM=C0 OP2=2
/* Hyp data fault address register */
__RME_CAV7_HDFAR_Get:
    CP15_GET            CRN=C6 OP1=4 CRM=C0 OP2=0
/* Hyp instruction fault address register */
__RME_CAV7_HIFAR_Get:
    CP15_GET            CRN=C6 OP1=4 CRM=C0 OP2=2
/* Hyp IPA fault address register */
__RME_CAV7_HPFAR_Get:
    CP15_GET            CRN=C6 OP1=4 CRM=C0 OP2=4

/* Physical address register */
__RME_CAV7_PAR_Get:
    CP15_GET            CRN=C7 OP1=0 CRM=C4 OP2=0

/* C9 registers currently unsupported */

/* TLB lockdown register - Cortex-A9 */
__RME_CAV7_TLBLR_Get:
    CP15_GET            CRN=C10 OP1=0 CRM=C0 OP2=0
/* Primary region remap register */
__RME_CAV7_PRRR_Get:
    CP15_GET            CRN=C10 OP1=0 CRM=C2 OP2=0
/* Normal memory remap register */
__RME_CAV7_NMRR_Get:
    CP15_GET            CRN=C10 OP1=0 CRM=C2 OP2=1
/* Auxiliary memory attribute indirection register 0 */
__RME_CAV7_AMAIR0_Get:
    CP15_GET            CRN=C10 OP1=0 CRM=C3 OP2=0
/* Auxiliary memory attribute indirection register 1 */
__RME_CAV7_AMAIR1_Get:
    CP15_GET            CRN=C10 OP1=0 CRM=C3 OP2=1
/* Hyp memory attribute indirection register 0 */
__RME_CAV7_HMAIR0_Get:
    CP15_GET            CRN=C10 OP1=4 CRM=C2 OP2=0
/* Hyp memory attribute indirection register 1 */
__RME_CAV7_HMAIR1_Get:
    CP15_GET            CRN=C10 OP1=4 CRM=C2 OP2=1
/* Hyp auxiliary memory attribute indirection register 0 */
__RME_CAV7_HAMAIR0_Get:
    CP15_GET            CRN=C10 OP1=4 CRM=C3 OP2=0
/* Hyp auxiliary memory attribute indirection register 1 */
__RME_CAV7_HAMAIR1_Get:
    CP15_GET            CRN=C10 OP1=4 CRM=C3 OP2=1

/* Vector base address register */
__RME_CAV7_VBAR_Get:
    CP15_GET            CRN=C12 OP1=0 CRM=C0 OP2=0
/* Vector base address register */
__RME_CAV7_MVBAR_Get:
    CP15_GET            CRN=C12 OP1=0 CRM=C0 OP2=1
/* Interrupt status register */
__RME_CAV7_ISR_Get:
    CP15_GET            CRN=C12 OP1=0 CRM=C1 OP2=0
/* Hyp vector base address register */
__RME_CAV7_HVBAR_Get:
    CP15_GET            CRN=C12 OP1=4 CRM=C0 OP2=0

/* FCSE PID register */
__RME_CAV7_FCSEIDR_Get:
    CP15_GET            CRN=C13 OP1=0 CRM=C0 OP2=0
/* Context ID register */
__RME_CAV7_CONTEXTIDR_Get:
    CP15_GET            CRN=C13 OP1=0 CRM=C0 OP2=1
/* User read/write software thread register */
__RME_CAV7_TPIDRURW_Get:
    CP15_GET            CRN=C13 OP1=0 CRM=C0 OP2=2
/* User read-only software thread register */
__RME_CAV7_TPIDRURO_Get:
    CP15_GET            CRN=C13 OP1=0 CRM=C0 OP2=3
/* PL1-only software thread register */
__RME_CAV7_TPIDRPRW_Get:
    CP15_GET            CRN=C13 OP1=0 CRM=C0 OP2=4
/* Hyp read/write software thread register */
__RME_CAV7_HTPIDR_Get:
    CP15_GET            CRN=C13 OP1=4 CRM=C0 OP2=2

/* Counter frequency register */
__RME_CAV7_CNTFRQ_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C0 OP2=0
/* Timer PL1 control register */
__RME_CAV7_CNTKCTL_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C1 OP2=0
/* PL1 physical timer value register */
__RME_CAV7_CNTP_TVAL_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C2 OP2=0
/* PL1 physical timer control register */
__RME_CAV7_CNTP_CTL_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C2 OP2=1
/* Virtual timer value register */
__RME_CAV7_CNTV_TVAL_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C3 OP2=0
/* Virtual timer control register */
__RME_CAV7_CNTV_CTL_Get:
    CP15_GET            CRN=C14 OP1=0 CRM=C3 OP2=1
/* Timer PL2 control register */
__RME_CAV7_CNTHCTL_Get:
    CP15_GET            CRN=C14 OP1=4 CRM=C1 OP2=0
/* PL2 physical timer value register */
__RME_CAV7_CNTHP_TVAL_Get:
    CP15_GET            CRN=C14 OP1=4 CRM=C2 OP2=0
/* PL2 physical timer control register */
__RME_CAV7_CNTHP_CTL_Get:
    CP15_GET            CRN=C14 OP1=4 CRM=C2 OP2=1
/* End Function:__RME_CAV7_XXXX_Get ******************************************/

/* Begin Function:__RME_CAV7_XXXX_DW_Get **************************************
Description : Get the XXXX register of the CPU. These registers must be read with
              MRRC instruction, and are all 64-bit double words.
Input       : None.
Output      : rme_ptr_t* R0 - The pointer to the lower bits.
              rme_ptr_t* R1 - The pointer to the higher bits.
Return      : None.
******************************************************************************/
/* Physical count register */
__RME_CAV7_CNTPCT_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=0
/* Virtual count register */
__RME_CAV7_CNTVCT_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=1
/* PL1 physical timer compare value register */
__RME_CAV7_CNTP_CVAL_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=2
/* Virtual timer compare value register */
__RME_CAV7_CNTV_CVAL_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=3
/* Virtual offset register */
__RME_CAV7_CNTVOFF_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=4
/* L2 physical timer compare value register */
__RME_CAV7_CNTHP_CVAL_DW_Get:
    CP15_GET_DOUBLE     CRM=C14 OP=6
/* End Function:__RME_CAV7_XXXX_DW_Get ***************************************/

/* Begin Function:__RME_CAV7_XXXX_Set *****************************************
Description : Set the XXXX register of the CPU.
Input       : rme_ptr_t R0 - The XXXX value to set.
Output      : None.
Return      : None.
******************************************************************************/
/* CPSR & SPSR */
__RME_CAV7_CPSR_Set:
    MSR                 CPSR,R0
    BX                  LR
__RME_CAV7_SPSR_Set:
    MSR                 SPSR,R0
    BX                  LR

/* Cache size selection register */
__RME_CAV7_ID_CSSELR_Set:
    CP15_SET            CRN=C0 OP1=2 CRM=C0 OP2=0
/* Virtualization processor ID register  */
__RME_CAV7_ID_VPIDR_Set:
    CP15_SET            CRN=C0 OP1=4 CRM=C0 OP2=0
/* Virtualization multiprocessor ID register */
__RME_CAV7_ID_VMPIDR_Set:
    CP15_SET            CRN=C0 OP1=4 CRM=C0 OP2=5

/* System control register */
__RME_CAV7_SCTLR_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C0 OP2=0
/* Auxiliary control register */
__RME_CAV7_ACTLR_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C0 OP2=1
/* Coprocessor auxiliary control register */
__RME_CAV7_CPACR_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C0 OP2=2
/* Secure configuration register */
__RME_CAV7_SCR_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C1 OP2=0
/* Secure debug enable register */
__RME_CAV7_SDER_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C1 OP2=1
/* Non-secure access control register */
__RME_CAV7_NSACR_Set:
    CP15_SET            CRN=C1 OP1=0 CRM=C1 OP2=2
/* Hyp system control register */
__RME_CAV7_HSCTLR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C0 OP2=0
/* Hyp auxiliary control register */
__RME_CAV7_HACTLR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C0 OP2=1
/* Hyp configuration register */
__RME_CAV7_HCR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C1 OP2=0
/* Hyp debug configuration register */
__RME_CAV7_HDCR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C1 OP2=1
/* Hyp coprocessor trap register */
__RME_CAV7_HCPTR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C1 OP2=2
/* Hyp system trap register */
__RME_CAV7_HSTR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C1 OP2=3
/* Hyp auxiliary configuration register */
__RME_CAV7_HACR_Set:
    CP15_SET            CRN=C1 OP1=4 CRM=C1 OP2=7

/* Translation table base register 0 - 32bit. We do not support PAE of any kind.
 * This operation also sets the page table of this architecture */
__RME_CAV7_TTBR0_Set:
__RME_CAV7_Pgtbl_Set:
    CP15_SET            CRN=C2 OP1=0 CRM=C0 OP2=0
/* Translation table base register 1 - 32bit. We do not support PAE of any kind */
__RME_CAV7_TTBR1_Set:
    CP15_SET            CRN=C2 OP1=0 CRM=C0 OP2=1
/* Translation table base controle register */
__RME_CAV7_TTBCR_Set:
    CP15_SET            CRN=C2 OP1=0 CRM=C0 OP2=2
/* Hyp translation control register */
__RME_CAV7_HTCR_Set:
    CP15_SET            CRN=C2 OP1=4 CRM=C0 OP2=2
/* Virtualization translation control register */
__RME_CAV7_VTCR_Set:
    CP15_SET            CRN=C2 OP1=4 CRM=C1 OP2=2
/* Domain access control register */
__RME_CAV7_DACR_Set:
    CP15_SET            CRN=C3 OP1=0 CRM=C0 OP2=0

/* Data fault status register */
__RME_CAV7_DFSR_Set:
    CP15_SET            CRN=C5 OP1=0 CRM=C0 OP2=0
/* Instruction fault status register */
__RME_CAV7_IFSR_Set:
    CP15_SET            CRN=C5 OP1=0 CRM=C0 OP2=1
/* Auxiliary data fault status register */
__RME_CAV7_ADFSR_Set:
    CP15_SET            CRN=C5 OP1=0 CRM=C1 OP2=0
/* Auxiliary instruction fault status register */
__RME_CAV7_AIFSR_Set:
    CP15_SET            CRN=C5 OP1=0 CRM=C1 OP2=1
/* Hyp auxiliary data fault status register */
__RME_CAV7_HADFSR_Set:
    CP15_SET            CRN=C5 OP1=4 CRM=C1 OP2=0
/* Hyp auxiliary instruction fault status register */
__RME_CAV7_HAIFSR_Set:
    CP15_SET            CRN=C5 OP1=4 CRM=C1 OP2=1
/* Hyp syndrome register */
__RME_CAV7_HSR_Set:
    CP15_SET            CRN=C5 OP1=4 CRM=C2 OP2=0
/* Data fault address register */
__RME_CAV7_DFAR_Set:
    CP15_SET            CRN=C6 OP1=0 CRM=C0 OP2=0
/* Instruction fault address register */
__RME_CAV7_IFAR_Set:
    CP15_SET            CRN=C6 OP1=0 CRM=C0 OP2=2
/* Hyp data fault address register */
__RME_CAV7_HDFAR_Set:
    CP15_SET            CRN=C6 OP1=4 CRM=C0 OP2=0
/* Hyp instruction fault address register */
__RME_CAV7_HIFAR_Set:
    CP15_SET            CRN=C6 OP1=4 CRM=C0 OP2=2
/* Hyp IPA fault address register */
__RME_CAV7_HPFAR_Set:
    CP15_SET            CRN=C6 OP1=4 CRM=C0 OP2=4

/* Instruction cache invalidate all to PoU inner shareable */
__RME_CAV7_ICIALLUIS_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C1 OP2=0
/* Branch predictor invalidate all inner shareable */
__RME_CAV7_BPIALLIS_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C1 OP2=6
/* Physical address register */
__RME_CAV7_PAR_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C4 OP2=6
/* Instruction cache invalidate all to PoU */
__RME_CAV7_ICIALLU_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C5 OP2=0
/* Invalidate instruction cache by MVA to PoU */
__RME_CAV7_ICIMVAU_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C5 OP2=1
/* ISB register - deprecated */
__RME_CAV7_CP15ISB_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C5 OP2=4
/* Invalidate entire branch predictor array */
__RME_CAV7_BPIALL_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C5 OP2=6
/* Invalidate MVA from branch predictors */
__RME_CAV7_BPIMVA_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C5 OP2=7
/* Invalidate data cache by MVA to PoC */
__RME_CAV7_DCIMVAC_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C6 OP2=1
/* Invalidate data cache line by set/way */
__RME_CAV7_DCISW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C6 OP2=2
/* Priviledged read VA to PA translation */
__RME_CAV7_ATS1CPR_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=0
/* Priviledged write VA to PA translation */
__RME_CAV7_ATS1CPW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=1
/* User read VA to PA translation */
__RME_CAV7_ATS1CUR_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=2
/* User write VA to PA translation */
__RME_CAV7_ATS1CUW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=3
/* Priviledged read VA to PA translation, other security state */
__RME_CAV7_ATS12NSOPR_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=4
/* Priviledged write VA to PA translation, other security state */
__RME_CAV7_ATS12NSOPW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=5
/* User read VA to PA translation, other security state */
__RME_CAV7_ATS12NSOUR_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=6
/* User write VA to PA translation, other security state */
__RME_CAV7_ATS12NSOUW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C8 OP2=7
/* Clean data cache line by MVA to PoC */
__RME_CAV7_DCCMVAC_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C10 OP2=1
/* Clean data cache line by set/way */
__RME_CAV7_DCCSW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C10 OP2=2
/* DSB register - deprecated */
__RME_CAV7_CP15DSB_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C10 OP2=4
/* DMB register - deprecated */
__RME_CAV7_CP15DMB_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C10 OP2=5
/* Clean data cache line by MVA to PoU */
__RME_CAV7_DCCMVAU_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C11 OP2=1
/* Clean and invalidate data cache line by MVA to PoC */
__RME_CAV7_DCCIMVAC_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C14 OP2=1
/* Clean and invalidate data cache line by set/way */
__RME_CAV7_DCCISW_Set:
    CP15_SET            CRN=C7 OP1=0 CRM=C14 OP2=2
/* Hyp mode read translation */
__RME_CAV7_ATS1HR_Set:
    CP15_SET            CRN=C7 OP1=4 CRM=C8 OP2=0
/* Hyp mode write translation */
__RME_CAV7_ATS1HW_Set:
    CP15_SET            CRN=C7 OP1=4 CRM=C8 OP2=1

/* Invalidate entire TLB IS */
__RME_CAV7_TLBIALLIS_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C3 OP2=0
/* Invalidate unified TLB entry by MVA and ASID IS */
__RME_CAV7_TLBIMVAIS_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C3 OP2=1
/* Invalidate unified TLB by ASID match IS */
__RME_CAV7_TLBIASIDIS_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C3 OP2=2
/* Invalidate unified TLB entry by MVA all ASID IS */
__RME_CAV7_TLBIMVAAIS_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C3 OP2=3
/* Invalidate instruction TLB */
__RME_CAV7_ITLBIALL_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C5 OP2=0
/* Invalidate instruction TLB entry by MVA and ASID */
__RME_CAV7_ITLBIMVA_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C5 OP2=1
/* Invalidate instruction TLB by ASID match */
__RME_CAV7_ITLBIASID_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C5 OP2=2
/* Invalidate data TLB */
__RME_CAV7_DTLBIALL_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C6 OP2=0
/* Invalidate data TLB entry by MVA and ASID */
__RME_CAV7_DTLBIMVA_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C6 OP2=1
/* Invalidate data TLB by ASID match */
__RME_CAV7_DTLBIASID_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C6 OP2=2
/* Invalidate unified TLB */
__RME_CAV7_TLBIALL_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C7 OP2=0
/* Invalidate unified TLB entry by MVA and ASID */
__RME_CAV7_TLBIMVA_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C7 OP2=1
/* Invalidate unified TLB by ASID match */
__RME_CAV7_TLBIASID_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C7 OP2=2
/* Invalidate unified TLB entries by MVA all ASID */
__RME_CAV7_TLBIMVAA_Set:
    CP15_SET            CRN=C8 OP1=0 CRM=C7 OP2=3
/* Invalidate entire Hyp unified TLB IS */
__RME_CAV7_TLBIALLHIS_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C3 OP2=0
/* Invalidate Hyp unified TLB entry by MVA IS */
__RME_CAV7_TLBIMVAHIS_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C3 OP2=1
/* Invalidate entire Non-secure non-Hyp unified TLB IS */
__RME_CAV7_TLBIALLNSNHIS_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C3 OP2=4
/* Invalidate entire Hyp unified TLB */
__RME_CAV7_TLBIALLH_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C7 OP2=0
/* Invalidate Hyp unified TLB entry by MVA */
__RME_CAV7_TLBIMVAH_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C7 OP2=1
/* Invalidate entire Non-secure non-Hyp unified TLB */
__RME_CAV7_TLBIALLNSNH_Set:
    CP15_SET            CRN=C8 OP1=4 CRM=C7 OP2=4

/* C9 registers currently unsupported */

/* TLB lockdown register - Cortex-A9 */
__RME_CAV7_TLBLR_Set:
    CP15_SET            CRN=C10 OP1=0 CRM=C0 OP2=0
/* Primary region remap register */
__RME_CAV7_PRRR_Set:
    CP15_SET            CRN=C10 OP1=0 CRM=C2 OP2=0
/* Normal memory remap register */
__RME_CAV7_NMRR_Set:
    CP15_SET            CRN=C10 OP1=0 CRM=C2 OP2=1
/* Auxiliary memory attribute indirection register 0 */
__RME_CAV7_AMAIR0_Set:
    CP15_SET            CRN=C10 OP1=0 CRM=C3 OP2=0
/* Auxiliary memory attribute indirection register 1 */
__RME_CAV7_AMAIR1_Set:
    CP15_SET            CRN=C10 OP1=0 CRM=C3 OP2=1
/* Hyp memory attribute indirection register 0 */
__RME_CAV7_HMAIR0_Set:
    CP15_SET            CRN=C10 OP1=4 CRM=C2 OP2=0
/* Hyp memory attribute indirection register 1 */
__RME_CAV7_HMAIR1_Set:
    CP15_SET            CRN=C10 OP1=4 CRM=C2 OP2=1
/* Hyp auxiliary memory attribute indirection register 0 */
__RME_CAV7_HAMAIR0_Set:
    CP15_SET            CRN=C10 OP1=4 CRM=C3 OP2=0
/* Hyp auxiliary memory attribute indirection register 1 */
__RME_CAV7_HAMAIR1_Set:
    CP15_SET            CRN=C10 OP1=4 CRM=C3 OP2=1

/* Vector base address register */
__RME_CAV7_VBAR_Set:
    CP15_SET            CRN=C12 OP1=0 CRM=C0 OP2=0
/* Vector base address register */
__RME_CAV7_MVBAR_Set:
    CP15_SET            CRN=C12 OP1=0 CRM=C0 OP2=1
/* Hyp vector base address register */
__RME_CAV7_HVBAR_Set:
    CP15_SET            CRN=C12 OP1=4 CRM=C0 OP2=0

/* Context ID register */
__RME_CAV7_CONTEXTIDR_Set:
    CP15_SET            CRN=C13 OP1=0 CRM=C0 OP2=1
/* User read/write software thread register */
__RME_CAV7_TPIDRURW_Set:
    CP15_SET            CRN=C13 OP1=0 CRM=C0 OP2=2
/* User read-only software thread register */
__RME_CAV7_TPIDRURO_Set:
    CP15_SET            CRN=C13 OP1=0 CRM=C0 OP2=3
/* PL1-only software thread register */
__RME_CAV7_TPIDRPRW_Set:
    CP15_SET            CRN=C13 OP1=0 CRM=C0 OP2=4
/* Hyp read/write software thread register */
__RME_CAV7_HTPIDR_Set:
    CP15_SET            CRN=C13 OP1=4 CRM=C0 OP2=2

/* Counter frequency register */
__RME_CAV7_CNTFRQ_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C0 OP2=0
/* Timer PL1 control register */
__RME_CAV7_CNTKCTL_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C1 OP2=0
/* PL1 physical timer value register */
__RME_CAV7_CNTP_TVAL_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C2 OP2=0
/* PL1 physical timer control register */
__RME_CAV7_CNTP_CTL_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C2 OP2=1
/* Virtual timer value register */
__RME_CAV7_CNTV_TVAL_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C3 OP2=0
/* Virtual timer control register */
__RME_CAV7_CNTV_CTL_Set:
    CP15_SET            CRN=C14 OP1=0 CRM=C3 OP2=1
/* Timer PL2 control register */
__RME_CAV7_CNTHCTL_Set:
    CP15_SET            CRN=C14 OP1=4 CRM=C1 OP2=0
/* PL2 physical timer value register */
__RME_CAV7_CNTHP_TVAL_Set:
    CP15_SET            CRN=C14 OP1=4 CRM=C2 OP2=0
/* PL2 physical timer control register */
__RME_CAV7_CNTHP_CTL_Set:
    CP15_SET            CRN=C14 OP1=4 CRM=C2 OP2=1
/* End Function:__RME_CAV7_XXXX_Set ******************************************/

/* Begin Function:__RME_CAV7_XXXX_DW_Set **************************************
Description : Set the XXXX register of the CPU. These registers must be written
              with MCRR instruction, and are all 64-bit double words.
Input       : rme_ptr_t R0 - The lower bits.
              rme_ptr_t R1 - The higher bits.
Output      : None.
Return      : None.
******************************************************************************/
/* PL1 physical timer compare value register */
__RME_CAV7_CNTP_CVAL_DW_Set:
    CP15_SET_DOUBLE     CRM=C14 OP=2
/* Virtual timer compare value register */
__RME_CAV7_CNTV_CVAL_DW_Set:
    CP15_SET_DOUBLE     CRM=C14 OP=3
/* Virtual offset register */
__RME_CAV7_CNTVOFF_DW_Set:
    CP15_SET_DOUBLE     CRM=C14 OP=4
/* L2 physical timer compare value register */
__RME_CAV7_CNTHP_CVAL_DW_Set:
    CP15_SET_DOUBLE     CRM=C14 OP=6
/* End Function:__RME_CAV7_XXXX_DW_Set ***************************************/

/* Begin Function:__RME_CAV7_Comp_Swap ****************************************
Description : The compare-and-swap atomic instruction. If the Old value is equal to
              *Ptr, then set the *Ptr as New and return 1; else return 0.
              This implementation is optimal on Cortex-A. Many compilers will generate
              the equivalent of this.
              It is easily notable that this operation is no longer predictable
Input       : ptr_t* Ptr - The pointer to the data.
              ptr_t Old - The old value.
              ptr_t New - The new value.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - If successful, 1; else 0.
******************************************************************************/
__RME_CAV7_Comp_Swap:
    DMB                 SY
    LDREX               R3,[R0]
    CMP                 R3,R1
    BNE                 __RME_CAV7_Comp_Swap_Fail
    STREX               R3,R2,[R0]
    CMP                 R3,#0x00
    BNE                 __RME_CAV7_Comp_Swap
    MOV                 R0,#0x01
    DMB                 SY
    BX                  LR
__RME_CAV7_Comp_Swap_Fail:
    CLREX
    MOV                 R0,#0x00
    BX                  LR
/* End Function:__RME_CAV7_Comp_Swap *****************************************/

/* Begin Function:__RME_CAV7_Fetch_Add ****************************************
Description : The fetch-and-add atomic instruction. Increase the value that is
              pointed to by the pointer, and return the value before addition.
              On ARM, the R12 is also a scratch register that we can use.
Input       : ptr_t* Ptr - The pointer to the data.
              cnt_t Addend - The number to add.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - The value before the addition.
******************************************************************************/
__RME_CAV7_Fetch_Add:
    LDREX               R2,[R0]
    ADD                 R3,R2,R1
    STREX               R12,R3,[R0]
    CMP                 R12,#0x00
    BNE                 __RME_CAV7_Fetch_Add
    MOV                 R0,R2
    BX                  LR
/* End Function:__RME_CAV7_Fetch_Add *****************************************/

/* Begin Function:__RME_CAV7_Fetch_And ****************************************
Description : The fetch-and-logic-and atomic instruction. Logic AND the pointer
              value with the operand, and return the value before logic AND.
Input       : ptr_t* Ptr - The pointer to the data.
              cnt_t Operand - The number to logic AND with the destination.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - The value before the AND operation.
******************************************************************************/
__RME_CAV7_Fetch_And:
    LDREX               R2,[R0]
    AND                 R3,R2,R1
    STREX               R12,R3,[R0]
    CMP                 R12,#0x00
    BNE                 __RME_CAV7_Fetch_Add
    MOV                 R0,R2
    BX                  LR
/* End Function:__RME_CAV7_Fetch_And *****************************************/

/* Begin Function:__RME_CAV7_Read_Acquire *************************************
Description : The read-acquire memory fence, to avoid read/write reorderings.
Input       : rme_ptr_t* R0 - Address to read from.
Output      : None.
Return      : None.
******************************************************************************/
__RME_CAV7_Read_Acquire:
    LDR                 R0,[R0]
    DMB
    BX                  LR
/* End Function:__RME_CAV7_Read_Acquire **************************************/

/* Begin Function:__RME_CAV7_Write_Release ************************************
Description : The write-release memory fence, to avoid read/write reorderings.
Input       : rme_ptr_t* R0 - Address to write to.
              rme_ptr_t R1 - Content to write to the address.
Output      : None.
Return      : None.
******************************************************************************/
__RME_CAV7_Write_Release:
    DMB
    STR                 R1,[R0]
    BX                  LR
/* End Function:__RME_CAV7_Write_Release *************************************/

/* Begin Function:__RME_Disable_Int *******************************************
Description : The function for disabling all interrupts.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_Disable_Int:
    CPSID               I
    BX                  LR
/* End Function:__RME_Disable_Int ********************************************/

/* Begin Function:__RME_Enable_Int ********************************************
Description : The function for enabling all interrupts.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_Enable_Int:
    CPSIE               I
    BX                  LR
/* End Function:__RME_Enable_Int *********************************************/

/* Begin Function:__RME_CAV7_Halt *********************************************
Description : Wait until a new interrupt comes, to save power.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_CAV7_Halt:
    /* Wait for interrupt */
    WFI
    BX                  LR
/* End Function:__RME_CAV7_Halt **********************************************/

/* Begin Function:_RME_Kmain **************************************************
Description : The entry address of the kernel. Never returns.
Input       : ptr_t Stack - The stack address to set SP to.
Output      : None.
Return      : None.
******************************************************************************/
_RME_Kmain:
    MOV                 R0,SP
    BL                  RME_Kmain
/* End Function:_RME_Kmain ***************************************************/

/* Begin Function:__RME_CAV7_MSB_Get ******************************************
Description : Get the MSB of the word. The kernel is guaranteed not to call this
              function with a zero word, so we don't need to handle this edge case
              actually.
Input       : ptr_t Val - The value.
Output      : None.
Return      : ptr_t - The MSB position.
******************************************************************************/
__RME_CAV7_MSB_Get:
    CLZ                 R1,R0
    MOV                 R0,#31
    SUB                 R0,R1
    BX                  LR
/* End Function:__RME_CAV7_MSB_Get *******************************************/

/* Begin Function:__RME_Enter_User_Mode ***************************************
Description : Entering of the user mode, after the system finish its preliminary
              booting. The function shall never return. This function should only
              be used to boot the first process in the system.
Input       : ptr_t Entry - The user execution startpoint.
              ptr_t Stack - The user stack.
              ptr_t CPUID - The CPUID.
Output      : None.
Return      : None.
******************************************************************************/
__RME_Enter_User_Mode:
    PUSH                {R0}
    MOV                 R0,R2
    /* Prepare the SPSR for user-level */
    MOV                 R2,#0x10
    MSR                 SPSR,R2
    MSR                 SP_usr,R1
    LDMIA               SP!,{PC}^
/* End Function:__RME_Enter_User_Mode ****************************************/

/* Begin Function:Reset_Handler ***********************************************
Description : The reset handler routine. This is not used in ARM, and is thus
              a dead loop.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
Reset_Handler:
Unused_Handler:
    B                   .
/* End Function:Reset_Handler ************************************************/

/* Begin Function:Undefined_Handler *******************************************
Description : The undefined instruction handler routine.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
/* Save all general-purpose registers */
.macro SAVE_GP_REGS
    /* Save user-mode PC */
    PUSH                {LR}
    /* Save user-mode SP and LR */
    STMDB               SP,{SP,LR}^
    SUB                 SP,SP,#0x08
    /* Save user-mode general-purpose registers */
    PUSH                {R0-R12}
    /* Save user-mode PSR */
    MRS                 R0,SPSR
    PUSH                {R0}
    MOV                 R0,SP
.endm

/* Restore all general-purpose registers */
.macro RESTORE_GP_REGS
    /* Restore user-mode PSR */
    POP                 {R0}
    MSR                 SPSR_cxsf,R0
    /* Restore user-mode general purpose registers */
    POP                 {R0-R12}
    /* Restore user-mode SP and LR */
    LDMIA               SP,{SP,LR}^
    ADD                 SP,SP,#0x08
    /* Restore user-mode PC */
    LDMIA               SP!,{PC}^
.endm

Undefined_Handler:
    SAVE_GP_REGS
    BL                  __RME_CAV7_Undefined_Handler
    RESTORE_GP_REGS
/* End Function:Undefined_Handler ********************************************/

/* Begin Function:Prefetch_Abort_Handler **************************************
Description : The prefetch abort handler routine.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
Prefetch_Abort_Handler:
    SAVE_GP_REGS
    BL                  __RME_CAV7_Prefetch_Abort_Handler
    RESTORE_GP_REGS
/* End Function:Prefetch_Abort_Handler ***************************************/

/* Begin Function:Data_Abort_Handler ******************************************
Description : The data abort handler routine.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
Data_Abort_Handler:
    SAVE_GP_REGS
    BL                  __RME_CAV7_Data_Abort_Handler
    RESTORE_GP_REGS
/* End Function:Data_Abort_Handler *******************************************/

/* Begin Function:SVC_Handler *************************************************
Description : The SVC handler routine.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
SVC_Handler:
    SAVE_GP_REGS
    BL                  _RME_Svc_Handler
    RESTORE_GP_REGS
/* End Function:SVC_Handler **************************************************/

/* Begin Function:IRQ_Handler *************************************************
Description : The IRQ handler routine.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
IRQ_Handler:
    LDR                 R0,=0x41210000
    LDR                 R1,=0x55555555
    STR                 R1,[R0]
    B .
    SAVE_GP_REGS
    BL                  __RME_CAV7_IRQ_Handler
    RESTORE_GP_REGS
/* End Function:IRQ_Handler **************************************************/

/* Begin Function:FIQ_Handler *************************************************
Description : The FIQ handler routine. Different from other routines, this routine
              will not call a C function, but should be supplied by the user him/
              herself. It is the user's responsibility to fill in anything he/she
              wants.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
FIQ_Handler:
    B                   .
    .ltorg
/* End Function:FIQ_Handler **************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
