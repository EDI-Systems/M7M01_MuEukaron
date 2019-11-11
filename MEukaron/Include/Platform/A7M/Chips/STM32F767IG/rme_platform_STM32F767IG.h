/******************************************************************************
Filename   : rme_platform_STM32F767IG.h
Author     : pry
Date       : 24/06/2017
Licence    : The Unlicense; see LICENSE for details.
Description: The configuration file for STM32F767IG.
******************************************************************************/

/* Defines *******************************************************************/
/* Generator *****************************************************************/
/* Are we using the generator in the first place? */
#define RME_GEN_ENABLE                                  RME_FALSE
/* Modifiable ****************************************************************/
/* The virtual memory start address for the kernel objects */
#define RME_KMEM_VA_START                               (0x20003000)
/* The size of the kernel object virtual memory */
#define RME_KMEM_SIZE                                   (0xD000)
/* The virtual memory start address for the virtual machines - If no VM is used, set to 0 */
#define RME_HYP_VA_START                                (0x20020000)
/* The size of the hypervisor reserved virtual memory */
#define RME_HYP_SIZE                                    (0x60000)
/* Kernel stack address - we have 4kB stack */
#define RME_KMEM_STACK_ADDR                             (0x20000FF0)
/* The maximum number of preemption priority levels in the system.
 * This parameter must be divisible by the word length - 32 is usually sufficient */
#define RME_MAX_PREEMPT_PRIO                            (32)
/* Size of capability table */
#if(RME_GEN_ENABLE==RME_TRUE)
#define RME_A7M_BOOT_CAPTBL_SIZE                        (18)
#else
#define RME_A7M_BOOT_CAPTBL_SIZE                        (18)
#endif
/* Shared vector flag region address - always 512B memory for ARMv7-M */
#define RME_A7M_VECT_FLAG_ADDR                          (0x2000FC00)
/* Shared interrupt flag region address - always 512B memory for ARMv7-M */
#define RME_A7M_EVT_FLAG_ADDR                           (0x2000FE00)
/* Initial kernel object frontier limit */
#define RME_A7M_KMEM_BOOT_FRONTIER                      (0x20003400)
/* Init process's first thread's entry point address */
#define RME_A7M_INIT_ENTRY                              (0x08010001)
/* Init process's first thread's stack address */
#define RME_A7M_INIT_STACK                              (0x2001FFF0)
/* What is the NVIC priority grouping? */
#define RME_A7M_NVIC_GROUPING                           (RME_A7M_NVIC_GROUPING_P2S6)
/* What is the Systick value? */
#define RME_A7M_SYSTICK_VAL                             (2160000)

/* What is the external crystal frequency? */
#define RME_A7M_STM32F767IG_XTAL                        (25)
/* What are the PLL values? */
#define RME_A7M_STM32F767IG_PLLM                        (25)
#define RME_A7M_STM32F767IG_PLLN                        (432)
#define RME_A7M_STM32F767IG_PLLP                        (2)
#define RME_A7M_STM32F767IG_PLLQ                        (9)
#define RME_A7M_STM32F767IG_PLLR                        (0)

/* Fixed *********************************************************************/
/* The granularity of kernel memory allocation, in bytes */
#define RME_KMEM_SLOT_ORDER                             (4)
/* Number of MPU regions available */
#define RME_A7M_MPU_REGIONS                             (8)
/* What is the FPU type? */
#define RME_A7M_FPU_TYPE                                (RME_A7M_FPU_FPV5_DP)
/* What is the vector number excluding system vectors? */
#define RME_A7M_VECT_NUM                                (110)

