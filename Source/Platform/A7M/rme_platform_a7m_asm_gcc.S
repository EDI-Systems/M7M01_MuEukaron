/******************************************************************************
Filename    : rme_platform_a7m_asm_gcc.S
Author      : pry
Date        : 19/01/2017
Description : The Cortex-M assembly support of the RME RTOS, for gcc.
******************************************************************************/

/* The ARM Cortex-M3/4/7 Architecture *****************************************
R0-R7:General purpose registers that are accessible. 
R8-R12:general purpose registers that can only be reached by 32-bit instructions.
R13:SP/SP_process/SP_main    Stack pointer
R14:LR                       Link Register(used for returning from a subfunction)
R15:PC                       Program counter.
IPSR                         Interrupt Program Status Register.
APSR                         Application Program Status Register.
EPSR                         Execute Program Status Register.
The above 3 registers are saved into the stack in combination(xPSR).

The ARM Cortex-M4 include a single-precision FPU, and the Cortex-M7 will feature
a double-precision FPU.
******************************************************************************/

/* Begin Stacks **************************************************************/

/* End Stacks ****************************************************************/

/* Begin Header **************************************************************/
    .section            .text
    .arch               armv7-m
    .thumb_func
    .syntax             unified
    .align              3
/* End Header ****************************************************************/

/* Begin Exports *************************************************************/
    /* Disable all interrupts */
    .global             __RME_Disable_Int
    /* Enable all interrupts */
    .global             __RME_Enable_Int
    /* Wait until interrupts happen */
    .global             __RME_A7M_Wait_Int
    /* Get the MSB in a word */
    .global             __RME_A7M_MSB_Get
    /* Kernel main function wrapper */
    .global             _RME_Kmain
    /* Entering of the user mode */
    .global             __RME_Enter_User_Mode
    /* The FPU register save routine */
    .global             ___RME_A7M_Thd_Cop_Save
    /* The FPU register restore routine */
    .global             ___RME_A7M_Thd_Cop_Restore
    /* The MPU setup routine */
    .global             ___RME_A7M_MPU_Set
/* End Exports ***************************************************************/

/* Begin Imports *************************************************************/
    /* What CMSIS provided. Have to call these. */
    .global             SystemInit
    /* The kernel entry of RME. This will be defined in C language. */
    .global             RME_Kmain
    /* The system call handler of RME. This will be defined in C language. */
    .global             _RME_Svc_Handler
    /* The system tick handler of RME. This will be defined in C language. */
    .global             _RME_Tick_Handler
    /* The memory management fault handler of RME. This will be defined in C language. */
    .global             __RME_A7M_Fault_Handler
    /* The generic interrupt handler for all other vectors. */
    .global             __RME_A7M_Generic_Handler
/* End Imports ***************************************************************/

/* Begin Vector Table ********************************************************/
    .global             __Vectors
    .global             __Vectors_End
    .global             __initial_sp

