/******************************************************************************
Filename    : platform_x64_asm.s
Author      : pry
Date        : 19/01/2017
Description : The x64 assembly support of the RME RTOS.
              Parts of this is adapted from the xv6 port on x64.
******************************************************************************/

/* The X64 Structure **********************************************************
General-purpose registers:
64-Bit    32-Bit   16-Bit   8-Bit High   8-Bit Low           Name
  RAX      EAX       AX         AH           AL       Register A extended
  RBX      EBX       BX         BH           BL       Register B extended
  RCX      ECX       CX         CH           CL       Register C extended
  RDX      EDX       DX         DH           DL       Register D extended
  RBP      EBP       BP         --           BPL      Register Base Pointer
  RSP      ESP       SP         --           SPL      Register Stack Pointer
  RSI      ESI       SI         --           SIL      Register Source Index
  RDI      EDI       DI         --           DIL      Register Destination Index
  R8       R8D       R8W        --           R8L      Register 8
  R9       R9D       R9W        --           R9L      Register 9
  R10      R10D      R10W       --           R10L     Register 10
  R11      R11D      R11W       --           R11L     Register 11
  R12      R12D      R12W       --           R12L     Register 12
  R13      R13D      R13W       --           R13L     Register 13
  R14      R14D      R14W       --           R14L     Register 14
  R15      R15D      R15W       --           R15L     Register 15
  --       --        CS         --           --       Code Segment
  --       --        DS         --           --       Data Segment
  --       --        SS         --           --       Stack Segment
  --       --        ES         --           --       Extra Segment
  --       --        FS         --           --       Free Segment
  --       --        GS         --           --       Global Segment
  RFLAGS   EFLAGS    FLAGS      --           --       FLAGS Register

x87 FPU registers: all of them are 64-bit
  MMX0 - MMX7

AVX-512 registers:
  512-Bit            256-Bit                128-Bit
  ZMM0**             YMM0*                  XMM0
  ZMM1**             YMM1*                  XMM1
  ZMM2**             YMM2*                  XMM2
  ZMM3**             YMM3*                  XMM3
  ZMM4**             YMM4*                  XMM4
  ZMM5**             YMM5*                  XMM5
  ZMM6**             YMM6*                  XMM6
  ZMM7**             YMM7*                  XMM7
  ZMM8**             YMM8*                  XMM8
  ZMM9**             YMM9*                  XMM9
  ZMM10**            YMM10*                 XMM10
  ZMM11**            YMM11*                 XMM11
  ZMM12**            YMM12*                 XMM12
  ZMM13**            YMM13*                 XMM13
  ZMM14**            YMM14*                 XMM14
  ZMM15**            YMM15*                 XMM15
  ZMM16**            YMM16**                XMM16**
  ZMM17**            YMM17**                XMM17**
  ZMM18**            YMM18**                XMM18**
  ZMM19**            YMM19**                XMM19**
  ZMM20**            YMM20**                XMM20**
  ZMM21**            YMM21**                XMM21**
  ZMM22**            YMM22**                XMM22**
  ZMM23**            YMM23**                XMM23**
  ZMM24**            YMM24**                XMM24**
  ZMM25**            YMM25**                XMM25**
  ZMM26**            YMM26**                XMM26**
  ZMM27**            YMM27**                XMM27**
  ZMM28**            YMM28**                XMM28**
  ZMM29**            YMM29**                XMM29**
  ZMM30**            YMM30**                XMM30**
  ZMM31**            YMM31**                XMM31**
*Present only in AVX-2
**Present only in AVX-512
******************************************************************************/

/* Begin Exports *************************************************************/
    /* Disable all interrupts */
    .global             __RME_Disable_Int
    /* Enable all interrupts */
    .global             __RME_Enable_Int
    /* Atomic compare and exchange */
    .global             __RME_X64_Comp_Swap
    /* Atomic add */
    .global             __RME_X64_Fetch_Add
    /* Atomic and */
    .global             __RME_X64_Fetch_And
    /* Write release for x64 */
    .global             __RME_X64_Write_Release
    /* Get the MSB in a word */
    .global             __RME_X64_MSB_Get
    /* Get the CPU-local data structure */
    .global             __RME_X64_CPU_Local_Get
    /* Kernel main function wrapper */
    .global             _RME_Kmain
    /* Entering of the user mode */
    .global             __RME_Enter_User_Mode

    /* X64 specific stuff */
    /* Input from a port */
    .global             __RME_X64_In
    /* Output to a port */
    .global             __RME_X64_Out
    /* Read MSR */
    .global             __RME_X64_Read_MSR
    /* Write MSR */
    .global             __RME_X64_Write_MSR
    /* Load GDT */
    .global             __RME_X64_GDT_Load
    /* Load IDT */
    .global             __RME_X64_IDT_Load
    /* Load TSS */
    .global             __RME_X64_TSS_Load
    /* CPUID instruction */
    .global             __RME_X64_CPUID_Get
    /* HALT processor to wait for interrupt */
    .global             __RME_X64_Halt
    /* Load page table */
    .global             __RME_X64_Pgtbl_Set
    /* Acknowledge LAPIC interrupt */
    .global             __RME_X64_LAPIC_Ack

    /* Booting specific stuff */
    .global             _start
    .global             __RME_X64_Mboot_Header
    .global             __RME_X64_Mboot_Entry
    .global             RME_X64_Kpgt
    .global             __RME_X64_Kern_Boot_Stack
    .global             __RME_X64_SMP_Boot_32

    /* Fault handlers and user handlers are exported on their spot */
/* End Exports ***************************************************************/

/* Begin Imports *************************************************************/
    /* The kernel entry of RME. This will be defined in C language. */
    .global             RME_Kmain
    /* The fault handler of RME. This will be defined in C language. */
    .global             __RME_X64_Fault_Handler
    /* The generic interrupt handler of RME. This will be defined in C language. */
    .global             __RME_X64_Generic_Handler
    /* The system call handler of RME. This will be defined in C language. */
    .global             _RME_Svc_Handler
    /* The system tick handler of RME. This will be defined in C language. */
    .global             _RME_Tick_Handler
    /* The entry of SMP after they have finished their initialization */
    .global             __RME_SMP_Low_Level_Init
    /* All other processor's timer interrupt handler */
    .global             __RME_X64_SMP_Tick
/* End Imports ***************************************************************/

/* Begin Memory Init *********************************************************/
/* Multiboot header that will be located at 0x100000 *************************/
    .section            .text
    .code32
    .align              16