/* Interrupts ****************************************************************/
#define  WWDG_IRQHandler                                IRQ0_Handler        /* Window WatchDog */                                       
#define  PVD_IRQHandler                                 IRQ1_Handler        /* PVD through EXTI Line detection */
#define  TAMP_STAMP_IRQHandler                          IRQ2_Handler        /* Tamper and TimeStamps through the EXTI line */
#define  RTC_WKUP_IRQHandler                            IRQ3_Handler        /* RTC Wakeup through the EXTI line */
#define  FLASH_IRQHandler                               IRQ4_Handler        /* FLASH */
#define  RCC_IRQHandler                                 IRQ5_Handler        /* RCC */
#define  EXTI0_IRQHandler                               IRQ6_Handler        /* EXTI Line0 */
#define  EXTI1_IRQHandler                               IRQ7_Handler        /* EXTI Line1 */
#define  EXTI2_IRQHandler                               IRQ8_Handler        /* EXTI Line2 */
#define  EXTI3_IRQHandler                               IRQ9_Handler        /* EXTI Line3 */

#define  EXTI4_IRQHandler                               IRQ10_Handler       /* EXTI Line4 */
#define  DMA1_Stream0_IRQHandler                        IRQ11_Handler       /* DMA1 Stream 0 */
#define  DMA1_Stream1_IRQHandler                        IRQ12_Handler       /* DMA1 Stream 1 */
#define  DMA1_Stream2_IRQHandler                        IRQ13_Handler       /* DMA1 Stream 2 */
#define  DMA1_Stream3_IRQHandler                        IRQ14_Handler       /* DMA1 Stream 3 */
#define  DMA1_Stream4_IRQHandler                        IRQ15_Handler       /* DMA1 Stream 4 */
#define  DMA1_Stream5_IRQHandler                        IRQ16_Handler       /* DMA1 Stream 5 */
#define  DMA1_Stream6_IRQHandler                        IRQ17_Handler       /* DMA1 Stream 6 */
#define  ADC_IRQHandler                                 IRQ18_Handler       /* ADC1, ADC2 and ADC3s */
#define  CAN1_TX_IRQHandler                             IRQ19_Handler       /* CAN1 TX */         

#define  CAN1_RX0_IRQHandler                            IRQ20_Handler       /* CAN1 RX0 */
#define  CAN1_RX1_IRQHandler                            IRQ21_Handler       /* CAN1 RX1 */
#define  CAN1_SCE_IRQHandler                            IRQ22_Handler       /* CAN1 SCE */
#define  EXTI9_5_IRQHandler                             IRQ23_Handler       /* External Line[9:5]s */
#define  TIM1_BRK_TIM9_IRQHandler                       IRQ24_Handler       /* TIM1 Break and TIM9 */
#define  TIM1_UP_TIM10_IRQHandler                       IRQ25_Handler       /* TIM1 Update and TIM10 */
#define  TIM1_TRG_COM_TIM11_IRQHandler                  IRQ26_Handler       /* TIM1 Trigger and Commutation and TIM11 */
#define  TIM1_CC_IRQHandler                             IRQ27_Handler       /* TIM1 Capture Compare */
#define  TIM2_IRQHandler                                IRQ28_Handler       /* TIM2 */
#define  TIM3_IRQHandler                                IRQ29_Handler       /* TIM3 */

#define  TIM4_IRQHandler                                IRQ30_Handler       /* TIM4 */
#define  I2C1_EV_IRQHandler                             IRQ31_Handler       /* I2C1 Event */
#define  I2C1_ER_IRQHandler                             IRQ32_Handler       /* I2C1 Error */
#define  I2C2_EV_IRQHandler                             IRQ33_Handler       /* I2C2 Event */
#define  I2C2_ER_IRQHandler                             IRQ34_Handler       /* I2C2 Error */
#define  SPI1_IRQHandler                                IRQ35_Handler       /* SPI1 */
#define  SPI2_IRQHandler                                IRQ36_Handler       /* SPI2 */
#define  USART1_IRQHandler                              IRQ37_Handler       /* USART1 */
#define  USART2_IRQHandler                              IRQ38_Handler       /* USART2 */
#define  USART3_IRQHandler                              IRQ39_Handler       /* USART3 */

