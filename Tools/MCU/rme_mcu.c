/******************************************************************************
Filename    : rme_mcu.c
Author      : pry
Date        : 20/04/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The configuration generator for the MCU ports. This does not
              apply to the desktop or mainframe port; it uses its own generator.
			  This generator includes 12 big steps, and is considerably complex.
               1. Process the command line arguments and figure out where the source
                  are located at.
               2. Read the project-level configuration XMLs and device-level 
                  configuration XMLs into its internal data structures. Should
                  we find any parsing errors, we report and error out.
               3. Align memory. For program memory and data memory, rounding their
                  size is allowed; for specifically pointed out memory, rounding
                  their size is not allowed. 
               4. Generate memory map. This places all the memory segments into
                  the memory map, and fixes their specific size, etc. 
               5. Check if the generated memory map is valid. Each process should have
                  at least one code section and one data section, and they shall all
                  be STATIC.
               6. Allocate local and global linear capability IDs for all kernel objects.
                  The global linear capability ID assumes that all capability in the
                  same class are in the same capability table, and in 32-bit systems
                  this may not be the case.
               7. Set up the folder structure of the project so that the port-specific
                  generators can directly use them.
               8. Call the port-level generator to generate the project and port-specific
                  files for the project.
                  1. Detect any errors in the configuration structure. If any is found,
                     error out.
                  2. Allocate the page table contents and allocate capid/macros for them.
                  3. Call the tool-level project generator to generate project files.
                     Should the tool have any project group or workspace creation capability,
                     create the project group or workspace.
                     Memory map and linker file is also generated in this phase. 
                     1. The generator should generate separate projects for the RME.
                     2. Then generates project for RVM. 
                     3. And generates project for all other processes.
               9. Generate the vector creation scripts for RME.
              10. Generate the kernel object creation and delegation scripts for RVM.
                  1. Generate the capability tables.
                  2. Generate the page tables, calls the port-specific generator callback.
                  3. Generate the processes.
                  4. Generate the threads.
                  5. Generate the invocations.
                  6. Generate the receive endpoints.
                  7. Generate the delegation scripts.
              11. Generate stubs for all processes.
              12. Report to the user that the project generation is complete.
******************************************************************************/

/* Includes ******************************************************************/
/* Kill CRT warnings for MS. This also relies on Shlwapi.lib, remember to add it */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdio.h"
#include "memory.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "sys/types.h"
#include "sys/stat.h"

#include "xml.h"
#include "pbfs.h"

#if(defined _MSC_VER)
#include "Windows.h"
#include "shlwapi.h"
#elif(defined linux)
#include <dirent.h>
#include <errno.h>
#else
#error "The target platform is not supported. Please compile on Windows or Linux."
#endif
/* End Includes **************************************************************/

/* Defines *******************************************************************/
/* Power of 2 macros */
#define ALIGN_POW(X,POW)    (((X)>>(POW))<<(POW))
#define POW2(POW)           (((ptr_t)1)<<(POW))
/* Optimization levels */
#define OPT_O0              (0)
#define OPT_O1              (1)
#define OPT_O2              (2)
#define OPT_O3              (3)
#define OPT_OS              (4)
/* Time or size optimization choice */
#define PRIO_SIZE           (0)
#define PRIO_TIME           (1)
/* Capability ID placement */
#define AUTO                ((ptr_t)(-1LL))
#define INVALID             ((ptr_t)(-2LL))
/* Recovery options */
#define RECOVERY_THD        (0)
#define RECOVERY_PROC       (1)
#define RECOVERY_SYS        (2)
/* Memory access permissions */
#define MEM_READ            POW2(0)
#define MEM_WRITE           POW2(1)
#define MEM_EXECUTE         POW2(2)
#define MEM_BUFFERABLE      POW2(3)
#define MEM_CACHEABLE       POW2(4)
#define MEM_STATIC          POW2(5)
/* Option types */
#define OPTION_RANGE        (0)
#define OPTION_SELECT       (1)
/* Failure reporting macros */
#define EXIT_FAIL(Reason) \
do \
{ \
    printf(Reason); \
    printf("\n\n"); \
    Free_All(); \
    exit(-1); \
} \
while(0)

/* The alignment value used when printing macros */
#define MACRO_ALIGNMENT     (56)
/* The code generator author name */
#define CODE_AUTHOR         ("The A7M project generator.")
/* End Defines ***************************************************************/

/* Typedefs ******************************************************************/
typedef char s8_t;
typedef short s16_t;
typedef int s32_t;
typedef long long s64_t;
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;
/* Make things compatible in 32-bit or 64-bit environments */
typedef s64_t ret_t;
typedef u64_t ptr_t;
/* End Typedefs **************************************************************/

/* Structs *******************************************************************/
/* List head structure */
struct List
{
	struct List* Prev;
	struct List* Next;
};

/* Platform agnostic structures */
/* Compiler information */
struct Comp_Info
{
    /* Optimization level */
	ptr_t Opt;
    /* Priority */
	ptr_t Prio;
};

/* Raw information to be fed to the platform-specific parser */
struct Raw_Info
{
	struct List Head;
    /* Tags */
	s8_t* Tag;
    /* Value of tags */
	s8_t* Val;
};

/* RME kernel information */
struct RME_Info
{
    /* Compiler information */
	struct Comp_Info Comp;
    /* RME code section start address */
	ptr_t Code_Start;
    /* RME code section size */
	ptr_t Code_Size;
    /* RME data section start address */
	ptr_t Data_Start;
    /* RME data section size */
	ptr_t Data_Size;
    /* Extra amount of kernel memory */
	ptr_t Extra_Kmem;
    /* Slot order of kernel memory */
	ptr_t Kmem_Order;
    /* Priorities supported */
	ptr_t Kern_Prios;
    /* Raw information about platform, to be deal with by the platform-specific generator */
	struct List Plat;
    /* Raw information about chip, to be deal with by the platform-specific generator */
	struct List Chip;
};

/* RVM's capability information, from the user processes */
struct RVM_Cap_Info
{
    /* What process is this capability in? */
    struct Proc_Info* Proc;
    /* What's the content of the capability, exactly? */
    struct List* Cap;
};

/* RVM user-level library information. */
struct RVM_Info
{
    /* Compiler information */
	struct Comp_Info Comp;
    /* Size of the code section. This always immediately follow the code section of RME. */
    ptr_t Code_Size;
    /* Size of the data section. This always immediately follow the data section of RME. */
    ptr_t Data_Size;
    /* The extra amount in the main capability table */
	ptr_t Extra_Captbl;
    /* The recovery mode - by thread, process or the whole system? */
	ptr_t Recovery;

    /* Global captbl containing captbls */
    ptr_t Captbl_Front;
    struct List Captbl;
    /* Global captbl containing processes */
    ptr_t Proc_Front;
    struct List Proc;
    /* Global captbl containing threads */
    ptr_t Thd_Front;
    struct List Thd;
    /* Global captbl containing invocations */
    ptr_t Inv_Front;
    struct List Inv;
    /* Global captbl containing receive endpoints */
    ptr_t Recv_Front;
    struct List Recv;
    /* Global captbl containing kernel endpoints - actually created by kernel itself */
    ptr_t Vect_Front;
    struct List Vect;
};

/* Memory segment information */
struct Mem_Info
{
    struct List Head;
    /* The start address */
	ptr_t Start;
    /* The size */
	ptr_t Size;
    /* The attributes - read, write, execute, cacheable, bufferable, static */
	ptr_t Attr;
    /* The alignment granularity */
    ptr_t Align;
};

/* Capability information - not all fields used for every capability */
struct Cap_Info
{
    /* The local capid of the port */
    ptr_t Loc_Capid;
    /* The global linear capid of the endpoint */
    ptr_t RVM_Capid;
    /* The macro denoting the global capid */
    s8_t* Loc_Macro;
    /* The macro denoting the global capid */
    s8_t* RVM_Macro;
    /* The macro denoting the global capid - for RME */
    s8_t* RME_Macro;
};

/* Port-specific stack initialization routine parameters */
struct Plat_Stack
{
    /* Address of the entry - including ths stub, etc */
    ptr_t Entry_Addr;
    /* Value of the parameter at creation time */
    ptr_t Param_Value;
    /* Port-specific stack initialization parameter */
    ptr_t Stack_Init_Param;
    /* Port-specific stack initialization address */
    ptr_t Stack_Init_Addr;
};

/* Thread information */
struct Thd_Info
{
    struct List Head;
    /* Name of the thread, unique in a process */
	s8_t* Name;
    /* The entry of the thread */
	s8_t* Entry;
    /* The stack address of the thread */
	ptr_t Stack_Addr;
    /* The stack size of the thread */
	ptr_t Stack_Size;
    /* The parameter passed to the thread */
	s8_t* Parameter;
    /* The priority of the thread */
	ptr_t Priority;

    /* Capability related information */
    struct Cap_Info Cap;
    /* Platform-specific initialization parameters */
    struct Plat_Stack Plat;
};

/* Invocation information */
struct Inv_Info
{
    struct List Head;
    /* The name of the invocation, unique in a process */
	s8_t* Name;
    /* The entry address of the invocation */
	s8_t* Entry;
    /* The stack address of the invocation */
	ptr_t Stack_Addr;
    /* The stack size of the invocation */
	ptr_t Stack_Size;

    /* Capability related information */
    struct Cap_Info Cap;
    /* Port-specific initialization parameters */
    struct Plat_Stack Port;
};

/* Port information */
struct Port_Info
{
    struct List Head;
    /* The name of the port, unique in a process, and must have
     * a corresponding invocation in the process designated. */
	s8_t* Name;
    /* The process's name */
    s8_t* Proc_Name;

    /* Capability related information */
    struct Cap_Info Cap;
};