__RME_X64_Mboot_Header:
    .long               0x1BADB002                  /* Magic number */
    .long               0x00010000                  /* Boot flags */
    .long               (-0x1BADB002-0x00010000)    /* Checksum */
    .long               __RME_X64_Mboot_Load_Addr   /* header_addr */
    .long               __RME_X64_Mboot_Load_Addr   /* .text addr */
    .long               __RME_X64_Mboot_Load_End    /* .data end addr */
    .long               __RME_X64_Mboot_BSS_End     /* .bss end addr */
    .long               __RME_X64_Mboot_Entry_Addr  /* Entry of the kernel */

/* Initial page table ********************************************************/
    /* 7 pages that start from 0x101000. We can't use 1GB pages here
     * because we do not know whether the CPU supports that or not.
     * We will detect these later when we are making the kernel pages */
    .align              4096
    .space              7*4096

/* The entry point after the bootloader finishes *****************************/
    .code32
    .align              16
__RME_X64_Mboot_Entry:
     /* EBX contains Multiboot data structure, let's relocate it to some other places */
     MOV                %EBX,%ESI
     /* Zero 7 pages for our bootstrap page tables, PML4 @ 0x101000 */
     CLD
     XOR                %EAX,%EAX
     MOV                $0x101000,%EDI
     MOV                $0x7000,%ECX
     REP STOSB
     /* PML4[0] -> PDP-A @ 0x102000 */
     MOV                $(0x102000|3),%EAX
     MOV                %EAX,0x101000
     /* PML4[256] -> PDP-A @ 0x102000 */
     MOV                $(0x102000|3),%EAX
     MOV                %EAX,0x101800
     /* PML4[511] -> PDP-B @ 0x103000 */
     MOV                $(0x103000|3),%EAX
     MOV                %EAX,0x101FF8
     /* PDP-A[0] -> PDE-A @ 0x104000 */
     MOV                $(0x104000|3),%EAX
     MOV                %EAX,0x102000
     /* PDP-A[1] -> PDE-B @ 0x105000 */
     MOV                $(0x105000|3),%EAX
     MOV                %EAX,0x102008
     /* PDP-A[2] -> PDE-C @ 0x106000 */
     MOV                $(0x106000|3),%EAX
     MOV                %EAX,0x102010
     /* PDP-A[3] -> PDE-D @ 0x107000 unbufferable uncacheable */
     MOV                $(0x107000|3|0x18),%EAX
     MOV                %EAX,0x102018
     /* PDP-B[510] -> PDE-A @ 0x104000 */
     MOV                $(0x104000|3),%EAX
     MOV                %EAX,0x103FF0
     /* PDP-B[511] -> PDE-B @ 0x105000 */
     MOV                $(0x105000|3),%EAX
     MOV                %EAX,0x103FF8
     /* PDE-A/B/C/D [0..511/0...511/0..511/0..511] -> 0..4094MB */
     MOV                $(0x83),%EAX
     MOV                $0x104000,%EBX
     MOV                $(512*4),%ECX
PDE_Loop:
     MOV                %EAX,(%EBX)
     ADD                $0x200000,%EAX
     ADD                $0x8,%EBX
     DEC                %ECX
     JNZ                PDE_Loop
	 /* Clear ebx for initial processor boot.
      * When secondary processors boot, they'll call through
      * __RME_X64_SMP_Boot, but with a nonzero ebx.
      * We'll reuse these bootstrap pagetables and GDT. */
     XOR                %EBX,%EBX
     /* SMP boot will run directly from here */
__RME_X64_SMP_Boot_32:
     /* CR3 -> PML4 @ 0x101000 */
     MOV                $0x101000,%EAX
     MOV                %EAX,%CR3
     /* Load GDT */
     LGDT               (Boot_GDT_Desc-__RME_X64_Mboot_Header+__RME_X64_Mboot_Load_Addr)
     /* Enable PAE - CR4.PAE=1 */
     MOV                %CR4,%EAX
     BTS                $5,%EAX
     MOV                %EAX,%CR4
     /* Enable long mode and no execute bit - EFER.LME=1, EFER.NXE=1 */
     MOV                $0xC0000080,%ECX
     RDMSR
     BTS                $8,%EAX
     BTS                $11,%EAX
     WRMSR
     /* Enable paging */
     MOV                %CR0,%EAX
     BTS                $31,%EAX
     MOV                %EAX,%CR0
     /* Enable PCID - CR4.PCIDE=1 FIXME: this made things slower - due to extra logic for PCID processing *//*
     MOV                %CR4,%EAX
     BTS                $17,%EAX
     MOV                %EAX,%CR4 */
     /* shift to 64bit segment */
     LJMP               $8,$(Boot_Low_64-__RME_X64_Mboot_Header+__RME_X64_Mboot_Load_Addr)

/* Now we are in 64-bit mode *************************************************/
    .align              16
    .code64
Boot_Low_64:
    movq                $Boot_High_64,%RAX
    jmp                 *%RAX
_start:
Boot_High_64:
    /* ensure data segment registers are sane - zero out all of them */
    XOR                 %RAX,%RAX
    /* Below are all 16-bit moves */
    MOV                 %AX,%SS
    MOV                 %AX,%DS
    MOV                 %AX,%ES
    MOV                 %AX,%FS
    MOV                 %AX,%GS
    /* Check to see if we're booting a secondary core */
    TEST                %EBX,%EBX
    JNZ                 Boot_SMP_64
    /* Setup initial stack - this is hard-coded at low memory */
    MOV                 $__RME_X64_Kern_Boot_Stack,%RAX
    MOV                 %RAX,%RSP
    /* Pass the physical address of RSI to it */
    MOV                 %RSI,%RDI
    JMP                 main
    JMP                 .
Boot_SMP_64:
    MOV                 $0x7000,%RAX
    MOV                 -16(%RAX),%RSP
    JMP                 __RME_SMP_Low_Level_Init
    JMP                 .

    /* The initial gdt. Later we will have one GDT per CPU */
    .align              16
Boot_GDT:
    /* 0: Null descriptor */
    .long               0x00000000
    .long               0x00000000
    /* 1: Code, R/X, Nonconforming (transfers only possible with exceptions) */
    .long               0x00000000
    .long               0x00209800
    /* 2: Data, R/W, Expand Down */
    .long               0x00000000
    .long               0x00009000
Boot_GDT_Desc:
    .word               Boot_GDT_Desc-Boot_GDT-1;
    .quad               Boot_GDT-__RME_X64_Mboot_Header+__RME_X64_Mboot_Load_Addr;
    .align              16

    /* The kernel page table */
    .align              4096
RME_X64_Kpgt:
    .space              8*(256+256*512)

    /* The initial kernel stack */
    .space              8192
__RME_X64_Kern_Boot_Stack:
/* End Memory Init ***********************************************************/