#define  EXTI15_10_IRQHandler                           IRQ40_Handler       /* External Line[15:10]s */
#define  RTC_Alarm_IRQHandler                           IRQ41_Handler       /* RTC Alarm (A and B) through EXTI Line */
#define  OTG_FS_WKUP_IRQHandler                         IRQ42_Handler       /* USB OTG FS Wakeup through EXTI line */
#define  TIM8_BRK_TIM12_IRQHandler                      IRQ43_Handler       /* TIM8 Break and TIM12 */
#define  TIM8_UP_TIM13_IRQHandler                       IRQ44_Handler       /* TIM8 Update and TIM13 */
#define  TIM8_TRG_COM_TIM14_IRQHandler                  IRQ45_Handler       /* TIM8 Trigger and Commutation and TIM14 */
#define  TIM8_CC_IRQHandler                             IRQ46_Handler       /* TIM8 Capture Compare */
#define  DMA1_Stream7_IRQHandler                        IRQ47_Handler       /* DMA1 Stream7 */
#define  FMC_IRQHandler                                 IRQ48_Handler       /* FMC */
#define  SDMMC1_IRQHandler                              IRQ49_Handler       /* SDMMC1 */

#define  TIM5_IRQHandler                                IRQ50_Handler       /* TIM5 */
#define  SPI3_IRQHandler                                IRQ51_Handler       /* SPI3 */
#define  UART4_IRQHandler                               IRQ52_Handler       /* UART4 */
#define  UART5_IRQHandler                               IRQ53_Handler       /* UART5 */
#define  TIM6_DAC_IRQHandler                            IRQ54_Handler       /* TIM6 and DAC1&2 underrun errors */
#define  TIM7_IRQHandler                                IRQ55_Handler       /* TIM7 */
#define  DMA2_Stream0_IRQHandler                        IRQ56_Handler       /* DMA2 Stream 0 */
#define  DMA2_Stream1_IRQHandler                        IRQ57_Handler       /* DMA2 Stream 1 */
#define  DMA2_Stream2_IRQHandler                        IRQ58_Handler       /* DMA2 Stream 2 */
#define  DMA2_Stream3_IRQHandler                        IRQ59_Handler       /* DMA2 Stream 3 */   

#define  DMA2_Stream4_IRQHandler                        IRQ60_Handler       /* DMA2 Stream 4 */
#define  ETH_IRQHandler                                 IRQ61_Handler       /* Ethernet */
#define  ETH_WKUP_IRQHandler                            IRQ62_Handler       /* Ethernet Wakeup through EXTI line */
#define  CAN2_TX_IRQHandler                             IRQ63_Handler       /* CAN2 TX */
#define  CAN2_RX0_IRQHandler                            IRQ64_Handler       /* CAN2 RX0 */
#define  CAN2_RX1_IRQHandler                            IRQ65_Handler       /* CAN2 RX1 */
#define  CAN2_SCE_IRQHandler                            IRQ66_Handler       /* CAN2 SCE */
#define  OTG_FS_IRQHandler                              IRQ67_Handler       /* USB OTG FS */
#define  DMA2_Stream5_IRQHandler                        IRQ68_Handler       /* DMA2 Stream 5 */
#define  DMA2_Stream6_IRQHandler                        IRQ69_Handler       /* DMA2 Stream 6 */

#define  DMA2_Stream7_IRQHandler                        IRQ70_Handler       /* DMA2 Stream 7 */
#define  USART6_IRQHandler                              IRQ71_Handler       /* USART6 */
#define  I2C3_EV_IRQHandler                             IRQ72_Handler       /* I2C3 event */
#define  I2C3_ER_IRQHandler                             IRQ73_Handler       /* I2C3 error */
#define  OTG_HS_EP1_OUT_IRQHandler                      IRQ74_Handler       /* USB OTG HS End Point 1 Out */
#define  OTG_HS_EP1_IN_IRQHandler                       IRQ75_Handler       /* USB OTG HS End Point 1 In */
#define  OTG_HS_WKUP_IRQHandler                         IRQ76_Handler       /* USB OTG HS Wakeup through EXTI */
#define  OTG_HS_IRQHandler                              IRQ77_Handler       /* USB OTG HS */
#define  DCMI_IRQHandler                                IRQ78_Handler       /* DCMI */
#define  Reserved0_IRQHandler                           IRQ79_Handler       /* Reserved */

