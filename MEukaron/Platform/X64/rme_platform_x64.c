/******************************************************************************
Filename    : platform_x64.c
Author      : pry
Date        : 01/04/2017
Licence     : The Unlicense; see LICENSE for details.
Description : The hardware abstraction layer for ACPI compliant x86-64 machines.
              TODO list:
              1. Fix all the system call/int gates, make sure that they cannot cause DOS.
              2. Implement NMI to a different stack, thus the NMI will not currupt the
                 kernel stack. The NMI stack does not have to be large. Use NMIw/LAPIC
                 timer as watchdog on each core, and if something bad happens we just reboot.
                 we also need to display whether this is due to hardware fault or something
                 else.
              3. Fix error handling - now they just display info.
              4. Test multi-core stuff.
              5. User-level: Consider running VMs. We support FULL virtualization
                 instead of paravirtualization. Use virtualization extensions to do that.
              6. Display something on screen at system boot-up. A console will be great.
              7. Consider how do we pass COMPLEX system configuration data to the INIT
                 process. We have multi-sockets and this is going to be very complex if
                 not well handled.
              8. Look into FPU support. Also, FPU support on x86-64 should be reexamined
                 if possible. The current FPU support is purely theoretical.
              9. Consider using the AML interpreter. Or we will not be able to detect
                 more complex peripherals.
              10. Run this on a real machine to watch output.
              11. Consider complex PCI device support and driver compatibility.
              12. Make the system NUMA-aware.
******************************************************************************/

/* Includes ******************************************************************/
#define __HDR_DEFS__
#include "Kernel/rme_kernel.h"
#include "Platform/X64/rme_platform_x64.h"
#undef __HDR_DEFS__

#define __HDR_STRUCTS__
#include "Platform/X64/rme_platform_x64.h"
#include "Kernel/rme_kernel.h"
#undef __HDR_STRUCTS__

/* Private include */
#include "Platform/X64/rme_platform_x64.h"

#define __HDR_PUBLIC_MEMBERS__
#include "Kernel/rme_kernel.h"
#undef __HDR_PUBLIC_MEMBERS__
/* End Includes **************************************************************/

/* Begin Function:main ********************************************************
Description : The entrance of the operating system.
Input       : rme_ptr_t MBInfo - The multiboot information structure's physical address.
Output      : None.
Return      : int - This function never returns.
******************************************************************************/
int main(rme_ptr_t MBInfo)
{
    RME_X64_MBInfo=(struct multiboot_info*)(MBInfo+RME_X64_VA_BASE);
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
    if(RME_X64_UART_Exist==0)
        return 0;

    /* Wait until we have transmitted */
    while((__RME_X64_In(RME_X64_COM1+5)&0x20)==0);

    __RME_X64_Out(RME_X64_COM1, Char);

    return 0;
}
/* End Function:__RME_Putchar ************************************************/

/* Begin Function:__RME_X64_UART_Init *****************************************
Description : Initialize the UART of X64 platform.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_UART_Init(void)
{
    /* Disable interrupts */
    __RME_X64_Out(RME_X64_COM1+1, 0);
    /* Unlock divisor */
    __RME_X64_Out(RME_X64_COM1+3, 0x80);
    /* Set baudrate - on some computer, the hardware only support this reliably */
    __RME_X64_Out(RME_X64_COM1+0, 115200/9600);
    __RME_X64_Out(RME_X64_COM1+1, 0);
    /* Lock divisor, 8 data bits, 1 stop bit, parity off */
    __RME_X64_Out(RME_X64_COM1+3, 0x03);
    /* Turn on the FIFO */
    __RME_X64_Out(RME_X64_COM1+2, 0xC7);
    /* Turn off all model control, fully asynchronous */
    __RME_X64_Out(RME_X64_COM1+4, 0);

    /* If status is 0xFF, no serial port */
    if(__RME_X64_In(RME_X64_COM1+5)==0xFF)
        RME_X64_UART_Exist=0;
    else
        RME_X64_UART_Exist=1;
}
/* End Function:__RME_X64_UART_Init ******************************************/

/* Begin Function:__RME_X64_RDSP_Scan *****************************************
Description : Scan for a valid RDSP structure in the given physical memory segment.
Input       : rme_ptr_t Base - The base address of the physical memory segment.
              rme_ptr_t Base - The length of the memory segment.
Output      : None.
Return      : struct RME_X64_ACPI_RDSP_Desc* - The descriptor physical address.
******************************************************************************/
struct RME_X64_ACPI_RDSP_Desc* __RME_X64_RDSP_Scan(rme_ptr_t Base, rme_ptr_t Len)
{
    rme_u8_t* Pos;
    rme_cnt_t Count;
    rme_ptr_t Checksum;
    rme_cnt_t Check_Cnt;

    Pos=(rme_u8_t*)RME_X64_PA2VA(Base);

    /* Search a word at a time */
    for(Count=0;Count<=Len-sizeof(struct RME_X64_ACPI_RDSP_Desc);Count+=4)
    {
        /* It seemed that we have found one. See if the checksum is good */
        if(_RME_Memcmp(&(Pos[Count]),"RSD PTR ",8)==0)
        {
            Checksum=0;
            /* 20 is the length of the first part of the table */
            for(Check_Cnt=0;Check_Cnt<20;Check_Cnt++)
                Checksum+=Pos[Count+Check_Cnt];
            /* Is the checksum good? */
            if((Checksum&0xFF)==0)
                return (struct RME_X64_ACPI_RDSP_Desc*)&(Pos[Count]);
        }
    }
    return 0;
}
/* End Function:__RME_X64_RDSP_Scan ******************************************/

/* Begin Function:__RME_X64_RDSP_Find *****************************************
Description : Find a valid RDSP structure and return it.
Input       : None.
Output      : None.
Return      : struct RME_X64_ACPI_RDSP_Desc* - The descriptor address.
******************************************************************************/
struct RME_X64_ACPI_RDSP_Desc*__RME_X64_RDSP_Find(void)
{
    struct RME_X64_ACPI_RDSP_Desc* RDSP;
    rme_ptr_t Paddr;
    /* 0x40E contains the address of Extended BIOS Data Area (EBDA). Let's try
     * to find the RDSP there first */
    Paddr=*((rme_u16_t*)RME_X64_PA2VA(0x40E))<<4;

    if(Paddr!=0)
    {
        RDSP=__RME_X64_RDSP_Scan(Paddr,1024);
        /* Found */
        if(RDSP!=0)
            return RDSP;
    }

    /* If that fails, the RDSP must be here */
    return __RME_X64_RDSP_Scan(0xE0000, 0x20000);
}
/* End Function:__RME_X64_RDSP_Find ******************************************/

/* Begin Function:__RME_X64_SMP_Detect ****************************************
Description : Detect the SMP configuration in the system and set up the per-CPU info.
Input       : struct RME_X64_ACPI_MADT_Hdr* MADT - The pointer to the MADT header.
Output      : None.
Return      : rme_ret_t - If successful, 0; else -1.
******************************************************************************/
rme_ret_t __RME_X64_SMP_Detect(struct RME_X64_ACPI_MADT_Hdr* MADT)
{
    struct RME_X64_ACPI_MADT_LAPIC_Record* LAPIC;
    struct RME_X64_ACPI_MADT_IOAPIC_Record* IOAPIC;
    struct RME_X64_ACPI_MADT_SRC_OVERRIDE_Record* OVERRIDE;
    rme_ptr_t Length;
    rme_u8_t* Ptr;
    rme_u8_t* End;

    /* Is there a MADT? */
    if(MADT==0)
        return -1;
    /* Is the MADT valid? */
    if(MADT->Header.Length<sizeof(struct RME_X64_ACPI_MADT_Hdr))
        return -1;

    RME_X64_LAPIC_Addr=MADT->LAPIC_Addr_Phys;

    /* Where does the actual table contents start? */
    Ptr=MADT->Table;
    /* Where does it end? */
    End=Ptr+MADT->Header.Length-sizeof(struct RME_X64_ACPI_MADT_Hdr);

    RME_X64_Num_IOAPIC=0;
    RME_X64_Num_CPU=0;
    while(Ptr<End)
    {
        /* See if we have finished scanning the table */
        if((End-Ptr)<2)
            break;
        Length=Ptr[1];
        if((End-Ptr)<Length)
            break;

        /* See what is in the table */
        switch(Ptr[0])
        {
            /* This is a LAPIC */
            case RME_X64_MADT_LAPIC:
            {
                LAPIC=(struct RME_X64_ACPI_MADT_LAPIC_Record*)Ptr;
                /* Is the length correct? */
                if(Length<sizeof(struct RME_X64_ACPI_MADT_LAPIC_Record))
                    break;
                /* Is this LAPIC enabled? */
                if((LAPIC->Flags&RME_X64_APIC_LAPIC_ENABLED)==0)
                    break;

                RME_PRINTK_S("\n\rACPI: CPU ");
                RME_Print_Int(RME_X64_Num_CPU);
                RME_PRINTK_S(", LAPIC ID ");
                RME_Print_Int(LAPIC->APIC_ID);

                /* Log this CPU into our per-CPU data structure */
                RME_X64_CPU_Info[RME_X64_Num_CPU].LAPIC_ID=LAPIC->APIC_ID;
                RME_X64_CPU_Info[RME_X64_Num_CPU].Boot_Done=0;
                RME_X64_Num_CPU++;
                RME_ASSERT(RME_X64_Num_CPU<=RME_X64_CPU_NUM);
                break;
            }
            /* This is an IOAPIC */
            case RME_X64_MADT_IOAPIC:
            {
                IOAPIC=(struct RME_X64_ACPI_MADT_IOAPIC_Record*)Ptr;
                /* Is the length correct? */
                if(Length<sizeof(struct RME_X64_ACPI_MADT_IOAPIC_Record))
                    break;

                RME_PRINTK_S("\n\rACPI: IOAPIC ");
                RME_Print_Int(RME_X64_Num_IOAPIC);
                RME_PRINTK_S(" @ ");
                RME_Print_Uint(IOAPIC->Addr);
                RME_PRINTK_S(", ID ");
                RME_Print_Int(IOAPIC->ID);
                RME_PRINTK_S(", IBASE ");
                RME_Print_Int(IOAPIC->Interrupt_Base);

                /* Support multiple APICS */
                if(RME_X64_Num_IOAPIC!=0)
                {
                    RME_PRINTK_S("Warning: multiple ioapics are not supported - currently we will not initialize IOAPIC > 1\n");
                }
                else
                {
                    RME_X64_IOAPIC_Info[RME_X64_Num_IOAPIC].IOAPIC_ID=IOAPIC->ID;
                }

                RME_X64_Num_IOAPIC++;
                RME_ASSERT(RME_X64_Num_IOAPIC<=RME_X64_IOAPIC_NUM);
                break;
            }
            /* This is interrupt override information */
            case RME_X64_MADT_INT_SRC_OVERRIDE:
            {
                OVERRIDE=(struct RME_X64_ACPI_MADT_SRC_OVERRIDE_Record*)Ptr;
                if(Length<sizeof(struct RME_X64_ACPI_MADT_SRC_OVERRIDE_Record))
                    break;
                RME_PRINTK_S("\n\rACPI: OVERRIDE Bus ");
                RME_Print_Int(OVERRIDE->Bus);
                RME_PRINTK_S(", Source ");
                RME_Print_Uint(OVERRIDE->Source);
                RME_PRINTK_S(", GSI ");
                RME_Print_Int(OVERRIDE->GS_Interrupt);
                RME_PRINTK_S(", Flags ");
                RME_Print_Int(OVERRIDE->MPS_Int_Flags);

                break;
            }
            /* All other types are ignored */
            default:break;
        }
        
        Ptr+=Length;
    }

    return 0;
}
/* End Function:__RME_X64_SMP_Detect *****************************************/

/* Begin Function:__RME_X64_ACPI_Debug ****************************************
Description : Print the information about the ACPI table entry.
Input       : struct RME_X64_ACPI_MADT_Hdr* MADT - The pointer to the MADT header.
Output      : None.
Return      : rme_ret_t - If successful, 0; else -1.
******************************************************************************/
void __RME_X64_ACPI_Debug(struct RME_X64_ACPI_Desc_Hdr *Header)
{
    rme_u8_t Signature[5];
    rme_u8_t ID[7];
    rme_u8_t Table_ID[9];
    rme_u8_t Creator[5];
    rme_ptr_t OEM_Rev;
    rme_ptr_t Creator_Rev;

    /* Copy everything into our buffer */
    _RME_Memcpy(Signature, Header->Signature, 4);
    Signature[4]='\0';
    _RME_Memcpy(ID, Header->OEM_ID, 6);
    ID[6]='\0';
    _RME_Memcpy(Table_ID, Header->OEM_Table_ID, 8); 
    Table_ID[8]='\0';
    _RME_Memcpy(Creator, Header->Creator_ID, 4);
    Creator[4]='\0';

    OEM_Rev=Header->OEM_Revision;
    Creator_Rev=Header->Creator_Revision;

    /* And print these entries */
    RME_PRINTK_S("\n\rACPI:");
    RME_PRINTK_S(Signature);
    RME_PRINTK_S(", ");
    RME_PRINTK_S(ID);
    RME_PRINTK_S(", ");
    RME_PRINTK_S(Table_ID);
    RME_PRINTK_S(", ");
    RME_PRINTK_S(OEM_Rev);
    RME_PRINTK_S(", ");
    RME_PRINTK_S(Creator);
    RME_PRINTK_S(", ");
    RME_PRINTK_S(Creator_Rev);
    RME_PRINTK_S(".");
}
/* End Function:__RME_X64_ACPI_Debug *****************************************/