/* Begin Function:__RME_X64_In ************************************************
Description : The function for outputting something to an I/O port.
Input       : ptr_t Port - The port to output to.
Output      : None.
Return      : ptr_t - The data received from that port.
******************************************************************************/
__RME_X64_In:
    PUSHQ               %RDX
    MOVQ                %RDI,%RDX
    MOVQ                %RAX,%RAX
    INB                 (%DX),%AL
    POPQ                %RDX
    RETQ
/* End Function:__RME_X64_In *************************************************/

/* Begin Function:__RME_X64_Out ***********************************************
Description    : The function for outputting something to an I/O port.
Input          : ptr_t Port - The port to output to.
                 ptr_t Data - The data to send to that port.
Output         : None.
Return         : None.
******************************************************************************/
__RME_X64_Out:
    PUSHQ               %RDX
    PUSHQ               %RAX
    MOVQ                %RDI,%RDX
    MOVQ                %RSI,%RAX
    OUTB                %AL,(%DX)
    POPQ                %RAX
    POPQ                %RDX
    RETQ
/* End Function:__RME_X64_Out ************************************************/

/* Begin Function:__RME_X64_Read_MSR ******************************************
Description : The function for reading a MSR.
Input       : ptr_t MSR - The MSR to read.
Output      : None.
Return      : ptr_t - The content of the MSR.
******************************************************************************/
__RME_X64_Read_MSR:
    PUSHQ               %RCX
    PUSHQ               %RDX
    MOVQ                %RDI,%RCX
    XORQ                %RAX,%RAX
    RDMSR
    SHLQ                $32,%RDX
    ADDQ                %RDX,%RAX
    POPQ                %RDX
    POPQ                %RCX
    RETQ
/* End Function:__RME_X64_Read_MSR *******************************************/

/* Begin Function:__RME_X64_Write_MSR *****************************************
Description : The function for writing a MSR.
Input       : ptr_t MSR - The MSR to write.
              ptr_t Value - The value to write to it.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_Write_MSR:
    PUSHQ               %RCX
    PUSHQ               %RDX
    PUSHQ               %RAX
    MOVQ                %RDI,%RCX
    MOVL                %ESI,%EAX
    MOVQ                %RSI,%RDX
    SHR                 $32,%RDX
    WRMSR
    POPQ                %RAX
    POPQ                %RDX
    POPQ                %RCX
    RETQ
/* End Function:__RME_X64_Write_MSR ******************************************/

/* Begin Function:__RME_X64_CPU_Local_Get *************************************
Description : Get the CPU-local data structures. This is to identify where we are
              executing.
Input       : None.
Output      : None.
Return      : struct RME_CPU_Local* - The CPU-local data structures.
******************************************************************************/
__RME_X64_CPU_Local_Get:
    MOVQ                %GS:(8192-8*3),%RAX
    RETQ
/* End Function:__RME_X64_CPU_Local_Get **************************************/

/* Begin Function:__RME_X64_CPUID_Get *****************************************
Description : The function for outputting something to an I/O port.
Input       : ptr_t EAX - The EAX value to get the CPUID for.
              ptr_t* EBX - The EBX info.
              ptr_t* ECX - The ECX info.
              ptr_t* EDX - The EDX info.
Output      : ptr_t* EBX - The EBX info.
              ptr_t* ECX - The ECX info.
              ptr_t* EDX - The EDX info.
Return      : ptr_t - The maximum number for CPUID instruction.
******************************************************************************/
__RME_X64_CPUID_Get:
    PUSHQ               %R8
    PUSHQ               %R9
    PUSHQ               %RBX
    PUSHQ               %RCX
    PUSHQ               %RDX

    MOVQ                %RDI,%RAX
    MOVQ                %RDX,%R8
    MOVQ                %RCX,%R9

    MOVQ                (%RSI),%RBX
    MOVQ                (%R8),%RCX
    MOVQ                (%R9),%RDX
    CPUID
    MOVQ                %RBX,(%RSI)
    MOVQ                %RCX,(%R8)
    MOVQ                %RDX,(%R9)

    POPQ                %RDX
    POPQ                %RCX
    POPQ                %RBX
    POPQ                %R9
    POPQ                %R8
    RETQ
/* End Function:__RME_X64_CPUID_Get ******************************************/

/* Begin Function:__RME_X64_GDT_Load ******************************************
Description : The function for loading the GDT. Every CPU needs to load their
              own GDT. No need to flush segment registers cause we're in 64-bit
              mode.
Input       : ptr_t* GDTR - The pointer to the GDT descriptor.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_GDT_Load:
    LGDT                (%RDI)
    RETQ
/* End Function:__RME_X64_GDT_Load *******************************************/

/* Begin Function:__RME_X64_IDT_Load ******************************************
Description : The function for loading the IDT. Every CPU needs to load their
              own IDT.
Input       : ptr_t* IDTR - The pointer to the IDT descriptor.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_IDT_Load:
    LIDT                (%RDI)
    RETQ
/* End Function:__RME_X64_IDT_Load *******************************************/

/* Begin Function:__RME_X64_TSS_Load ******************************************
Description : The function for loading the TSS's entry in GDT.
Input       : ptr_t TSS - The TSS's position in GDT.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_TSS_Load:
    LTR                 %DI
    RETQ
/* End Function:__RME_X64_TSS_Load *******************************************/

/* Begin Function:__RME_X64_Comp_Swap *****************************************
Description : The compare-and-swap atomic instruction. If the Old value is equal to
              *Ptr, then set the *Ptr as New and return 1; else return 0.
Input       : ptr_t* Ptr - The pointer to the data.
              ptr_t Old - The old value.
              ptr_t New - The new value.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - If successful, 1; else 0.
******************************************************************************/
__RME_X64_Comp_Swap:
    MOVQ                %RSI,%RAX
    XOR                 %RSI,%RSI
    /* In x86, locked instructions act as a fence */
    LOCK CMPXCHGQ       %RDX,(%RDI)
    /* Set the bit if ZF is set */
    SETZ                %SIL
    MOVQ                %RSI,%RAX
    RETQ
/* End Function:__RME_X64_Comp_Swap ******************************************/

/* Begin Function:__RME_X64_Fetch_Add *****************************************
Description : The fetch-and-add atomic instruction. Increase the value that is
              pointed to by the pointer, and return the value before addition.
Input       : ptr_t* Ptr - The pointer to the data.
              cnt_t Addend - The number to add.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - The value before the addition.
******************************************************************************/
__RME_X64_Fetch_Add:
    MOVQ                %RSI,%RAX
    /* In x86, locked instructions act as a fence */
    LOCK XADDQ          %RAX,(%RDI)
    RETQ
/* End Function:__RME_X64_Fetch_Add ******************************************/