#define  RNG_IRQHandler                                 IRQ80_Handler       /* Rng */
#define  FPU_IRQHandler                                 IRQ81_Handler       /* FPU */
#define  UART7_IRQHandler                               IRQ82_Handler       /* UART7 */
#define  UART8_IRQHandler                               IRQ83_Handler       /* UART8 */
#define  SPI4_IRQHandler                                IRQ84_Handler       /* SPI4 */
#define  SPI5_IRQHandler                                IRQ85_Handler       /* SPI5 */
#define  SPI6_IRQHandler                                IRQ86_Handler       /* SPI6 */
#define  SAI1_IRQHandler                                IRQ87_Handler       /* SAI1 */
#define  LTDC_IRQHandler                                IRQ88_Handler       /* LTDC */
#define  LTDC_ER_IRQHandler                             IRQ89_Handler       /* LTDC error */

#define  DMA2D_IRQHandler                               IRQ90_Handler       /* DMA2D */
#define  SAI2_IRQHandler                                IRQ91_Handler       /* SAI2 */
#define  QUADSPI_IRQHandler                             IRQ92_Handler       /* QUADSPI */
#define  LPTIM1_IRQHandler                              IRQ93_Handler       /* LPTIM1 */
#define  CEC_IRQHandler                                 IRQ94_Handler       /* HDMI_CEC */
#define  I2C4_EV_IRQHandler                             IRQ95_Handler       /* I2C4 Event */
#define  I2C4_ER_IRQHandler                             IRQ96_Handler       /* I2C4 Error */
#define  SPDIF_RX_IRQHandler                            IRQ97_Handler       /* SPDIF_RX */
#define  Reserved1_IRQHandler                           IRQ98_Handler       /* Reserved */
#define  DFSDM1_FLT0_IRQHandler                         IRQ99_Handler       /* DFSDM1 Filter 0 global Interrupt */

#define  DFSDM1_FLT1_IRQHandler                         IRQ100_Handler      /* DFSDM1 Filter 1 global Interrupt */
#define  DFSDM1_FLT2_IRQHandler                         IRQ101_Handler      /* DFSDM1 Filter 2 global Interrupt */
#define  DFSDM1_FLT3_IRQHandler                         IRQ102_Handler      /* DFSDM1 Filter 3 global Interrupt */
#define  SDMMC2_IRQHandler                              IRQ103_Handler      /* SDMMC2 */
#define  CAN3_TX_IRQHandler                             IRQ104_Handler      /* CAN3 TX */
#define  CAN3_RX0_IRQHandler                            IRQ105_Handler      /* CAN3 RX0 */
#define  CAN3_RX1_IRQHandler                            IRQ106_Handler      /* CAN3 RX1 */
#define  CAN3_SCE_IRQHandler                            IRQ107_Handler      /* CAN3 SCE */
#define  JPEG_IRQHandler                                IRQ108_Handler      /* JPEG */
#define  MDIOS_IRQHandler                               IRQ109_Handler      /* MDIOS */

/* Initialization registers **************************************************/
#define RME_A7M_RCC_APB1ENR                             RME_A7M_REG(0x40023840)
#define RME_A7M_RCC_APB1ENR_PWREN                       (1U<<28)

#define RME_A7M_PWR_CR1                                 RME_A7M_REG(0x40007000)
#define RME_A7M_PWR_CR1_VOS_SCALE1                      (3U<<14)
#define RME_A7M_PWR_CR1_ODEN                            (1U<<16)
#define RME_A7M_PWR_CR1_ODSWEN                          (1U<<17)
#define RME_A7M_PWR_CSR1                                RME_A7M_REG(0x40007004)
#define RME_A7M_PWR_CSR1_ODRDY                          (1U<<16)
#define RME_A7M_PWR_CSR1_ODSWRDY                        (1U<<17)