/* Begin Function:__RME_X64_ACPI_Init *****************************************
Description : Detect the SMP configuration in the system and set up the per-CPU info.
Input       : struct RME_X64_ACPI_MADT_Hdr* MADT - The pointer to the MADT header.
Output      : None.
Return      : rme_ret_t - If successful, 0; else -1.
******************************************************************************/
rme_ret_t __RME_X64_ACPI_Init(void)
{
    rme_cnt_t Count;
    rme_cnt_t Table_Num;
    struct RME_X64_ACPI_RDSP_Desc* RDSP;
    struct RME_X64_ACPI_RSDT_Hdr* RSDT;
    struct RME_X64_ACPI_MADT_Hdr* MADT;
    struct RME_X64_ACPI_Desc_Hdr* Header;

    /* Try to find RDSP */
    RDSP=__RME_X64_RDSP_Find();
    RME_PRINTK_S("\r\nRDSP address: ");
    RME_PRINTK_U((rme_ptr_t)RDSP);
    /* Find the RSDT */
    RSDT=(struct RME_X64_ACPI_RSDT_Hdr*)RME_X64_PA2VA(RDSP->RSDT_Addr_Phys);
    RME_PRINTK_S("\r\nRSDT address: ");
    RME_PRINTK_U((rme_ptr_t)RSDT);
    Table_Num=(RSDT->Header.Length-sizeof(struct RME_X64_ACPI_RSDT_Hdr))>>2;

    for(Count=0;Count<Table_Num;Count++)
    {
        /* See what did we find */
        Header=(struct RME_X64_ACPI_Desc_Hdr*)RME_X64_PA2VA(RSDT->Entry[Count]);
        __RME_X64_ACPI_Debug(Header);
        /* See if this is the MADT */
        if(_RME_Memcmp(Header->Signature, "APIC", 4)==0)
            MADT=(struct RME_X64_ACPI_MADT_Hdr*)Header;
    }

    return __RME_X64_SMP_Detect(MADT);
}
/* End Function:__RME_X64_ACPI_Init ******************************************/

/* Begin Function:__RME_X64_Feature_Get ***************************************
Description : Use the CPUID instruction extensively to get all the processor
              information. We assume that all processors installed have the same
              features.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_Feature_Get(void)
{
    rme_cnt_t Count;

    /* What's the maximum feature? */
    RME_X64_Feature.Max_Func=__RME_X64_CPUID_Get(RME_X64_CPUID_0_VENDOR_ID,
                                                 (rme_ptr_t*)&(RME_X64_Feature.Func[0][1]),
                                                 (rme_ptr_t*)&(RME_X64_Feature.Func[0][2]),
                                                 (rme_ptr_t*)&(RME_X64_Feature.Func[0][3]));
    RME_X64_Feature.Func[0][0]=RME_X64_Feature.Max_Func;

    /* Get all the feature bits */
    for(Count=1;Count<=RME_X64_Feature.Max_Func;Count++)
    {
        RME_X64_Feature.Func[Count][0]=__RME_X64_CPUID_Get(Count,
                                                           (rme_ptr_t*)&(RME_X64_Feature.Func[Count][1]),
                                                           (rme_ptr_t*)&(RME_X64_Feature.Func[Count][2]),
                                                           (rme_ptr_t*)&(RME_X64_Feature.Func[Count][3]));
    }

    /* What's the maximum extended feature? */
    RME_X64_Feature.Max_Ext=__RME_X64_CPUID_Get(RME_X64_CPUID_E0_EXT_MAX,
                                                (rme_ptr_t*)&(RME_X64_Feature.Ext[0][1]),
                                                (rme_ptr_t*)&(RME_X64_Feature.Ext[0][2]),
                                                (rme_ptr_t*)&(RME_X64_Feature.Ext[0][3]));
    RME_X64_Feature.Ext[0][0]=RME_X64_Feature.Max_Ext;


    /* Get all the feature bits */
    for(Count=1;Count<=RME_X64_Feature.Max_Ext-RME_X64_CPUID_E0_EXT_MAX;Count++)
    {
        RME_X64_Feature.Ext[Count][0]=__RME_X64_CPUID_Get(RME_X64_CPUID_E0_EXT_MAX|Count,
                                                          (rme_ptr_t*)&(RME_X64_Feature.Ext[Count][1]),
                                                          (rme_ptr_t*)&(RME_X64_Feature.Ext[Count][2]),
                                                          (rme_ptr_t*)&(RME_X64_Feature.Ext[Count][3]));
    }

    /* TODO: Check these flags. If not satisfied, we hang immediately. */
}
/* End Function:__RME_X64_Feature_Get ****************************************/

/* Begin Function:__RME_X64_Mem_Init ******************************************
Description : Initialize the memory map, and get the size of kernel object
              allocation registration table(Kotbl) and page table reference
              count registration table(Pgreg).
Input       : rme_ptr_t MMap_Addr - The GRUB multiboot memory map data address.
              rme_ptr_t MMap_Length - The GRUB multiboot memory map data length.
Output      : None.
Return      : None.
******************************************************************************/
/* We place this here because these are never exported, and are local to this
 * file. This is a little workaround for the header inclusion problem */
struct __RME_X64_Mem
{
    struct RME_List Head;
    rme_ptr_t Start_Addr;
    rme_ptr_t Length;
};
/* The header of the physical memory linked list */
struct RME_List RME_X64_Phys_Mem;
/* The BIOS wouldn't really report more than 1024 blocks of memory */
struct __RME_X64_Mem RME_X64_Mem[1024];

void __RME_X64_Mem_Init(rme_ptr_t MMap_Addr, rme_ptr_t MMap_Length)
{
    struct multiboot_mmap_entry* MMap;
    volatile struct RME_List* Trav_Ptr;
    rme_ptr_t MMap_Cnt;
    rme_ptr_t Info_Cnt;

    MMap_Cnt=0;
    Info_Cnt=0;

    __RME_List_Crt(&RME_X64_Phys_Mem);

    while(MMap_Cnt<MMap_Length)
    {
        MMap=(struct multiboot_mmap_entry*)(MMap_Addr+MMap_Cnt);
        MMap_Cnt+=MMap->size+4;

        if(MMap->type!=1)
            continue;

        Trav_Ptr=RME_X64_Phys_Mem.Next;
        while(Trav_Ptr!=&RME_X64_Phys_Mem)
        {
            if(((struct __RME_X64_Mem*)(Trav_Ptr))->Start_Addr>MMap->addr)
                break;
            Trav_Ptr=Trav_Ptr->Next;
        }
        RME_X64_Mem[Info_Cnt].Start_Addr=MMap->addr;
        RME_X64_Mem[Info_Cnt].Length=MMap->len;
        __RME_List_Ins(&(RME_X64_Mem[Info_Cnt].Head),Trav_Ptr->Prev,Trav_Ptr);

        /* Just print them then */
        RME_PRINTK_S("\n\rPhysical memory: 0x");
        RME_Print_Uint(MMap->addr);
        RME_PRINTK_S(", 0x");
        RME_Print_Uint(MMap->len);
        RME_PRINTK_S(", ");
        RME_Print_Uint(MMap->type);

        Info_Cnt++;
    }

    /* Check if any memory segment overlaps. If yes, merge them into one,
     * until there is no overlapping segments */
    Trav_Ptr=RME_X64_Phys_Mem.Next;
    while((Trav_Ptr!=&RME_X64_Phys_Mem)&&((Trav_Ptr->Next)!=&RME_X64_Phys_Mem))
    {
        if((((struct __RME_X64_Mem*)(Trav_Ptr))->Start_Addr+
            ((struct __RME_X64_Mem*)(Trav_Ptr))->Length)>
            ((struct __RME_X64_Mem*)(Trav_Ptr->Next))->Start_Addr)
        {
            /* Merge these two blocks */
            ((struct __RME_X64_Mem*)(Trav_Ptr))->Length=
            ((struct __RME_X64_Mem*)(Trav_Ptr->Next))->Start_Addr+
            ((struct __RME_X64_Mem*)(Trav_Ptr->Next))->Length-
            ((struct __RME_X64_Mem*)(Trav_Ptr))->Start_Addr;
            __RME_List_Del(Trav_Ptr,Trav_Ptr->Next->Next);
            continue;
        }
        Trav_Ptr=Trav_Ptr->Next;
    }

    /* Calculate total memory */
    MMap_Cnt=0;
    Trav_Ptr=RME_X64_Phys_Mem.Next;
    while(Trav_Ptr!=&RME_X64_Phys_Mem)
    {
        MMap_Cnt+=((struct __RME_X64_Mem*)(Trav_Ptr))->Length;
        Trav_Ptr=Trav_Ptr->Next;
    }
    RME_PRINTK_S("\n\rTotal physical memory: 0x");
    RME_Print_Uint(MMap_Cnt);

    /* At least 256MB memory required on x64 architecture */
    RME_ASSERT(MMap_Cnt>=RME_POW2(RME_PGTBL_SIZE_256M));

    /* Kernel virtual memory layout */
    RME_X64_Layout.Kotbl_Start=(rme_ptr_t)RME_KOTBL;
    /* +1G in cases where we have > 3GB memory for covering the memory hole */
    Info_Cnt=(MMap_Cnt>3*RME_POW2(RME_PGTBL_SIZE_1G))?(MMap_Cnt+RME_POW2(RME_PGTBL_SIZE_1G)):MMap_Cnt;
    RME_X64_Layout.Kotbl_Size=((Info_Cnt>>RME_KMEM_SLOT_ORDER)>>RME_WORD_ORDER)+1;

    /* Calculate the size of page table registration table size - we always assume 4GB range */
    Info_Cnt=(MMap_Cnt>RME_POW2(RME_PGTBL_SIZE_4G))?RME_POW2(RME_PGTBL_SIZE_4G):MMap_Cnt;
    RME_X64_Layout.Pgreg_Start=RME_X64_Layout.Kotbl_Start+RME_X64_Layout.Kotbl_Size;
    RME_X64_Layout.Pgreg_Size=((Info_Cnt>>RME_PGTBL_SIZE_4K)+1)*sizeof(struct __RME_X64_Pgreg);

    /* Calculate the per-CPU data structure size - each CPU have two 4k pages */
    RME_X64_Layout.PerCPU_Start=RME_ROUND_UP(RME_X64_Layout.Pgreg_Start+RME_X64_Layout.Pgreg_Size,RME_PGTBL_SIZE_4K);
    RME_X64_Layout.PerCPU_Size=2*RME_POW2(RME_PGTBL_SIZE_4K)*RME_X64_Num_CPU;

    /* Now decide the size of the stack */
    RME_X64_Layout.Stack_Size=RME_X64_Num_CPU<<RME_X64_KSTACK_ORDER;
}
/* End Function:__RME_X64_Mem_Init *******************************************/