/* Begin Function:__RME_X64_Fetch_And *****************************************
Description : The fetch-and-logic-and atomic instruction. Logic AND the pointer
              value with the operand, and return the value before logic AND.
Input       : ptr_t* Ptr - The pointer to the data.
              cnt_t Operand - The number to logic AND with the destination.
Output      : ptr_t* Ptr - The pointer to the data.
Return      : ptr_t - The value before the AND operation.
******************************************************************************/
__RME_X64_Fetch_And:
    MOVQ                (%RDI),%RAX
    /* In x86, locked instructions act as a fence */
    LOCK ANDQ           %RSI,(%RDI)
    RETQ
/* End Function:__RME_X64_Fetch_And ******************************************/

/* Begin Function:__RME_X64_Write_Release *************************************
Description : The write-release memory fence, to avoid read/write reorderings.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_Write_Release:
    MFENCE
    RETQ
/* End Function:__RME_X64_Write_Release **************************************/

/* Begin Function:__RME_X64_Pgtbl_Set *****************************************
Description : Set the processor's page table.
Input       : ptr_t Pgtbl - The physical address of the page table.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_Pgtbl_Set:
    MOV                 %RDI,%CR3
    RETQ
/* End Function:__RME_X64_Pgtbl_Set ******************************************/

/* Begin Function:__RME_Disable_Int *******************************************
Description : The function for disabling all interrupts.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_Disable_Int:
    /* Disable all interrupts */
    CLI
    RETQ
/* End Function:__RME_Disable_Int ********************************************/

/* Begin Function:__RME_Enable_Int ********************************************
Description : The function for enabling all interrupts.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_Enable_Int:
    /* Enable all interrupts */
    STI
    RETQ
/* End Function:__RME_Enable_Int *********************************************/

/* Begin Function:__RME_X64_Halt **********************************************
Description : Wait until a new interrupt comes, to save power.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
__RME_X64_Halt:
    /* Wait for interrupt */
    HLT
    RETQ
/* End Function:__RME_X64_Halt ***********************************************/

/* Begin Function:_RME_Kmain **************************************************
Description : The entry address of the kernel. Never returns.
Input       : ptr_t Stack - The stack address to set SP to.
Output      : None.
Return      : None.
******************************************************************************/
_RME_Kmain:
    MOVQ                %RDI,%RSP
    JMP                 RME_Kmain
/* End Function:_RME_Kmain ***************************************************/

/* Begin Function:__RME_X64_MSB_Get *******************************************
Description : Get the MSB of the word. The kernel is guaranteed not to call this
              function with a zero word, so we don't need to handle this edge case
              actually.
Input       : ptr_t Val - The value.
Output      : None.
Return      : ptr_t - The MSB position.
******************************************************************************/
__RME_X64_MSB_Get:
    BSRQ                %RDI,%RAX
    RETQ
/* End Function:__RME_X64_MSB_Get ********************************************/

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
    MOVQ                %RDI,%RCX           /* Entry */
    MOVQ                %RSI,%RSP           /* Stack */
    MOVQ                $0x3200,%R11        /* Flags - IOPL 3, IF */
    MOVQ                %RDX,%RDI           /* CPUID */
    SYSRETQ
/* End Function:__RME_Enter_User_Mode ****************************************/

/* Begin Function:Fault_Handler ***********************************************
Description : The multi-purpose fault handler routine. This macro will in fact
              call a C function to resolve the system service routines.
              x86-64 use a full descending stack model. Note that this assumes that
              the error code push is already complete, if any.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
    .set                KERNEL_CODE,(1*8)
    .set                KERNEL_DATA,(2*8)
    .set                USER_CODE,(5*8+3)
    .set                USER_DATA,(4*8+3)
/* Save all general-purpose registers */
.macro SAVE_GP_REGS
    PUSHQ               %R15
    PUSHQ               %R14
    PUSHQ               %R13
    PUSHQ               %R12
    PUSHQ               %R11
    PUSHQ               %R10
    PUSHQ               %R9
    PUSHQ               %R8
    PUSHQ               %RBP
    PUSHQ               %RDI
    PUSHQ               %RSI
    PUSHQ               %RDX
    PUSHQ               %RCX
    PUSHQ               %RBX
    PUSHQ               %RAX
    /* GS, CS, SS is known to be good, we just need FS, ES and DS */
    MOVW                $(KERNEL_DATA),%AX
    /* Horrible horseracing! faster than seL4
    MOVW                %AX,%FS
    MOVW                %AX,%ES */
    MOVW                %AX,%DS
.endm
/* Restore all general-purpose registers */
.macro RESTORE_GP_REGS
    /* Put the kernel GS back, and the user GS in */
    SWAPGS
    MOVW                $(USER_DATA),%AX
    /* Horrible horseracing! faster than seL4
    MOVW                %AX,%GS
    MOVW                %AX,%FS
    MOVW                %AX,%ES */
    MOVW                %AX,%DS
    POPQ                %RAX
    POPQ                %RBX
    POPQ                %RCX
    POPQ                %RDX
    POPQ                %RSI
    POPQ                %RDI
    POPQ                %RBP
    POPQ                %R8
    POPQ                %R9
    POPQ                %R10
    POPQ                %R11
    POPQ                %R12
    POPQ                %R13
    POPQ                %R14
    POPQ                %R15
.endm
/* Fault handler macro */
.macro FAULT_HANDLER NAME ERRCODE REASON
    .global             \NAME
\NAME:
    SWAPGS
.if \ERRCODE==0
    /* Make a dummy error code */
    PUSHQ               $0
.endif
    /* We went into this with a INT - flag = INTNUM */
    PUSHQ               $\REASON
    JMP                 Fault_Handler
.endm