__Vectors:
	.long 				__initial_sp  		/* Top of Stack */
    .long               Reset_Handler       /* Reset Handler */
    .long               NMI_Handler         /* NMI Handler */
    .long               HardFault_Handler   /* Hard Fault Handler */
    .long               MemManage_Handler   /* MPU Fault Handler */
    .long               BusFault_Handler    /* Bus Fault Handler */
    .long               UsageFault_Handler  /* Usage Fault Handler */
    .long               0                   /* Reserved */
    .long               0                   /* Reserved */
    .long               0                   /* Reserved */
    .long               0                   /* Reserved */
    .long               SVC_Handler         /* SVCall Handler */
    .long               DebugMon_Handler    /* Debug Monitor Handler */
    .long               0                   /* Reserved */
    .long               PendSV_Handler      /* PendSV Handler */
    .long               SysTick_Handler     /* SysTick Handler */

    .long               IRQ0_Handler        /* 240 External Interrupts */
    .long               IRQ1_Handler
    .long               IRQ2_Handler
    .long               IRQ3_Handler
    .long               IRQ4_Handler
    .long               IRQ5_Handler
    .long               IRQ6_Handler
    .long               IRQ7_Handler
    .long               IRQ8_Handler
    .long               IRQ9_Handler

    .long               IRQ10_Handler
    .long               IRQ11_Handler
    .long               IRQ12_Handler
    .long               IRQ13_Handler
    .long               IRQ14_Handler
    .long               IRQ15_Handler
    .long               IRQ16_Handler
    .long               IRQ17_Handler
    .long               IRQ18_Handler
    .long               IRQ19_Handler

    .long               IRQ20_Handler
    .long               IRQ21_Handler
    .long               IRQ22_Handler
    .long               IRQ23_Handler
    .long               IRQ24_Handler
    .long               IRQ25_Handler
    .long               IRQ26_Handler
    .long               IRQ27_Handler
    .long               IRQ28_Handler
    .long               IRQ29_Handler

    .long               IRQ30_Handler
    .long               IRQ31_Handler
    .long               IRQ32_Handler
    .long               IRQ33_Handler
    .long               IRQ34_Handler
    .long               IRQ35_Handler
    .long               IRQ36_Handler
    .long               IRQ37_Handler
    .long               IRQ38_Handler
    .long               IRQ39_Handler

    .long               IRQ40_Handler
    .long               IRQ41_Handler
    .long               IRQ42_Handler
    .long               IRQ43_Handler
    .long               IRQ44_Handler
    .long               IRQ45_Handler
    .long               IRQ46_Handler
    .long               IRQ47_Handler
    .long               IRQ48_Handler
    .long               IRQ49_Handler

    .long               IRQ50_Handler
    .long               IRQ51_Handler
    .long               IRQ52_Handler
    .long               IRQ53_Handler
    .long               IRQ54_Handler
    .long               IRQ55_Handler
    .long               IRQ56_Handler
    .long               IRQ57_Handler
    .long               IRQ58_Handler
    .long               IRQ59_Handler

    .long               IRQ60_Handler
    .long               IRQ61_Handler
    .long               IRQ62_Handler
    .long               IRQ63_Handler
    .long               IRQ64_Handler
    .long               IRQ65_Handler
    .long               IRQ66_Handler
    .long               IRQ67_Handler
    .long               IRQ68_Handler
    .long               IRQ69_Handler

    .long               IRQ70_Handler
    .long               IRQ71_Handler
    .long               IRQ72_Handler
    .long               IRQ73_Handler
    .long               IRQ74_Handler
    .long               IRQ75_Handler
    .long               IRQ76_Handler
    .long               IRQ77_Handler
    .long               IRQ78_Handler
    .long               IRQ79_Handler

    .long               IRQ80_Handler
    .long               IRQ81_Handler
    .long               IRQ82_Handler
    .long               IRQ83_Handler
    .long               IRQ84_Handler
    .long               IRQ85_Handler
    .long               IRQ86_Handler
    .long               IRQ87_Handler
    .long               IRQ88_Handler
    .long               IRQ89_Handler

    .long               IRQ90_Handler
    .long               IRQ91_Handler
    .long               IRQ92_Handler
    .long               IRQ93_Handler
    .long               IRQ94_Handler
    .long               IRQ95_Handler
    .long               IRQ96_Handler
    .long               IRQ97_Handler
    .long               IRQ98_Handler
    .long               IRQ99_Handler

    .long               IRQ100_Handler
    .long               IRQ101_Handler
    .long               IRQ102_Handler
    .long               IRQ103_Handler
    .long               IRQ104_Handler
    .long               IRQ105_Handler
    .long               IRQ106_Handler
    .long               IRQ107_Handler
    .long               IRQ108_Handler
    .long               IRQ109_Handler

    .long               IRQ110_Handler
    .long               IRQ111_Handler
    .long               IRQ112_Handler
    .long               IRQ113_Handler
    .long               IRQ114_Handler
    .long               IRQ115_Handler
    .long               IRQ116_Handler
    .long               IRQ117_Handler
    .long               IRQ118_Handler
    .long               IRQ119_Handler

    .long               IRQ120_Handler
    .long               IRQ121_Handler
    .long               IRQ122_Handler
    .long               IRQ123_Handler
    .long               IRQ124_Handler
    .long               IRQ125_Handler
    .long               IRQ126_Handler
    .long               IRQ127_Handler
    .long               IRQ128_Handler
    .long               IRQ129_Handler

    .long               IRQ130_Handler
    .long               IRQ131_Handler
    .long               IRQ132_Handler
    .long               IRQ133_Handler
    .long               IRQ134_Handler
    .long               IRQ135_Handler
    .long               IRQ136_Handler
    .long               IRQ137_Handler
    .long               IRQ138_Handler
    .long               IRQ139_Handler

    .long               IRQ140_Handler
    .long               IRQ141_Handler
    .long               IRQ142_Handler
    .long               IRQ143_Handler
    .long               IRQ144_Handler
    .long               IRQ145_Handler
    .long               IRQ146_Handler
    .long               IRQ147_Handler
    .long               IRQ148_Handler
    .long               IRQ149_Handler

    .long               IRQ150_Handler
    .long               IRQ151_Handler
    .long               IRQ152_Handler
    .long               IRQ153_Handler
    .long               IRQ154_Handler
    .long               IRQ155_Handler
    .long               IRQ156_Handler
    .long               IRQ157_Handler
    .long               IRQ158_Handler
    .long               IRQ159_Handler

    .long               IRQ160_Handler
    .long               IRQ161_Handler
    .long               IRQ162_Handler
    .long               IRQ163_Handler
    .long               IRQ164_Handler
    .long               IRQ165_Handler
    .long               IRQ166_Handler
    .long               IRQ167_Handler
    .long               IRQ168_Handler
    .long               IRQ169_Handler

    .long               IRQ170_Handler
    .long               IRQ171_Handler
    .long               IRQ172_Handler
    .long               IRQ173_Handler
    .long               IRQ174_Handler
    .long               IRQ175_Handler
    .long               IRQ176_Handler
    .long               IRQ177_Handler
    .long               IRQ178_Handler
    .long               IRQ179_Handler

    .long               IRQ180_Handler
    .long               IRQ181_Handler
    .long               IRQ182_Handler
    .long               IRQ183_Handler
    .long               IRQ184_Handler
    .long               IRQ185_Handler
    .long               IRQ186_Handler
    .long               IRQ187_Handler
    .long               IRQ188_Handler
    .long               IRQ189_Handler

    .long               IRQ190_Handler
    .long               IRQ191_Handler
    .long               IRQ192_Handler
    .long               IRQ193_Handler
    .long               IRQ194_Handler
    .long               IRQ195_Handler
    .long               IRQ196_Handler
    .long               IRQ197_Handler
    .long               IRQ198_Handler
    .long               IRQ199_Handler
    
    .long               IRQ200_Handler
    .long               IRQ201_Handler
    .long               IRQ202_Handler
    .long               IRQ203_Handler
    .long               IRQ204_Handler
    .long               IRQ205_Handler
    .long               IRQ206_Handler
    .long               IRQ207_Handler
    .long               IRQ208_Handler
    .long               IRQ209_Handler

    .long               IRQ210_Handler
    .long               IRQ211_Handler
    .long               IRQ212_Handler
    .long               IRQ213_Handler
    .long               IRQ214_Handler
    .long               IRQ215_Handler
    .long               IRQ216_Handler
    .long               IRQ217_Handler
    .long               IRQ218_Handler
    .long               IRQ219_Handler

    .long               IRQ220_Handler
    .long               IRQ221_Handler
    .long               IRQ222_Handler
    .long               IRQ223_Handler
    .long               IRQ224_Handler
    .long               IRQ225_Handler
    .long               IRQ226_Handler
    .long               IRQ227_Handler
    .long               IRQ228_Handler
    .long               IRQ229_Handler

    .long               IRQ230_Handler
    .long               IRQ231_Handler
    .long               IRQ232_Handler
    .long               IRQ233_Handler
    .long               IRQ234_Handler
    .long               IRQ235_Handler
    .long               IRQ236_Handler
    .long               IRQ237_Handler
    .long               IRQ238_Handler
    .long               IRQ239_Handler