/* Begin Function:__RME_X64_CPU_Local_Init ************************************
Description : Initialize CPU-local data structures. The layout of each CPU-local
              data structure is:
              |       4kB      |    1kB    |  3kB-3*8Bytes  |   3*8Bytes   |
              |    IDT[255:0]  |  GDT/TSS  |  RME_CPU_Local | RME_X64_Temp |
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_CPU_Local_Init(void)
{
    volatile rme_u16_t Desc[5];
    struct RME_X64_IDT_Entry* IDT_Table;
    struct RME_X64_Temp* Temp;
    struct RME_CPU_Local* CPU_Local;
    rme_ptr_t* GDT_Table;
    rme_ptr_t TSS_Table;
    rme_cnt_t Count;

    IDT_Table=(struct RME_X64_IDT_Entry*)RME_X64_CPU_LOCAL_BASE(RME_X64_CPU_Cnt);
    /* Clean up the whole IDT */
    for(Count=0;Count<256;Count++)
        IDT_Table[Count].Type_Attr=0;

    /* Install the vectors - only the INT3 is trap (for debugging), all other ones are interrupt */
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_DE, RME_X64_IDT_VECT, __RME_X64_FAULT_DE_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_TRAP_DB, RME_X64_IDT_VECT, __RME_X64_TRAP_DB_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_INT_NMI, RME_X64_IDT_VECT, __RME_X64_INT_NMI_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_TRAP_BP, RME_X64_IDT_TRAP, __RME_X64_TRAP_BP_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_TRAP_OF, RME_X64_IDT_VECT, __RME_X64_TRAP_OF_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_BR, RME_X64_IDT_VECT, __RME_X64_FAULT_BR_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_UD, RME_X64_IDT_VECT, __RME_X64_FAULT_UD_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_NM, RME_X64_IDT_VECT, __RME_X64_FAULT_NM_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_ABORT_DF, RME_X64_IDT_VECT, __RME_X64_ABORT_DF_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_ABORT_OLD_MF, RME_X64_IDT_VECT, __RME_X64_ABORT_OLD_MF_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_TS, RME_X64_IDT_VECT, __RME_X64_FAULT_TS_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_NP, RME_X64_IDT_VECT, __RME_X64_FAULT_NP_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_SS, RME_X64_IDT_VECT, __RME_X64_FAULT_SS_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_GP, RME_X64_IDT_VECT, __RME_X64_FAULT_GP_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_PF, RME_X64_IDT_VECT, __RME_X64_FAULT_PF_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_MF, RME_X64_IDT_VECT, __RME_X64_FAULT_MF_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_AC, RME_X64_IDT_VECT, __RME_X64_FAULT_AC_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_ABORT_MC, RME_X64_IDT_VECT, __RME_X64_ABORT_MC_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_XM, RME_X64_IDT_VECT, __RME_X64_FAULT_XM_Handler);
    RME_X64_SET_IDT(IDT_Table, RME_X64_FAULT_VE, RME_X64_IDT_VECT, __RME_X64_FAULT_VE_Handler);

    /* Install user handlers */
    RME_X64_USER_IDT(IDT_Table, 32); RME_X64_USER_IDT(IDT_Table, 33);
    RME_X64_USER_IDT(IDT_Table, 34); RME_X64_USER_IDT(IDT_Table, 35);
    RME_X64_USER_IDT(IDT_Table, 36); RME_X64_USER_IDT(IDT_Table, 37);
    RME_X64_USER_IDT(IDT_Table, 38); RME_X64_USER_IDT(IDT_Table, 39);

    RME_X64_USER_IDT(IDT_Table, 40); RME_X64_USER_IDT(IDT_Table, 41);
    RME_X64_USER_IDT(IDT_Table, 42); RME_X64_USER_IDT(IDT_Table, 43);
    RME_X64_USER_IDT(IDT_Table, 44); RME_X64_USER_IDT(IDT_Table, 45);
    RME_X64_USER_IDT(IDT_Table, 46); RME_X64_USER_IDT(IDT_Table, 47);
    RME_X64_USER_IDT(IDT_Table, 48); RME_X64_USER_IDT(IDT_Table, 49);

    RME_X64_USER_IDT(IDT_Table, 50); RME_X64_USER_IDT(IDT_Table, 51);
    RME_X64_USER_IDT(IDT_Table, 52); RME_X64_USER_IDT(IDT_Table, 53);
    RME_X64_USER_IDT(IDT_Table, 54); RME_X64_USER_IDT(IDT_Table, 55);
    RME_X64_USER_IDT(IDT_Table, 56); RME_X64_USER_IDT(IDT_Table, 57);
    RME_X64_USER_IDT(IDT_Table, 58); RME_X64_USER_IDT(IDT_Table, 59);

    RME_X64_USER_IDT(IDT_Table, 60); RME_X64_USER_IDT(IDT_Table, 61);
    RME_X64_USER_IDT(IDT_Table, 62); RME_X64_USER_IDT(IDT_Table, 63);
    RME_X64_USER_IDT(IDT_Table, 64); RME_X64_USER_IDT(IDT_Table, 65);
    RME_X64_USER_IDT(IDT_Table, 66); RME_X64_USER_IDT(IDT_Table, 67);
    RME_X64_USER_IDT(IDT_Table, 68); RME_X64_USER_IDT(IDT_Table, 69);

    RME_X64_USER_IDT(IDT_Table, 70); RME_X64_USER_IDT(IDT_Table, 71);
    RME_X64_USER_IDT(IDT_Table, 72); RME_X64_USER_IDT(IDT_Table, 73);
    RME_X64_USER_IDT(IDT_Table, 74); RME_X64_USER_IDT(IDT_Table, 75);
    RME_X64_USER_IDT(IDT_Table, 76); RME_X64_USER_IDT(IDT_Table, 77);
    RME_X64_USER_IDT(IDT_Table, 78); RME_X64_USER_IDT(IDT_Table, 79);

    RME_X64_USER_IDT(IDT_Table, 80); RME_X64_USER_IDT(IDT_Table, 81);
    RME_X64_USER_IDT(IDT_Table, 82); RME_X64_USER_IDT(IDT_Table, 83);
    RME_X64_USER_IDT(IDT_Table, 84); RME_X64_USER_IDT(IDT_Table, 85);
    RME_X64_USER_IDT(IDT_Table, 86); RME_X64_USER_IDT(IDT_Table, 87);
    RME_X64_USER_IDT(IDT_Table, 88); RME_X64_USER_IDT(IDT_Table, 89);

    RME_X64_USER_IDT(IDT_Table, 90); RME_X64_USER_IDT(IDT_Table, 91);
    RME_X64_USER_IDT(IDT_Table, 92); RME_X64_USER_IDT(IDT_Table, 93);
    RME_X64_USER_IDT(IDT_Table, 94); RME_X64_USER_IDT(IDT_Table, 95);
    RME_X64_USER_IDT(IDT_Table, 96); RME_X64_USER_IDT(IDT_Table, 97);
    RME_X64_USER_IDT(IDT_Table, 98); RME_X64_USER_IDT(IDT_Table, 99);

    RME_X64_USER_IDT(IDT_Table, 100); RME_X64_USER_IDT(IDT_Table, 101);
    RME_X64_USER_IDT(IDT_Table, 102); RME_X64_USER_IDT(IDT_Table, 103);
    RME_X64_USER_IDT(IDT_Table, 104); RME_X64_USER_IDT(IDT_Table, 105);
    RME_X64_USER_IDT(IDT_Table, 106); RME_X64_USER_IDT(IDT_Table, 107);
    RME_X64_USER_IDT(IDT_Table, 108); RME_X64_USER_IDT(IDT_Table, 109);

    RME_X64_USER_IDT(IDT_Table, 110); RME_X64_USER_IDT(IDT_Table, 111);
    RME_X64_USER_IDT(IDT_Table, 112); RME_X64_USER_IDT(IDT_Table, 113);
    RME_X64_USER_IDT(IDT_Table, 114); RME_X64_USER_IDT(IDT_Table, 115);
    RME_X64_USER_IDT(IDT_Table, 116); RME_X64_USER_IDT(IDT_Table, 117);
    RME_X64_USER_IDT(IDT_Table, 118); RME_X64_USER_IDT(IDT_Table, 119);

    RME_X64_USER_IDT(IDT_Table, 120); RME_X64_USER_IDT(IDT_Table, 121);
    RME_X64_USER_IDT(IDT_Table, 122); RME_X64_USER_IDT(IDT_Table, 123);
    RME_X64_USER_IDT(IDT_Table, 124); RME_X64_USER_IDT(IDT_Table, 125);
    RME_X64_USER_IDT(IDT_Table, 126); RME_X64_USER_IDT(IDT_Table, 127);
    RME_X64_USER_IDT(IDT_Table, 128); RME_X64_USER_IDT(IDT_Table, 129);

    RME_X64_USER_IDT(IDT_Table, 130); RME_X64_USER_IDT(IDT_Table, 131);
    RME_X64_USER_IDT(IDT_Table, 132); RME_X64_USER_IDT(IDT_Table, 133);
    RME_X64_USER_IDT(IDT_Table, 134); RME_X64_USER_IDT(IDT_Table, 135);
    RME_X64_USER_IDT(IDT_Table, 136); RME_X64_USER_IDT(IDT_Table, 137);
    RME_X64_USER_IDT(IDT_Table, 138); RME_X64_USER_IDT(IDT_Table, 139);

    RME_X64_USER_IDT(IDT_Table, 140); RME_X64_USER_IDT(IDT_Table, 141);
    RME_X64_USER_IDT(IDT_Table, 142); RME_X64_USER_IDT(IDT_Table, 143);
    RME_X64_USER_IDT(IDT_Table, 144); RME_X64_USER_IDT(IDT_Table, 145);
    RME_X64_USER_IDT(IDT_Table, 146); RME_X64_USER_IDT(IDT_Table, 147);
    RME_X64_USER_IDT(IDT_Table, 148); RME_X64_USER_IDT(IDT_Table, 149);

    RME_X64_USER_IDT(IDT_Table, 150); RME_X64_USER_IDT(IDT_Table, 151);
    RME_X64_USER_IDT(IDT_Table, 152); RME_X64_USER_IDT(IDT_Table, 153);
    RME_X64_USER_IDT(IDT_Table, 154); RME_X64_USER_IDT(IDT_Table, 155);
    RME_X64_USER_IDT(IDT_Table, 156); RME_X64_USER_IDT(IDT_Table, 157);
    RME_X64_USER_IDT(IDT_Table, 158); RME_X64_USER_IDT(IDT_Table, 159);

    RME_X64_USER_IDT(IDT_Table, 160); RME_X64_USER_IDT(IDT_Table, 161);
    RME_X64_USER_IDT(IDT_Table, 162); RME_X64_USER_IDT(IDT_Table, 163);
    RME_X64_USER_IDT(IDT_Table, 164); RME_X64_USER_IDT(IDT_Table, 165);
    RME_X64_USER_IDT(IDT_Table, 166); RME_X64_USER_IDT(IDT_Table, 167);
    RME_X64_USER_IDT(IDT_Table, 168); RME_X64_USER_IDT(IDT_Table, 169);

    RME_X64_USER_IDT(IDT_Table, 170); RME_X64_USER_IDT(IDT_Table, 171);
    RME_X64_USER_IDT(IDT_Table, 172); RME_X64_USER_IDT(IDT_Table, 173);
    RME_X64_USER_IDT(IDT_Table, 174); RME_X64_USER_IDT(IDT_Table, 175);
    RME_X64_USER_IDT(IDT_Table, 176); RME_X64_USER_IDT(IDT_Table, 177);
    RME_X64_USER_IDT(IDT_Table, 178); RME_X64_USER_IDT(IDT_Table, 179);

    RME_X64_USER_IDT(IDT_Table, 180); RME_X64_USER_IDT(IDT_Table, 181);
    RME_X64_USER_IDT(IDT_Table, 182); RME_X64_USER_IDT(IDT_Table, 183);
    RME_X64_USER_IDT(IDT_Table, 184); RME_X64_USER_IDT(IDT_Table, 185);
    RME_X64_USER_IDT(IDT_Table, 186); RME_X64_USER_IDT(IDT_Table, 187);
    RME_X64_USER_IDT(IDT_Table, 188); RME_X64_USER_IDT(IDT_Table, 189);

    RME_X64_USER_IDT(IDT_Table, 190); RME_X64_USER_IDT(IDT_Table, 191);
    RME_X64_USER_IDT(IDT_Table, 192); RME_X64_USER_IDT(IDT_Table, 193);
    RME_X64_USER_IDT(IDT_Table, 194); RME_X64_USER_IDT(IDT_Table, 195);
    RME_X64_USER_IDT(IDT_Table, 196); RME_X64_USER_IDT(IDT_Table, 197);
    RME_X64_USER_IDT(IDT_Table, 198); RME_X64_USER_IDT(IDT_Table, 199);

    RME_X64_USER_IDT(IDT_Table, 200); RME_X64_USER_IDT(IDT_Table, 201);
    RME_X64_USER_IDT(IDT_Table, 202); RME_X64_USER_IDT(IDT_Table, 203);
    RME_X64_USER_IDT(IDT_Table, 204); RME_X64_USER_IDT(IDT_Table, 205);
    RME_X64_USER_IDT(IDT_Table, 206); RME_X64_USER_IDT(IDT_Table, 207);
    RME_X64_USER_IDT(IDT_Table, 208); RME_X64_USER_IDT(IDT_Table, 209);

    RME_X64_USER_IDT(IDT_Table, 210); RME_X64_USER_IDT(IDT_Table, 211);
    RME_X64_USER_IDT(IDT_Table, 212); RME_X64_USER_IDT(IDT_Table, 213);
    RME_X64_USER_IDT(IDT_Table, 214); RME_X64_USER_IDT(IDT_Table, 215);
    RME_X64_USER_IDT(IDT_Table, 216); RME_X64_USER_IDT(IDT_Table, 217);
    RME_X64_USER_IDT(IDT_Table, 218); RME_X64_USER_IDT(IDT_Table, 219);

    RME_X64_USER_IDT(IDT_Table, 220); RME_X64_USER_IDT(IDT_Table, 221);
    RME_X64_USER_IDT(IDT_Table, 222); RME_X64_USER_IDT(IDT_Table, 223);
    RME_X64_USER_IDT(IDT_Table, 224); RME_X64_USER_IDT(IDT_Table, 225);
    RME_X64_USER_IDT(IDT_Table, 226); RME_X64_USER_IDT(IDT_Table, 227);
    RME_X64_USER_IDT(IDT_Table, 228); RME_X64_USER_IDT(IDT_Table, 229);

    RME_X64_USER_IDT(IDT_Table, 230); RME_X64_USER_IDT(IDT_Table, 231);
    RME_X64_USER_IDT(IDT_Table, 232); RME_X64_USER_IDT(IDT_Table, 233);
    RME_X64_USER_IDT(IDT_Table, 234); RME_X64_USER_IDT(IDT_Table, 235);
    RME_X64_USER_IDT(IDT_Table, 236); RME_X64_USER_IDT(IDT_Table, 237);
    RME_X64_USER_IDT(IDT_Table, 238); RME_X64_USER_IDT(IDT_Table, 239);

    RME_X64_USER_IDT(IDT_Table, 240); RME_X64_USER_IDT(IDT_Table, 241);
    RME_X64_USER_IDT(IDT_Table, 242); RME_X64_USER_IDT(IDT_Table, 243);
    RME_X64_USER_IDT(IDT_Table, 244); RME_X64_USER_IDT(IDT_Table, 245);
    RME_X64_USER_IDT(IDT_Table, 246); RME_X64_USER_IDT(IDT_Table, 247);
    RME_X64_USER_IDT(IDT_Table, 248); RME_X64_USER_IDT(IDT_Table, 249);

    RME_X64_USER_IDT(IDT_Table, 250); RME_X64_USER_IDT(IDT_Table, 251);
    RME_X64_USER_IDT(IDT_Table, 252); RME_X64_USER_IDT(IDT_Table, 253);
    RME_X64_USER_IDT(IDT_Table, 254); RME_X64_USER_IDT(IDT_Table, 255);

    /* Replace systick handler with customized ones - spurious interrupts
     * and IPIs are handled in the general interrupt path. SysTick handler
     * is only processed by the first processor, so we don't register it
     * for other auxiliary processors */
    if(RME_X64_CPU_Cnt==0)
        RME_X64_SET_IDT(IDT_Table, RME_X64_INT_SYSTICK, RME_X64_IDT_VECT, SysTick_Handler);
    /* Register SMP handlers */
    RME_X64_SET_IDT(IDT_Table, RME_X64_INT_SMP_SYSTICK, RME_X64_IDT_VECT, SysTick_SMP_Handler);

    /* Load the IDT */
    Desc[0]=RME_POW2(RME_PGTBL_SIZE_4K)-1;
    Desc[1]=(rme_ptr_t)IDT_Table;
    Desc[2]=((rme_ptr_t)IDT_Table)>>16;
    Desc[3]=((rme_ptr_t)IDT_Table)>>32;
    Desc[4]=((rme_ptr_t)IDT_Table)>>48;
    __RME_X64_IDT_Load((rme_ptr_t*)Desc);

    GDT_Table=(rme_ptr_t*)(RME_X64_CPU_LOCAL_BASE(RME_X64_CPU_Cnt)+RME_POW2(RME_PGTBL_SIZE_4K));
    TSS_Table=(rme_ptr_t)(RME_X64_CPU_LOCAL_BASE(RME_X64_CPU_Cnt)+RME_POW2(RME_PGTBL_SIZE_4K)+16*sizeof(rme_ptr_t));

    /* Dummy entry */
    GDT_Table[0]=0x0000000000000000ULL;
    /* Kernel code, DPL=0, R/X */
    GDT_Table[1]=0x0020980000000000ULL;
    /* Kernel data, DPL=0, W */
    GDT_Table[2]=0x0000920000000000ULL;
    /* Unused entry - this is for sysret instruction's requirement */
    GDT_Table[3]=0x0000000000000000ULL;
    /* User data, DPL=3, W */
    GDT_Table[4]=0x0000F20000000000ULL;
    /* User code, DPL=3, R/X */
    GDT_Table[5]=0x0020F80000000000ULL;
    /* TSS */
    GDT_Table[6]=(0x0067)|((TSS_Table&0xFFFFFFULL)<<16)|(0x0089ULL<<40)|(((TSS_Table>>24)&0xFFULL)<<56);
    GDT_Table[7]=(TSS_Table>>32);

    /* Load the GDT */
    Desc[0]=8*sizeof(rme_ptr_t)-1;
    Desc[1]=(rme_ptr_t)GDT_Table;
    Desc[2]=((rme_ptr_t)GDT_Table)>>16;
    Desc[3]=((rme_ptr_t)GDT_Table)>>32;
    Desc[4]=((rme_ptr_t)GDT_Table)>>48;
    __RME_X64_GDT_Load((rme_ptr_t*)Desc);
    /* Set the RSP to TSS */
    ((rme_u32_t*)TSS_Table)[1]=RME_X64_KSTACK(RME_X64_CPU_Cnt);
    ((rme_u32_t*)TSS_Table)[2]=RME_X64_KSTACK(RME_X64_CPU_Cnt)>>32;
    /* IO Map Base = End of TSS (What's this?) */
    ((rme_u32_t*)TSS_Table)[16]=0x00680000;
    __RME_X64_TSS_Load(6*sizeof(rme_ptr_t));

    /* Initialize the RME per-cpu data here */
    CPU_Local=(struct RME_CPU_Local*)(RME_X64_CPU_LOCAL_BASE(RME_X64_CPU_Cnt)+
    		                          RME_POW2(RME_PGTBL_SIZE_4K)+
									  RME_POW2(RME_PGTBL_SIZE_1K));
    _RME_CPU_Local_Init(CPU_Local,RME_X64_CPU_Cnt);

    /* Initialize x64 specific CPU-local data structure */
    Temp=(struct RME_X64_Temp*)(RME_X64_CPU_LOCAL_BASE(RME_X64_CPU_Cnt+1)-sizeof(struct RME_X64_Temp));
    Temp->CPU_Local_Addr=(rme_ptr_t)CPU_Local;
    Temp->Kernel_SP=RME_X64_KSTACK(RME_X64_CPU_Cnt);
    Temp->Temp_User_SP=0;

    /* Set the base of GS to this memory */
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_KERNEL_GS_BASE, (rme_ptr_t)IDT_Table);
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_GS_BASE, (rme_ptr_t)IDT_Table);
    /* Enable SYSCALL/SYSRET */
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_EFER,__RME_X64_Read_MSR(RME_X64_MSR_IA32_EFER)|RME_X64_MSR_IA32_EFER_SCE);
    /* Set up SYSCALL/SYSRET parameters */
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_LSTAR, (rme_ptr_t)SVC_Handler);
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_FMASK, ~RME_X64_RFLAGS_IF);
    /* The SYSRET, when returning to user mode in 64-bit, will load the SS from +8, and CS from +16.
     * The original place for CS is reserved for 32-bit usages and is thus not usable by 64-bit */
    __RME_X64_Write_MSR(RME_X64_MSR_IA32_STAR, (((rme_ptr_t)RME_X64_SEG_EMPTY)<<48)|(((rme_ptr_t)RME_X64_SEG_KERNEL_CODE)<<32));
}
/* End Function:__RME_X64_CPU_Local_Init *************************************/