/* Receive endpoint information */
struct Recv_Info
{
    struct List Head;
    /* The name of the receive endpoint, unique in a process */
	s8_t* Name;

    /* Capability related information */
    struct Cap_Info Cap;
};

/* Send endpoint information */
struct Send_Info
{
    struct List Head;
    /* The name of the send endpoint, unique in a process, and must 
     * have a corresponding receive endpoint in the process designated. */
	s8_t* Name;
    /* The process's name, only useful for send endpoints */
    s8_t* Proc_Name;

    /* Capability related information */
    struct Cap_Info Cap;
};

/* Vector endpoint information */
struct Vect_Info
{
    struct List Head;
    /* Globally unique vector name */
	s8_t* Name;
    /* Vector number */
    ptr_t Vect_Num;

    /* Capability related information */
    struct Cap_Info Cap;
};

/* Process information */
struct Proc_Info
{
    struct List Head;
    /* Name of the process */
    s8_t* Name;
    /* Extra first level captbl capacity required */
	ptr_t Extra_Captbl;
    /* Current local capability table frontier */ 
    ptr_t Captbl_Front;
    /* Compiler information */
	struct Comp_Info Comp;
    /* Memory trunk information */
	struct List Code;
	struct List Data;
	struct List Device;
    /* Kernel object information */
	struct List Thd;
	struct List Inv;
	struct List Port;
	struct List Recv;
	struct List Send;
	struct List Vect;
    /* Capability information for itself */
    struct Cap_Info Captbl;
    struct Cap_Info Pgtbl;
    struct Cap_Info Proc;
};

/* Whole project information */
struct Proj_Info
{
    /* The name of the project */
	s8_t* Name;
    /* The platform used */
    s8_t* Plat;
    /* The chip class used */
	s8_t* Chip_Class;
    /* The full name of the exact chip used */
    s8_t* Chip_Full;
    /* The RME kernel information */
	struct RME_Info RME;
    /* The RVM user-library information */
	struct RVM_Info RVM;
    /* The process information */
	struct List Proc;
};

/* The option information */
struct Chip_Option_Info
{
    struct List Head;
    /* Name*/
    s8_t* Name;
    /* Type of the option, either range or select */
    ptr_t Type;
    /* Macro of the option */
    s8_t* Macro;
    /* Range of the option */
    s8_t* Range;
};

/* Vector informations */
struct Chip_Vect_Info
{
    struct List Head;
    /* The name of the vector */
	s8_t* Name;
    /* The vector number */
	ptr_t Num;
};

/* Chip information - this is platform independent as well */
struct Chip_Info
{
    /* The name of the chip class */
	s8_t* Class;
    /* Compatible chip list */
	s8_t* Compat;
    /* The vendor */
    s8_t* Vendor;
    /* The platform */
	s8_t* Plat;
    /* The number of CPU cores */
	ptr_t Cores;
    /* The number of MPU regions */
    ptr_t Regions;
    /* The platform-specific attributes to be passed to the platform-specific generator */
    struct List Attr;
    /* Memory information */
	struct List Code;
	struct List Data;
	struct List Device;
    /* Raw option information */
	struct List Option;
    /* Interrupt vector information */
	struct List Vect;
};

/* Memory map */
struct Mem_Map_Info
{
    struct List Head;
    /* The memory information itself */
    struct Mem_Info* Mem;
    /* The bitmap of the memory trunk, aligned on 32-byte boundary */
    u8_t* Bitmap;
};

/* Memory map information - min granularity 4B */
struct Mem_Map
{
    struct List Info;
    /* The exact list of these unallocated requirements */
    struct List Auto_Mem;
};

/* The capability and kernel memory information supplied by the port-specific generator */
struct Cap_Alloc_Info
{
    /* Processor bits */
    ptr_t Processor_Bits;
    /* Kernel memory base */
    ptr_t Kmem_Abs_Base;
    /* When we go into creating the kernel endpoints */
    ptr_t Cap_Vect_Front;
    ptr_t Kmem_Vect_Front;
    /* When we go into creating capability tables */
    ptr_t Cap_Captbl_Front;
    ptr_t Kmem_Captbl_Front;
    /* When we go into creating page tables */
    ptr_t Cap_Pgtbl_Front;
    ptr_t Kmem_Pgtbl_Front;
    /* When we go into creating processes */
    ptr_t Cap_Proc_Front;
    ptr_t Kmem_Proc_Front;
    /* When we go into creating threads */
    ptr_t Cap_Thd_Front;
    ptr_t Kmem_Thd_Front;
    /* When we go into creating invocations */
    ptr_t Cap_Inv_Front;
    ptr_t Kmem_Inv_Front;
    /* When we go into creating receive endpoints */
    ptr_t Cap_Recv_Front;
    ptr_t Kmem_Recv_Front;
    /* After the booting all finishes */ 
    ptr_t Cap_Boot_Front;
    ptr_t Kmem_Boot_Front;
    /* Callbacks for generating the RVM page table part */
    void* Plat_Info;
    void (*Gen_RVM_Pgtbl_Macro)(FILE* File, struct Proj_Info* Proj, struct Chip_Info* Chip,
                                struct Cap_Alloc_Info* Alloc);
    void (*Gen_RVM_Pgtbl_Crt)(FILE* File, struct Proj_Info* Proj, struct Chip_Info* Chip,
                              struct Cap_Alloc_Info* Alloc);
    void (*Gen_RVM_Pgtbl_Init)(FILE* File, struct Proj_Info* Proj, struct Chip_Info* Chip,
                               struct Cap_Alloc_Info* Alloc);
};
/* End Structs ***************************************************************/

/* Global Variables **********************************************************/
/* The list containing all memory allocated */
struct List Mem_List;
/* End Global Variables ******************************************************/

/* Begin Function:List_Crt ****************************************************
Description : Create a doubly linkled list.
Input       : volatile struct List* Head - The pointer to the list head.
Output      : None.
Return      : None.
******************************************************************************/
void List_Crt(volatile struct List* Head)
{
    Head->Prev=(struct List*)Head;
    Head->Next=(struct List*)Head;
}
/* End Function:List_Crt *****************************************************/

/* Begin Function:List_Del ****************************************************
Description : Delete a node from the doubly-linked list.
Input       : volatile struct RMP_List* Prev - The prevoius node of the target node.
              volatile struct RMP_List* Next - The next node of the target node.
Output      : None.
Return      : None.
******************************************************************************/
void List_Del(volatile struct List* Prev,volatile struct List* Next)
{
    Next->Prev=(struct List*)Prev;
    Prev->Next=(struct List*)Next;
}
/* End Function:List_Del *****************************************************/

/* Begin Function:List_Ins ****************************************************
Description : Insert a node to the doubly-linked list.
Input       : volatile struct List* New - The new node to insert.
              volatile struct List* Prev - The previous node.
              volatile struct List* Next - The next node.
Output      : None.
Return      : None.
******************************************************************************/
void List_Ins(volatile struct List* New,
              volatile struct List* Prev,
              volatile struct List* Next)
{
    Next->Prev=(struct List*)New;
    New->Next=(struct List*)Next;
    New->Prev=(struct List*)Prev;
    Prev->Next=(struct List*)New;
}
/* End Function:List_Ins *****************************************************/

/* Begin Function:Free_All ****************************************************
Description : We encountered a failure somewhere and need to free all memory allocated
              so far.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void Free_All(void)
{
	struct List* Ptr;

	while(Mem_List.Next!=&Mem_List)
	{
		Ptr=Mem_List.Next;
		List_Del(Ptr->Prev,Ptr->Next);
		free(Ptr);
	}
}
/* End Function:Free_All *****************************************************/

/* Begin Function:Malloc ******************************************************
Description : Allocate some memory and register it with the system.
Input       : ptr_t Size - The size to allocate, in bytes.
Output      : None.
Return      : void* - If successful, the address; else 0.
******************************************************************************/
void* Malloc(ptr_t Size)
{
	struct List* Addr;

	/* See if the allocation is successful */
	Addr=malloc((size_t)(Size+sizeof(struct List)));
	if(Addr==0)
        EXIT_FAIL("Memory allocation failed.");

	/* Insert into the queue */
	List_Ins(Addr,&Mem_List,Mem_List.Next);

	return &Addr[1];
}
/* End Function:Malloc *******************************************************/

/* Begin Function:Free ********************************************************
Description : Deallocate the memory and deregister it.
Input       : void* Addr - The address to free.
Output      : None.
Return      : None.
******************************************************************************/
void Free(void* Addr)
{
	struct List* List;
	
	/* Get the memory block and deregister it in the queue */
	List=(struct List*)Addr;
	List=List-1;
	List_Del(List->Prev,List->Next);
	free(List);
}
/* End Function:Free *********************************************************/

/* Begin Function:Dir_Present *************************************************
Description : Figure out whether the directory is present.
Input       : s8_t* Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for present, -1 for non-present.
******************************************************************************/
ret_t Dir_Present(s8_t* Path)
{
#ifdef _MSC_VER
    u32_t Attr;
    Attr=PathIsDirectory(Path);
    if(Attr!=0)
        return 0;
    else
        return -1;
#else
    DIR* Dir;
    Dir=opendir(Path);
    if(Dir!=0)
    {
        closedir(Dir);
        return 0;
    }
    else
        return -1;
#endif
}
/* End Function:Dir_Present **************************************************/

/* Begin Function:Dir_Empty ***************************************************
Description : Figure out whether the directory is empty. When using this function,
              the directory must be present.
Input       : s8_t* Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for empty, -1 for non-empty.
******************************************************************************/
ret_t Dir_Empty(s8_t* Path)
{
#ifdef _MSC_VER
    u32_t Attr;
    Attr=PathIsDirectoryEmpty(Path);
    if(Attr!=0)
        return 0;
    else
        return -1;
#else
    ptr_t Num;
    DIR* Dir;

    Dir=opendir(Path);
    if(Dir==0)
        return -1;

    while(1)
    {
        if(readdir(Dir)==0)
            break;

        Num++;
        if(Num>2)
        {
            closedir(Dir);
            return -1;
        }
    }

    closedir(Dir);
    return 0;
#endif
}
/* End Function:Dir_Empty ****************************************************/