/* Fault handlers */
FAULT_HANDLER NAME=__RME_X64_FAULT_DE_Handler ERRCODE=0 REASON=0
FAULT_HANDLER NAME=__RME_X64_TRAP_DB_Handler ERRCODE=0 REASON=1
FAULT_HANDLER NAME=__RME_X64_INT_NMI_Handler ERRCODE=0 REASON=2
FAULT_HANDLER NAME=__RME_X64_TRAP_BP_Handler ERRCODE=0 REASON=3
FAULT_HANDLER NAME=__RME_X64_TRAP_OF_Handler ERRCODE=0 REASON=4
FAULT_HANDLER NAME=__RME_X64_FAULT_BR_Handler ERRCODE=0 REASON=5
FAULT_HANDLER NAME=__RME_X64_FAULT_UD_Handler ERRCODE=0 REASON=6
FAULT_HANDLER NAME=__RME_X64_FAULT_NM_Handler ERRCODE=0 REASON=7
FAULT_HANDLER NAME=__RME_X64_ABORT_DF_Handler ERRCODE=1 REASON=8
FAULT_HANDLER NAME=__RME_X64_ABORT_OLD_MF_Handler ERRCODE=0 REASON=9
FAULT_HANDLER NAME=__RME_X64_FAULT_TS_Handler ERRCODE=1 REASON=10
FAULT_HANDLER NAME=__RME_X64_FAULT_NP_Handler ERRCODE=1 REASON=11
FAULT_HANDLER NAME=__RME_X64_FAULT_SS_Handler ERRCODE=1 REASON=12
FAULT_HANDLER NAME=__RME_X64_FAULT_GP_Handler ERRCODE=1 REASON=13
FAULT_HANDLER NAME=__RME_X64_FAULT_PF_Handler ERRCODE=1 REASON=14
FAULT_HANDLER NAME=__RME_X64_FAULT_MF_Handler ERRCODE=0 REASON=16
FAULT_HANDLER NAME=__RME_X64_FAULT_AC_Handler ERRCODE=1 REASON=17
FAULT_HANDLER NAME=__RME_X64_ABORT_MC_Handler ERRCODE=0 REASON=18
FAULT_HANDLER NAME=__RME_X64_FAULT_XM_Handler ERRCODE=0 REASON=19
FAULT_HANDLER NAME=__RME_X64_FAULT_VE_Handler ERRCODE=0 REASON=20
/* Common function body for fault handlers - reduce I-Cache usage */
Fault_Handler:
    SAVE_GP_REGS
    /* Pass the stack pointer to system call handler */
    MOVQ                %RSP,%RDI
    MOVQ                (15*8)(%RSP),%RSI
    CALLQ               __RME_X64_Fault_Handler

    RESTORE_GP_REGS
    ADDQ                $16,%RSP
    IRETQ
/* End Function:Fault_Handler ************************************************/

/* Begin Function:__RME_X64_INT_USER_Handler **********************************
Description : The General Interrupt handler routine. This will in fact call a
              C function to resolve the system service routines.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
/* User generic interrupt handler macro */
.macro USER_HANDLER NAME VECTNUM
    .global             \NAME
\NAME:
    SWAPGS
    PUSHQ               $0                  /* Make a dummy error code */
    PUSHQ               $\VECTNUM           /* Interrupt number */
    JMP                 User_Handler
.endm
/* User handlers */
USER_HANDLER NAME=__RME_X64_USER32_Handler VECTNUM=32
USER_HANDLER NAME=__RME_X64_USER33_Handler VECTNUM=33
USER_HANDLER NAME=__RME_X64_USER34_Handler VECTNUM=34
USER_HANDLER NAME=__RME_X64_USER35_Handler VECTNUM=35
USER_HANDLER NAME=__RME_X64_USER36_Handler VECTNUM=36
USER_HANDLER NAME=__RME_X64_USER37_Handler VECTNUM=37
USER_HANDLER NAME=__RME_X64_USER38_Handler VECTNUM=38
USER_HANDLER NAME=__RME_X64_USER39_Handler VECTNUM=39

USER_HANDLER NAME=__RME_X64_USER40_Handler VECTNUM=40
USER_HANDLER NAME=__RME_X64_USER41_Handler VECTNUM=41
USER_HANDLER NAME=__RME_X64_USER42_Handler VECTNUM=42
USER_HANDLER NAME=__RME_X64_USER43_Handler VECTNUM=43
USER_HANDLER NAME=__RME_X64_USER44_Handler VECTNUM=44
USER_HANDLER NAME=__RME_X64_USER45_Handler VECTNUM=45
USER_HANDLER NAME=__RME_X64_USER46_Handler VECTNUM=46
USER_HANDLER NAME=__RME_X64_USER47_Handler VECTNUM=47
USER_HANDLER NAME=__RME_X64_USER48_Handler VECTNUM=48
USER_HANDLER NAME=__RME_X64_USER49_Handler VECTNUM=49

USER_HANDLER NAME=__RME_X64_USER50_Handler VECTNUM=50
USER_HANDLER NAME=__RME_X64_USER51_Handler VECTNUM=51
USER_HANDLER NAME=__RME_X64_USER52_Handler VECTNUM=52
USER_HANDLER NAME=__RME_X64_USER53_Handler VECTNUM=53
USER_HANDLER NAME=__RME_X64_USER54_Handler VECTNUM=54
USER_HANDLER NAME=__RME_X64_USER55_Handler VECTNUM=55
USER_HANDLER NAME=__RME_X64_USER56_Handler VECTNUM=56
USER_HANDLER NAME=__RME_X64_USER57_Handler VECTNUM=57
USER_HANDLER NAME=__RME_X64_USER58_Handler VECTNUM=58
USER_HANDLER NAME=__RME_X64_USER59_Handler VECTNUM=59

USER_HANDLER NAME=__RME_X64_USER60_Handler VECTNUM=60
USER_HANDLER NAME=__RME_X64_USER61_Handler VECTNUM=61
USER_HANDLER NAME=__RME_X64_USER62_Handler VECTNUM=62
USER_HANDLER NAME=__RME_X64_USER63_Handler VECTNUM=63
USER_HANDLER NAME=__RME_X64_USER64_Handler VECTNUM=64
USER_HANDLER NAME=__RME_X64_USER65_Handler VECTNUM=65
USER_HANDLER NAME=__RME_X64_USER66_Handler VECTNUM=66
USER_HANDLER NAME=__RME_X64_USER67_Handler VECTNUM=67
USER_HANDLER NAME=__RME_X64_USER68_Handler VECTNUM=68
USER_HANDLER NAME=__RME_X64_USER69_Handler VECTNUM=69

USER_HANDLER NAME=__RME_X64_USER70_Handler VECTNUM=70
USER_HANDLER NAME=__RME_X64_USER71_Handler VECTNUM=71
USER_HANDLER NAME=__RME_X64_USER72_Handler VECTNUM=72
USER_HANDLER NAME=__RME_X64_USER73_Handler VECTNUM=73
USER_HANDLER NAME=__RME_X64_USER74_Handler VECTNUM=74
USER_HANDLER NAME=__RME_X64_USER75_Handler VECTNUM=75
USER_HANDLER NAME=__RME_X64_USER76_Handler VECTNUM=76
USER_HANDLER NAME=__RME_X64_USER77_Handler VECTNUM=77
USER_HANDLER NAME=__RME_X64_USER78_Handler VECTNUM=78
USER_HANDLER NAME=__RME_X64_USER79_Handler VECTNUM=79