/* Begin Function:__RME_X64_CPU_Local_Get_By_CPUID ****************************
Description : Given the CPUID of a CPU, get its RME CPU-local data structure.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
struct RME_CPU_Local* __RME_X64_CPU_Local_Get_By_CPUID(rme_ptr_t CPUID)
{
	return (struct RME_CPU_Local*)(RME_X64_CPU_LOCAL_BASE(CPUID)+
			                       RME_POW2(RME_PGTBL_SIZE_4K)+
								   RME_POW2(RME_PGTBL_SIZE_1K));
}
/* End Function:__RME_X64_CPU_Local_Get_By_CPUID *****************************/

/* Begin Function:__RME_X64_LAPIC_Ack *****************************************
Description : Acknowledge the interrupt on LAPIC.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_LAPIC_Ack(void)
{
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_EOI, 0);
}
/* End Function:__RME_X64_LAPIC_Ack ******************************************/

/* Begin Function:__RME_X64_LAPIC_Init ****************************************
Description : Initialize LAPIC controllers - this will be run once on everycore.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_LAPIC_Init(void)
{
    /* LAPIC initialization - Check if there is any LAPIC */
    RME_ASSERT(RME_X64_LAPIC_Addr!=0);

    /* Enable local APIC; set spurious interrupt vector to 32 */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_SVR, RME_X64_LAPIC_SVR_ENABLE|RME_X64_INT_SPUR);

    /* Disable local interrupt lines */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_LINT0, RME_X64_LAPIC_MASKED);
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_LINT1, RME_X64_LAPIC_MASKED);

    /* Disable performance counter overflow interrupts when there is one */
    if(((RME_X64_LAPIC_READ(RME_X64_LAPIC_VER)>>16)&0xFF)>=4)
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_PCINT, RME_X64_LAPIC_MASKED);

    /* Map error interrupt to IRQ_ERROR */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ERROR, RME_X64_INT_ERROR);

    /* Clear error status register (requires back-to-back writes) */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ESR, 0);
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ESR, 0);

    /* Acknowledge any outstanding interrupts */
    __RME_X64_LAPIC_Ack();

    /* Send an Init Level De-Assert to synchronise arbitration IDs */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRHI, 0);
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_BCAST|
                                             RME_X64_LAPIC_ICRLO_INIT|
                                             RME_X64_LAPIC_ICRLO_LEVEL);
    while(RME_X64_LAPIC_READ(RME_X64_LAPIC_ICRLO)&RME_X64_LAPIC_ICRLO_DELIVS);

    /* Enable interrupts on the APIC */
    RME_X64_LAPIC_WRITE(RME_X64_LAPIC_TPR, 0);
}
/* End Function:__RME_X64_LAPIC_Init *****************************************/

/* Begin Function:__RME_X64_PIC_Init ******************************************
Description : Initialize PIC controllers - we just disable it once and for all.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_PIC_Init(void)
{
    /* Mask all interrupts */
    __RME_X64_Out(RME_X64_PIC1+1, 0xFF);
    __RME_X64_Out(RME_X64_PIC2+1, 0xFF);

    /* Set up master (8259A-1) */
    __RME_X64_Out(RME_X64_PIC1, 0x11);
    __RME_X64_Out(RME_X64_PIC1+1, RME_X64_INT_USER(0));
    __RME_X64_Out(RME_X64_PIC1+1, 1<<2);
    __RME_X64_Out(RME_X64_PIC1+1, 0x3);

    /* Set up slave (8259A-2) */
    __RME_X64_Out(RME_X64_PIC2, 0x11);
    __RME_X64_Out(RME_X64_PIC2+1, RME_X64_INT_USER(8));
    __RME_X64_Out(RME_X64_PIC2+1, 2);
    __RME_X64_Out(RME_X64_PIC2+1, 0x3);

    __RME_X64_Out(RME_X64_PIC1, 0x68);
    __RME_X64_Out(RME_X64_PIC1, 0x0A);

    __RME_X64_Out(RME_X64_PIC2, 0x68);
    __RME_X64_Out(RME_X64_PIC2, 0x0A);

    /* Mask all interrupts - we do not use the PIC at all */
    __RME_X64_Out(RME_X64_PIC1+1, 0xFF);
    __RME_X64_Out(RME_X64_PIC2+1, 0xFF);
}
/* End Function:__RME_X64_PIC_Init *******************************************/

/* Begin Function:__RME_X64_IOAPIC_Int_Enable *********************************
Description : Enable a specific vector on one CPU.
Input       : rme_ptr_t IRQ - The user vector to enable.
              rme_ptr_t CPUID - The CPU to enable this IRQ on.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_IOAPIC_Int_Enable(rme_ptr_t IRQ, rme_ptr_t CPUID)
{
    /* Mark interrupt edge-triggered, active high, enabled, and routed to the
     * given cpunum, which happens to be that cpu's APIC ID. */
    RME_X64_IOAPIC_WRITE(RME_X64_IOAPIC_REG_TABLE+(IRQ<<1),RME_X64_INT_USER(IRQ));
    RME_X64_IOAPIC_WRITE(RME_X64_IOAPIC_REG_TABLE+(IRQ<<1)+1,CPUID<<24);
}
/* End Function:__RME_X64_IOAPIC_Int_Enable **********************************/

/* Begin Function:__RME_X64_IOAPIC_Int_Disable ********************************
Description : Disable a specific vector.
Input       : rme_ptr_t IRQ - The user vector to enable.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_IOAPIC_Int_Disable(rme_ptr_t IRQ)
{
    /* Mark interrupt edge-triggered, active high, enabled, and routed to the
     * given cpunum, which happens to be that cpu's APIC ID. */
    RME_X64_IOAPIC_WRITE(RME_X64_IOAPIC_REG_TABLE+(IRQ<<1),RME_X64_IOAPIC_INT_DISABLED|RME_X64_INT_USER(IRQ));
    RME_X64_IOAPIC_WRITE(RME_X64_IOAPIC_REG_TABLE+(IRQ<<1)+1,0);
}
/* End Function:__RME_X64_IOAPIC_Int_Disable *********************************/

/* Begin Function:__RME_X64_IOAPIC_Init ***************************************
Description : Initialize IOAPIC controllers - this will be run once only.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_IOAPIC_Init(void)
{
    rme_ptr_t Max_Int;
    rme_ptr_t IOAPIC_ID;
    rme_cnt_t Count;
    /* IOAPIC initialization */
    RME_X64_IOAPIC_READ(RME_X64_IOAPIC_REG_VER,Max_Int);
    Max_Int=((Max_Int>>16)&0xFF);
    RME_PRINTK_S("\n\rMax int is: ");
    RME_PRINTK_I(Max_Int);
    RME_X64_IOAPIC_READ(RME_X64_IOAPIC_REG_ID,IOAPIC_ID);
    IOAPIC_ID>>=24;
    /* This is not necessarily true when we have >1 IOAPICs */
    /* RME_ASSERT(IOAPIC_ID==RME_X64_IOAPIC_Info[0].IOAPIC_ID); */
    RME_PRINTK_S("\n\rIOAPIC ID is: ");
    RME_PRINTK_I(IOAPIC_ID);

    /* Disable all interrupts */
    for(Count=0;Count<=Max_Int;Count++)
        __RME_X64_IOAPIC_Int_Disable(Count);
}
/* End Function:__RME_X64_IOAPIC_Init ****************************************/