/* Begin Function:Get_Size ****************************************************
Description : Get the size of the file. The file is known to be present somewhere.
Input       : s8_t* Path - The path of the file.
Output      : None.
Return      : ptr_t - The size of the file.
******************************************************************************/
ptr_t Get_Size(s8_t* Path)
{
    struct stat Buf;
    if(stat(Path,&Buf)!=0)
        EXIT_FAIL("Windows/Linux stat failed.");
    return Buf.st_size;
}
/* End Function:Get_Size *****************************************************/

/* Begin Function:Make_Dir ****************************************************
Description : Create a directory if it does not exist.
Input       : s8_t* Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for successful, -1 for failure.
******************************************************************************/
ret_t Make_Dir(s8_t* Path)
{
    if(Dir_Present(Path)==0)
        return 0;

#ifdef _WIN32
    if(CreateDirectory(Path, NULL)!=0)
        return 0;
#else
    if(mkdir(Path, S_IRWXU)==0)
        return 0;
#endif

    return -1;
}
/* End Function:Make_Dir *****************************************************/

/* Begin Function:Copy_File ***************************************************
Description : Copy a file from some position to another position. If the file
              exists, we need to overwrite it with the new files.
Input       : s8_t* Dst - The destination path.
              s8_t* Src - The source path.
Output      : None.
Return      : ret_t - 0 for successful, -1 for failure.
******************************************************************************/
ret_t Copy_File(s8_t* Dst, s8_t* Src)
{
    FILE* Dst_File;
    FILE* Src_File;
    s8_t Buf[128];
    ptr_t Size;

    Src_File=fopen(Src, "rb");
    if(Src_File==0)
        return -1;
    /* This will wipe the contents of the file */
    Dst_File=fopen(Dst, "wb");
    if(Dst_File==0)
    {
        fclose(Src_File);
        return -1;
    }

    Size=fread(Buf, 1, 128, Src_File);
    while(Size!=0)
    {
        fwrite(Buf, 1, (size_t)Size, Dst_File);
        Size=fread(Buf, 1, 128, Src_File);
    }

    fclose(Src_File);
    fclose(Dst_File);

    return 0;
}
/* End Function:Copy_File ****************************************************/

/* Begin Function:Cmdline_Proc ************************************************
Description : Preprocess the input parameters, and generate a preprocessed
              instruction listing with all the comments stripped.
Input       : int argc - The number of arguments.
              char* argv[] - The arguments.
Output      : s8_t** Input_File - The input project file path.
              s8_t** Output_File - The output folder path, must be empty.
			  s8_t** RME_Path - The RME root folder path, must contain RME files.
			  s8_t** RVM_Path - The RME root folder path, must contain RME files.
			  s8_t** Format - The output format.
Return      : None.
******************************************************************************/
void Cmdline_Proc(int argc, char* argv[], s8_t** Input_File, s8_t** Output_Path,
                  s8_t** RME_Path, s8_t** RVM_Path, s8_t** Format)
{
    ptr_t Count;

    if(argc!=11)
        EXIT_FAIL("Too many or too few input parameters.\n"
                  "Usage: -i input.xml -o output_path -k rme_root -u rvm_root -f format.\n"
                  "       -i: Project description file name and path, with extension.\n"
                  "       -o: Output path, must be empty.\n"
                  "       -k: RME root path, must contain all necessary files.\n"
                  "       -u: RVM root path, must contain all necessary files.\n"
                  "       -f: Output file format.\n"
                  "           keil: Keil uVision IDE.\n"
                  "           eclipse: Eclipse IDE.\n"
                  "           makefile: Makefile project.\n");

	*Input_File=0;
	*Output_Path=0;
	*RME_Path=0;
	*RVM_Path=0;
	*Format=0;

    Count=1;
    /* Read the command line one by one */
    while(Count<(ptr_t)argc)
    {
        /* We need to open some input file */
        if(strcmp(argv[Count],"-i")==0)
        {
            if(*Input_File!=0)
                EXIT_FAIL("More than one input file.");

            *Input_File=argv[Count+1];

            Count+=2;
        }
        /* We need to check some output path. */
        else if(strcmp(argv[Count],"-o")==0)
        {
            if(*Output_Path!=0)
                EXIT_FAIL("More than one output path.");

            *Output_Path=argv[Count+1];
            if(Dir_Present(*Output_Path)!=0)
                EXIT_FAIL("Output path is not present.");
            if(Dir_Empty(*Output_Path)!=0)
                EXIT_FAIL("Output path is not empty.");

            Count+=2;
        }
        /* We need to check the RME root folder. */
        else if(strcmp(argv[Count],"-k")==0)
        {
            if(*RME_Path!=0)
                EXIT_FAIL("More than one RME root folder.");

            *RME_Path=argv[Count+1];
            if(Dir_Present(*RME_Path)!=0)
                EXIT_FAIL("RME root path is not present.");
            if(Dir_Empty(*RME_Path)==0)
                EXIT_FAIL("RME root path is empty, wrong path selected.");

            Count+=2;
        }
        /* We need to check the RVM root folder. */
        else if(strcmp(argv[Count],"-u")==0)
        {
            if(*RVM_Path!=0)
                EXIT_FAIL("More than one RVM root folder.");

            *RVM_Path=argv[Count+1];
            if(Dir_Present(*RVM_Path)!=0)
                EXIT_FAIL("RVM root path is not present.");
            if(Dir_Empty(*RVM_Path)==0)
                EXIT_FAIL("RVM root path is empty, wrong path selected.");

            Count+=2;
        }
        /* We need to set the format of the output project */
        else if(strcmp(argv[Count],"-f")==0)
        {
            if(*Format!=0)
                EXIT_FAIL("Conflicting output project format designated.");
            
            *Format=argv[Count+1];

            Count+=2;
        }
        else
            EXIT_FAIL("Unrecognized argument designated.");
    }

    if(*Input_File==0)
        EXIT_FAIL("No input file specified.");
    if(*Output_Path==0)
        EXIT_FAIL("No output path specified.");
    if(*RME_Path==0)
        EXIT_FAIL("No RME root path specified.");
    if(*RVM_Path==0)
        EXIT_FAIL("No RVM root path specified.");
    if(*Format==0)
        EXIT_FAIL("No output project type specified.");
}
/* End Function:Cmdline_Proc *************************************************/

/* Begin Function:Read_XML ****************************************************
Description : Read the content of the whole XML file into the buffer. This only
              reads the XML.
Input       : s8_t* Path - The path to the file.
Output      : None.
Return      : s8_t* - The buffer containing the file contents.
******************************************************************************/
s8_t* Read_XML(s8_t* Path)
{
	ptr_t Size;
	FILE* Handle;
	s8_t* Buf;
    
	Size=Get_Size(Path);

	Handle=fopen(Path,"r");
	if(Handle==0)
		EXIT_FAIL("Input file open failed.");


	Buf=Malloc(Size+1);
	fread(Buf, 1, (size_t)Size, Handle);

	fclose(Handle);
	Buf[Size]='\0';

	return Buf;
}
/* End Function:Read_XML *****************************************************/