__Vectors_End:
/* End Vector Table **********************************************************/

/* Begin Memory Init *********************************************************/

/* End Memory Init ***********************************************************/

/* Begin Handlers ************************************************************/
.macro THUMB FUNC
	.thumb_func
    \FUNC
.endm

    .global             _sidata
    .global             __data_start__
    .global             __data_end__
    .global             __bss_start__
    .global             __bss_end__
    .global             main
    .global				_start
THUMB _start:
THUMB Reset_Handler:
    LDR                 R0, =SystemInit
    BLX                 R0
    /* Initialize the data section */
    LDR                 R0,=__data_start__
    LDR                 R1,=__data_end__
    LDR                 R2,=_sidata
load_data:
    CMP                 R0,R1
    BEQ                 load_done
    LDR					R3,[R2]
    STR                 R3,[R0]
    ADD                 R0,#0x04
    ADD                 R2,#0x04
    B                   load_data
load_done:
	/* Initialize the bss section */
    LDR                 R0,=__bss_start__
    LDR                 R1,=__bss_end__
    LDR                 R2,=0x00
clear_bss:
    CMP                 R0,R1
    BEQ                 clear_done
    STR                 R2,[R0]
    ADD                 R0,#0x04
    B                   clear_bss
clear_done:
    LDR                 R0, =main
    BX                  R0