/* Begin Function:__RME_X64_SMP_Init ******************************************
Description : Start all other processors, one by one. We cannot start all of them
              at once because of the stupid self modifying code of X64!
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_SMP_Init(void)
{
    rme_u8_t* Code;
    rme_cnt_t Count;
    rme_u16_t* Warm_Reset;

    /* Write entry code to unused memory at 0x7000 */
    Code=(rme_u8_t*)RME_X64_PA2VA(0x7000);
    for(Count=0;Count<sizeof(RME_X64_Boot_Code);Count++)
        Code[Count]=RME_X64_Boot_Code[Count];

    /* Start the CPUs one by one - the first one is ourself */
    RME_X64_CPU_Cnt=1;
    for(Count=1;Count<RME_X64_Num_CPU;Count++)
    {
        RME_PRINTK_S("\n\rBooting CPU ");
        RME_PRINTK_I(Count);
        /* Temporary stack */
        *(rme_u32_t*)(Code-4)=0x8000;
        *(rme_u32_t*)(Code-8)=RME_X64_TEXT_VA2PA(__RME_X64_SMP_Boot_32);
        *(rme_ptr_t*)(Code-16)=RME_X64_KSTACK(Count);

        /* Initialize CMOS shutdown code to 0AH */
        __RME_X64_Out(RME_X64_RTC_CMD,0xF);
        __RME_X64_Out(RME_X64_RTC_DATA,0xA);
        /* Warm reset vector point to AP code */
        Warm_Reset=(rme_u16_t*)RME_X64_PA2VA((0x40<<4|0x67));
        Warm_Reset[0]=0;
        Warm_Reset[1]=0x7000>>4;

        /* Send INIT (level-triggered) interrupt to reset other CPU */
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRHI, RME_X64_CPU_Info[Count].LAPIC_ID<<24);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_INIT|
                                                 RME_X64_LAPIC_ICRLO_LEVEL|
                                                 RME_X64_LAPIC_ICRLO_ASSERT);
        RME_X64_UDELAY(200);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_INIT|
                                                 RME_X64_LAPIC_ICRLO_LEVEL);
        RME_X64_UDELAY(10000);

        /* Send startup IPI twice according to Intel manuals */
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRHI, RME_X64_CPU_Info[Count].LAPIC_ID<<24);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_STARTUP|(0x7000>>12));
        RME_X64_UDELAY(200);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRHI, RME_X64_CPU_Info[Count].LAPIC_ID<<24);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_STARTUP|(0x7000>>12));
        RME_X64_UDELAY(200);

        /* Wait for CPU to finish its own initialization */
        while(RME_X64_CPU_Info[RME_X64_CPU_Cnt].Boot_Done==0);
        RME_X64_CPU_Cnt++;
    }
}
/* End Function:__RME_X64_SMP_Init *******************************************/

/* Begin Function:__RME_X64_SMP_Tick ******************************************
Description : Send IPI to all other cores,to run their handler on the time.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_SMP_Tick(void)
{
    /* Is this a SMP? */
    if(RME_X64_Num_CPU>1)
    {
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRHI, 0xFFULL<<24);
        RME_X64_LAPIC_WRITE(RME_X64_LAPIC_ICRLO, RME_X64_LAPIC_ICRLO_EXC_SELF|
                                                 RME_X64_LAPIC_ICRLO_FIXED|
                                                 RME_X64_INT_SMP_SYSTICK);
    }
}
/* End Function:__RME_X64_SMP_Tick *******************************************/

/* Begin Function:__RME_X64_Timer_Init ****************************************
Description : Initialize the on-board timer. We use the PIT because it is stable;
              Then we let the main CPU send out timer IPI interrupts to all other
              CPUs.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_X64_Timer_Init(void)
{
    /* For timer interrupts, they will always be handled by core 1, and all the other
     * cores should receive a IPI for that, so their scheduler can look after their
     * threads. We are using square wave mode. */
    __RME_X64_Out(RME_X64_PIT_CMD,0x34);
    __RME_X64_Out(RME_X64_PIT_CH0,(1193182/2/RME_X64_TIMER_FREQ)&0xFF);
    __RME_X64_Out(RME_X64_PIT_CH0,((1193182/2/RME_X64_TIMER_FREQ)>>8)&0xFF);
}
/* End Function:__RME_X64_Timer_Init *****************************************/