USER_HANDLER NAME=__RME_X64_USER80_Handler VECTNUM=80
USER_HANDLER NAME=__RME_X64_USER81_Handler VECTNUM=81
USER_HANDLER NAME=__RME_X64_USER82_Handler VECTNUM=82
USER_HANDLER NAME=__RME_X64_USER83_Handler VECTNUM=83
USER_HANDLER NAME=__RME_X64_USER84_Handler VECTNUM=84
USER_HANDLER NAME=__RME_X64_USER85_Handler VECTNUM=85
USER_HANDLER NAME=__RME_X64_USER86_Handler VECTNUM=86
USER_HANDLER NAME=__RME_X64_USER87_Handler VECTNUM=87
USER_HANDLER NAME=__RME_X64_USER88_Handler VECTNUM=88
USER_HANDLER NAME=__RME_X64_USER89_Handler VECTNUM=89

USER_HANDLER NAME=__RME_X64_USER90_Handler VECTNUM=90
USER_HANDLER NAME=__RME_X64_USER91_Handler VECTNUM=91
USER_HANDLER NAME=__RME_X64_USER92_Handler VECTNUM=92
USER_HANDLER NAME=__RME_X64_USER93_Handler VECTNUM=93
USER_HANDLER NAME=__RME_X64_USER94_Handler VECTNUM=94
USER_HANDLER NAME=__RME_X64_USER95_Handler VECTNUM=95
USER_HANDLER NAME=__RME_X64_USER96_Handler VECTNUM=96
USER_HANDLER NAME=__RME_X64_USER97_Handler VECTNUM=97
USER_HANDLER NAME=__RME_X64_USER98_Handler VECTNUM=98
USER_HANDLER NAME=__RME_X64_USER99_Handler VECTNUM=99

USER_HANDLER NAME=__RME_X64_USER100_Handler VECTNUM=100
USER_HANDLER NAME=__RME_X64_USER101_Handler VECTNUM=101
USER_HANDLER NAME=__RME_X64_USER102_Handler VECTNUM=102
USER_HANDLER NAME=__RME_X64_USER103_Handler VECTNUM=103
USER_HANDLER NAME=__RME_X64_USER104_Handler VECTNUM=104
USER_HANDLER NAME=__RME_X64_USER105_Handler VECTNUM=105
USER_HANDLER NAME=__RME_X64_USER106_Handler VECTNUM=106
USER_HANDLER NAME=__RME_X64_USER107_Handler VECTNUM=107
USER_HANDLER NAME=__RME_X64_USER108_Handler VECTNUM=108
USER_HANDLER NAME=__RME_X64_USER109_Handler VECTNUM=109

USER_HANDLER NAME=__RME_X64_USER110_Handler VECTNUM=110
USER_HANDLER NAME=__RME_X64_USER111_Handler VECTNUM=111
USER_HANDLER NAME=__RME_X64_USER112_Handler VECTNUM=112
USER_HANDLER NAME=__RME_X64_USER113_Handler VECTNUM=113
USER_HANDLER NAME=__RME_X64_USER114_Handler VECTNUM=114
USER_HANDLER NAME=__RME_X64_USER115_Handler VECTNUM=115
USER_HANDLER NAME=__RME_X64_USER116_Handler VECTNUM=116
USER_HANDLER NAME=__RME_X64_USER117_Handler VECTNUM=117
USER_HANDLER NAME=__RME_X64_USER118_Handler VECTNUM=118
USER_HANDLER NAME=__RME_X64_USER119_Handler VECTNUM=119

USER_HANDLER NAME=__RME_X64_USER120_Handler VECTNUM=120
USER_HANDLER NAME=__RME_X64_USER121_Handler VECTNUM=121
USER_HANDLER NAME=__RME_X64_USER122_Handler VECTNUM=122
USER_HANDLER NAME=__RME_X64_USER123_Handler VECTNUM=123
USER_HANDLER NAME=__RME_X64_USER124_Handler VECTNUM=124
USER_HANDLER NAME=__RME_X64_USER125_Handler VECTNUM=125
USER_HANDLER NAME=__RME_X64_USER126_Handler VECTNUM=126
USER_HANDLER NAME=__RME_X64_USER127_Handler VECTNUM=127
USER_HANDLER NAME=__RME_X64_USER128_Handler VECTNUM=128
USER_HANDLER NAME=__RME_X64_USER129_Handler VECTNUM=129

USER_HANDLER NAME=__RME_X64_USER130_Handler VECTNUM=130
USER_HANDLER NAME=__RME_X64_USER131_Handler VECTNUM=131
USER_HANDLER NAME=__RME_X64_USER132_Handler VECTNUM=132
USER_HANDLER NAME=__RME_X64_USER133_Handler VECTNUM=133
USER_HANDLER NAME=__RME_X64_USER134_Handler VECTNUM=134
USER_HANDLER NAME=__RME_X64_USER135_Handler VECTNUM=135
USER_HANDLER NAME=__RME_X64_USER136_Handler VECTNUM=136
USER_HANDLER NAME=__RME_X64_USER137_Handler VECTNUM=137
USER_HANDLER NAME=__RME_X64_USER138_Handler VECTNUM=138
USER_HANDLER NAME=__RME_X64_USER139_Handler VECTNUM=139

USER_HANDLER NAME=__RME_X64_USER140_Handler VECTNUM=140
USER_HANDLER NAME=__RME_X64_USER141_Handler VECTNUM=141
USER_HANDLER NAME=__RME_X64_USER142_Handler VECTNUM=142
USER_HANDLER NAME=__RME_X64_USER143_Handler VECTNUM=143
USER_HANDLER NAME=__RME_X64_USER144_Handler VECTNUM=144
USER_HANDLER NAME=__RME_X64_USER145_Handler VECTNUM=145
USER_HANDLER NAME=__RME_X64_USER146_Handler VECTNUM=146
USER_HANDLER NAME=__RME_X64_USER147_Handler VECTNUM=147
USER_HANDLER NAME=__RME_X64_USER148_Handler VECTNUM=148
USER_HANDLER NAME=__RME_X64_USER149_Handler VECTNUM=149

USER_HANDLER NAME=__RME_X64_USER150_Handler VECTNUM=150
USER_HANDLER NAME=__RME_X64_USER151_Handler VECTNUM=151
USER_HANDLER NAME=__RME_X64_USER152_Handler VECTNUM=152
USER_HANDLER NAME=__RME_X64_USER153_Handler VECTNUM=153
USER_HANDLER NAME=__RME_X64_USER154_Handler VECTNUM=154
USER_HANDLER NAME=__RME_X64_USER155_Handler VECTNUM=155
USER_HANDLER NAME=__RME_X64_USER156_Handler VECTNUM=156
USER_HANDLER NAME=__RME_X64_USER157_Handler VECTNUM=157
USER_HANDLER NAME=__RME_X64_USER158_Handler VECTNUM=158
USER_HANDLER NAME=__RME_X64_USER159_Handler VECTNUM=159