THUMB Default_Handler:
    .weak               IRQ0_Handler        /* 240 External Interrupts */
    .weak               IRQ1_Handler
    .weak               IRQ2_Handler
    .weak               IRQ3_Handler
    .weak               IRQ4_Handler
    .weak               IRQ5_Handler
    .weak               IRQ6_Handler
    .weak               IRQ7_Handler
    .weak               IRQ8_Handler
    .weak               IRQ9_Handler

    .weak               IRQ10_Handler
    .weak               IRQ11_Handler
    .weak               IRQ12_Handler
    .weak               IRQ13_Handler
    .weak               IRQ14_Handler
    .weak               IRQ15_Handler
    .weak               IRQ16_Handler
    .weak               IRQ17_Handler
    .weak               IRQ18_Handler
    .weak               IRQ19_Handler

    .weak               IRQ20_Handler
    .weak               IRQ21_Handler
    .weak               IRQ22_Handler
    .weak               IRQ23_Handler
    .weak               IRQ24_Handler
    .weak               IRQ25_Handler
    .weak               IRQ26_Handler
    .weak               IRQ27_Handler
    .weak               IRQ28_Handler
    .weak               IRQ29_Handler

    .weak               IRQ30_Handler
    .weak               IRQ31_Handler
    .weak               IRQ32_Handler
    .weak               IRQ33_Handler
    .weak               IRQ34_Handler
    .weak               IRQ35_Handler
    .weak               IRQ36_Handler
    .weak               IRQ37_Handler
    .weak               IRQ38_Handler
    .weak               IRQ39_Handler

    .weak               IRQ40_Handler
    .weak               IRQ41_Handler
    .weak               IRQ42_Handler
    .weak               IRQ43_Handler
    .weak               IRQ44_Handler
    .weak               IRQ45_Handler
    .weak               IRQ46_Handler
    .weak               IRQ47_Handler
    .weak               IRQ48_Handler
    .weak               IRQ49_Handler

    .weak               IRQ50_Handler
    .weak               IRQ51_Handler
    .weak               IRQ52_Handler
    .weak               IRQ53_Handler
    .weak               IRQ54_Handler
    .weak               IRQ55_Handler
    .weak               IRQ56_Handler
    .weak               IRQ57_Handler
    .weak               IRQ58_Handler
    .weak               IRQ59_Handler

    .weak               IRQ60_Handler
    .weak               IRQ61_Handler
    .weak               IRQ62_Handler
    .weak               IRQ63_Handler
    .weak               IRQ64_Handler
    .weak               IRQ65_Handler
    .weak               IRQ66_Handler
    .weak               IRQ67_Handler
    .weak               IRQ68_Handler
    .weak               IRQ69_Handler

    .weak               IRQ70_Handler
    .weak               IRQ71_Handler
    .weak               IRQ72_Handler
    .weak               IRQ73_Handler
    .weak               IRQ74_Handler
    .weak               IRQ75_Handler
    .weak               IRQ76_Handler
    .weak               IRQ77_Handler
    .weak               IRQ78_Handler
    .weak               IRQ79_Handler

    .weak               IRQ80_Handler
    .weak               IRQ81_Handler
    .weak               IRQ82_Handler
    .weak               IRQ83_Handler
    .weak               IRQ84_Handler
    .weak               IRQ85_Handler
    .weak               IRQ86_Handler
    .weak               IRQ87_Handler
    .weak               IRQ88_Handler
    .weak               IRQ89_Handler

    .weak               IRQ90_Handler
    .weak               IRQ91_Handler
    .weak               IRQ92_Handler
    .weak               IRQ93_Handler
    .weak               IRQ94_Handler
    .weak               IRQ95_Handler
    .weak               IRQ96_Handler
    .weak               IRQ97_Handler
    .weak               IRQ98_Handler
    .weak               IRQ99_Handler

    .weak               IRQ100_Handler
    .weak               IRQ101_Handler
    .weak               IRQ102_Handler
    .weak               IRQ103_Handler
    .weak               IRQ104_Handler
    .weak               IRQ105_Handler
    .weak               IRQ106_Handler
    .weak               IRQ107_Handler
    .weak               IRQ108_Handler
    .weak               IRQ109_Handler

    .weak               IRQ110_Handler
    .weak               IRQ111_Handler
    .weak               IRQ112_Handler
    .weak               IRQ113_Handler
    .weak               IRQ114_Handler
    .weak               IRQ115_Handler
    .weak               IRQ116_Handler
    .weak               IRQ117_Handler
    .weak               IRQ118_Handler
    .weak               IRQ119_Handler

    .weak               IRQ120_Handler
    .weak               IRQ121_Handler
    .weak               IRQ122_Handler
    .weak               IRQ123_Handler
    .weak               IRQ124_Handler
    .weak               IRQ125_Handler
    .weak               IRQ126_Handler
    .weak               IRQ127_Handler
    .weak               IRQ128_Handler
    .weak               IRQ129_Handler

    .weak               IRQ130_Handler
    .weak               IRQ131_Handler
    .weak               IRQ132_Handler
    .weak               IRQ133_Handler
    .weak               IRQ134_Handler
    .weak               IRQ135_Handler
    .weak               IRQ136_Handler
    .weak               IRQ137_Handler
    .weak               IRQ138_Handler
    .weak               IRQ139_Handler

    .weak               IRQ140_Handler
    .weak               IRQ141_Handler
    .weak               IRQ142_Handler
    .weak               IRQ143_Handler
    .weak               IRQ144_Handler
    .weak               IRQ145_Handler
    .weak               IRQ146_Handler
    .weak               IRQ147_Handler
    .weak               IRQ148_Handler
    .weak               IRQ149_Handler

    .weak               IRQ150_Handler
    .weak               IRQ151_Handler
    .weak               IRQ152_Handler
    .weak               IRQ153_Handler
    .weak               IRQ154_Handler
    .weak               IRQ155_Handler
    .weak               IRQ156_Handler
    .weak               IRQ157_Handler
    .weak               IRQ158_Handler
    .weak               IRQ159_Handler

    .weak               IRQ160_Handler
    .weak               IRQ161_Handler
    .weak               IRQ162_Handler
    .weak               IRQ163_Handler
    .weak               IRQ164_Handler
    .weak               IRQ165_Handler
    .weak               IRQ166_Handler
    .weak               IRQ167_Handler
    .weak               IRQ168_Handler
    .weak               IRQ169_Handler

    .weak               IRQ170_Handler
    .weak               IRQ171_Handler
    .weak               IRQ172_Handler
    .weak               IRQ173_Handler
    .weak               IRQ174_Handler
    .weak               IRQ175_Handler
    .weak               IRQ176_Handler
    .weak               IRQ177_Handler
    .weak               IRQ178_Handler
    .weak               IRQ179_Handler

    .weak               IRQ180_Handler
    .weak               IRQ181_Handler
    .weak               IRQ182_Handler
    .weak               IRQ183_Handler
    .weak               IRQ184_Handler
    .weak               IRQ185_Handler
    .weak               IRQ186_Handler
    .weak               IRQ187_Handler
    .weak               IRQ188_Handler
    .weak               IRQ189_Handler

    .weak               IRQ190_Handler
    .weak               IRQ191_Handler
    .weak               IRQ192_Handler
    .weak               IRQ193_Handler
    .weak               IRQ194_Handler
    .weak               IRQ195_Handler
    .weak               IRQ196_Handler
    .weak               IRQ197_Handler
    .weak               IRQ198_Handler
    .weak               IRQ199_Handler
    
    .weak               IRQ200_Handler
    .weak               IRQ201_Handler
    .weak               IRQ202_Handler
    .weak               IRQ203_Handler
    .weak               IRQ204_Handler
    .weak               IRQ205_Handler
    .weak               IRQ206_Handler
    .weak               IRQ207_Handler
    .weak               IRQ208_Handler
    .weak               IRQ209_Handler

    .weak               IRQ210_Handler
    .weak               IRQ211_Handler
    .weak               IRQ212_Handler
    .weak               IRQ213_Handler
    .weak               IRQ214_Handler
    .weak               IRQ215_Handler
    .weak               IRQ216_Handler
    .weak               IRQ217_Handler
    .weak               IRQ218_Handler
    .weak               IRQ219_Handler

    .weak               IRQ220_Handler
    .weak               IRQ221_Handler
    .weak               IRQ222_Handler
    .weak               IRQ223_Handler
    .weak               IRQ224_Handler
    .weak               IRQ225_Handler
    .weak               IRQ226_Handler
    .weak               IRQ227_Handler
    .weak               IRQ228_Handler
    .weak               IRQ229_Handler

    .weak               IRQ230_Handler
    .weak               IRQ231_Handler
    .weak               IRQ232_Handler
    .weak               IRQ233_Handler
    .weak               IRQ234_Handler
    .weak               IRQ235_Handler
    .weak               IRQ236_Handler
    .weak               IRQ237_Handler
    .weak               IRQ238_Handler
    .weak               IRQ239_Handler