/* Begin Function:__RME_Low_Level_Init ****************************************
Description : Initialize the low-level hardware.
Input       : None.
Output      : None.
Return      : rme_ptr_t - Always 0.
******************************************************************************/
rme_ptr_t __RME_Low_Level_Init(void)
{
    /* We are here now ! */
    __RME_X64_UART_Init();
    /* Read APIC tables and detect the configurations. Now we are not NUMA-aware */
    RME_ASSERT(__RME_X64_ACPI_Init()==0);
    /* Detect CPU features */
    __RME_X64_Feature_Get();
    /* Extract memory specifications */
    __RME_X64_Mem_Init(RME_X64_MBInfo->mmap_addr,RME_X64_MBInfo->mmap_length);

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
    rme_cnt_t PML4_Cnt;
    rme_cnt_t PDP_Cnt;
    rme_cnt_t PDE_Cnt;
    rme_cnt_t Addr_Cnt;
    struct __RME_X64_Pgreg* Pgreg;
    struct __RME_X64_Mem* Mem;

    /* Now initialize the kernel object allocation table */
    _RME_Kotbl_Init(RME_X64_Layout.Kotbl_Size/sizeof(rme_ptr_t));
    /* Reset PCID counter */
    RME_X64_PCID_Inc=0;

    /* And the page table registration table as well */
    Pgreg=(struct __RME_X64_Pgreg*)RME_X64_Layout.Pgreg_Start;
    for(PML4_Cnt=0;PML4_Cnt<RME_X64_Layout.Pgreg_Size/sizeof(struct __RME_X64_Pgreg);PML4_Cnt++)
    {
        Pgreg[PML4_Cnt].Child_Cnt=0;
        Pgreg[PML4_Cnt].Parent_Cnt=0;
    }

    /* Create the frame for kernel page tables */
    for(PML4_Cnt=0;PML4_Cnt<256;PML4_Cnt++)
    {
        RME_X64_Kpgt.PML4[PML4_Cnt]=RME_X64_MMU_ADDR(RME_X64_TEXT_VA2PA(&(RME_X64_Kpgt.PDP[PML4_Cnt][0])))|
        		                    RME_X64_MMU_KERN_PML4;

        for(PDP_Cnt=0;PDP_Cnt<512;PDP_Cnt++)
            RME_X64_Kpgt.PDP[PML4_Cnt][PDP_Cnt]=RME_X64_MMU_KERN_PDP;
    }

    /* Map in the first 4GB as linear mappings as always, 4 super pages, including the device hole.
     * We need to detect whether the 1GB page is supported. If not, we just map the initial tables
     * in, and we know where they are hard-coded in the assembly file */
    if((RME_X64_EXT(RME_X64_CPUID_E1_INFO_FEATURE,3)&RME_X64_E1_EDX_PDPE1GB)!=0)
    {
        /* Can use 1GB pages */
        RME_PRINTK_S("\n\rThis CPU have 1GB superpage support");
        RME_X64_Kpgt.PDP[0][0]|=RME_X64_MMU_ADDR(0)|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][1]|=RME_X64_MMU_ADDR(RME_POW2(RME_PGTBL_SIZE_1G))|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][2]|=RME_X64_MMU_ADDR(2*RME_POW2(RME_PGTBL_SIZE_1G))|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
        /* We need to mark the device hole as unbufferable */
        RME_X64_Kpgt.PDP[0][3]|=RME_X64_MMU_ADDR(3*RME_POW2(RME_PGTBL_SIZE_1G))|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][3]|=RME_X64_MMU_PWT|RME_X64_MMU_PCD;

        /* Map the first 2GB to the last position too, where the kernel text segment is at */
        RME_X64_Kpgt.PDP[255][510]|=RME_X64_MMU_ADDR(0)|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[255][511]|=RME_X64_MMU_ADDR(RME_POW2(RME_PGTBL_SIZE_1G))|RME_X64_MMU_PDE_SUP|RME_X64_MMU_P;
    }
    else
    {
        RME_PRINTK_S("\n\rThis CPU do not have 1GB superpage support");
        /* Cannot use 1GB pages, we revert to 2MB pages used during kernel startup */
        RME_X64_Kpgt.PDP[0][0]|=0x104000|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][1]|=0x105000|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][2]|=0x106000|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[0][3]|=0x107000|RME_X64_MMU_PCD|RME_X64_MMU_PWT|RME_X64_MMU_P;

        /* Map the first 2GB to the last position too, where the kernel text segment is at */
        RME_X64_Kpgt.PDP[255][510]|=0x104000|RME_X64_MMU_P;
        RME_X64_Kpgt.PDP[255][511]|=0x105000|RME_X64_MMU_P;
    }

    /* Ignore all memory below 4G, but we need to get the size of such memory above 16MB */
    Mem=(struct __RME_X64_Mem*)RME_X64_Phys_Mem.Next;
    while(Mem!=(struct __RME_X64_Mem*)(&RME_X64_Phys_Mem))
    {
        /* See if this memory segment passes 16MB limit */
        if((Mem->Start_Addr+Mem->Length)<=RME_POW2(RME_PGTBL_SIZE_16M))
            Mem=(struct __RME_X64_Mem*)(Mem->Head.Next);
        else
            break;
    }

    /* The first Kmem1 trunk must start at smaller or equal to 16MB */
    RME_ASSERT(Mem->Start_Addr<=RME_POW2(RME_PGTBL_SIZE_16M));
    /* The raw sizes of kernel memory segment 1 - per CPU area is already aligned so no need to align again */
    RME_X64_Layout.Kmem1_Start[0]=RME_X64_Layout.PerCPU_Start+RME_X64_Layout.PerCPU_Size;
    RME_X64_Layout.Kmem1_Size[0]=Mem->Start_Addr+Mem->Length-RME_POW2(RME_PGTBL_SIZE_16M)-
    		                     RME_X64_VA2PA(RME_X64_Layout.Kmem1_Start[0]);

    /* Add the rest of Kmem1 into the array */
    Addr_Cnt=1;
    while(Mem!=(struct __RME_X64_Mem*)(&RME_X64_Phys_Mem))
    {
        /* Add all segments under 4GB to Kmem1 */
        Mem=(struct __RME_X64_Mem*)(Mem->Head.Next);
        /* If detected anything above 4GB, then this is not Kmem1, exiting */
        if(Mem->Start_Addr>=RME_POW2(RME_PGTBL_SIZE_4G))
            break;
        /* If this memory trunk have less than 4MB, drop it */
        if(Mem->Length<RME_POW2(RME_PGTBL_SIZE_4M))
        {
            RME_PRINTK_S("\n\rAbandoning physical memory below 4G: addr 0x");
            RME_PRINTK_U(Mem->Start_Addr);
            RME_PRINTK_S(", length 0x");
            RME_PRINTK_U(Mem->Length);
            continue;
        }
        if(Addr_Cnt>=RME_X64_KMEM1_MAXSEGS)
        {
            RME_PRINTK_S("\r\nThe memory under 4G is too fragmented. Aborting.");
            RME_ASSERT(0);
        }
        RME_X64_Layout.Kmem1_Start[Addr_Cnt]=RME_X64_PA2VA(RME_ROUND_UP(Mem->Start_Addr,RME_PGTBL_SIZE_2M));
        RME_X64_Layout.Kmem1_Size[Addr_Cnt]=RME_ROUND_DOWN(Mem->Length,RME_PGTBL_SIZE_2M);
        Addr_Cnt++;
    }
    RME_X64_Layout.Kmem1_Trunks=Addr_Cnt;

    /* This is the hole */
    RME_X64_Layout.Hole_Start=RME_X64_Layout.Kmem1_Start[Addr_Cnt-1]+RME_X64_Layout.Kmem1_Size[Addr_Cnt-1];
    RME_X64_Layout.Hole_Size=RME_POW2(RME_PGTBL_SIZE_4G)-RME_X64_VA2PA(RME_X64_Layout.Hole_Start);

    /* Create kernel page mappings for memory above 4GB - we assume only one segment below 4GB */
    RME_X64_Layout.Kpgtbl_Start=RME_X64_Layout.Kmem1_Start[0];
    RME_X64_Layout.Kmem2_Start=RME_X64_PA2VA(RME_POW2(RME_PGTBL_SIZE_4G));
    RME_X64_Layout.Kmem2_Size=0;

    /* We have filled the first 4 1GB superpages */
    PML4_Cnt=0;
    PDP_Cnt=3;
    PDE_Cnt=511;
    while(Mem!=(struct __RME_X64_Mem*)(&RME_X64_Phys_Mem))
    {
        /* Throw away small segments */
        if(Mem->Length<2*RME_POW2(RME_PGTBL_SIZE_2M))
        {
            RME_PRINTK_S("\n\rAbandoning physical memory above 4G: addr 0x");
            RME_PRINTK_U(Mem->Start_Addr);
            RME_PRINTK_S(", length 0x");
            RME_PRINTK_U(Mem->Length);
            Mem=(struct __RME_X64_Mem*)(Mem->Head.Next);
            continue;
        }

        /* Align the memory segment to 2MB */
        Mem->Start_Addr=RME_ROUND_UP(Mem->Start_Addr,RME_PGTBL_SIZE_2M);
        Mem->Length=RME_ROUND_DOWN(Mem->Length-1,RME_PGTBL_SIZE_2M);

        /* Add these pages into the kernel at addresses above 4GB offset as 2MB pages */
        for(Addr_Cnt=0;Addr_Cnt<Mem->Length;Addr_Cnt+=RME_POW2(RME_PGTBL_SIZE_2M))
        {
            PDE_Cnt++;
            if(PDE_Cnt==512)
            {
                PDE_Cnt=0;
                PDP_Cnt++;
                if(PDP_Cnt==512)
                {
                    PDP_Cnt=0;
                    PML4_Cnt++;
                }
                /* Map this PDE into the PDP */
                RME_X64_Kpgt.PDP[PML4_Cnt][PDP_Cnt]|=RME_X64_MMU_ADDR(RME_X64_VA2PA(RME_X64_Layout.Kmem1_Start[0]))|RME_X64_MMU_P;
            }

            ((rme_ptr_t*)(RME_X64_Layout.Kmem1_Start[0]))[0]=RME_X64_MMU_ADDR(Mem->Start_Addr+Addr_Cnt)|RME_X64_MMU_KERN_PDE;
            RME_X64_Layout.Kmem1_Start[0]+=sizeof(rme_ptr_t);
            RME_X64_Layout.Kmem1_Size[0]-=sizeof(rme_ptr_t);
            RME_X64_Layout.Kmem2_Size+=RME_POW2(RME_PGTBL_SIZE_2M);
        }

        Mem=(struct __RME_X64_Mem*)(Mem->Head.Next);
    }

    /* Copy the new page tables to the temporary entries, so that we can boot SMP */
    for(PML4_Cnt=0;PML4_Cnt<256;PML4_Cnt++)
        ((rme_ptr_t*)RME_X64_PA2VA(0x101000))[PML4_Cnt+256]=RME_X64_Kpgt.PML4[PML4_Cnt];

    /* Page table allocation finished. Now need to align Kmem1 to 2MB page boundary */
    RME_X64_Layout.Kmem1_Start[0]=RME_ROUND_UP(RME_X64_Layout.Kmem1_Start[0],RME_PGTBL_SIZE_2M);
    RME_X64_Layout.Kmem1_Size[0]=RME_ROUND_DOWN(RME_X64_Layout.Kmem1_Size[0]-1,RME_PGTBL_SIZE_2M);

    /* All memory is mapped. Now figure out the size of kernel stacks */
    RME_X64_Layout.Kpgtbl_Size=RME_X64_Layout.Kmem1_Start[0]-RME_X64_Layout.Kpgtbl_Start;

    /* See if we are allocating the stack from Kmem2 or Kmem1 */
    if(RME_X64_Layout.Kmem2_Size==0)
    {
        RME_X64_Layout.Stack_Start=RME_ROUND_DOWN(RME_X64_Layout.Kmem1_Start[0]+RME_X64_Layout.Kmem1_Size[0]-1,RME_X64_KSTACK_ORDER);
        RME_X64_Layout.Stack_Start-=RME_X64_Layout.Stack_Size;
        RME_X64_Layout.Kmem1_Size[0]=RME_X64_Layout.Stack_Start-RME_X64_Layout.Kmem1_Start[0];
    }


    else
    {
        RME_X64_Layout.Stack_Start=RME_ROUND_DOWN(RME_X64_Layout.Kmem2_Start+RME_X64_Layout.Kmem2_Size-1,RME_X64_KSTACK_ORDER);
        RME_X64_Layout.Stack_Start-=RME_X64_Layout.Stack_Size;
        RME_X64_Layout.Kmem2_Size=RME_X64_Layout.Stack_Start-RME_X64_Layout.Kmem2_Start;
    }

    /* Now report all mapping info */
    RME_PRINTK_S("\n\r\n\rKotbl_Start:     0x");
    RME_PRINTK_U(RME_X64_Layout.Kotbl_Start);
    RME_PRINTK_S("\n\rKotbl_Size:      0x");
    RME_PRINTK_U(RME_X64_Layout.Kotbl_Size);
    RME_PRINTK_S("\n\rPgreg_Start:     0x");
    RME_PRINTK_U(RME_X64_Layout.Pgreg_Start);
    RME_PRINTK_S("\n\rPgreg_Size:      0x");
    RME_PRINTK_U(RME_X64_Layout.Pgreg_Size);
    RME_PRINTK_S("\n\rPerCPU_Start:    0x");
    RME_PRINTK_U(RME_X64_Layout.PerCPU_Start);
    RME_PRINTK_S("\n\rPerCPU_Size:     0x");
    RME_PRINTK_U(RME_X64_Layout.PerCPU_Size);
    RME_PRINTK_S("\n\rKpgtbl_Start:    0x");
    RME_PRINTK_U(RME_X64_Layout.Kpgtbl_Start);
    RME_PRINTK_S("\n\rKpgtbl_Size:     0x");
    RME_PRINTK_U(RME_X64_Layout.Kpgtbl_Size);
    for(Addr_Cnt=0;Addr_Cnt<RME_X64_Layout.Kmem1_Trunks;Addr_Cnt++)
    {
        RME_PRINTK_S("\n\rKmem1_Start[");
        RME_PRINTK_I(Addr_Cnt);
        RME_PRINTK_S("]:  0x");
        RME_PRINTK_U(RME_X64_Layout.Kmem1_Start[Addr_Cnt]);
        RME_PRINTK_S("\n\rKmem1_Size[");
        RME_PRINTK_I(Addr_Cnt);
        RME_PRINTK_S("]:   0x");
        RME_PRINTK_U(RME_X64_Layout.Kmem1_Size[Addr_Cnt]);
    }
    RME_PRINTK_S("\n\rHole_Start:      0x");
    RME_PRINTK_U(RME_X64_Layout.Hole_Start);
    RME_PRINTK_S("\n\rHole_Size:       0x");
    RME_PRINTK_U(RME_X64_Layout.Hole_Size);
    RME_PRINTK_S("\n\rKmem2_Start:     0x");
    RME_PRINTK_U(RME_X64_Layout.Kmem2_Start);
    RME_PRINTK_S("\n\rKmem2_Size:      0x");
    RME_PRINTK_U(RME_X64_Layout.Kmem2_Size);
    RME_PRINTK_S("\n\rStack_Start:     0x");
    RME_PRINTK_U(RME_X64_Layout.Stack_Start);
    RME_PRINTK_S("\n\rStack_Size:      0x");
    RME_PRINTK_U(RME_X64_Layout.Stack_Size);

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
    struct RME_CPU_Local* CPU_Local;

    /* Initialize all vector tables */
    __RME_X64_CPU_Local_Init();
    /* Initialize LAPIC */
    __RME_X64_LAPIC_Init();

    /* Check to see if we are booting this correctly */
    CPU_Local=RME_CPU_LOCAL();
    RME_ASSERT(CPU_Local->CPUID==RME_X64_CPU_Cnt);

    RME_X64_CPU_Info[RME_X64_CPU_Cnt].Boot_Done=1;
    /* Spin until the global CPU counter is zero again, which means the booting
     * processor has done booting and we can proceed now */
    while(RME_X64_CPU_Cnt!=0);

    /* The booting CPU must have created everything necessary for us. Let's
     * check to see if they are there. */
    RME_ASSERT(CPU_Local->Cur_Thd!=0);
    RME_ASSERT(CPU_Local->Tick_Sig!=0);
    RME_ASSERT(CPU_Local->Vect_Sig!=0);

    /* Change page tables */
    __RME_Pgtbl_Set(RME_CAP_GETOBJ((CPU_Local->Cur_Thd)->Sched.Proc->Pgtbl,rme_ptr_t));
    /* Boot into the init thread - never returns */
    __RME_Enter_User_Mode(0, RME_X64_USTACK(CPU_Local->CPUID), CPU_Local->CPUID);

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
    rme_ptr_t Cur_Addr;
    rme_cnt_t Count;
    rme_cnt_t Kmem1_Cnt;
    rme_ptr_t Phys_Addr;
    rme_ptr_t Page_Ptr;
    struct RME_Cap_Captbl* Captbl;
    struct RME_CPU_Local* CPU_Local;

    /* Initialize our own CPU-local data structures */
    RME_X64_CPU_Cnt=0;
    RME_PRINTK_S("\r\nCPU 0 local IDT/GDT init");
    __RME_X64_CPU_Local_Init();
    /* Initialize interrupt controllers (PIC, LAPIC, IOAPIC) */
    RME_PRINTK_S("\r\nCPU 0 LAPIC init");
    __RME_X64_LAPIC_Init();
    RME_PRINTK_S("\r\nPIC init");
    __RME_X64_PIC_Init();
    RME_PRINTK_S("\r\nIOAPIC init");
    __RME_X64_IOAPIC_Init();
    /* Start other processors, if there are any. They will keep spinning until
     * the booting processor finish all its work. */
    __RME_X64_SMP_Init();

    /* Create all initial tables in Kmem1, which is sure to be present. We reserve 16
     * pages at the start to load the init process */
    Cur_Addr=RME_X64_Layout.Kmem1_Start[0]+16*RME_POW2(RME_PGTBL_SIZE_2M);
    RME_PRINTK_S("\r\nKotbl registration start offset: 0x");
    RME_PRINTK_U(((Cur_Addr-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER)/8);

    /* Create the capability table for the init process - always 16 */
    Captbl=(struct RME_Cap_Captbl*)Cur_Addr;
    RME_ASSERT(_RME_Captbl_Boot_Init(RME_BOOT_CAPTBL,Cur_Addr,16)==RME_BOOT_CAPTBL);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(16));

    /* Create the capability table for initial page tables - now we are only
     * adding 2MB pages. There will be 1 PML4, 16 PDP, and 16*512=8192 PGD.
     * This should provide support for up to 4TB of memory, which will be sufficient
     * for at least a decade. These data structures will eat 32MB of memory, which
     * is fine */
    RME_ASSERT(_RME_Captbl_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_TBL_PGTBL, Cur_Addr, 1+16+8192)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(1+16+8192));

    /* Align the address to 4096 to prepare for page table creation */
    Cur_Addr=RME_ROUND_UP(Cur_Addr,12);
    /* Create PML4 */
    RME_ASSERT(_RME_Pgtbl_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_PGTBL, RME_BOOT_PML4,
                                   Cur_Addr, 0, RME_PGTBL_TOP, RME_PGTBL_SIZE_512G, RME_PGTBL_NUM_512)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_PGTBL_SIZE_TOP(RME_PGTBL_NUM_512));
    /* Create all our 16 PDPs, and cons them into the PML4 */
    for(Count=0;Count<16;Count++)
    {
        RME_ASSERT(_RME_Pgtbl_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_PGTBL, RME_BOOT_PDP(Count),
                                       Cur_Addr, 0, RME_PGTBL_NOM, RME_PGTBL_SIZE_1G, RME_PGTBL_NUM_512)==0);
        Cur_Addr+=RME_KOTBL_ROUND(RME_PGTBL_SIZE_NOM(RME_PGTBL_NUM_512));
        RME_ASSERT(_RME_Pgtbl_Boot_Con(RME_X64_CPT, RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PML4), Count,
                                       RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PDP(Count)), RME_PGTBL_ALL_PERM)==0);
    }

    /* Create 8192 PDEs, and cons them into their respective PDPs */
    for(Count=0;Count<8192;Count++)
    {
        RME_ASSERT(_RME_Pgtbl_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_PGTBL, RME_BOOT_PDE(Count),
                                       Cur_Addr, 0, RME_PGTBL_NOM, RME_PGTBL_SIZE_2M, RME_PGTBL_NUM_512)==0);
        Cur_Addr+=RME_KOTBL_ROUND(RME_PGTBL_SIZE_NOM(RME_PGTBL_NUM_512));
        RME_ASSERT(_RME_Pgtbl_Boot_Con(RME_X64_CPT, RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PDP(Count>>9)), Count&0x1FF,
                                       RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PDE(Count)), RME_PGTBL_ALL_PERM)==0);
    }

    /* Map all the Kmem1 that we have into it */
    Page_Ptr=0;
    for(Kmem1_Cnt=0;Kmem1_Cnt<RME_X64_Layout.Kmem1_Trunks;Kmem1_Cnt++)
    {
        for(Count=0;Count<RME_X64_Layout.Kmem1_Size[Kmem1_Cnt];Count+=RME_POW2(RME_PGTBL_SIZE_2M))
        {
            Phys_Addr=RME_X64_VA2PA(RME_X64_Layout.Kmem1_Start[Kmem1_Cnt])+Count;
            RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_X64_CPT, RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PDE(Page_Ptr>>9)),
                                           Phys_Addr, Page_Ptr&0x1FF, RME_PGTBL_ALL_PERM)==0);
            Page_Ptr++;
        }
    }
    RME_PRINTK_S("\r\nKmem1 pages: 0x");
    RME_PRINTK_U(Page_Ptr);
    RME_PRINTK_S(", [0x0, 0x");
    RME_PRINTK_U(Page_Ptr*RME_POW2(RME_PGTBL_SIZE_2M)+RME_POW2(RME_PGTBL_SIZE_2M)-1);
    RME_PRINTK_S("]");

    /* Map the Kmem2 in - don't want lookups, we know where they are. Offset by 2048 because they are mapped above 4G */
    RME_PRINTK_S("\r\nKmem2 pages: 0x");
    RME_PRINTK_U(RME_X64_Layout.Kmem2_Size/RME_POW2(RME_PGTBL_SIZE_2M));
    RME_PRINTK_S(", [0x");
    RME_PRINTK_U(Page_Ptr*RME_POW2(RME_PGTBL_SIZE_2M)+RME_POW2(RME_PGTBL_SIZE_2M));
    RME_PRINTK_S(", 0x");
    for(Count=2048;Count<(RME_X64_Layout.Kmem2_Size/RME_POW2(RME_PGTBL_SIZE_2M)+2048);Count++)
    {
        Phys_Addr=RME_X64_PA2VA(RME_X64_MMU_ADDR(RME_X64_Kpgt.PDP[Count>>18][(Count>>9)&0x1FF]));
        Phys_Addr=RME_X64_MMU_ADDR(((rme_ptr_t*)Phys_Addr)[Count&0x1FF]);
        RME_ASSERT(_RME_Pgtbl_Boot_Add(RME_X64_CPT, RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PDE(Page_Ptr>>9)),
                                       Phys_Addr, Page_Ptr&0x1FF, RME_PGTBL_ALL_PERM)==0);
        Page_Ptr++;
    }
    RME_PRINTK_U(Page_Ptr*RME_POW2(RME_PGTBL_SIZE_2M)+RME_POW2(RME_PGTBL_SIZE_2M)-1);
    RME_PRINTK_S("]");

    /* Activate the first process - This process cannot be deleted */
    RME_ASSERT(_RME_Proc_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_PROC,
                                  RME_BOOT_CAPTBL, RME_CAPID(RME_BOOT_TBL_PGTBL,RME_BOOT_PML4))==0);

    /* Create the initial kernel function capability */
    RME_ASSERT(_RME_Kern_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_INIT_KERN)==0);

    /* Create a capability table for initial kernel memory capabilities. We need a few for Kmem1, and another one for Kmem2 */
    RME_ASSERT(_RME_Captbl_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_TBL_KMEM, Cur_Addr, RME_X64_KMEM1_MAXSEGS+1)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(RME_X64_KMEM1_MAXSEGS+1));
    /* Create Kmem1 capabilities - can create page tables here */
    for(Count=0;Count<RME_X64_Layout.Kmem1_Trunks;Count++)
    {
        RME_ASSERT(_RME_Kmem_Boot_Crt(RME_X64_CPT,
                                      RME_BOOT_TBL_KMEM, Count,
                                      RME_X64_Layout.Kmem1_Start[Count],
                                      RME_X64_Layout.Kmem1_Start[Count]+RME_X64_Layout.Kmem1_Size[Count],
                                      RME_KMEM_FLAG_ALL)==0);
    }
    /* Create Kmem2 capability - cannot create page tables here */
    RME_ASSERT(_RME_Kmem_Boot_Crt(RME_X64_CPT,
                                  RME_BOOT_TBL_KMEM, RME_X64_KMEM1_MAXSEGS,
                                  RME_X64_Layout.Kmem2_Start,
                                  RME_X64_Layout.Kmem2_Start+RME_X64_Layout.Kmem2_Size,
                                  RME_KMEM_FLAG_CAPTBL|RME_KMEM_FLAG_THD|RME_KMEM_FLAG_INV)==0);

    /* Create the initial kernel endpoints for timer ticks */
    RME_ASSERT(_RME_Captbl_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_TBL_TIMER, Cur_Addr, RME_X64_Num_CPU)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(RME_X64_Num_CPU));
    for(Count=0;Count<RME_X64_Num_CPU;Count++)
    {
    	CPU_Local=__RME_X64_CPU_Local_Get_By_CPUID(Count);
    	CPU_Local->Tick_Sig=&(RME_GETOBJ(&(RME_X64_CPT[RME_BOOT_TBL_TIMER]), struct RME_Cap_Sig*)[Count]);
        RME_ASSERT(_RME_Sig_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_TIMER, Count)==0);
    }

    /* Create the initial kernel endpoints for all other interrupts */
    RME_ASSERT(_RME_Captbl_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_TBL_INT, Cur_Addr, RME_X64_Num_CPU)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(RME_X64_Num_CPU));
    for(Count=0;Count<RME_X64_Num_CPU;Count++)
    {
    	CPU_Local=__RME_X64_CPU_Local_Get_By_CPUID(Count);
    	CPU_Local->Vect_Sig=&(RME_GETOBJ(&(RME_X64_CPT[RME_BOOT_TBL_INT]), struct RME_Cap_Sig*)[Count]);
        RME_ASSERT(_RME_Sig_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_INT, Count)==0);
    }

    /* Activate the first thread, and set its priority */
    RME_ASSERT(_RME_Captbl_Boot_Crt(RME_X64_CPT, RME_BOOT_CAPTBL, RME_BOOT_TBL_THD, Cur_Addr, RME_X64_Num_CPU)==0);
    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(RME_X64_Num_CPU));
    for(Count=0;Count<RME_X64_Num_CPU;Count++)
    {
    	CPU_Local=__RME_X64_CPU_Local_Get_By_CPUID(Count);
        RME_ASSERT(_RME_Thd_Boot_Crt(RME_X64_CPT, RME_BOOT_TBL_THD, Count, RME_BOOT_INIT_PROC, Cur_Addr, 0, CPU_Local)>=0);
        Cur_Addr+=RME_KOTBL_ROUND(RME_THD_SIZE);
    }

    RME_PRINTK_S("\r\nKotbl registration end offset: 0x");
    RME_PRINTK_U(((Cur_Addr-RME_KMEM_VA_START)>>RME_KMEM_SLOT_ORDER)/8);
    RME_PRINTK_S("\r\nKmem1 frontier: 0x");
    RME_PRINTK_U(Cur_Addr);

    /* Print sizes and halt */
    RME_PRINTK_S("\r\nThread object size: ");
    RME_PRINTK_I(sizeof(struct RME_Thd_Struct)/sizeof(rme_ptr_t));
    RME_PRINTK_S("\r\nInvocation object size: ");
    RME_PRINTK_I(sizeof(struct RME_Inv_Struct)/sizeof(rme_ptr_t));

    /* Initialize the timer and start its interrupt routing */
    RME_PRINTK_S("\r\nTimer init\r\n");
    __RME_X64_Timer_Init();
    //__RME_X64_IOAPIC_Int_Enable(2,0);
    /* Change page tables */
    __RME_Pgtbl_Set(RME_CAP_GETOBJ((RME_CPU_LOCAL()->Cur_Thd)->Sched.Proc->Pgtbl,rme_ptr_t));


    /* Load the init process to address 0x00 - It should be smaller than 2MB */
    extern const unsigned char UVM_Init[];
    _RME_Memcpy(0,(void*)UVM_Init,RME_POW2(RME_PGTBL_SIZE_2M));


    /* Now other non-booting processors may proceed and go into their threads */
    RME_X64_CPU_Cnt=0;
    /* Boot into the init thread */
    __RME_Enter_User_Mode(0, RME_X64_USTACK(0), 0);
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
    /* Currently we cannot parse th FADT yet. We need these info to shutdown the machine */
    /* outportb(FADT->ResetReg.Address, FADT->ResetValue); */
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
    /* Currently we cannot parse th DSDT yet. We need these info to shutdown the machine */
    /* outw(PM1a_CNT,SLP_TYPa|SLP_EN) */
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
    *Svc=(Reg->RDI)>>32;
    *Capid=(Reg->RDI)&0xFFFFFFFF;
    Param[0]=Reg->RSI;
    Param[1]=Reg->RDX;
    Param[2]=Reg->R8;
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
    Reg->RAX=(rme_ptr_t)Retval;
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
    /* We use the SYSRET path on creation if possible */
    Reg->INT_NUM=0x10000;
    Reg->ERROR_CODE=0;
    Reg->RIP=Entry;
    Reg->CS=RME_X64_SEG_USER_CODE;
    /* IOPL 3, IF */
    Reg->RFLAGS=0x3200;
    Reg->RSP=Stack;
    Reg->SS=RME_X64_SEG_USER_DATA;
    /* Pass the parameter */
    Reg->RDI=Param;
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
    Dst->RAX=Src->RAX;
    Dst->RBX=Src->RBX;
    Dst->RCX=Src->RCX;
    Dst->RDX=Src->RDX;
    Dst->RSI=Src->RSI;
    Dst->RDI=Src->RDI;
    Dst->RBP=Src->RBP;
    Dst->R8=Src->R8;
    Dst->R9=Src->R9;
    Dst->R10=Src->R10;
    Dst->R11=Src->R11;
    Dst->R12=Src->R12;
    Dst->R13=Src->R13;
    Dst->R14=Src->R14;
    Dst->R15=Src->R15;
    /* Don't worry about user modifying INTNUM. If he or she did that it will corrupt userspace */
    Dst->INT_NUM=Src->INT_NUM;
    Dst->ERROR_CODE=Src->ERROR_CODE;
    /* This will always be canonical upon SYSRET, because we will truncate in on return */
    Dst->RIP=Src->RIP;
    Dst->CS=Src->CS;
    Dst->RFLAGS=Src->RFLAGS;
    Dst->RSP=Src->RSP;
    Dst->SS=Src->SS;
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
    Ret->RIP=Reg->RIP;
    Ret->RSP=Reg->RSP;
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
    Reg->RIP=Ret->RIP;
    Reg->RSP=Ret->RSP;
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
    Reg->RDI=(rme_ptr_t)Retval;
}
/* End Function:__RME_Set_Inv_Retval *****************************************/