/* Begin Function:Parse_Compiler **********************************************
Description : Parse the compiler section of the user-supplied project configuration file.
Input       : struct Comp_Info* Comp - The compiler structure.
              xml_node_t* Node - The compiler section's XML node.
Output      : struct Comp_Info* Comp - The updated compiler structure.
Return      : None.
******************************************************************************/
void Parse_Compiler(struct Comp_Info* Comp, xml_node_t* Node)
{
    xml_node_t* Temp;

    /* Optimization level */
    if((XML_Child(Node,"Optimization",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Compiler Optimization section missing.");

    if(Temp->XML_Val_Len!=2)
        EXIT_FAIL("The optimization option is malformed.");

    if(strncmp(Temp->XML_Val,"O0",2)==0)
        Comp->Opt=OPT_O0;
    else if(strncmp(Temp->XML_Val,"O1",2)==0)
        Comp->Opt=OPT_O1;
    else if(strncmp(Temp->XML_Val,"O2",2)==0)
        Comp->Opt=OPT_O2;
    else if(strncmp(Temp->XML_Val,"O3",2)==0)
        Comp->Opt=OPT_O3;
    else if(strncmp(Temp->XML_Val,"OS",2)==0)
        Comp->Opt=OPT_OS;
    else
        EXIT_FAIL("The optimization option is malformed.");

    /* Time or size optimization */
    if((XML_Child(Node,"Prioritization",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Compiler Prioritization section missing.");

    if(Temp->XML_Val_Len!=4)
        EXIT_FAIL("The optimization option is malformed.");

    if(strncmp(Temp->XML_Val,"Time",4)==0)
        Comp->Prio=PRIO_TIME;
    else if(strncmp(Temp->XML_Val,"Size",4)==0)
        Comp->Prio=PRIO_SIZE;
    else
        EXIT_FAIL("The prioritization option is malformed.");
}
/* End Function:Parse_Compiler ***********************************************/

/* Begin Function:Parse_Proj_RME **********************************************
Description : Parse the RME section of the user-supplied project configuration file.
Input       : struct RME_Info* RME - The RME information.
              xml_node_t* Node - The RME section's XML node.
Output      : struct RME_Info* RME - The updated RME information.
Return      : None.
******************************************************************************/
void Parse_Proj_RME(struct RME_Info* RME, xml_node_t* Node)
{
    xml_node_t* Compiler;
    xml_node_t* General;
    xml_node_t* Platform;
    xml_node_t* Chip;
    xml_node_t* Temp;
    struct Raw_Info* Raw;

    /* Compiler */
    if((XML_Child(Node,"Compiler",&Compiler)<0)||(Compiler==0))
        EXIT_FAIL("RME Complier section missing.");
    /* General */
    if((XML_Child(Node,"General",&General)<0)||(General==0))
        EXIT_FAIL("RME General section missing.");
    /* Platform */
    if((XML_Child(Node,"Platform",&Platform)<0)||(Platform==0))
        EXIT_FAIL("RME Platform section missing.");
    /* Chip */
    if((XML_Child(Node,"Chip",&Chip)<0)||(Chip==0))
        EXIT_FAIL("RME chip section missing.");

    /* Parse compiler section */
    Parse_Compiler(&(RME->Comp),Compiler);

    /* Parse general section */
    /* Code start address */
    if((XML_Child(General,"Code_Start",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Code_Start section missing.");
    if(XML_Get_Hex(Temp,&(RME->Code_Start))<0)
        EXIT_FAIL("RME General Code_Start is not a valid hex number.");
    /* Code size */
    if((XML_Child(General,"Code_Size",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Code_Size section missing.");
    if(XML_Get_Hex(Temp,&(RME->Code_Size))<0)
        EXIT_FAIL("RME General Code_Size is not a valid hex number.");
    /* Data start address */
    if((XML_Child(General,"Data_Start",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Data_Start section missing.");
    if(XML_Get_Hex(Temp,&(RME->Data_Start))<0)
        EXIT_FAIL("RME General Data_Start is not a valid hex number.");
    /* Data size */
    if((XML_Child(General,"Data_Size",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Data_Size section missing.");
    if(XML_Get_Hex(Temp,&(RME->Data_Size))<0)
        EXIT_FAIL("RME General Data_Size is not a valid hex number.");
    /* Extra Kmem */
    if((XML_Child(General,"Extra_Kmem",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Extra_Kmem section missing.");
    if(XML_Get_Hex(Temp,&(RME->Extra_Kmem))<0)
        EXIT_FAIL("RME General Extra_Kmem is not a valid hex number.");
    /* Kmem_Order */
    if((XML_Child(General,"Kmem_Order",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Kmem_Order section missing.");
    if(XML_Get_Uint(Temp,&(RME->Extra_Kmem))<0)
        EXIT_FAIL("RME General Kmem_Order is not a valid unsigned integer.");
    /* Priorities */
    if((XML_Child(General,"Kern_Prios",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Kern_Prios section missing.");
    if(XML_Get_Uint(Temp,&(RME->Kern_Prios))<0)
        EXIT_FAIL("RME General Kern_Prios is not a valid unsigned integer.");

    /* Now read the platform section */
    List_Crt(&(RME->Plat));
    if(XML_Child(Platform,0,&Temp)<0)
        EXIT_FAIL("Internal error.");
    while(Temp!=0)
    {
        Raw=Malloc(sizeof(struct Raw_Info));
        if(XML_Get_Tag(Temp,Malloc,&Raw->Tag)<0)
            EXIT_FAIL("RME Platform tag read failed.");
        if(XML_Get_Val(Temp,Malloc,&Raw->Val)<0)
            EXIT_FAIL("RME Platform value read failed.");
        
        List_Ins(&(Raw->Head),RME->Plat.Prev,&(RME->Plat));
        if(XML_Child(Platform,"",&Temp)<0)
            EXIT_FAIL("Internal error.");
    }

    /* Now read the chip section */
    List_Crt(&(RME->Chip));
    if(XML_Child(Chip,0,&Temp)<0)
        EXIT_FAIL("Internal error.");
    while(Temp!=0)
    {
        Raw=Malloc(sizeof(struct Raw_Info));
        if(XML_Get_Tag(Temp,Malloc,&Raw->Tag)<0)
            EXIT_FAIL("RME Chip tag read failed.");
        if(XML_Get_Val(Temp,Malloc,&Raw->Val)<0)
            EXIT_FAIL("RME Chip value read failed.");

        List_Ins(&(Raw->Head),RME->Chip.Prev,&(RME->Chip));
        if(XML_Child(Chip,"",&Temp)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proj_RME ***********************************************/

/* Begin Function:Parse_Proj_RVM **********************************************
Description : Parse the RVM section of the user-supplied project configuration file.
Input       : struct RVM_Info* RVM - The RVM information.
              xml_node_t* Node - The RVM section's XML node.
Output      : struct RVM_Info* RVM - The updated RVM information.
Return      : None.
******************************************************************************/
void Parse_Proj_RVM(struct RVM_Info* RVM, xml_node_t* Node)
{
    /* We don't parse RVM now as the functionality is not supported */
    xml_node_t* Compiler;
    xml_node_t* General;
    xml_node_t* VMM;
    xml_node_t* Temp;

    /* Compiler */
    if((XML_Child(Node,"Compiler",&Compiler)<0)||(Compiler==0))
        EXIT_FAIL("RVM Complier section missing.");
    /* General */
    if((XML_Child(Node,"General",&General)<0)||(General==0))
        EXIT_FAIL("RVM General section missing.");
    /* VMM */
    if((XML_Child(Node,"VMM",&VMM)<0)||(VMM==0))
        EXIT_FAIL("RVM VMM section missing.");

    /* Parse Compiler section */
    Parse_Compiler(&(RVM->Comp),Compiler);

    /* Now read the contents of the General section */
    /* Code size */
    if((XML_Child(General,"Code_Size",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Code_Size section missing.");
    if(XML_Get_Hex(Temp,&(RVM->Code_Size))<0)
        EXIT_FAIL("RME General Code_Size is not a valid hex number.");
    /* Data size */
    if((XML_Child(General,"Data_Size",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Data_Size section missing.");
    if(XML_Get_Hex(Temp,&(RVM->Data_Size))<0)
        EXIT_FAIL("RME General Data_Size is not a valid hex number.");
    /* Extra Captbl */
    if((XML_Child(General,"Extra_Captbl",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Extra_Captbl section missing.");
    if(XML_Get_Uint(Temp,&(RVM->Extra_Captbl))<0)
        EXIT_FAIL("RME General Extra_Captbl is not a valid unsigned integer.");
    /* Recovery */
    if((XML_Child(General,"Recovery",&Temp)<0)||(Temp==0))
        EXIT_FAIL("RME General Recovery section missing.");
    if((Temp->XML_Val_Len==6)&&(strncmp(Temp->XML_Val,"Thread",6)==0))
        RVM->Recovery=RECOVERY_THD;
    else if((Temp->XML_Val_Len==7)&&(strncmp(Temp->XML_Val,"Process",7)==0))
        RVM->Recovery=RECOVERY_PROC;
    else if((Temp->XML_Val_Len==6)&&(strncmp(Temp->XML_Val,"System",6)==0))
        RVM->Recovery=RECOVERY_SYS;
    else
        EXIT_FAIL("RME General Recovery option is malformed.");

    /* The VMM section is currently unused. We don't care about this now */
}
/* End Function:Parse_Proj_RVM ***********************************************/

/* Begin Function:Parse_Proc_Mem **********************************************
Description : Parse the memory section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The memory section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Mem(struct Proc_Info* Proc, xml_node_t* Node)
{
    s8_t* Attr_Temp;
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Mem_Info* Mem;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Code));
    List_Crt(&(Proc->Data));
    List_Crt(&(Proc->Device));

    while(Trunk!=0)
    {
        Mem=Malloc(sizeof(struct Mem_Info));

        /* Start */
        if((XML_Child(Trunk,"Start",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Memory Start section missing.");
        if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Auto",4)==0))
            Mem->Start=AUTO;
        else if(XML_Get_Hex(Temp,&(Mem->Start))<0)
            EXIT_FAIL("Process Memory Start is not a valid hex number.");

        /* Size */
        if((XML_Child(Trunk,"Size",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Memory Size section missing.");
        if(XML_Get_Hex(Temp,&(Mem->Size))<0)
            EXIT_FAIL("Process Memory Size is not a valid hex number.");
        if(Mem->Size==0)
            EXIT_FAIL("Process Memory Size cannot be zero.");
        if(Mem->Start!=AUTO)
        {
            if((Mem->Start+Mem->Size)>0x100000000ULL)
                EXIT_FAIL("Process Memory Size out of bound.");
        }

        /* Type */
        if((XML_Child(Trunk,"Type",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Memory Type section missing.");
        if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Code",4)==0))
            List_Ins(&(Mem->Head),Proc->Code.Prev,&(Proc->Code));
        else if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Data",4)==0))
            List_Ins(&(Mem->Head),Proc->Data.Prev,&(Proc->Data));
        else if((Temp->XML_Val_Len==6)&&(strncmp(Temp->XML_Val,"Device",6)==0))
            List_Ins(&(Mem->Head),Proc->Device.Prev,&(Proc->Device));
        else
            EXIT_FAIL("Process Memory Type is malformed.");

        /* Attribute */
        if((XML_Child(Trunk,"Attribute",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Memory Attribute section missing.");
        if(XML_Get_Val(Temp,Malloc,&Attr_Temp)<0)
            EXIT_FAIL("Internal error.");

        if(strchr(Attr_Temp,'R')!=0)
            Mem->Attr|=MEM_READ;
        if(strchr(Attr_Temp,'W')!=0)
            Mem->Attr|=MEM_WRITE;
        if(strchr(Attr_Temp,'X')!=0)
            Mem->Attr|=MEM_EXECUTE;

        if(Mem->Attr==0)
            EXIT_FAIL("Process Memory Attribute does not allow any access and is malformed.");

        if(strchr(Attr_Temp,'B')!=0)
            Mem->Attr|=MEM_BUFFERABLE;
        if(strchr(Attr_Temp,'C')!=0)
            Mem->Attr|=MEM_CACHEABLE;
        if(strchr(Attr_Temp,'S')!=0)
            Mem->Attr|=MEM_STATIC;

        Free(Attr_Temp);

        if(XML_Child(Node,0,&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Mem ***********************************************/

/* Begin Function:Parse_Proc_Thd **********************************************
Description : Parse the thread section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The thread section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : ret_t - Always 0.
******************************************************************************/
void Parse_Proc_Thd(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Thd_Info* Thd;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Thd));

    while(Trunk!=0)
    {
        Thd=Malloc(sizeof(struct Thd_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Thd->Name))<0)
            EXIT_FAIL("Internal error.");
        /* Entry */
        if((XML_Child(Trunk,"Entry",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Entry section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Thd->Entry))<0)
            EXIT_FAIL("Internal error.");
        /* Stack Addr */
        if((XML_Child(Trunk,"Stack_Addr",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Stack_Addr section missing.");
        if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Auto",4)==0))
            Thd->Stack_Addr=AUTO;
        else if(XML_Get_Hex(Temp,&(Thd->Stack_Addr))<0)
            EXIT_FAIL("Process Thread Stack_Addr is not a valid hex number.");
        /* Stack Size */
        if((XML_Child(Trunk,"Stack_Size",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Stack_Size section missing.");
        if(XML_Get_Hex(Temp,&(Thd->Stack_Size))<0)
            EXIT_FAIL("Process Thread Stack_Size is not a valid hex number.");
        /* Parameter */
        if((XML_Child(Trunk,"Parameter",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Parameter section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Thd->Parameter))<0)
            EXIT_FAIL("Internal error.");
        /* Priority */
        if((XML_Child(Trunk,"Priority",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Thread Priority section missing.");
        if(XML_Get_Uint(Temp,&(Thd->Priority))<0)
            EXIT_FAIL("Process Thread Priority is not a valid unsigned integer.");

        List_Ins(&(Thd->Head),Proc->Thd.Prev,&(Proc->Thd));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Thd ***********************************************/

/* Begin Function:Parse_Proc_Inv **********************************************
Description : Parse the invocation section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The invocation section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Inv(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Inv_Info* Inv;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Inv));

    while(Trunk!=0)
    {
        Inv=Malloc(sizeof(struct Inv_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Invocation Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Inv->Name))<0)
            EXIT_FAIL("Internal error.");
        /* Entry */
        if((XML_Child(Trunk,"Entry",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Invocation Entry section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Inv->Entry))<0)
            EXIT_FAIL("Internal error.");
        /* Stack Addr */
        if((XML_Child(Trunk,"Stack_Addr",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Invocation Stack_Addr section missing.");
        if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Auto",4)==0))
            Inv->Stack_Addr=AUTO;
        else if(XML_Get_Hex(Temp,&(Inv->Stack_Addr))<0)
            EXIT_FAIL("Process Invocation Stack_Addr is not a valid hex number.");
        /* Stack Size */
        if((XML_Child(Trunk,"Stack_Size",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Invocation Stack_Size section missing.");
        if(XML_Get_Hex(Temp,&(Inv->Stack_Size))<0)
            EXIT_FAIL("Process Invocation Stack_Size is not a valid hex number.");

        List_Ins(&(Inv->Head),Proc->Inv.Prev,&(Proc->Inv));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Inv ***********************************************/

/* Begin Function:Parse_Proc_Port *********************************************
Description : Parse the port section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The port section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Port(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Port_Info* Port;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Port));

    while(Trunk!=0)
    {
        Port=Malloc(sizeof(struct Port_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Port Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Port->Name))<0)
            EXIT_FAIL("Internal error.");
        /* Proc_Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Port Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Port->Proc_Name))<0)
            EXIT_FAIL("Internal error.");

        List_Ins(&(Port->Head),Proc->Port.Prev,&(Proc->Port));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Port **********************************************/

/* Begin Function:Parse_Proc_Recv *********************************************
Description : Parse the receive endpoint section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The receive endpoint section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Recv(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Recv_Info* Recv;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Recv));

    while(Trunk!=0)
    {
        Recv=Malloc(sizeof(struct Recv_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Receive Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Recv->Name))<0)
            EXIT_FAIL("Internal error.");

        List_Ins(&(Recv->Head),Proc->Recv.Prev,&(Proc->Recv));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Recv **********************************************/

/* Begin Function:Parse_Proc_Send *********************************************
Description : Parse the send endpoint section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The send endpoint section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Send(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Send_Info* Send;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Send));

    while(Trunk!=0)
    {
        Send=Malloc(sizeof(struct Send_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Send Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Send->Name))<0)
            EXIT_FAIL("Internal error.");
        /* Proc_Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Send Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Send->Proc_Name))<0)
            EXIT_FAIL("Internal error.");

        List_Ins(&(Send->Head),Proc->Send.Prev,&(Proc->Send));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Send **********************************************/

/* Begin Function:Parse_Proc_Vect *********************************************
Description : Parse the vector endpoint section of a particular process.
Input       : struct Proc_Info* Proc - The process information.
              xml_node_t* Node - The vector endpoint section's XML node.
Output      : struct Proc_Info* Proc - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proc_Vect(struct Proc_Info* Proc, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Vect_Info* Vect;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proc->Vect));

    while(Trunk!=0)
    {
        Vect=Malloc(sizeof(struct Vect_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Vector Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Vect->Name))<0)
            EXIT_FAIL("Internal error.");

        List_Ins(&(Vect->Head),Proc->Vect.Prev,&(Proc->Vect));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proc_Vect **********************************************/

/* Begin Function:Parse_Proj_Proc *********************************************
Description : Parse project process section.
Input       : struct Proj_Info* Proj - The project information.
              xml_node_t* Node - The process section's XML node.
Output      : struct Proj_Info* Proj - The updated process information.
Return      : None.
******************************************************************************/
void Parse_Proj_Proc(struct Proj_Info* Proj, xml_node_t* Node)
{
    xml_node_t* General;
    xml_node_t* Compiler;
    xml_node_t* Memory;
    xml_node_t* Thread;
    xml_node_t* Invocation;
    xml_node_t* Port;
    xml_node_t* Receive;
    xml_node_t* Send;
    xml_node_t* Vector;

    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Proc_Info* Proc;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Proj->Proc));

    while(Trunk!=0)
    {
        Proc=Malloc(sizeof(struct Proc_Info));

        /* General */
        if((XML_Child(Trunk,"General",&General)<0)||(General==0))
            EXIT_FAIL("Process General section missing.");
        /* Compiler */
        if((XML_Child(Trunk,"Compiler",&Compiler)<0)||(Compiler==0))
            EXIT_FAIL("Process Compiler section missing.");
        /* Memory */
        if((XML_Child(Trunk,"Memory",&Memory)<0)||(Memory==0))
            EXIT_FAIL("Process Memory section missing.");
        /* Thread */
        if((XML_Child(Trunk,"Thread",&Thread)<0)||(Thread==0))
            EXIT_FAIL("Process Thread section missing.");
        /* Invocation */
        if((XML_Child(Trunk,"Invocation",&Invocation)<0)||(Invocation==0))
            EXIT_FAIL("Process Invocation section missing.");
        /* Port */
        if((XML_Child(Trunk,"Port",&Port)<0)||(Port==0))
            EXIT_FAIL("Process Port section missing.");
        /* Receive */
        if((XML_Child(Trunk,"Receive",&Receive)<0)||(Receive==0))
            EXIT_FAIL("Process Receive section missing.");
        /* Send */
        if((XML_Child(Trunk,"Send",&Send)<0)||(Send==0))
            EXIT_FAIL("Process Send section missing.");
        /* Vector */
        if((XML_Child(Trunk,"Vector",&Vector)<0)||(Vector==0))
            EXIT_FAIL("Process Vector section missing.");

        /* Parse General section */
        /* Name */
        if((XML_Child(General,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Proc->Name))<0)
            EXIT_FAIL("Internal error.");
        /* Extra Captbl */
        if((XML_Child(General,"Extra_Captbl",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Process Extra_Captbl section missing.");
        if(XML_Get_Uint(Temp,&(Proc->Extra_Captbl))<0)
            EXIT_FAIL("Internal error.");

        /* Parse Compiler section */
        Parse_Compiler(&(Proc->Comp),Compiler);
        /* Parse Memory section */
        Parse_Proc_Mem(Proc,Memory);
        /* Parse Thread section */
        Parse_Proc_Thd(Proc,Thread);
        /* Parse Invocation section */
        Parse_Proc_Inv(Proc,Invocation);
        /* Parse Port section */
        Parse_Proc_Port(Proc,Port);
        /* Parse Receive section */
        Parse_Proc_Recv(Proc,Receive);
        /* Parse Send section */
        Parse_Proc_Send(Proc,Send);
        /* Parse Vector section */
        Parse_Proc_Vect(Proc,Vector);

        List_Ins(&(Proc->Head),Proj->Proc.Prev,&(Proj->Proc));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Proj_Proc **********************************************/

/* Begin Function:Parse_Proj **************************************************
Description : Parse the project description file, and fill in the struct.
Input       : s8_t* Proj_File - The buffer containing the project file contents.
Output      : None.
Return      : struct Proj_Info* - The struct containing the project information.
******************************************************************************/
struct Proj_Info* Parse_Proj(s8_t* Proj_File)
{
    xml_node_t* Node;
    xml_node_t* Temp;
    xml_node_t* RME;
    xml_node_t* RVM;
    xml_node_t* Process;
    struct Proj_Info* Proj;

    /* Allocate the project information structure */
    Proj=Malloc(sizeof(struct Proj_Info));

    /* Parse the XML content */
    if(XML_Parse(&Node,Proj_File)<0)
        EXIT_FAIL("Project XML is malformed.");
    if((Node->XML_Tag_Len!=7)||(strncmp(Node->XML_Tag,"Project",7)!=0))
        EXIT_FAIL("Project XML is malformed.");

    /* Name */
    if((XML_Child(Node,"Name",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Project Name section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Proj->Name))<0)
        EXIT_FAIL("Internal error.");
    /* Platform */
    if((XML_Child(Node,"Platform",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Project Platform section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Proj->Plat))<0)
        EXIT_FAIL("Internal error.");
    /* Chip_Class */
    if((XML_Child(Node,"Chip_Class",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Project Chip_Class section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Proj->Chip_Class))<0)
        EXIT_FAIL("Internal error.");
    /* Chip_Full */
    if((XML_Child(Node,"Chip_Full",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Project Chip_Full section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Proj->Chip_Full))<0)
        EXIT_FAIL("Internal error.");

    /* RME */
    if((XML_Child(Node,"RME",&RME)<0)||(RME==0))
        EXIT_FAIL("Project RME section missing.");
    /* RVM */
    if((XML_Child(Node,"RVM",&RVM)<0)||(RVM==0))
        EXIT_FAIL("Project RVM section missing.");
    /* Process */
    if((XML_Child(Node,"Process",&Process)<0)||(Process==0))
        EXIT_FAIL("Project Process section missing.");

    /* Parse RME section */
    Parse_Proj_RME(&(Proj->RME),RME);
    /* Parse RVM section */
    Parse_Proj_RVM(&(Proj->RVM),RVM);
    /* Parse Process section */
    Parse_Proj_Proc(&(Proj->Proc),Process);

    /* Destroy XML DOM */
    if(XML_Del(Node)<0)
        EXIT_FAIL("Internal error.");

    return Proj;
}
/* End Function:Parse_Proj ***************************************************/

/* Begin Function:Parse_Chip_Mem **********************************************
Description : Parse the memory section of a particular chip.
Input       : struct Chip_Info* Chip - The chip information.
              xml_node_t* Node - The process section's XML node.
Output      : struct Chip_Info* Chip - The updated chip information.
Return      : None.
******************************************************************************/
void Parse_Chip_Mem(struct Chip_Info* Chip, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Mem_Info* Mem;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Chip->Code));
    List_Crt(&(Chip->Data));
    List_Crt(&(Chip->Device));

    while(Trunk!=0)
    {
        Mem=Malloc(sizeof(struct Mem_Info));

        /* Start */
        if((XML_Child(Trunk,"Start",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Memory Start section missing.");
        if(XML_Get_Hex(Temp,&(Mem->Start))<0)
            EXIT_FAIL("Chip Memory Start is not a valid hex number.");

        /* Size */
        if((XML_Child(Trunk,"Size",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Memory Size section missing.");
        if(XML_Get_Hex(Temp,&(Mem->Size))<0)
            EXIT_FAIL("Chip Memory Size is not a valid hex number.");
        if(Mem->Size==0)
            EXIT_FAIL("Chip Memory Size cannot be zero.");
        if((Mem->Start+Mem->Size)>0x100000000ULL)
            EXIT_FAIL("Chip Memory Size out of bound.");

        /* Type */
        if((XML_Child(Trunk,"Type",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Memory Type section missing.");
        if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Code",4)==0))
            List_Ins(&(Mem->Head),Chip->Code.Prev,&(Chip->Code));
        else if((Temp->XML_Val_Len==4)&&(strncmp(Temp->XML_Val,"Data",4)==0))
            List_Ins(&(Mem->Head),Chip->Data.Prev,&(Chip->Data));
        else if((Temp->XML_Val_Len==6)&&(strncmp(Temp->XML_Val,"Device",6)==0))
            List_Ins(&(Mem->Head),Chip->Device.Prev,&(Chip->Device));
        else
            EXIT_FAIL("Chip Memory Type is malformed.");

        if(XML_Child(Node,0,&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Chip_Mem ***********************************************/

/* Begin Function:Parse_Chip_Option *******************************************
Description : Parse the option section of a particular chip.
Input       : struct Chip_Info* Chip - The chip information.
              xml_node_t* Node - The option section's XML node.
Output      : struct Chip_Info* Chip - The updated chip information.
Return      : None.
******************************************************************************/
void Parse_Chip_Option(struct Chip_Info* Chip, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Chip_Option_Info* Option;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Chip->Option));

    while(Trunk!=0)
    {
        Option=Malloc(sizeof(struct Chip_Option_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Option Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Option->Name))<0)
            EXIT_FAIL("Internal error.");

        /* Type */
        if((XML_Child(Trunk,"Type",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Option Type section missing.");
        if((Temp->XML_Val_Len==5)&&(strncmp(Temp->XML_Val,"Range",5)==0))
            Option->Type=OPTION_RANGE;
        else if((Temp->XML_Val_Len==6)&&(strncmp(Temp->XML_Val,"Select",6)==0))
            Option->Type=OPTION_SELECT;
        else
            EXIT_FAIL("Chip Option Type is malformed.");

        /* Macro */
        if((XML_Child(Trunk,"Macro",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Option Macro section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Option->Macro))<0)
            EXIT_FAIL("Internal error.");

        /* Range */
        if((XML_Child(Trunk,"Range",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Option Range section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Option->Range))<0)
            EXIT_FAIL("Internal error.");

        List_Ins(&(Option->Head),Chip->Option.Prev,&(Chip->Option));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Chip_Option ********************************************/

/* Begin Function:Parse_Chip_Vect *********************************************
Description : Parse the vector section of a particular chip.
Input       : struct Chip_Info* Chip - The chip information.
              xml_node_t* Node - The vector section's XML node.
Output      : struct Chip_Info* Chip - The updated chip information.
Return      : None.
******************************************************************************/
void Parse_Chip_Vect(struct Chip_Info* Chip, xml_node_t* Node)
{
    xml_node_t* Trunk;
    xml_node_t* Temp;
    struct Chip_Vect_Info* Vect;

    if(XML_Child(Node,0,&Trunk)<0)
        EXIT_FAIL("Internal error.");

    List_Crt(&(Chip->Vect));

    while(Trunk!=0)
    {
        Vect=Malloc(sizeof(struct Chip_Vect_Info));

        /* Name */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Vector Name section missing.");
        if(XML_Get_Val(Temp,Malloc,&(Vect->Name))<0)
            EXIT_FAIL("Internal error.");

        /* Number */
        if((XML_Child(Trunk,"Name",&Temp)<0)||(Temp==0))
            EXIT_FAIL("Chip Vector Number section missing.");
        if(XML_Get_Uint(Temp,&(Vect->Num))<0)
            EXIT_FAIL("Chip Vector Number is not an unsigned integer.");

        List_Ins(&(Vect->Head),Chip->Vect.Prev,&(Chip->Vect));
        if(XML_Child(Node,"",&Trunk)<0)
            EXIT_FAIL("Internal error.");
    }
}
/* End Function:Parse_Chip_Vect ********************************************/

/* Begin Function:Parse_Chip **************************************************
Description : Parse the chip description file, and fill in the struct.
Input       : s8_t* Chip_File - The buffer containing the chip file contents.
Output      : None.
Return      : struct Chip_Info* - The struct containing the chip information.
******************************************************************************/
struct Chip_Info* Parse_Chip(s8_t* Chip_File)
{
    xml_node_t* Attribute;
    xml_node_t* Memory;
    xml_node_t* Option;
    xml_node_t* Vector;
    xml_node_t* Node;
    xml_node_t* Temp;
    struct Raw_Info* Raw;
    struct Chip_Info* Chip;

    /* Allocate the project information structure */
    Chip=Malloc(sizeof(struct Chip_Info));
   
    /* Parse the XML content */
    if(XML_Parse(&Node,Chip_File)<0)
        EXIT_FAIL("Chip XML is malformed.");
    if((Node->XML_Tag_Len!=4)||(strncmp(Node->XML_Tag,"Chip",4)!=0))
        EXIT_FAIL("Chip XML is malformed.");

    /* Class */
    if((XML_Child(Node,"Class",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Class section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Chip->Class))<0)
        EXIT_FAIL("Internal error.");
    /* Compatible */
    if((XML_Child(Node,"Platform",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Compatible section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Chip->Compat))<0)
        EXIT_FAIL("Internal error.");
    /* Vendor */
    if((XML_Child(Node,"Vendor",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Vendor section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Chip->Vendor))<0)
        EXIT_FAIL("Internal error.");
    /* Platform */
    if((XML_Child(Node,"Platform",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Platform section missing.");
    if(XML_Get_Val(Temp,Malloc,&(Chip->Plat))<0)
        EXIT_FAIL("Internal error.");
    /* Cores */
    if((XML_Child(Node,"Cores",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Cores section missing.");
    if(XML_Get_Uint(Temp,Malloc,&(Chip->Cores))<0)
        EXIT_FAIL("Chip Cores is not an unsigned integer.");
    /* Regions */
    if((XML_Child(Node,"Regions",&Temp)<0)||(Temp==0))
        EXIT_FAIL("Chip Regions section missing.");
    if(XML_Get_Uint(Temp,Malloc,&(Chip->Regions))<0)
        EXIT_FAIL("Chip Regions is not an unsigned integer.");
    
    /* Attribute */
    if((XML_Child(Node,"Attribute",&Attribute)<0)||(Attribute==0))
        EXIT_FAIL("Chip Attribute section missing.");
    /* Memory */
    if((XML_Child(Node,"Memory",&Memory)<0)||(Memory==0))
        EXIT_FAIL("Chip Memory section missing.");
    /* Option */
    if((XML_Child(Node,"Option",&Option)<0)||(Option==0))
        EXIT_FAIL("Chip Option section missing.");
    /* Vector */
    if((XML_Child(Node,"Vector",&Vector)<0)||(Vector==0))
        EXIT_FAIL("Chip Vector section missing.");

    /* Parse Attribute section */
    List_Crt(&(Chip->Attr));
    if(XML_Child(Attribute,0,&Temp)<0)
        EXIT_FAIL("Internal error.");
    while(Temp!=0)
    {
        Raw=Malloc(sizeof(struct Raw_Info));
        if(XML_Get_Tag(Temp,Malloc,&Raw->Tag)<0)
            EXIT_FAIL("Chip Attribute tag read failed.");
        if(XML_Get_Val(Temp,Malloc,&Raw->Val)<0)
            EXIT_FAIL("Chip Attribute value read failed.");

        List_Ins(&(Raw->Head),Chip->Attr.Prev,&(Chip->Attr));
        if(XML_Child(Chip,"",&Temp)<0)
            EXIT_FAIL("Internal error.");
    }

    /* Parse Memory section */
    Parse_Chip_Mem(Chip,Memory);
    /* Parse Option section */
    Parse_Chip_Option(Chip,Option);
    /* Parse Vector section */
    Parse_Chip_Vect(Chip,Vector);

    /* Destroy XML DOM */
    if(XML_Del(Node)<0)
        EXIT_FAIL("Internal error.");

    return Chip;
}
/* End Function:Parse_Chip ***************************************************/

/* Begin Function:Insert_Mem **************************************************
Description : Insert memory blocks into a queue with increasing start address/size.
Input       : struct Mem_Info** Array - The array containing all the memory blocks to sort.
              ptr_t Len - The maximum length of the array.
              struct Mem_Info* Mem - The memory block to insert.
              ptr_t Category - The insert option, 0 for start address, 1 for size.
Output      : struct Mem_Info** Array - The updated array.
Return      : ret_t - If successful, 0; else -1.
******************************************************************************/
ret_t Insert_Mem(struct Mem_Info** Array, ptr_t Len, struct Mem_Info* Mem, ptr_t Category)
{
    ptr_t Pos;
    ptr_t End;

    for(Pos=0;Pos<Len;Pos++)
    {
        if(Array[Pos]==0)
            break;
        if(Category==0)
        {
            if(Array[Pos]->Start>Mem->Start)
                break;
        }
        else
        {
            if(Array[Pos]->Size>Mem->Size)
                break;
        }
    }
    if(Pos>=Len)
        return -1;
    for(End=Pos;End<Len;End++)
    {
        if(Array[End]==0)
            break;
    }
    if(End>0)
    {
        for(End--;End>=Pos;End--)
            Array[End+1]=Array[End];
    }

    Array[Pos]=Mem;
    return 0;
}
/* End Function:Insert_Mem ***************************************************/

/* Begin Function:Try_Bitmap **************************************************
Description : See if this bitmap segment is already covered.
Input       : s8_t* Bitmap - The bitmap.
              ptr_t Start - The starting bit location.
              ptr_t Size - The number of bits.
Output      : None.
Return      : ret_t - If can be marked, 0; else -1.
******************************************************************************/
ret_t Try_Bitmap(s8_t* Bitmap, ptr_t Start, ptr_t Size)
{
    ptr_t Count;

    for(Count=0;Count<Size;Count++)
    {
        if((Bitmap[(Start+Count)/8]&POW2((Start+Count)%8))!=0)
            return -1;
    }
    return 0;
}
/* End Function:Try_Bitmap ***************************************************/

/* Begin Function:Mark_Bitmap *************************************************
Description : Actually mark this bitmap segment.
Input       : s8_t* Bitmap - The bitmap.
              ptr_t Start - The starting bit location.
              ptr_t Size - The number of bits.
Output      : s8_t* Bitmap - The updated bitmap.
Return      : None.
******************************************************************************/
void Mark_Bitmap(s8_t* Bitmap, ptr_t Start, ptr_t Size)
{
    ptr_t Count;

    for(Count=0;Count<Size;Count++)
        Bitmap[(Start+Count)/8]|=POW2((Start+Count)%8);
}
/* End Function:Mark_Bitmap **************************************************/

/* Begin Function:Populate_Mem ************************************************
Description : Populate the memory data structure with this memory segment.
              This operation will be conducted with no respect to whether this
              portion have been populated with someone else.
Input       : struct Mem_Map* Map - The memory map.
              ptr_t Start - The start address of the memory.
              ptr_t Size - The size of the memory.
Output      : struct Mem_Map* Map - The updated memory map.
Return      : ret_t - If successful, 0; else -1.
******************************************************************************/
ret_t Populate_Mem(struct Mem_Map* Map, ptr_t Start, ptr_t Size)
{
    ptr_t Mem_Cnt;
    ptr_t Rel_Start;

    for(Mem_Cnt=0;Mem_Cnt<Map->Mem_Num;Mem_Cnt++)
    {
        if((Start>=Map->Mem_Array[Mem_Cnt]->Start)&&
           (Start<=Map->Mem_Array[Mem_Cnt]->Start+(Map->Mem_Array[Mem_Cnt]->Size-1)))
            break;
    }

    /* Must be in this segment. See if we can fit there */
    if(Mem_Cnt==Map->Mem_Num)
        return -1;
    if((Map->Mem_Array[Mem_Cnt]->Start+(Map->Mem_Array[Mem_Cnt]->Size-1))<(Start+(Size-1)))
        return -1;
    
    /* It is clear that we can fit now. Mark all the bits */
    Rel_Start=Start-Map->Mem_Array[Mem_Cnt]->Start;
    Mark_Bitmap(Map->Mem_Bitmap[Mem_Cnt],Rel_Start/4,Size/4);
    return 0;
}
/* End Function:Populate_Mem *************************************************/

/* Begin Function:Fit_Mem *****************************************************
Description : Fit the auto-placed memory segments to a fixed location.
Input       : struct Mem_Map* Map - The memory map.
              ptr_t Mem_Num - The memory info number in the process memory array.
Output      : struct Mem_Map* Map - The updated memory map.
Return      : ret_t - If successful, 0; else -1.
******************************************************************************/
ret_t Fit_Mem(struct Mem_Map* Map, ptr_t Mem_Num)
{
    ptr_t Fit_Cnt;
    ptr_t Start_Addr;
    ptr_t End_Addr;
    ptr_t Try_Addr;
    ptr_t Bitmap_Start;
    ptr_t Bitmap_End;
    struct Mem_Info* Mem;
    struct Mem_Info* Fit;

    Mem=Map->Proc_Mem_Array[Mem_Num];
    /* Find somewhere to fit this memory trunk, and if found, we will populate it */
    for(Fit_Cnt=0;Fit_Cnt<Map->Mem_Num;Fit_Cnt++)
    {
        Fit=Map->Mem_Array[Fit_Cnt];
        if(Mem->Size>Fit->Size)
            continue;
        /* Round start address up, round end address down, to alignment */
        Start_Addr=((Fit->Start+Mem->Align-1)/Mem->Align)*Mem->Align;
        End_Addr=((Fit->Start+Fit->Size)/Mem->Align)*Mem->Align;
        if(Mem->Size>(End_Addr-Start_Addr))
            continue;
        End_Addr-=Mem->Size;
        for(Try_Addr=Start_Addr;Try_Addr<End_Addr;Try_Addr+=Mem->Align)
        {
            Bitmap_Start=(Try_Addr-Fit->Start)/4;
            Bitmap_End=Mem->Size/4;
            if(Try_Bitmap(Map->Mem_Bitmap[Fit_Cnt], Bitmap_Start,Bitmap_End)==0)
            {
                Mark_Bitmap(Map->Mem_Bitmap[Fit_Cnt], Bitmap_Start,Bitmap_End);
                Mem->Start=Try_Addr;
                /* Found a fit */
                return 0;
            }
        }
    }
    /* Can't find any fit */
    return -1;
}
/* End Function:Fit_Mem ******************************************************/

/* Begin Function:Alloc_Code **************************************************
Description : Allocate the code section of all processes.
Input       : struct Proj_Info* Proj - The struct containing the project information.
              struct Chip_Info* Chip - The struct containing the chip information.
Output      : struct Proj_Info* Proj - The struct containing the project information,
                                       with all memory location allocated.
Return      : None.
******************************************************************************/
void Alloc_Code(struct Proj_Info* Proj, struct Chip_Info* Chip, ptr_t Type)
{
    ptr_t Count;
    ptr_t Proc_Cnt;
    ptr_t Mem_Cnt;
    struct Mem_Map* Map;

    Map=Malloc(sizeof(struct Mem_Map));
    List_Init(&(Map->Info));

    /* Insert all memory trunks in a incremental order by address */
    for(Mem_Cnt=0;Mem_Cnt<Chip->Mem_Num;Mem_Cnt++)
    {
        if(Chip->Mem[Mem_Cnt].Type==Type)
        {
            if(Insert_Mem(Map->Mem_Array,Map->Mem_Num,&Chip->Mem[Mem_Cnt],0)!=0)
                EXIT_FAIL("Code memory insertion sort failed.");
        }
    }

    /* Now allocate the bitmap array according to their size */
    for(Mem_Cnt=0;Mem_Cnt<Map->Mem_Num;Mem_Cnt++)
    {
        /* We insist that one bit represents 4 bytes in the bitmap */
        Map->Mem_Bitmap[Mem_Cnt]=Malloc((Map->Mem_Array[Mem_Cnt]->Size/4)+1);
        if(Map->Mem_Bitmap[Mem_Cnt]==0)
            EXIT_FAIL("Code bitmap allocation failed");
        memset(Map->Mem_Bitmap[Mem_Cnt],0,(size_t)((Map->Mem_Array[Mem_Cnt]->Size/4)+1));
    }

    /* Now populate the RME & RVM sections */
    if(Populate_Mem(Map, Proj->RME.Code_Start,Proj->RME.Code_Size)!=0)
        EXIT_FAIL("Invalid address designated.");
    if(Populate_Mem(Map, Proj->RME.Code_Start+Proj->RME.Code_Size,Proj->RVM.Code_Size)!=0)
        EXIT_FAIL("Invalid address designated.");

    /* Merge sort all processes's memory in according to their size */

    /* Populate these memory sections one by one */

    /* Find all project code memory sections */

    /* Clean up before returning */

    Count=0;
    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==Type)
            {
                if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Start==AUTO)
                    Count++;
                else
                {
                    if(Populate_Mem(Map, Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Start, Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Size)!=0)
                        EXIT_FAIL("Invalid address designated.");
                }
            }
        }
    }

    if(Count!=0)
    {
        Map->Proc_Mem_Num=Count;
        Map->Proc_Mem_Array=Malloc(sizeof(struct Mem_Info*)*Map->Proc_Mem_Num);
        if(Map->Proc_Mem_Array==0)
            EXIT_FAIL("Memory map allocation failed.");
        memset(Map->Proc_Mem_Array,0,(size_t)(sizeof(struct Mem_Info*)*Map->Proc_Mem_Num));

        /* Insert sort according to size */
        for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
        {
            for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
            {
                if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==Type)
                {
                    if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Start==AUTO)
                    {
                        if(Insert_Mem(Map->Proc_Mem_Array,Map->Proc_Mem_Num,&(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt]),1)!=0)
                            EXIT_FAIL("Code memory insertion sort failed.");
                    }
                }
            }
        }

        /* Fit whatever that does not have a fixed address */
        for(Mem_Cnt=0;Mem_Cnt<Map->Proc_Mem_Num;Mem_Cnt++)
        {
            if(Fit_Mem(Map,Mem_Cnt)!=0)
                EXIT_FAIL("Memory fitter failed.");
        }

        Free(Map->Proc_Mem_Array);
    }

    /* Clean up before returning */
    for(Mem_Cnt=0;Mem_Cnt<Map->Mem_Num;Mem_Cnt++)
        Free(Map->Mem_Bitmap[Mem_Cnt]);
    
    Free(Map->Mem_Array);
    Free(Map->Mem_Bitmap);
    Free(Map);
}
/* End Function:Alloc_Mem ****************************************************/

/* Begin Function:Check_Mem ***************************************************
Description : Check the memory layout to make sure that they don't overlap.
              Also check if the device memory of all processes are in the device
              memory range, and if all processes have at least a data segment and
              a code segment. If not, we need to abort immediately.
              These algorithms are far from efficient; there are O(nlogn) variants,
              which we leave as a possible future optimization.
Input       : struct Proj_Info* Proj - The struct containing the project information.
              struct Chip_Info* Chip - The struct containing the chip information.
Output      : None.
Return      : None.
******************************************************************************/
void Check_Mem(struct Proj_Info* Proj, struct Chip_Info* Chip)
{
    ptr_t Proc_Cnt;
    ptr_t Mem_Cnt;
    ptr_t Proc_Temp_Cnt;
    ptr_t Mem_Temp_Cnt;
    ptr_t Chip_Mem_Cnt;
    struct Mem_Info* Mem1;
    struct Mem_Info* Mem2;


    /* Is it true that each process have a code segment and a data segment? */
    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==MEM_CODE)
                break;
        }
        if(Mem_Cnt==Proj->Proc[Proc_Cnt].Mem_Num)
            EXIT_FAIL("At least one process does not have a single code segment.");

        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==MEM_DATA)
                break;
        }
        if(Mem_Cnt==Proj->Proc[Proc_Cnt].Mem_Num)
            EXIT_FAIL("At least one process does not have a single data segment.");
    }
    
    /* Is it true that the device memory is in device memory range？
     * Also, device memory cannot have AUTO placement, position must be designated. */
    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==MEM_DEVICE)
            {
                if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Start>=INVALID)
                    EXIT_FAIL("Device memory cannot have auto placement.");

                for(Chip_Mem_Cnt=0;Chip_Mem_Cnt<Chip->Mem_Num;Chip_Mem_Cnt++)
                {
                    if(Chip->Mem[Chip_Mem_Cnt].Type==MEM_DEVICE)
                    {
                        Mem1=&(Chip->Mem[Chip_Mem_Cnt]);
                        Mem2=&(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt]);
                        if((Mem1->Start<=Mem2->Start)&&((Mem1->Start+(Mem1->Size-1))>=(Mem2->Start+(Mem2->Size-1))))
                            break;
                    }
                }
                if(Chip_Mem_Cnt==Chip->Mem_Num)
                    EXIT_FAIL("At least one device memory segment is out of bound.");
            }
        }
    }

    /* Is it true that the primary code memory does not overlap with each other? */
    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Type==MEM_CODE)
                break;
        }
        for(Proc_Temp_Cnt=0;Proc_Temp_Cnt<Proj->Proc_Num;Proc_Temp_Cnt++)
        {
            if(Proc_Temp_Cnt==Proc_Cnt)
                continue;
            for(Mem_Temp_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Temp_Cnt].Mem_Num;Mem_Cnt++)
            {
                if(Proj->Proc[Proc_Temp_Cnt].Mem[Mem_Temp_Cnt].Type==MEM_CODE)
                    break;
            }
            
            Mem1=&(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt]);
            Mem2=&(Proj->Proc[Proc_Temp_Cnt].Mem[Mem_Temp_Cnt]);

            if(((Mem1->Start+(Mem1->Size-1))<Mem2->Start)||((Mem2->Start+(Mem2->Size-1))<Mem1->Start))
                continue;
            else
                EXIT_FAIL("Two process's main code sections overlapped.");
        }
    }
}
/* End Function:Check_Mem ****************************************************/

/* Begin Function:main ********************************************************
Description : The entry of the tool.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
int main(int argc, char* argv[])
{
	/* The command line arguments */
	s8_t* Input_Path;
	s8_t* Output_Path;
	s8_t* RME_Path;
	s8_t* RVM_Path;
	s8_t* Format;
	/* The input buffer */
	s8_t* Input_Buf;
    /* The path synthesis buffer */
    s8_t* Path_Buf;
	/* The project and chip pointers */
	struct Proj_Info* Proj;
	struct Chip_Info* Chip;

	/* Initialize memory pool */
	List_Crt(&Mem_List);

    /* Process the command line first */
    Cmdline_Proc(argc,argv, &Input_Path, &Output_Path, &RME_Path, &RVM_Path, &Format);

	/* Read the project contents */
	Input_Buf=Read_File(Input_Path);
	Proj=Parse_Proj(Input_Buf);
	Free(Input_Buf);

	/* Parse the chip in a platform-agnostic way - we need to know where the chip file is. Now, we just give a fixed path */
    Path_Buf=Malloc(4096);
    if(Path_Buf==0)
        EXIT_FAIL("Platform path synthesis buffer allocation failed.");
    sprintf(Path_Buf, "%s/MEukaron/Include/Platform/%s/Chips/%s/rme_platform_%s.xml",
                      RME_Path, Proj->Plat, Proj->Chip_Class, Proj->Chip_Class);
	Input_Buf=Read_File(Path_Buf);
	Chip=Parse_Chip(Input_Buf);
	Free(Input_Buf);
    Free(Path_Buf);

    /* Check if the platform is the same */
    if(strcmp(Proj->Plat, Chip->Plat)!=0)
        EXIT_FAIL("The chip description file platform conflicted with the project file.");

    /* Check the general validity of everything */
    Check_Validity(Proj, Chip);

	/* Align memory to what it should be */
	if(strcmp(Proj->Plat,"A7M")==0)
	    A7M_Align_Mem(Proj);
    else
		EXIT_FAIL("Other platforms not currently supported.");

	/* Allocate and check code memory */
	Alloc_Code(Proj, Chip);
    Check_Code(Proj, Chip);

    /* Allocate and check data memory */
	Alloc_Data(Proj, Chip);
    Check_Data(Proj, Chip);

    /* Check device memory */
    Check_Device(Proj, Chip);

    /* Allocate the local and global capid of all kernel objects */
    Alloc_Captbl(Proj, Chip);

    /* Set the folder and selection include headers up */
    Setup_Folder(Proj, Chip, RME_Path, RVM_Path, Output_Path);
    Setup_Include(Proj, Chip, RME_Path, RVM_Path, Output_Path);

	/* Generate the project-specific files */
	if(strcmp(Proj->Plat,"A7M")==0)
		A7M_Gen_Proj(Proj, Chip, RME_Path, RVM_Path, Output_Path, Format);

    /* Create all files */
    struct Cap_Alloc_Info Alloc;
    memset(&Alloc,0,sizeof(struct Cap_Alloc_Info));
    Alloc.Processor_Bits=32;
    Gen_Files(Proj, Chip, &Alloc, RME_Path, RVM_Path, Output_Path);
    
	/* All done, free all memory and we quit */
	Free_All();
    return 0;
}
/* End Function:main *********************************************************/

/* Begin Function:Align_Mem ***************************************************
Description : Align the memory according to the platform's alignment functions.
              We will only align the memory of the processes.
Input       : struct Proj_Info* Proj - The struct containing the project information.
              ret_t (*Align)(struct Mem_Info*) - The platform's alignment function pointer.
Output      : struct Proj_Info* Proj - The struct containing the project information,
                                       with all memory size aligned.
Return      : None.
******************************************************************************/
void Align_Mem(struct List* Mem, ret_t (*Align)(struct Mem_Info*))
{
    ptr_t Proc_Cnt;
    ptr_t Mem_Cnt;

    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if(Align(&(Proj->Proc[Proc_Cnt].Mem[Mem_Cnt]))!=0)
                EXIT_FAIL("Memory aligning failed.");
        }
    }
}
/* End Function:Align_Mem ****************************************************/