THUMB IRQ0_Handler:
THUMB IRQ1_Handler:
THUMB IRQ2_Handler:
THUMB IRQ3_Handler:
THUMB IRQ4_Handler:
THUMB IRQ5_Handler:
THUMB IRQ6_Handler:
THUMB IRQ7_Handler:
THUMB IRQ8_Handler:
THUMB IRQ9_Handler:

THUMB IRQ10_Handler:
THUMB IRQ11_Handler:
THUMB IRQ12_Handler:
THUMB IRQ13_Handler:
THUMB IRQ14_Handler:
THUMB IRQ15_Handler:
THUMB IRQ16_Handler:
THUMB IRQ17_Handler:
THUMB IRQ18_Handler:
THUMB IRQ19_Handler:

THUMB IRQ20_Handler:
THUMB IRQ21_Handler:
THUMB IRQ22_Handler:
THUMB IRQ23_Handler:
THUMB IRQ24_Handler:
THUMB IRQ25_Handler:
THUMB IRQ26_Handler:
THUMB IRQ27_Handler:
THUMB IRQ28_Handler:
THUMB IRQ29_Handler:

THUMB IRQ30_Handler:
THUMB IRQ31_Handler:
THUMB IRQ32_Handler:
THUMB IRQ33_Handler:
THUMB IRQ34_Handler:
THUMB IRQ35_Handler:
THUMB IRQ36_Handler:
THUMB IRQ37_Handler:
THUMB IRQ38_Handler:
THUMB IRQ39_Handler:

THUMB IRQ40_Handler:
THUMB IRQ41_Handler:
THUMB IRQ42_Handler:
THUMB IRQ43_Handler:
THUMB IRQ44_Handler:
THUMB IRQ45_Handler:
THUMB IRQ46_Handler:
THUMB IRQ47_Handler:
THUMB IRQ48_Handler:
THUMB IRQ49_Handler:

THUMB IRQ50_Handler:
THUMB IRQ51_Handler:
THUMB IRQ52_Handler:
THUMB IRQ53_Handler:
THUMB IRQ54_Handler:
THUMB IRQ55_Handler:
THUMB IRQ56_Handler:
THUMB IRQ57_Handler:
THUMB IRQ58_Handler:
THUMB IRQ59_Handler:

THUMB IRQ60_Handler:
THUMB IRQ61_Handler:
THUMB IRQ62_Handler:
THUMB IRQ63_Handler:
THUMB IRQ64_Handler:
THUMB IRQ65_Handler:
THUMB IRQ66_Handler:
THUMB IRQ67_Handler:
THUMB IRQ68_Handler:
THUMB IRQ69_Handler:
 
THUMB IRQ70_Handler:
THUMB IRQ71_Handler:
THUMB IRQ72_Handler:
THUMB IRQ73_Handler:
THUMB IRQ74_Handler:
THUMB IRQ75_Handler:
THUMB IRQ76_Handler:
THUMB IRQ77_Handler:
THUMB IRQ78_Handler:
THUMB IRQ79_Handler:

THUMB IRQ80_Handler:
THUMB IRQ81_Handler:
THUMB IRQ82_Handler:
THUMB IRQ83_Handler:
THUMB IRQ84_Handler:
THUMB IRQ85_Handler:
THUMB IRQ86_Handler:
THUMB IRQ87_Handler:
THUMB IRQ88_Handler:
THUMB IRQ89_Handler:
 
THUMB IRQ90_Handler:
THUMB IRQ91_Handler:
THUMB IRQ92_Handler:
THUMB IRQ93_Handler:
THUMB IRQ94_Handler:
THUMB IRQ95_Handler:
THUMB IRQ96_Handler:
THUMB IRQ97_Handler:
THUMB IRQ98_Handler:
THUMB IRQ99_Handler:

THUMB IRQ100_Handler:
THUMB IRQ101_Handler:
THUMB IRQ102_Handler:
THUMB IRQ103_Handler:
THUMB IRQ104_Handler:
THUMB IRQ105_Handler:
THUMB IRQ106_Handler:
THUMB IRQ107_Handler:
THUMB IRQ108_Handler:
THUMB IRQ109_Handler:

THUMB IRQ110_Handler:
THUMB IRQ111_Handler:
THUMB IRQ112_Handler:
THUMB IRQ113_Handler:
THUMB IRQ114_Handler:
THUMB IRQ115_Handler:
THUMB IRQ116_Handler:
THUMB IRQ117_Handler:
THUMB IRQ118_Handler:
THUMB IRQ119_Handler:

THUMB IRQ120_Handler:
THUMB IRQ121_Handler:
THUMB IRQ122_Handler:
THUMB IRQ123_Handler:
THUMB IRQ124_Handler:
THUMB IRQ125_Handler:
THUMB IRQ126_Handler:
THUMB IRQ127_Handler:
THUMB IRQ128_Handler:
THUMB IRQ129_Handler:

THUMB IRQ130_Handler:
THUMB IRQ131_Handler:
THUMB IRQ132_Handler:
THUMB IRQ133_Handler:
THUMB IRQ134_Handler:
THUMB IRQ135_Handler:
THUMB IRQ136_Handler:
THUMB IRQ137_Handler:
THUMB IRQ138_Handler:
THUMB IRQ139_Handler:

THUMB IRQ140_Handler:
THUMB IRQ141_Handler:
THUMB IRQ142_Handler:
THUMB IRQ143_Handler:
THUMB IRQ144_Handler:
THUMB IRQ145_Handler:
THUMB IRQ146_Handler:
THUMB IRQ147_Handler:
THUMB IRQ148_Handler:
THUMB IRQ149_Handler:

THUMB IRQ150_Handler:
THUMB IRQ151_Handler:
THUMB IRQ152_Handler:
THUMB IRQ153_Handler:
THUMB IRQ154_Handler:
THUMB IRQ155_Handler:
THUMB IRQ156_Handler:
THUMB IRQ157_Handler:
THUMB IRQ158_Handler:
THUMB IRQ159_Handler:

THUMB IRQ160_Handler:
THUMB IRQ161_Handler:
THUMB IRQ162_Handler:
THUMB IRQ163_Handler:
THUMB IRQ164_Handler:
THUMB IRQ165_Handler:
THUMB IRQ166_Handler:
THUMB IRQ167_Handler:
THUMB IRQ168_Handler:
THUMB IRQ169_Handler:
 
THUMB IRQ170_Handler:
THUMB IRQ171_Handler:
THUMB IRQ172_Handler:
THUMB IRQ173_Handler:
THUMB IRQ174_Handler:
THUMB IRQ175_Handler:
THUMB IRQ176_Handler:
THUMB IRQ177_Handler:
THUMB IRQ178_Handler:
THUMB IRQ179_Handler:

THUMB IRQ180_Handler:
THUMB IRQ181_Handler:
THUMB IRQ182_Handler:
THUMB IRQ183_Handler:
THUMB IRQ184_Handler:
THUMB IRQ185_Handler:
THUMB IRQ186_Handler:
THUMB IRQ187_Handler:
THUMB IRQ188_Handler:
THUMB IRQ189_Handler:
 