void NDBG(void)
{
    write_string( 0x07, "Here", 0);
}

/* Crap for test */
void write_string( int colour, const char *string, rme_ptr_t pos)
{
    volatile char *video = (volatile char*)RME_X64_PA2VA(pos+0xB8000);
    while( *string != 0 )
    {
        *video++ = *string++;
        *video++ = colour;
    }
}

/* Begin Function:__RME_Kern_Func_Handler *************************************
Description : Handle kernel function calls.
Input       : struct RME_Cap_Captbl* Captbl - The current capability table.
              struct RME_Reg_Struct* Reg - The current register set.
              rme_ptr_t Func_ID - The function ID.
              rme_ptr_t Sub_ID - The sub function ID.
              rme_ptr_t Param1 - The first parameter.
              rme_ptr_t Param2 - The second parameter.
Output      : None.
Return      : rme_ret_t - The value that the function returned.
******************************************************************************/
rme_ret_t __RME_Kern_Func_Handler(struct RME_Cap_Captbl* Captbl, struct RME_Reg_Struct* Reg,
                                  rme_ptr_t Func_ID, rme_ptr_t Sub_ID, rme_ptr_t Param1, rme_ptr_t Param2)
{
    /* Now always call the HALT */
    char String[16];

    String[0]=Param1/10000000+'0';
    String[1]=(Param1/1000000)%10+'0';
    String[2]=(Param1/100000)%10+'0';
    String[3]=(Param1/10000)%10+'0';
    String[4]=(Param1/1000)%10+'0';
    String[5]=(Param1/100)%10+'0';
    String[6]=(Param1/10)%10+'0';
    String[7]=(Param1)%10+'0';
    String[8]='\0';
    write_string(Func_ID, (const char *)String, Sub_ID);

    //__RME_X64_Halt();
    return 0;
}
/* End Function:__RME_Kern_Func_Handler **************************************/

/* Begin Function:__RME_X64_Fault_Handler *************************************
Description : The fault handler of RME. In x64, this is used to handle multiple
              faults.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t Reason - The fault source.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_X64_Fault_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Reason)
{
    /* Not handling faults */
    RME_PRINTK_S("\n\r\n\r*** Fault: ");RME_PRINTK_I(Reason);RME_PRINTK_S(" - ");
    /* When handling debug exceptions, note CVE 2018-8897, we may get something at
     * kernel level - If this is what we have, the user must have touched SS + INT */
    /* Print reason */
    switch(Reason)
    {
        case RME_X64_FAULT_DE:RME_PRINTK_S("Divide error");break;
        case RME_X64_TRAP_DB:RME_PRINTK_S("Debug exception");break;
        case RME_X64_INT_NMI:RME_PRINTK_S("NMI error");break;
        case RME_X64_TRAP_BP:RME_PRINTK_S("Debug breakpoint");break;
        case RME_X64_TRAP_OF:RME_PRINTK_S("Overflow exception");break;
        case RME_X64_FAULT_BR:RME_PRINTK_S("Bound range exception");break;
        case RME_X64_FAULT_UD:RME_PRINTK_S("Undefined instruction");break;
        case RME_X64_FAULT_NM:RME_PRINTK_S("Device not available");break;
        case RME_X64_ABORT_DF:RME_PRINTK_S("Double(nested) fault exception");break;
        case RME_X64_ABORT_OLD_MF:RME_PRINTK_S("Coprocessor overrun - not used later on");break;
        case RME_X64_FAULT_TS:RME_PRINTK_S("Invalid TSS exception");break;
        case RME_X64_FAULT_NP:RME_PRINTK_S("Segment not present");break;
        case RME_X64_FAULT_SS:RME_PRINTK_S("Stack fault exception");break;
        case RME_X64_FAULT_GP:RME_PRINTK_S("General protection exception");break;
        case RME_X64_FAULT_PF:RME_PRINTK_S("Page fault exception");break;
        case RME_X64_FAULT_MF:RME_PRINTK_S("X87 FPU floating-point error:");break;
        case RME_X64_FAULT_AC:RME_PRINTK_S("Alignment check exception");break;
        case RME_X64_ABORT_MC:RME_PRINTK_S("Machine check exception");break;
        case RME_X64_FAULT_XM:RME_PRINTK_S("SIMD floating-point exception");break;
        case RME_X64_FAULT_VE:RME_PRINTK_S("Virtualization exception");break;
        default:RME_PRINTK_S("Unknown exception");break;
    }
    /* Print all registers */
    RME_PRINTK_S("\n\rRAX:        0x");RME_PRINTK_U(Reg->RAX);
    RME_PRINTK_S("\n\rRBX:        0x");RME_PRINTK_U(Reg->RBX);
    RME_PRINTK_S("\n\rRCX:        0x");RME_PRINTK_U(Reg->RCX);
    RME_PRINTK_S("\n\rRDX:        0x");RME_PRINTK_U(Reg->RDX);
    RME_PRINTK_S("\n\rRSI:        0x");RME_PRINTK_U(Reg->RSI);
    RME_PRINTK_S("\n\rRDI:        0x");RME_PRINTK_U(Reg->RDI);
    RME_PRINTK_S("\n\rRBP:        0x");RME_PRINTK_U(Reg->RBP);
    RME_PRINTK_S("\n\rR8:         0x");RME_PRINTK_U(Reg->R8);
    RME_PRINTK_S("\n\rR9:         0x");RME_PRINTK_U(Reg->R9);
    RME_PRINTK_S("\n\rR10:        0x");RME_PRINTK_U(Reg->R10);
    RME_PRINTK_S("\n\rR11:        0x");RME_PRINTK_U(Reg->R11);
    RME_PRINTK_S("\n\rR12:        0x");RME_PRINTK_U(Reg->R12);
    RME_PRINTK_S("\n\rR13:        0x");RME_PRINTK_U(Reg->R13);
    RME_PRINTK_S("\n\rR14:        0x");RME_PRINTK_U(Reg->R14);
    RME_PRINTK_S("\n\rR15:        0x");RME_PRINTK_U(Reg->R15);
    RME_PRINTK_S("\n\rINT_NUM:    0x");RME_PRINTK_U(Reg->INT_NUM);
    RME_PRINTK_S("\n\rERROR_CODE: 0x");RME_PRINTK_U(Reg->ERROR_CODE);
    RME_PRINTK_S("\n\rRIP:        0x");RME_PRINTK_U(Reg->RIP);
    RME_PRINTK_S("\n\rCS:         0x");RME_PRINTK_U(Reg->CS);
    RME_PRINTK_S("\n\rRFLAGS:     0x");RME_PRINTK_U(Reg->RFLAGS);
    RME_PRINTK_S("\n\rRSP:        0x");RME_PRINTK_U(Reg->RSP);
    RME_PRINTK_S("\n\rSS:         0x");RME_PRINTK_U(Reg->SS);
    RME_PRINTK_S("\n\rHang");

    while(1);
}
/* End Function:__RME_X64_Fault_Handler **************************************/