USER_HANDLER NAME=__RME_X64_USER160_Handler VECTNUM=160
USER_HANDLER NAME=__RME_X64_USER161_Handler VECTNUM=161
USER_HANDLER NAME=__RME_X64_USER162_Handler VECTNUM=162
USER_HANDLER NAME=__RME_X64_USER163_Handler VECTNUM=163
USER_HANDLER NAME=__RME_X64_USER164_Handler VECTNUM=164
USER_HANDLER NAME=__RME_X64_USER165_Handler VECTNUM=165
USER_HANDLER NAME=__RME_X64_USER166_Handler VECTNUM=166
USER_HANDLER NAME=__RME_X64_USER167_Handler VECTNUM=167
USER_HANDLER NAME=__RME_X64_USER168_Handler VECTNUM=168
USER_HANDLER NAME=__RME_X64_USER169_Handler VECTNUM=169

USER_HANDLER NAME=__RME_X64_USER170_Handler VECTNUM=170
USER_HANDLER NAME=__RME_X64_USER171_Handler VECTNUM=171
USER_HANDLER NAME=__RME_X64_USER172_Handler VECTNUM=172
USER_HANDLER NAME=__RME_X64_USER173_Handler VECTNUM=173
USER_HANDLER NAME=__RME_X64_USER174_Handler VECTNUM=174
USER_HANDLER NAME=__RME_X64_USER175_Handler VECTNUM=175
USER_HANDLER NAME=__RME_X64_USER176_Handler VECTNUM=176
USER_HANDLER NAME=__RME_X64_USER177_Handler VECTNUM=177
USER_HANDLER NAME=__RME_X64_USER178_Handler VECTNUM=178
USER_HANDLER NAME=__RME_X64_USER179_Handler VECTNUM=179

USER_HANDLER NAME=__RME_X64_USER180_Handler VECTNUM=180
USER_HANDLER NAME=__RME_X64_USER181_Handler VECTNUM=181
USER_HANDLER NAME=__RME_X64_USER182_Handler VECTNUM=182
USER_HANDLER NAME=__RME_X64_USER183_Handler VECTNUM=183
USER_HANDLER NAME=__RME_X64_USER184_Handler VECTNUM=184
USER_HANDLER NAME=__RME_X64_USER185_Handler VECTNUM=185
USER_HANDLER NAME=__RME_X64_USER186_Handler VECTNUM=186
USER_HANDLER NAME=__RME_X64_USER187_Handler VECTNUM=187
USER_HANDLER NAME=__RME_X64_USER188_Handler VECTNUM=188
USER_HANDLER NAME=__RME_X64_USER189_Handler VECTNUM=189

USER_HANDLER NAME=__RME_X64_USER190_Handler VECTNUM=190
USER_HANDLER NAME=__RME_X64_USER191_Handler VECTNUM=191
USER_HANDLER NAME=__RME_X64_USER192_Handler VECTNUM=192
USER_HANDLER NAME=__RME_X64_USER193_Handler VECTNUM=193
USER_HANDLER NAME=__RME_X64_USER194_Handler VECTNUM=194
USER_HANDLER NAME=__RME_X64_USER195_Handler VECTNUM=195
USER_HANDLER NAME=__RME_X64_USER196_Handler VECTNUM=196
USER_HANDLER NAME=__RME_X64_USER197_Handler VECTNUM=197
USER_HANDLER NAME=__RME_X64_USER198_Handler VECTNUM=198
USER_HANDLER NAME=__RME_X64_USER199_Handler VECTNUM=199

USER_HANDLER NAME=__RME_X64_USER200_Handler VECTNUM=200
USER_HANDLER NAME=__RME_X64_USER201_Handler VECTNUM=201
USER_HANDLER NAME=__RME_X64_USER202_Handler VECTNUM=202
USER_HANDLER NAME=__RME_X64_USER203_Handler VECTNUM=203
USER_HANDLER NAME=__RME_X64_USER204_Handler VECTNUM=204
USER_HANDLER NAME=__RME_X64_USER205_Handler VECTNUM=205
USER_HANDLER NAME=__RME_X64_USER206_Handler VECTNUM=206
USER_HANDLER NAME=__RME_X64_USER207_Handler VECTNUM=207
USER_HANDLER NAME=__RME_X64_USER208_Handler VECTNUM=208
USER_HANDLER NAME=__RME_X64_USER209_Handler VECTNUM=209

USER_HANDLER NAME=__RME_X64_USER210_Handler VECTNUM=210
USER_HANDLER NAME=__RME_X64_USER211_Handler VECTNUM=211
USER_HANDLER NAME=__RME_X64_USER212_Handler VECTNUM=212
USER_HANDLER NAME=__RME_X64_USER213_Handler VECTNUM=213
USER_HANDLER NAME=__RME_X64_USER214_Handler VECTNUM=214
USER_HANDLER NAME=__RME_X64_USER215_Handler VECTNUM=215
USER_HANDLER NAME=__RME_X64_USER216_Handler VECTNUM=216
USER_HANDLER NAME=__RME_X64_USER217_Handler VECTNUM=217
USER_HANDLER NAME=__RME_X64_USER218_Handler VECTNUM=218
USER_HANDLER NAME=__RME_X64_USER219_Handler VECTNUM=219

USER_HANDLER NAME=__RME_X64_USER220_Handler VECTNUM=220
USER_HANDLER NAME=__RME_X64_USER221_Handler VECTNUM=221
USER_HANDLER NAME=__RME_X64_USER222_Handler VECTNUM=222
USER_HANDLER NAME=__RME_X64_USER223_Handler VECTNUM=223
USER_HANDLER NAME=__RME_X64_USER224_Handler VECTNUM=224
USER_HANDLER NAME=__RME_X64_USER225_Handler VECTNUM=225
USER_HANDLER NAME=__RME_X64_USER226_Handler VECTNUM=226
USER_HANDLER NAME=__RME_X64_USER227_Handler VECTNUM=227
USER_HANDLER NAME=__RME_X64_USER228_Handler VECTNUM=228
USER_HANDLER NAME=__RME_X64_USER229_Handler VECTNUM=229