#define RME_A7M_RCC_CR                                  RME_A7M_REG(0x40023800)
#define RME_A7M_RCC_CR_HSEON                            (1U<<16)
#define RME_A7M_RCC_CR_HSERDY                           (1U<<17)
#define RME_A7M_RCC_CR_PLLON                            (1U<<24)
#define RME_A7M_RCC_CR_PLLRDY                           (1U<<25)

#define RME_A7M_RCC_PLLCFGR                             RME_A7M_REG(0x40023804)
#define RME_A7M_RCC_PLLCFGR_SOURCE_HSE                  (1U<<22)
#define RME_A7M_RCC_PLLCFGR_PLLM(X)                     (X)
#define RME_A7M_RCC_PLLCFGR_PLLN(X)                     ((X)<<6)
#define RME_A7M_RCC_PLLCFGR_PLLP(X)                     ((((X)>>1)-1)<<16)
#define RME_A7M_RCC_PLLCFGR_PLLQ(X)                     ((X)<<24)
#define RME_A7M_RCC_PLLCFGR_PLLR(X)                     ((X)<<28)

#define RME_A7M_RCC_CIR                                 RME_A7M_REG(0x4002380C)

#define RME_A7M_FLASH_ACR                               RME_A7M_REG(0x40023C00)
#define RME_A7M_FLASH_ACR_LATENCY(X)                    RME_A7M_FLASH_ACR=((RME_A7M_FLASH_ACR&~0x0FU)|(X))
#define RME_A7M_FLASH_ACR_ARTEN                         (1U<<9)
#define RME_A7M_FLASH_ACR_PRFTEN                        (1U<<8)

#define RME_A7M_RCC_CFGR                                RME_A7M_REG(0x40023808)
#define RME_A7M_RCC_CFGR_PCLK1(X)                       RME_A7M_RCC_CFGR=(RME_A7M_RCC_CFGR&~0x1C00U)|((X)<<10)
#define RME_A7M_RCC_CFGR_PCLK2(X)                       RME_A7M_RCC_CFGR=(RME_A7M_RCC_CFGR&~0xE000U)|((X)<<13)
#define RME_A7M_RCC_CFGR_HCLK(X)                        RME_A7M_RCC_CFGR=(RME_A7M_RCC_CFGR&~0x00F0U)|((X)<<4)
#define RME_A7M_RCC_CFGR_SYSCLK(X)                      RME_A7M_RCC_CFGR=(RME_A7M_RCC_CFGR&~0x0003U)|((X))

#define RME_A7M_RCC_CFGR_SYSCLKSOURCE_PLLCLK            (0x02U)
#define RME_A7M_RCC_CFGR_SYSCLK_DIV1                    (0x00U)
#define RME_A7M_RCC_CFGR_HCLK_DIV1                      (0x00U)
#define RME_A7M_RCC_CFGR_HCLK_DIV2                      (0x04U)
#define RME_A7M_RCC_CFGR_HCLK_DIV4                      (0x05U)
#define RME_A7M_RCC_CFGR_HCLK_DIV8                      (0x06U)
#define RME_A7M_RCC_CFGR_HCLK_DIV16                     (0x07U)

#define RME_A7M_TIM4_SR                                 RME_A7M_REG(0x40000810)
#define RME_A7M_TIM_FLAG_UPDATE                         (1U<<0)

#define RME_A7M_RCC_AHB1ENR                             RME_A7M_REG(0x40023830)
#define RME_A7M_RCC_AHB1ENR_GPIOAEN                     (1U<<0)

#define RME_A7M_RCC_APB2ENR                             RME_A7M_REG(0x40023844)
#define RME_A7M_RCC_APB2ENR_USART1EN                    (1U<<4)