/* Begin Function:__RME_X64_Generic_Handler ***********************************
Description : The generic interrupt handler of RME for x64.
Input       : struct RME_Reg_Struct* Reg - The register set when entering the handler.
              rme_ptr_t Int_Num - The interrupt number.
Output      : struct RME_Reg_Struct* Reg - The register set when exiting the handler.
Return      : None.
******************************************************************************/
void __RME_X64_Generic_Handler(struct RME_Reg_Struct* Reg, rme_ptr_t Int_Num)
{
    /* Not handling interrupts */
    RME_PRINTK_S("\r\nGeneral int:");
    RME_PRINTK_I(Int_Num);

    switch(Int_Num)
    {
        /* Is this a generic IPI from other processors? */

        default:break;
    }
    /* Remember to perform context switch after any kernel sends */
}
/* End Function:__RME_X64_Generic_Handler ************************************/

/* Begin Function:__RME_Pgtbl_Set *********************************************
Description : Set the processor's page table.
Input       : rme_ptr_t Pgtbl - The virtual address of the page table.
Output      : None.
Return      : None.
******************************************************************************/
void __RME_Pgtbl_Set(rme_ptr_t Pgtbl)
{
    __RME_X64_Pgtbl_Set(RME_X64_VA2PA(Pgtbl)|RME_X64_PGREG_POS(Pgtbl).PCID);
}
/* End Function:__RME_Pgtbl_Set **********************************************/

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
    /* Is the table address aligned to 4kB? */
    if((Vaddr&0xFFF)!=0)
        return RME_ERR_PGT_OPFAIL;

    /* Is the size order allowed? */
    if((Size_Order!=RME_PGTBL_SIZE_512G)&&(Size_Order!=RME_PGTBL_SIZE_1G)&&
       (Size_Order!=RME_PGTBL_SIZE_2M)&&(Size_Order!=RME_PGTBL_SIZE_4K))
        return RME_ERR_PGT_OPFAIL;

    /* Is the top-level relationship correct? */
    if(((Size_Order==RME_PGTBL_SIZE_512G)^(Top_Flag!=0))!=0)
        return RME_ERR_PGT_OPFAIL;

    /* Is the number order allowed? */
    if(Num_Order!=RME_PGTBL_NUM_512)
        return RME_ERR_PGT_OPFAIL;

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

    /* Hopefully the compiler optimize this to rep stos */
    for(Count=0;Count<256;Count++)
        Ptr[Count]=0;

    /* Hopefully the compiler optimize this to rep movs */
    if((Pgtbl_Op->Base_Addr&RME_PGTBL_TOP)!=0)
    {
        for(;Count<512;Count++)
            Ptr[Count]=RME_X64_Kpgt.PML4[Count-256];

        RME_X64_PGREG_POS(Ptr).PCID=RME_FETCH_ADD((rme_ptr_t*)&RME_X64_PCID_Inc,1)&0xFFF;
    }
    else
    {
        for(;Count<512;Count++)
            Ptr[Count]=0;
    }

    /* Initialize its pgreg table to all zeros */
    RME_X64_PGREG_POS(Ptr).Parent_Cnt=0;
    RME_X64_PGREG_POS(Ptr).Child_Cnt=0;

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

    /* Check if it is mapped into other page tables. If yes, then it cannot be deleted.
     * also, it must not contain mappings of lower levels, or it is not deletable. */
    if((RME_X64_PGREG_POS(Table).Parent_Cnt==0)&&(RME_X64_PGREG_POS(Table).Child_Cnt==0))
        return 0;

    return RME_ERR_PGT_OPFAIL;
}
/* End Function:__RME_Pgtbl_Del_Check ****************************************/

/* Begin Function:__RME_Pgtbl_Page_Map ****************************************
Description : Map a page into the page table. This architecture requires that the mapping is
              always at least readable.
Input       : struct RME_Cap_Pgtbl* - The cap ability to the page table to operate on.
              rme_ptr_t Paddr - The physical address to map to. If we are unmapping, this have no effect.
              rme_ptr_t Pos - The position in the page table.
              rme_ptr_t Flags - The RME standard page attributes. Need to translate them into
                                architecture specific page table's settings.
Output      : None.
Return      : rme_ptr_t - If successful, 0; else RME_ERR_PGT_OPFAIL.
******************************************************************************/
rme_ptr_t __RME_Pgtbl_Page_Map(struct RME_Cap_Pgtbl* Pgtbl_Op, rme_ptr_t Paddr, rme_ptr_t Pos, rme_ptr_t Flags)
{
    rme_ptr_t* Table;
    rme_ptr_t X64_Flags;

    /* It should at least be readable */
    if((Flags&RME_PGTBL_READ)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we trying to map into the kernel space on the top level? */
    if(((Pgtbl_Op->Base_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=256))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Generate flags */
    if(RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)==RME_PGTBL_SIZE_4K)
        X64_Flags=RME_X64_MMU_ADDR(Paddr)|RME_X64_PGFLG_RME2NAT(Flags)|RME_X64_MMU_US;
    else
        X64_Flags=RME_X64_MMU_ADDR(Paddr)|RME_X64_PGFLG_RME2NAT(Flags)|RME_X64_MMU_PDE_SUP|RME_X64_MMU_US;

    /* Try to map it in */
    if(RME_COMP_SWAP(&(Table[Pos]),0,X64_Flags)==0)
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
    if(((Pgtbl_Op->Base_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=256))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Make sure that there is something */
    Temp=Table[Pos];
    if(Temp==0)
        return RME_ERR_PGT_OPFAIL;

    /* Is this a page directory? We cannot unmap page directories like this */
    if((RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)!=RME_PGTBL_SIZE_4K)&&((Temp&RME_X64_MMU_PDE_SUP)==0))
        return RME_ERR_PGT_OPFAIL;

    /* Try to unmap it. Use CAS just in case */
    if(RME_COMP_SWAP(&(Table[Pos]),Temp,0)==0)
        return RME_ERR_PGT_OPFAIL;

    return 0;
}
/* End Function:__RME_Pgtbl_Page_Unmap ***************************************/

/* Begin Function:__RME_Pgtbl_Pgdir_Map ***************************************
Description : Map a page directory into the page table.
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
    rme_ptr_t X64_Flags;

    /* It should at least be readable */
    if((Flags&RME_PGTBL_READ)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we trying to map into the kernel space on the top level? */
    if(((Pgtbl_Parent->Base_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=256))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Parent_Table=RME_CAP_GETOBJ(Pgtbl_Parent,rme_ptr_t*);
    Child_Table=RME_CAP_GETOBJ(Pgtbl_Child,rme_ptr_t*);

    /* Generate the content */
    X64_Flags=RME_X64_MMU_ADDR(RME_X64_VA2PA(Child_Table))|RME_X64_PGFLG_RME2NAT(Flags)|RME_X64_MMU_US;

    /* Try to map it in - may need to increase some count */
    if(RME_COMP_SWAP(&(Parent_Table[Pos]),0,X64_Flags)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Map complete, increase reference count for both page tables */
    RME_FETCH_ADD((rme_ptr_t*)&(RME_X64_PGREG_POS(Child_Table).Parent_Cnt),1);
    RME_FETCH_ADD((rme_ptr_t*)&(RME_X64_PGREG_POS(Parent_Table).Child_Cnt),1);

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
    rme_ptr_t* Parent_Table;
    rme_ptr_t* Child_Table;
    rme_ptr_t Temp;
    rme_ptr_t 

    /* Are we trying to unmap the kernel space on the top level? */
    if(((Pgtbl_Op->Base_Addr&RME_PGTBL_TOP)!=0)&&(Pos>=256))
        return RME_ERR_PGT_OPFAIL;

    /* Get the table */
    Parent_Table=RME_CAP_GETOBJ(Pgtbl_Op,rme_ptr_t*);

    /* Make sure that there is something */
    Temp=Parent_Table[Pos];
    if(Temp==0)
        return RME_ERR_PGT_OPFAIL;

    /* Is this a page? We cannot unmap pages like this */
    if((RME_PGTBL_SIZEORD(Pgtbl_Op->Size_Num_Order)==RME_PGTBL_SIZE_4K)||((Temp&RME_X64_MMU_PDE_SUP)!=0))
        return RME_ERR_PGT_OPFAIL;

    /* Is this child table mapped here? - check that in the future */

    Child_Table=(rme_ptr_t*)Temp;
    /* Try to unmap it. Use CAS just in case */
    if(RME_COMP_SWAP(&(Parent_Table[Pos]),Temp,0)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Decrease reference count */
    RME_FETCH_ADD((rme_ptr_t*)&(RME_X64_PGREG_POS(Child_Table).Parent_Cnt),-1);
    RME_FETCH_ADD((rme_ptr_t*)&(RME_X64_PGREG_POS(Parent_Table).Child_Cnt),-1);

    return 0;
}
/* End Function:__RME_Pgtbl_Pgdir_Unmap **************************************/

/* Begin Function:__RME_Pgtbl_Lookup ********************************************
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
        if((Temp&RME_X64_MMU_P)==0)
            return RME_ERR_PGT_OPFAIL;
    }
    else
    {
        if(((Temp&RME_X64_MMU_P)==0)||((Temp&RME_X64_MMU_PDE_SUP)==0))
            return RME_ERR_PGT_OPFAIL;
    }

    /* This is a page. Return the physical address and flags */
    if(Paddr!=0)
        *Paddr=RME_X64_MMU_ADDR(Temp);

    if(Flags!=0)
        *Flags=RME_X64_PGFLG_NAT2RME(Temp);

    return 0;
}
/* End Function:__RME_Pgtbl_Lookup *******************************************/

/* Begin Function:__RME_Pgtbl_Walk ********************************************
Description : Walking function for the page table. This function just does page
              table lookups. The page table that is being walked must be the top-
              level page table. The output values are optional; only pass in pointers
              when you need that value.
              Walking kernel page tables is prohibited.
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
    rme_ptr_t Size_Cnt;
    /* Accumulates the flag information about each level - these bits are ANDed */
    rme_ptr_t Flags_Accum;
    /* No execute bit - this bit is ORed */
    rme_ptr_t No_Execute;

    /* Check if this is the top-level page table */
    if(((Pgtbl_Op->Base_Addr)&RME_PGTBL_TOP)==0)
        return RME_ERR_PGT_OPFAIL;

    /* Are we attempting a kernel or non-canonical lookup? If yes, stop immediately */
    if(Vaddr>=0x7FFFFFFFFFFFULL)
        return RME_ERR_PGT_OPFAIL;

    /* Get the table and start lookup */
    Table=RME_CAP_GETOBJ(Pgtbl_Op, rme_ptr_t*);

    /* Do lookup recursively */
    Size_Cnt=RME_PGTBL_SIZE_512G;
    Flags_Accum=0xFFF;
    No_Execute=0;
    while(1)
    {
        /* Calculate where is the entry - always 0 to 512*/
        Pos=(Vaddr>>Size_Cnt)&0x1FF;
        /* Atomic read */
        Temp=Table[Pos];
        /* Find the position of the entry - Is there a page, a directory, or nothing? */
        if((Temp&RME_X64_MMU_P)==0)
            return RME_ERR_PGT_OPFAIL;
        if(((Temp&RME_X64_MMU_PDE_SUP)!=0)||(Size_Cnt==RME_PGTBL_SIZE_4K))
        {
            /* This is a page - we found it */
            if(Pgtbl!=0)
                *Pgtbl=(rme_ptr_t)Table;
            if(Map_Vaddr!=0)
                *Map_Vaddr=RME_ROUND_DOWN(Vaddr,Size_Cnt);
            if(Paddr!=0)
                *Paddr=RME_X64_MMU_ADDR(Temp);
            if(Size_Order!=0)
                *Size_Order=Size_Cnt;
            if(Num_Order!=0)
                *Num_Order=RME_PGTBL_NUM_512;
            if(Flags!=0)
                *Flags=RME_X64_PGFLG_NAT2RME(No_Execute|(Temp&Flags_Accum));

            break;
        }
        else
        {
            /* This is a directory, we goto that directory to continue walking */
            Flags_Accum&=Temp;
            No_Execute|=Temp&RME_X64_MMU_NX;
            Table=(rme_ptr_t*)RME_X64_PA2VA(RME_X64_MMU_ADDR(Temp));
        }

        /* The size order always decreases by 512 */
        Size_Cnt-=RME_PGTBL_SIZE_512B;
    }

    return 0;
}
/* End Function:__RME_Pgtbl_Walk *********************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