USER_HANDLER NAME=__RME_X64_USER230_Handler VECTNUM=230
USER_HANDLER NAME=__RME_X64_USER231_Handler VECTNUM=231
USER_HANDLER NAME=__RME_X64_USER232_Handler VECTNUM=232
USER_HANDLER NAME=__RME_X64_USER233_Handler VECTNUM=233
USER_HANDLER NAME=__RME_X64_USER234_Handler VECTNUM=234
USER_HANDLER NAME=__RME_X64_USER235_Handler VECTNUM=235
USER_HANDLER NAME=__RME_X64_USER236_Handler VECTNUM=236
USER_HANDLER NAME=__RME_X64_USER237_Handler VECTNUM=237
USER_HANDLER NAME=__RME_X64_USER238_Handler VECTNUM=238
USER_HANDLER NAME=__RME_X64_USER239_Handler VECTNUM=239

USER_HANDLER NAME=__RME_X64_USER240_Handler VECTNUM=240
USER_HANDLER NAME=__RME_X64_USER241_Handler VECTNUM=241
USER_HANDLER NAME=__RME_X64_USER242_Handler VECTNUM=242
USER_HANDLER NAME=__RME_X64_USER243_Handler VECTNUM=243
USER_HANDLER NAME=__RME_X64_USER244_Handler VECTNUM=244
USER_HANDLER NAME=__RME_X64_USER245_Handler VECTNUM=245
USER_HANDLER NAME=__RME_X64_USER246_Handler VECTNUM=246
USER_HANDLER NAME=__RME_X64_USER247_Handler VECTNUM=247
USER_HANDLER NAME=__RME_X64_USER248_Handler VECTNUM=248
USER_HANDLER NAME=__RME_X64_USER249_Handler VECTNUM=249

USER_HANDLER NAME=__RME_X64_USER250_Handler VECTNUM=250
USER_HANDLER NAME=__RME_X64_USER251_Handler VECTNUM=251
USER_HANDLER NAME=__RME_X64_USER252_Handler VECTNUM=252
USER_HANDLER NAME=__RME_X64_USER253_Handler VECTNUM=253
USER_HANDLER NAME=__RME_X64_USER254_Handler VECTNUM=254
USER_HANDLER NAME=__RME_X64_USER255_Handler VECTNUM=255
/* Common function body for generic interrupt handlers - reduce I-Cache usage */
User_Handler:
    SAVE_GP_REGS
    /* Pass the stack pointer to system call handler */
    MOVQ                %RSP,%RDI
    MOVQ                (15*8)(%RSP),%RSI
    CALLQ               __RME_X64_Generic_Handler
    CALLQ               __RME_X64_LAPIC_Ack
    RESTORE_GP_REGS
    ADDQ                $16,%RSP
    IRETQ
/* End Function:__RME_X64_INT_USER_Handler ***********************************/

/* Begin Function:SysTick_SMP_Handler *****************************************
Description : The ticker timer handler for all other processors. This is in fact
              a handler triggered by IPI from the main processor.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
SysTick_SMP_Handler:
    .global             SysTick_SMP_Handler
    SWAPGS
    /* Make a dummy error code */
    PUSHQ               $0
    /* The interrupt number of SysTick is always 0xFFFF */
    PUSHQ               $0xFFFF
    SAVE_GP_REGS
    /* Pass the stack pointer to system call handler */
    MOVQ                %RSP,%RDI
    CALLQ               _RME_Tick_SMP_Handler
    CALLQ               __RME_X64_LAPIC_Ack
    RESTORE_GP_REGS
    ADDQ                $16,%RSP
    IRETQ
/* End Function:SysTick_SMP_Handler ******************************************/

/* Begin Function:SysTick_Handler *********************************************
Description : The System Tick Timer handler routine. This will in fact call a
              C function to resolve the system service routines.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
SysTick_Handler:
    .global             SysTick_Handler
    SWAPGS
    /* Make a dummy error code */
    PUSHQ               $0
    /* The interrupt number of SysTick is always 0xFFFF */
    PUSHQ               $0xFFFF
    SAVE_GP_REGS
    /* Pass the stack pointer to system call handler */
    MOVQ                %RSP,%RDI
    CALLQ               _RME_Tick_Handler
    CALLQ               __RME_X64_SMP_Tick
    CALLQ               __RME_X64_LAPIC_Ack
    RESTORE_GP_REGS
    ADDQ                $16,%RSP
    IRETQ
/* End Function:SysTick_Handler **********************************************/

/* Begin Function:SVC_Handler *************************************************
Description : The SVC handler routine. This will in fact call a C function to resolve
              the system service routines. None of the segment registers will be
              preserved in all paths, and you shouldn't use them at all.
              Due to the complexity of x86-64 architecture, there are 4 cases to handle:
              1. SYSCALL in - SYSRET out (easy case)
              2. INT in - IRET out (easy case)
              3. SYSCALL in - IRET out
                 In this case, we need to forge a portion of stack data to make SYSCALL's
                 on-stack context identical to that of INT. This requires some trivial work.
              4. INT in - SYSRET out
                 This case is not allowed because SYSRET always corrupts your RCX/R11,
                 because they are used for RIP and RFLAGS restoration. Thus, we need to
                 avoid this case and make sure that anything that INTs in can only be
                 IRETed out. Thus, we need to save an identifier on the stack to notify the
                 restoration stub.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
SVC_Handler:
    .global             SVC_Handler
    SWAPGS
    /* Swap the pointers with the per-core kernel RSP */
    MOVQ                %RSP,%GS:(8192-8)
    MOVQ                %GS:(8192-16),%RSP
    /* Simulate INT stack: High - [SS, RSP, RFLAGS, CS, RIP, ERRCODE] - Low */
    PUSHQ               $(USER_DATA)
    PUSHQ               %GS:(8192-8)
    PUSHQ               %R11
    PUSHQ               $(USER_CODE)
    PUSHQ               %RCX
    PUSHQ               $0
    /* We went into this from a SYSCALL - interrupt number 0x10000 */
    PUSHQ               $0x10000
    SAVE_GP_REGS
    /* Pass the stack pointer to system call handler */
    MOVQ                %RSP,%RDI
    CALLQ               _RME_Svc_Handler
    RESTORE_GP_REGS
    /* See if we are forced to use IRET */
    TESTQ               $0x10000,(%RSP)
    JZ                  Use_IRET
    /* Just move these into the registers desired */
    /* CVE-2012-0217, CVE-2014-4699: Force canonical address on RIP */
    MOVQ                $0x7FFFFFFFFFFF,%RCX
    ANDQ                16(%RSP),%RCX
    MOVQ                32(%RSP),%R11
    MOVQ                40(%RSP),%RSP
    /* Return to user-level */
    SYSRETQ
Use_IRET:
    /* Flag is zero, We are forced to use IRET, because this is from INT */
    ADDQ                $16,%RSP
    IRETQ
/* End Function:SVC_Handler **************************************************/

;/* End Of File **************************************************************/

;/* Copyright (C) Evo-Devo Instrum. All rights reserved **********************/