THUMB IRQ190_Handler:
THUMB IRQ191_Handler:
THUMB IRQ192_Handler:
THUMB IRQ193_Handler:
THUMB IRQ194_Handler:
THUMB IRQ195_Handler:
THUMB IRQ196_Handler:
THUMB IRQ197_Handler:
THUMB IRQ198_Handler:
THUMB IRQ199_Handler:
                
THUMB IRQ200_Handler:
THUMB IRQ201_Handler:
THUMB IRQ202_Handler:
THUMB IRQ203_Handler:
THUMB IRQ204_Handler:
THUMB IRQ205_Handler:
THUMB IRQ206_Handler:
THUMB IRQ207_Handler:
THUMB IRQ208_Handler:
THUMB IRQ209_Handler:

THUMB IRQ210_Handler:
THUMB IRQ211_Handler:
THUMB IRQ212_Handler:
THUMB IRQ213_Handler:
THUMB IRQ214_Handler:
THUMB IRQ215_Handler:
THUMB IRQ216_Handler:
THUMB IRQ217_Handler:
THUMB IRQ218_Handler:
THUMB IRQ219_Handler:

THUMB IRQ220_Handler:
THUMB IRQ221_Handler:
THUMB IRQ222_Handler:
THUMB IRQ223_Handler:
THUMB IRQ224_Handler:
THUMB IRQ225_Handler:
THUMB IRQ226_Handler:
THUMB IRQ227_Handler:
THUMB IRQ228_Handler:
THUMB IRQ229_Handler:

THUMB IRQ230_Handler:
THUMB IRQ231_Handler:
THUMB IRQ232_Handler:
THUMB IRQ233_Handler:
THUMB IRQ234_Handler:
THUMB IRQ235_Handler:
THUMB IRQ236_Handler:
THUMB IRQ237_Handler:
THUMB IRQ238_Handler:
THUMB IRQ239_Handler:
    PUSH                {LR}
    PUSH                {R4-R11}            /* Spill all the general purpose registers; empty descending */
    MRS                 R0,PSP
    PUSH                {R0}
    
    MOV                 R0,SP               /* Pass in the pt_regs parameter, and call the handler. */
    MRS                 R1,xPSR             /* Pass in the interrupt number */
    UBFX                R1,R1,#0,#9         /* Extract the interrupt number bitfield */
    SUB                 R1,#16              /* The IRQ0's starting number is 16. we subtract it here */
    BL                  __RME_A7M_Generic_Handler
    
    POP                 {R0}
    MSR                 PSP,R0
    POP                 {R4-R11}
    POP                 {PC}                /* Now we reset the PC. */
    B                   .                   /* Capture faults */
/* End Handlers **************************************************************/

/* Begin Function:__RME_Disable_Int *******************************************
Description : The function for disabling all interrupts.
Input       : None.
Output      : None. 
Return      : None.                                
******************************************************************************/    
THUMB __RME_Disable_Int:
    /* Disable all interrupts (I is primask, F is faultmask.) */
    CPSID               I 
    BX                  LR                                                 
/* End Function:__RME_Disable_Int ********************************************/

/* Begin Function:__RME_Enable_Int ********************************************
Description : The function for enabling all interrupts.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
THUMB __RME_Enable_Int:
    /* Enable all interrupts. */
    CPSIE               I 
    BX                  LR
/* End Function:__RME_Enable_Int *********************************************/

/* Begin Function:__RME_A7M_Wait_Int ******************************************
Description : Wait until a new interrupt comes, to save power.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
THUMB __RME_A7M_Wait_Int:
    /* Wait for interrupt */
    WFI 
    BX                  LR
/* End Function:__RME_A7M_Wait_Int *******************************************/

/* Begin Function:_RME_Kmain **************************************************
Description : The entry address of the kernel. Never returns.
Input       : ptr_t Stack - The stack address to set SP to.
Output      : None.
Return      : None.
******************************************************************************/
THUMB _RME_Kmain:
    MOV                 SP,R0
    B                   RME_Kmain
    B                   .
/* End Function:_RME_Kmain ***************************************************/

/* Begin Function:__RME_A7M_MSB_Get *******************************************
Description : Get the MSB of the word.
Input       : ptr_t Val - The value.
Output      : None.
Return      : ptr_t - The MSB position.
******************************************************************************/
THUMB __RME_A7M_MSB_Get:
    CLZ                 R1,R0
    MOV                 R0,#31
    SUB                 R0,R1
    BX                  LR
/* End Function:__RME_A7M_MSB_Get ********************************************/

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
THUMB __RME_Enter_User_Mode:
    MSR                 PSP,R1              /* Set the stack pointer */
    MOV                 R4,#0x03            /* Unprevileged thread mode */
    MSR                 CONTROL,R4
    MOV                 R1,R0               /* Save the entry to R1 */
    MOV                 R0,R2               /* Save CPUID(always 0) to R0 */
    BLX                 R1                  /* Branch to our target */
    B                   .                   /* Capture faults */