#define RME_A7M_GPIOA_MODER                             RME_A7M_REG(0x40020000)
#define RME_A7M_GPIOA_OTYPER                            RME_A7M_REG(0x40020004)
#define RME_A7M_GPIOA_OSPEEDR                           RME_A7M_REG(0x40020008)
#define RME_A7M_GPIOA_PUPDR                             RME_A7M_REG(0x4002000C)
#define RME_A7M_GPIOA_IDR                               RME_A7M_REG(0x40020010)
#define RME_A7M_GPIOA_ODR                               RME_A7M_REG(0x40020014)
#define RME_A7M_GPIOA_BSRR                              RME_A7M_REG(0x40020018)
#define RME_A7M_GPIOA_LCKR                              RME_A7M_REG(0x4002001C)
#define RME_A7M_GPIOA_AFR0                              RME_A7M_REG(0x40020020)
#define RME_A7M_GPIOA_AFR1                              RME_A7M_REG(0x40020024)

#define RME_A7M_GPIO_MODE_INPUT                         (0U)
#define RME_A7M_GPIO_MODE_OUTPUT                        (1U)
#define RME_A7M_GPIO_MODE_ALTERNATE                     (2U)
#define RME_A7M_GPIO_MODE_ANALOG                        (3U)

#define RME_A7M_GPIO_OTYPE_PUSHPULL                     (0U)
#define RME_A7M_GPIO_OTYPE_OPENDRAIN                    (1U)

#define RME_A7M_GPIO_OSPEED_LOW                         (0U)
#define RME_A7M_GPIO_OSPEED_MEDIUM                      (1U)
#define RME_A7M_GPIO_OSPEED_HIGH                        (2U)
#define RME_A7M_GPIO_OSPEED_VERYHIGH                    (3U)

#define RME_A7M_GPIO_PUPD_NONE                          (0U)
#define RME_A7M_GPIO_PUPD_PULLUP                        (1U)
#define RME_A7M_GPIO_PUPD_PULLDOWN                      (2U)

#define RME_A7M_GPIO_AF7_USART1                         (0x07U)

#define RME_A7M_GPIOA_MODE(MODE,PIN)                    RME_A7M_GPIOA_MODER=(RME_A7M_GPIOA_MODER&~(0x03<<((PIN)*2)))|((MODE)<<((PIN)*2))
#define RME_A7M_GPIOA_OTYPE(OTYPE,PIN)                  RME_A7M_GPIOA_OTYPER=(RME_A7M_GPIOA_OTYPER&~(0x01<<(PIN)))|((OTYPE)<<(PIN))
#define RME_A7M_GPIOA_OSPEED(OSPEED,PIN)                RME_A7M_GPIOA_OSPEEDR=(RME_A7M_GPIOA_OSPEEDR&~(0x03<<((PIN)*2)))|((OSPEED)<<((PIN)*2))
#define RME_A7M_GPIOA_PUPD(PUPD,PIN)                    RME_A7M_GPIOA_PUPDR=(RME_A7M_GPIOA_PUPDR&~(0x03<<((PIN)*2)))|((PUPD)<<((PIN)*2))
#define RME_A7M_GPIOA_AF0(AF,PIN)                       RME_A7M_GPIOA_AFR0=(RME_A7M_GPIOA_AFR0&~(0x0F<<((PIN)*4)))|((AF)<<((PIN)*4))
#define RME_A7M_GPIOA_AF1(AF,PIN)                       RME_A7M_GPIOA_AFR1=(RME_A7M_GPIOA_AFR1&~(0x0F<<((PIN-8)*4)))|((AF)<<((PIN-8)*4))

#define RME_A7M_USART1_CR1                              RME_A7M_REG(0x40011000)
#define RME_A7M_USART1_CR2                              RME_A7M_REG(0x40011004)
#define RME_A7M_USART1_CR3                              RME_A7M_REG(0x40011008)
#define RME_A7M_USART1_BRR                              RME_A7M_REG(0x4001100C)
#define RME_A7M_USART1_ISR                              RME_A7M_REG(0x4001101C)
#define RME_A7M_USART1_TDR                              RME_A7M_REG(0x40011028)

#define RME_A7M_USART1_CR1_UE                           (1U<<0)

/* Preinitialization of critical hardware */
#define RME_A7M_LOW_LEVEL_PREINIT() \
do \
{ \
    /* Set HSION bit */ \
    RME_A7M_RCC_CR|=0x00000001; \
    /* Reset CFGR register */ \
    RME_A7M_RCC_CFGR=0x00000000; \
    /* Reset HSEON, CSSON and PLLON bits */ \
    RME_A7M_RCC_CR&=0xFEF6FFFFU; \
    /* Reset PLLCFGR register */ \
    RME_A7M_RCC_PLLCFGR=0x24003010U; \
    /* Reset HSEBYP bit */ \
    RME_A7M_RCC_CR&=0xFFFBFFFFU; \
    /* Disable all interrupts */ \
    RME_A7M_RCC_CIR=0x00000000; \
    /* Vector table address */ \
    RME_A7M_SCB_VTOR=0x08000000; \
} \
while(0)

/* Other low-level initialization stuff - clock and serial
 * STM32F7xx APB1<45MHz, APB2<90MHz. When running at 216MHz,
 * actually we are overdriving the bus a little, which might
 * be fine. */
#define RME_A7M_LOW_LEVEL_INIT() \
do \
{ \
    /* Set the clock tree in the system */ \
    /* Enble power regulator clock, and configure voltage scaling function */ \
    RME_A7M_RCC_APB1ENR|=RME_A7M_RCC_APB1ENR_PWREN; \
    __RME_A7M_Barrier(); \
    RME_A7M_PWR_CR1|=RME_A7M_PWR_CR1_VOS_SCALE1; \
    __RME_A7M_Barrier(); \
    /* Initialize the oscillator */ \
    RME_A7M_RCC_CR|=RME_A7M_RCC_CR_HSEON; \
    __RME_A7M_Barrier(); \
    while((RME_A7M_RCC_CR&RME_A7M_RCC_CR_HSERDY)==0); \
    /* Fpll=Fin/PLLM*PLLN, Fsys=Fpll/PLLP, Fperiph=Fpll/PLLQ */ \
    RME_A7M_RCC_CR&=~RME_A7M_RCC_CR_PLLON; \
    __RME_A7M_Barrier(); \
    while((RME_A7M_RCC_CR&RME_A7M_RCC_CR_PLLRDY)!=0); \
    RME_A7M_RCC_PLLCFGR=RME_A7M_RCC_PLLCFGR_SOURCE_HSE| \
                        RME_A7M_RCC_PLLCFGR_PLLM(RME_A7M_STM32F767IG_PLLM)| \
                        RME_A7M_RCC_PLLCFGR_PLLN(RME_A7M_STM32F767IG_PLLN)| \
                        RME_A7M_RCC_PLLCFGR_PLLP(RME_A7M_STM32F767IG_PLLP)| \
                        RME_A7M_RCC_PLLCFGR_PLLQ(RME_A7M_STM32F767IG_PLLQ)| \
                        RME_A7M_RCC_PLLCFGR_PLLR(RME_A7M_STM32F767IG_PLLR); \
    __RME_A7M_Barrier(); \
    RME_A7M_RCC_CR|=RME_A7M_RCC_CR_PLLON; \
    __RME_A7M_Barrier(); \
    while((RME_A7M_RCC_CR&RME_A7M_RCC_CR_PLLRDY)==0); \
    /* Overdrive to 216MHz */ \
    RME_A7M_PWR_CR1|=RME_A7M_PWR_CR1_ODEN; \
    __RME_A7M_Barrier(); \
    while((RME_A7M_PWR_CSR1&RME_A7M_PWR_CSR1_ODRDY)==0); \
    RME_A7M_PWR_CR1|=RME_A7M_PWR_CR1_ODSWEN; \
    __RME_A7M_Barrier(); \
    while((RME_A7M_PWR_CSR1&RME_A7M_PWR_CSR1_ODSWRDY)==0); \
    \
    /* HCLK,PCLK1 & PCLK2 configuration */ \
    RME_A7M_FLASH_ACR_LATENCY(7); \
    RME_ASSERT((RME_A7M_FLASH_ACR&0x0F)==7); \
    RME_A7M_RCC_CFGR_PCLK1(RME_A7M_RCC_CFGR_HCLK_DIV4); \
    RME_A7M_RCC_CFGR_PCLK2(RME_A7M_RCC_CFGR_HCLK_DIV2); \
    RME_A7M_RCC_CFGR_HCLK(RME_A7M_RCC_CFGR_SYSCLK_DIV1); \
    RME_A7M_RCC_CFGR_SYSCLK(RME_A7M_RCC_CFGR_SYSCLKSOURCE_PLLCLK); \
    __RME_A7M_Barrier(); \
    \
    /* Cache/Flash ART enabling */ \
    __RME_A7M_Cache_Init(); \
    RME_A7M_FLASH_ACR|=RME_A7M_FLASH_ACR_ARTEN; \
    RME_A7M_FLASH_ACR|=RME_A7M_FLASH_ACR_PRFTEN; \
    \
    /* Enable USART 1 for user-level operations */ \
    /* Clock enabling */ \
    RME_A7M_RCC_AHB1ENR|=RME_A7M_RCC_AHB1ENR_GPIOAEN; \
    RME_A7M_RCC_APB2ENR|=RME_A7M_RCC_APB2ENR_USART1EN; \
    /* UART IO initialization */ \
    RME_A7M_GPIOA_MODE(RME_A7M_GPIO_MODE_ALTERNATE,9); \
    RME_A7M_GPIOA_OTYPE(RME_A7M_GPIO_OTYPE_PUSHPULL,9); \
    RME_A7M_GPIOA_OSPEED(RME_A7M_GPIO_OSPEED_HIGH,9); \
    RME_A7M_GPIOA_PUPD(RME_A7M_GPIO_PUPD_PULLUP,9); \
    RME_A7M_GPIOA_AF1(RME_A7M_GPIO_AF7_USART1,9); \
    /* UART initialization */ \
    RME_A7M_USART1_CR1=0x0000008; \
    RME_A7M_USART1_CR2=0x00; \
    RME_A7M_USART1_CR3=0x00; \
    RME_A7M_USART1_BRR=0x3AA; \
    RME_A7M_USART1_CR1|=RME_A7M_USART1_CR1_UE; \
} \
while(0)

/* This is for debugging output */
#define RME_A7M_PUTCHAR(CHAR) \
do \
{ \
    RME_A7M_USART1_TDR=(CHAR); \
    while((RME_A7M_USART1_ISR&0x40)==0); \
} \
while(0)
    
/* Prefetcher state set and get */
#define RME_A7M_PRFTH_STATE_SET(STATE) \
do \
{ \
    if((STATE)!=0) \
    { \
        RME_A7M_FLASH_ACR|=RME_A7M_FLASH_ACR_ARTEN; \
        RME_A7M_FLASH_ACR|=RME_A7M_FLASH_ACR_PRFTEN; \
    } \
    else \
    { \
        RME_A7M_FLASH_ACR&=~RME_A7M_FLASH_ACR_PRFTEN; \
        RME_A7M_FLASH_ACR&=~RME_A7M_FLASH_ACR_ARTEN; \
    } \
} \
while(0)
    
#define RME_A7M_PRFTH_STATE_GET() ((RME_A7M_FLASH_ACR&RME_A7M_FLASH_ACR_ARTEN)!=0)
/* End Defines ***************************************************************/

/* Structs *******************************************************************/

/* End Structs ***************************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