/* End Function:__RME_Enter_User_Mode ****************************************/

/* Begin Function:SysTick_Handler *********************************************
Description : The System Tick Timer handler routine. This will in fact call a
              C function to resolve the system service routines.             
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
THUMB SysTick_Handler:
    PUSH                {LR}
    PUSH                {R4-R11}            /* Spill all the general purpose registers; empty descending */
    MRS                 R0,PSP
    PUSH                {R0}
    
    MOV                 R0,SP               /* Pass in the pt_regs parameter, and call the handler. */
    BL                  _RME_Tick_Handler
    
    POP                 {R0}
    MSR                 PSP,R0
    POP                 {R4-R11}
    POP                 {PC}                /* Now we reset the PC. */
    B                   .                   /* Capture faults */
/* End Function:SysTick_Handler **********************************************/

/* Begin Function:SVC_Handler *************************************************
Description : The SVC handler routine. This will in fact call a C function to resolve
              the system service routines.             
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
THUMB SVC_Handler:
    PUSH                {LR}
    PUSH                {R4-R11}            /* Spill all the general purpose registers; empty descending */
    MRS                 R0,PSP
    PUSH                {R0}
    
    MOV                 R0,SP               /* Pass in the pt_regs parameter, and call the handler. */
    BL                  _RME_Svc_Handler
    
    POP                 {R0}
    MSR                 PSP,R0
    POP                 {R4-R11}
    POP                 {PC}                /* Now we reset the PC. */
    B                   .                   /* Capture faults */
/* End Function:SVC_Handler **************************************************/

/* Begin Function:NMI/HardFault/MemManage/BusFault/UsageFault_Handler *********
Description : The multi-purpose handler routine. This will in fact call
              a C function to resolve the system service routines.             
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
THUMB NMI_Handler:
    NOP
THUMB PendSV_Handler:
    NOP
THUMB DebugMon_Handler:
    NOP
THUMB HardFault_Handler:
    NOP
                
THUMB MemManage_Handler:
    NOP
THUMB BusFault_Handler:
    NOP
THUMB UsageFault_Handler:
    PUSH                {LR}
    PUSH                {R4-R11}            /* Spill all the general purpose registers; empty descending */
    MRS                 R0,PSP
    PUSH                {R0}
    
    MOV                 R0,SP               /* Pass in the pt_regs parameter, and call the handler. */
    BL                  __RME_A7M_Fault_Handler
    
    POP                 {R0}
    MSR                 PSP,R0
    POP                 {R4-R11}
    POP                 {PC}                /* Now we reset the PC. */
    B                   .                   /* Capture faults */
/* End Function:NMI/HardFault/MemManage/BusFault/UsageFault_Handler **********/

/* Begin Function:___RME_A7M_Thd_Cop_Save *************************************
Description : Save the coprocessor context on switch.         
Input       : R0 - The pointer to the coprocessor struct.
Output      : None.
Return      : None.
******************************************************************************/
THUMB ___RME_A7M_Thd_Cop_Save:
    /* Use DCI to avoid compilation errors when FPU not enabled. Anyway,
     * this will not be called when FPU not enabled. */
    .long               0x8A10ED20          /* VSTMDB    R0!,{S16-S31}, Save all the FPU registers */
    BX                  LR
    B                   .
/* End Function:___RME_A7M_Thd_Cop_Save **************************************/

/* Begin Function:___RME_A7M_Thd_Cop_Restore **********************************
Description : Restore the coprocessor context on switch.             
Input       : R0 - The pointer to the coprocessor struct.
Output      : None.
Return      : None.
******************************************************************************/
THUMB ___RME_A7M_Thd_Cop_Restore:
    /* Use DCI to avoid compilation errors when FPU not enabled. Anyway,
     * this will not be called when FPU not enabled. */
    .long               0x8A10ECB0              /* VLDMIA    R0!,{S16-S31}, Restore all the FPU registers */
    BX                  LR
    B                   .
/* End Function:___RME_A7M_Thd_Cop_Restore ***********************************/

/* Begin Function:___RME_A7M_MPU_Set ******************************************
Description : Set the MPU context. We write 8 registers at a time to increase efficiency.            
Input       : R0 - The pointer to the MPU content.
Output      : None.
Return      : None.
******************************************************************************/
THUMB ___RME_A7M_MPU_Set:
    PUSH                {R4-R9}             /* Clobber registers manually */
    LDR                 R1,=0xE000ED9C      /* The base address of MPU RBAR and all 4 registers */
    LDMIA               R0!,{R2-R9}         /* Read MPU settings from the array, and increase pointer */
    STMIA               R1,{R2-R9}          /* Write the settings but do not increase pointer */
    LDMIA               R0!,{R2-R9}
    STMIA               R1,{R2-R9}
    POP                 {R4-R9}
    DSB                                     /* Make sure that the MPU update completes. */
    ISB                                     /* Fetch new instructions */
    BX                  LR
/* End Function:___RME_A7M_MPU_Set *******************************************/

	.end
/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
