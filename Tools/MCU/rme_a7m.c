/******************************************************************************
Filename    : rme_a7m.c
Author      : pry
Date        : 12/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : This toolset is for ARMv7-M. Specifically, this suits Cortex-M0+,
              Cortex-M3, Cortex-M4, Cortex-M7. For ARMv8-M, please use A8M port.
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

#define __HDR_DEFS__
#include "rme_mcu.h"
#include "rme_a7m.h"
#undef __HDR_DEFS__

#define __HDR_STRUCTS__
#include "rme_mcu.h"
#include "rme_a7m.h"
#undef __HDR_STRUCTS__

/* Private include */
#include "rme_a7m.h"

#define __HDR_PUBLIC_MEMBERS__
#include "rme_mcu.h"
#undef __HDR_PUBLIC_MEMBERS__
/* End Includes **************************************************************/

/* Begin Function:A7M_Align_Mem ***********************************************
Description : Align the memory according to the A7M platform's alignment functions.
              We will only align the memory of the processes.
Input       : struct Proj_Info* Proj - The struct containing the project information.
Output      : struct Proj_Info* Proj - The struct containing the project information,
                                       with all memory size aligned.
Return      : None.
******************************************************************************/
void A7M_Align_Mem(struct Proj_Info* Proj)
{

}
/* End Function:Align_Mem ****************************************************/

/* A7M Toolset ***************************************************************/
ret_t A7M_Align(struct Mem_Info* Mem);
void A7M_Gen_Proj(struct Proj_Info* Proj, struct Chip_Info* Chip,
                  s8_t* RME_Path, s8_t* RVM_Path, s8_t* Output_Path, s8_t* Format);



/* C Function Prototypes *****************************************************/
struct A7M_Pgtbl* A7M_Gen_Pgtbl(struct Proc_Info* Proc, struct A7M_Info* A7M,
                                struct Mem_Info* Mem, ptr_t Num, ptr_t Total_Max);
/* End C Function Prototypes *************************************************/

/* Begin Function:A7M_Align ***************************************************
Description : Align the memory according to Cortex-M platform's requirements.
Input       : struct Mem_Info* Mem - The struct containing memory information.
Output      : struct Mem_Info* Mem - The struct containing memory information,
                                     with all memory size aligned.
Return      : ret_t - If the start address and size is acceptable, 0; else -1.
******************************************************************************/
ret_t A7M_Align(struct Mem_Info* Mem)
{
    ptr_t Temp;
    if(Mem->Start!=AUTO)
    {
        /* This memory already have a fixed start address. Can we map it in? */
        if((Mem->Start&0x1F)!=0)
            return -1;
        if((Mem->Size&0x1F)!=0)
            return -1;
        /* This is terrible. Or even horrible, this mapping algorithm is really hard */
    }
    else
    {
        /* This memory's start address is not designated yet. Decide its size after
         * alignment and calculate its start address alignment granularity. 
         * For Cortex-M, the roundup minimum granularity is 1/8 of the nearest power 
         * of 2 for the size. */
        Temp=1;
        while(Temp<Mem->Size)
            Temp<<=1;
        Mem->Align=Temp/8;
        Mem->Size=((Mem->Size-1)/Mem->Align+1)*Mem->Align;
    }
    
	return 0;
}
/* End Function:A7M_Align ****************************************************/

/* Begin Function:A7M_Parse_Options *******************************************
Description : Parse the options that are specific to ARMv7-M.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The platform sprcific project structure.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Parse_Options(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M)
{
    s8_t* Temp;

    /* What is the NVIC grouping that we use? */
    Temp=Raw_Match(&(Proj->RME.Plat), "NVIC_Grouping");
    if(Temp==0)
        EXIT_FAIL("Missing NVIC grouping settings.");
    if(strcmp(Temp,"0-8")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P0S8;
    else if(strcmp(Temp,"1-7")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P1S7;
    else if(strcmp(Temp,"2-6")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P2S6;
    else if(strcmp(Temp,"3-5")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P3S5;
    else if(strcmp(Temp,"4-4")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P4S4;
    else if(strcmp(Temp,"5-3")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P5S3;
    else if(strcmp(Temp,"6-2")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P6S2;
    else if(strcmp(Temp,"7-1")==0)
        A7M->NVIC_Grouping=A7M_NVIC_P7S1;
    else
        EXIT_FAIL("NVIC grouping value is invalid.");

    /* What is the systick value? */
    Temp=Raw_Match(&(Proj->RME.Plat), "Systick_Value");
    if(Temp==0)
        EXIT_FAIL("Missing systick value settings.");
    A7M->Systick_Val=Get_Uint(Temp, &Temp[strlen(Temp)-1]);
    if(A7M->Systick_Val>=INVALID)
        EXIT_FAIL("Wrong systick value entered.");

    /* What is the CPU type and the FPU type? */
    Temp=Raw_Match(&(Proj->RME.Plat), "CPU_Type");
    if(Temp==0)
        EXIT_FAIL("Missing CPU type settings.");
    if(strcmp(Temp,"Cortex-M0+")==0)
        A7M->CPU_Type=A7M_CPU_CM0P;
    else if(strcmp(Temp,"Cortex-M3")==0)
        A7M->CPU_Type=A7M_CPU_CM3;
    else if(strcmp(Temp,"Cortex-M4")==0)
        A7M->CPU_Type=A7M_CPU_CM4;
    else if(strcmp(Temp,"Cortex-M7")==0)
        A7M->CPU_Type=A7M_CPU_CM7;
    else
        EXIT_FAIL("CPU type value is invalid.");
    
    Temp=Raw_Match(&(Chip->Attr), "FPU_Type");
    if(Temp==0)
        EXIT_FAIL("Missing FPU type settings.");
    if(strcmp(Temp,"None")==0)
        A7M->FPU_Type=A7M_FPU_NONE;
    else if(strcmp(Temp,"Single")==0)
    {
        if(A7M->CPU_Type==A7M_CPU_CM4)
            A7M->FPU_Type=A7M_FPU_FPV4;
        else if(A7M->CPU_Type==A7M_CPU_CM7)
            A7M->FPU_Type=A7M_FPU_FPV5_SP;
        else
            EXIT_FAIL("FPU type and CPU type mismatch.");
    }
    else if(strcmp(Temp,"Double")==0)
    {
        if(A7M->CPU_Type==A7M_CPU_CM7)
            A7M->FPU_Type=A7M_FPU_FPV5_DP;
        else
            EXIT_FAIL("FPU type and CPU type mismatch.");

    }
    else
        EXIT_FAIL("FPU type value is invalid.");

    /* What is the endianness? */
    Temp=Raw_Match(&(Chip->Attr_Raw), "Endianness");
    if(Temp==0)
        EXIT_FAIL("Missing endianness settings.");
    if(strcmp(Temp,"Little")==0)
        A7M->Endianness=A7M_END_LITTLE;
    else if(strcmp(Temp,"Big")==0)
        A7M->Endianness=A7M_END_BIG;
    else
        EXIT_FAIL("Endianness value is invalid.");
}
/* End Function:A7M_Parse_Options ********************************************/

/* Begin Function:A7M_Total_Order *********************************************
Description : Get the total order and the start address of the page table. 
Input       : struct Mem_Info* Mem - The memory block list.
              ptr_t Num - The number of memory blocks in the list.
Output      : ptr_t* Start_Addr - The start address of this page table.
Return      : ptr_t - The total order of the page table.
******************************************************************************/
ptr_t A7M_Total_Order(struct Mem_Info* Mem, ptr_t Num, ptr_t* Start_Addr)
{
    /* Start is inclusive, end is exclusive */
    ptr_t Start;
    ptr_t End;
    ptr_t Total_Order;
    ptr_t Mem_Cnt;

    /* What ranges does these stuff cover? */
    Start=(ptr_t)(-1);
    End=0;
    for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
    {
        if(Start>Mem[Mem_Cnt].Start)
            Start=Mem[Mem_Cnt].Start;
        if(End<(Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1)))
            End=Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1);
    }
    
    /* Which power-of-2 box is this in? - do not shift more thyan 32 or you get undefined behavior */
    Total_Order=0;
    while(1)
    {  
        /* No bigger than 32 is ever possible */
        if(Total_Order>=32)
            break;
        if(End<=(ALIGN_POW(Start, Total_Order)+(POW2(Total_Order)-1)))
            break;
        Total_Order++;
    }
    /* If the total order less than 8, we wish to extend that to 8, because if we are smaller than this it makes no sense */
    if(Total_Order<8)
        Total_Order=8;

    /* Do not shift more than 32 or we get undefined behavior */
    if(Total_Order==32)
        *Start_Addr=0;
    else
        *Start_Addr=ALIGN_POW(Start, Total_Order);

    return Total_Order;
}
/* End Function:A7M_Total_Order **********************************************/

/* Begin Function:A7M_Num_Order ***********************************************
Description : Get the number order of the page table. 
Input       : struct Mem_Info* Mem - The memory block list.
              ptr_t Num - The number of memory blocks in the list.
              ptr_t Total_Order - The total order of the page table.
              ptr_t Start_Addr - The start address of the page table.
Output      : None.
Return      : ptr_t - The number order of the page table.
******************************************************************************/
ptr_t A7M_Num_Order(struct Mem_Info* Mem, ptr_t Num, ptr_t Total_Order, ptr_t Start_Addr)
{
    ptr_t Mem_Cnt;
    ptr_t Num_Order;
    ptr_t Pivot_Cnt;
    ptr_t Pivot_Addr;
    ptr_t Cut_Apart;

    /* Can the memory segments get fully mapped in? If yes, there are two conditions
     * that must be met:
     * 1. There cannot be different access permissions in these memory segments.
     * 2. The memory start address and the size must be fully divisible by POW2(Total_Order-3). */
    for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
    {
        if(Mem[Mem_Cnt].Attr!=Mem[0].Attr)
            break;
        if((Mem[Mem_Cnt].Start%POW2(Total_Order-3))!=0)
            break;
        if((Mem[Mem_Cnt].Size%POW2(Total_Order-3))!=0)
            break;
    }

    /* Is this directly mappable? If yes, we always create page tables with 8 pages. */
    if(Mem_Cnt==Num)
    {
        /* Yes, it is directly mappable. We choose the smallest number order, in this way
         * we have the largest size order. This will leave us plenty of chances to use huge
         * pages, as this facilitates delegation as well. Number order = 0 is also possible,
         * as this maps in a single huge page. */
        for(Num_Order=0;Num_Order<=3;Num_Order++)
        {
            for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
            {
                if((Mem[Mem_Cnt].Start%POW2(Total_Order-Num_Order))!=0)
                    break;
                if((Mem[Mem_Cnt].Size%POW2(Total_Order-Num_Order))!=0)
                    break;
            }
            if(Mem_Cnt==Num)
                break;
        }

        if(Num_Order>3)
            EXIT_FAIL("Internal number order miscalculation.");
    }
    else
    {
        /* Not directly mappable. What's the maximum number order that do not cut things apart? */
        Cut_Apart=0;
        for(Num_Order=1;Num_Order<=3;Num_Order++)
        {
            for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
            {
                for(Pivot_Cnt=1;Pivot_Cnt<POW2(Num_Order);Pivot_Cnt++)
                {
                    Pivot_Addr=Start_Addr+Pivot_Cnt*POW2(Total_Order-Num_Order);
                    if((Mem[Mem_Cnt].Start<Pivot_Addr)&&((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))>=Pivot_Addr))
                    {
                        Cut_Apart=1;
                        break;
                    }
                }
                if(Cut_Apart!=0)
                    break;
            }
            if(Cut_Apart!=0)
                break;
        }

        /* For whatever reason, if it breaks, then the last number order must be good */
        if(Num_Order>1)
            Num_Order--;
    }

    return Num_Order;
}
/* End Function:A7M_Num_Order ************************************************/

/* Begin Function:A7M_Map_Page ************************************************
Description : Map pages into the page table as we can. 
Input       : struct Mem_Info* Mem - The memory block list.
              ptr_t Num - The number of memory blocks in the list.
              struct A7M_Pgtbl* Pgtbl - The current page table.
Output      : struct A7M_Pgtbl* Pgtbl - The updated current page table.
Return      : None.
******************************************************************************/
void A7M_Map_Page(struct Mem_Info* Mem, ptr_t Num, struct A7M_Pgtbl* Pgtbl)
{
    ptr_t Page_Cnt;
    ptr_t Mem_Cnt;
    ptr_t Page_Start;
    ptr_t Page_End;
    ptr_t* Page_Num;
    ptr_t Max_Pages;
    ptr_t Max_Mem;

    Page_Num=Malloc(sizeof(ptr_t)*Num);
    if(Page_Num==0)
        EXIT_FAIL("Page count buffer allocation failed.");
    memset(Page_Num, 0, (size_t)(sizeof(ptr_t)*Num));

    /* Use the attribute of the block that covers most pages */
    for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
    {
        for(Page_Cnt=0;Page_Cnt<POW2(Pgtbl->Num_Order);Page_Cnt++)
        {
            Page_Start=Pgtbl->Start_Addr+Page_Cnt*POW2(Pgtbl->Size_Order);
            Page_End=Page_Start+(POW2(Pgtbl->Size_Order)-1);

            if((Mem[Mem_Cnt].Start<=Page_Start)&&((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))>=Page_End))
                Page_Num[Mem_Cnt]++;
        }
    }
    
    Max_Pages=0;
    for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
    {
        if(Page_Num[Mem_Cnt]>Max_Pages)
        {
            Max_Pages=Page_Num[Mem_Cnt];
            Max_Mem=Mem_Cnt;
        }
    }

    /* Is there anything that we should map? If no, we return early */
    if(Max_Pages==0)
        return;
    /* The attribute is the most pronounced memory block's */
    Pgtbl->Attr=Mem[Max_Mem].Attr;

    /* Map whatever we can map, and postpone whatever we will have to postpone */
    for(Page_Cnt=0;Page_Cnt<POW2(Pgtbl->Num_Order);Page_Cnt++)
    {
        Page_Start=Pgtbl->Start_Addr+Page_Cnt*POW2(Pgtbl->Size_Order);
        Page_End=Page_Start+(POW2(Pgtbl->Size_Order)-1);

        /* Can this compartment be mapped? It can if there is one segment covering the range */
        for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
        {
            if((Mem[Mem_Cnt].Start<=Page_Start)&&((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))>=Page_End))
            {
                /* The attribute must be the same as what we map */
                if(Pgtbl->Attr==Mem[Mem_Cnt].Attr)
                    Pgtbl->Mapping[Page_Cnt]=A7M_PGT_MAPPED;
            }
        }
    }
}
/* End Function:A7M_Map_Page *************************************************/

/* Begin Function:A7M_Map_Pgdir ***********************************************
Description : Map page directories into the page table. 
Input       : struct Proc_Info* Proc - The process we are generating pgtbls for.
              struct A7M_Info* A7M - The chip structure.
              struct Mem_Info* Mem - The memory block list.
              ptr_t Num - The number of memory blocks in the list.
              struct A7M_Pgtbl* Pgtbl - The current page table.
Output      : struct A7M_Pgtbl* Pgtbl - The updated current page table.
Return      : None.
******************************************************************************/
void A7M_Map_Pgdir(struct Proc_Info* Proc, struct A7M_Info* A7M, 
                   struct Mem_Info* Mem, ptr_t Num, struct A7M_Pgtbl* Pgtbl)
{
    ptr_t Page_Cnt;
    ptr_t Mem_Cnt;
    ptr_t Page_Start;
    ptr_t Page_End;
    ptr_t Mem_Num;
    struct Mem_Info* Mem_List;
    ptr_t Mem_List_Ptr;

    for(Page_Cnt=0;Page_Cnt<POW2(Pgtbl->Num_Order);Page_Cnt++)
    {
        Page_Start=Pgtbl->Start_Addr+Page_Cnt*POW2(Pgtbl->Size_Order);
        Page_End=Page_Start+(POW2(Pgtbl->Size_Order)-1);

        if(Pgtbl->Mapping[Page_Cnt]==A7M_PGT_NOTHING)
        {
            /* See if any residue memory list are here */
            Mem_Num=0;
            for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
            {
                if((Mem[Mem_Cnt].Start>Page_End)||((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))<Page_Start))
                    continue;
                Mem_Num++;
            }

            if(Mem_Num==0)
                continue;

            Mem_List=Malloc(sizeof(struct Mem_Info)*Mem_Num);
            if(Mem_List==0)
                EXIT_FAIL("Memory list allocation failed.");
                
            /* Collect the memory list */
            Mem_List_Ptr=0;
            for(Mem_Cnt=0;Mem_Cnt<Num;Mem_Cnt++)
            {
                if((Mem[Mem_Cnt].Start>Page_End)||((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))<Page_Start))
                    continue;

                /* Round anything inside to this range */
                if(Mem[Mem_Cnt].Start<Page_Start)
                    Mem_List[Mem_List_Ptr].Start=Page_Start;
                else
                    Mem_List[Mem_List_Ptr].Start=Mem[Mem_Cnt].Start;

                if((Mem[Mem_Cnt].Start+(Mem[Mem_Cnt].Size-1))>Page_End)
                    Mem_List[Mem_List_Ptr].Size=Page_End-Mem_List[Mem_List_Ptr].Start+1;
                else
                    Mem_List[Mem_List_Ptr].Size=Mem[Mem_Cnt].Start+Mem[Mem_Cnt].Size-Mem_List[Mem_List_Ptr].Start;

                Mem_List[Mem_List_Ptr].Attr=Mem[Mem_Cnt].Attr;
                Mem_List[Mem_List_Ptr].Type=Mem[Mem_Cnt].Type;
                /* The alignment is no longer important */
                Mem_List_Ptr++;
            }
            if(Mem_List_Ptr!=Mem_Num)
                EXIT_FAIL("Internal bug occurred at page table allocator.");

            Pgtbl->Mapping[Page_Cnt]=A7M_Gen_Pgtbl(Proc, A7M, Mem_List, Mem_Num, Pgtbl->Size_Order);
        }
    }
}
/* End Function:A7M_Map_Pgdir ************************************************/

/* Begin Function:A7M_Gen_Pgtbl ***********************************************
Description : Recursively construct the page table for the ARMv7-M port.
              This also allocates capid for page tables.
              We have no idea at all how many page tables we are gonna have,
              thus the A7M Pgtbl_Captbl needs to be preallocated with a very large
              number, say, 4096.
Input       : struct Proc_Info* Proc - The process we are generating pgtbls for.
              struct A7M_Info* A7M - The chip structure.
              struct Mem_Info* Mem - The struct containing memory segments to fit
                                     into this level (and below).
              ptr_t Num - The number of memory segments to fit in.
              ptr_t Total_Max - The maximum total order of the page table, cannot
                                be exceeded when deciding the total order of
                                the page table.
Output      : None.
Return      : struct A7M_Pgtbl* - The page table structure returned.
******************************************************************************/
struct A7M_Pgtbl* A7M_Gen_Pgtbl(struct Proc_Info* Proc, struct A7M_Info* A7M,
                                struct Mem_Info* Mem, ptr_t Num, ptr_t Total_Max)
{
    ptr_t Total_Order;
    struct A7M_Pgtbl* Pgtbl;
    static ptr_t Capid=0;
    s8 Buf[16];

    /* Allocate the page table data structure */
    Pgtbl=Malloc(sizeof(struct A7M_Pgtbl));
    if(Pgtbl==0)
        EXIT_FAIL("Page table data structure allocation failed.");
    memset(Pgtbl, 0, sizeof(struct A7M_Pgtbl));

    /* Allocate the capid for this page table */
    Pgtbl->RVM_Capid=Capid;
    sprintf(Buf,"%lld",Pgtbl->RVM_Capid);
    Pgtbl->RVM_Capid_Macro=Make_Macro("RVM_PROC_",Proc->Name,"_PGTBL",Buf);
    A7M->Pgtbl_Captbl[Capid].Proc=Proc;
    A7M->Pgtbl_Captbl[Capid].Cap=(void*)Pgtbl;
    Capid++;
    A7M->Pgtbl_Captbl_Front=Capid;
    if(Capid>A7M_PGTBL_CAPTBL_SIZE)
        EXIT_FAIL("Too many page tables allocated, exceeded the given pgtbl captbl size.");

    /* Total order and start address of the page table */
    Total_Order=A7M_Total_Order(Mem, Num, &(Pgtbl->Start_Addr));
    /* See if this will violate the extension limit */
    if(Total_Order>Total_Max)
        EXIT_FAIL("Memory segment too small, cannot find a reasonable placement.");
    /* Number order */
    Pgtbl->Num_Order=A7M_Num_Order(Mem, Num, Total_Order, Pgtbl->Start_Addr);
    /* Size order */
    Pgtbl->Size_Order=Total_Order-Pgtbl->Num_Order;
    /* Map in all pages */
    A7M_Map_Page(Mem, Num, Pgtbl);
    /* Map in all page directories - recursive */
    A7M_Map_Pgdir(Proc, A7M, Mem, Num, Pgtbl);

    return Pgtbl;
}
/* End Function:A7M_Gen_Pgtbl ************************************************/

/* Begin Function:A7M_Copy_Files **********************************************
Description : Copy all necessary files to the destination folder. This is generic;
              Format-specific files will be copied and generated in the format-specific
              generator.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The platform-specific project structure.
              s8_t* RME_Path - The RME root folder path.
              s8_t* RVM_Path - The RVM root folder path.
              s8_t* Output_Path - The output folder path.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Copy_Files(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M,
                    s8_t* RME_Path, s8_t* RVM_Path, s8_t* Output_Path)
{
    s8_t* Buf1;
    s8_t* Buf2;

    /* Allocate the buffer */
    Buf1=Malloc(4096);
    Buf2=Malloc(4096);

    /* Perhaps copy some other manuals, etc */

    Free(Buf1);
    Free(Buf2);
}
/* End Function:A7M_Copy_Files ***********************************************/

/* Begin Function:A7M_Gen_Keil_Proj *******************************************
Description : Generate the keil project for ARMv7-M architectures.
Input       : FILE* Keil - The file pointer to the project file.
              s8_t* Target - The target executable name for the project.
              s8_t* Device - The device name, must match the keil list.
              s8_t* Vendor - The vendor name, must match the keil list.
              s8_t* CPU_Type - The CPU type, must match the keil definitions.
                             "Cortex-M0+", "Cortex-M3", "Cortex-M4", "Cortex-M7".
              s8_t* FPU_Type - The FPU type, must match the keil definition.
                             "", "FPU2", "FPU3(SP)", "FPU3(DP)" 
              s8_t* Endianness - The endianness. "ELITTLE" "EBIG"
              ptr_t Timeopt - Set to 1 when optimizing for time.
              ptr_t Opt - Optimization levels, 1 to 4 corresponds to O0-O3.
              s8_t** Includes - The include paths.
              ptr_t Include_Num - The number of include paths.
              s8_t** Paths - The path of the files.
              s8_t** Files - The file names.
              ptr_t File_Num - The number of files.
Output      : FILE* Keil - The file pointer to the project file.
Return      : None.
******************************************************************************/
void A7M_Gen_Keil_Proj(FILE* Keil,
                       s8_t* Target, s8_t* Device, s8_t* Vendor, 
                       s8_t* CPU_Type, s8_t* FPU_Type, s8_t* Endianness,
                       ptr_t Timeopt, ptr_t Opt,
                       s8_t** Includes, ptr_t Include_Num,
                       s8_t** Paths, s8_t** Files, ptr_t File_Num)
{
    ptr_t Include_Cnt;
    ptr_t File_Cnt;
    s8_t* Dlloption;

    if(strcmp(CPU_Type, "Cortex-M0+")==0)
        Dlloption="-pCM0+";
    else if(strcmp(CPU_Type, "Cortex-M3")==0)
        Dlloption="-pCM3";
    else if(strcmp(CPU_Type, "Cortex-M4")==0)
        Dlloption="-pCM4";
    else if(strcmp(CPU_Type, "Cortex-M7")==0)
        Dlloption="-pCM7";
    else
        EXIT_FAIL("Internal CPU variant invalid.");

    fprintf(Keil, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n");
    fprintf(Keil, "<Project xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"project_projx.xsd\">\n");
    fprintf(Keil, "  <SchemaVersion>2.1</SchemaVersion>\n");
    fprintf(Keil, "  <Header>### uVision Project, (C) Keil Software</Header>\n");
    fprintf(Keil, "  <Targets>\n");
    fprintf(Keil, "    <Target>\n");
    fprintf(Keil, "      <TargetName>%s</TargetName>\n", Target);
    fprintf(Keil, "      <ToolsetNumber>0x4</ToolsetNumber>\n");
    fprintf(Keil, "      <ToolsetName>ARM-ADS</ToolsetName>\n");
    fprintf(Keil, "      <pCCUsed>ARMCC</pCCUsed>\n");
    fprintf(Keil, "      <uAC6>0</uAC6>\n");
    fprintf(Keil, "      <TargetOption>\n");
    fprintf(Keil, "        <TargetCommonOption>\n");
    fprintf(Keil, "          <Device>%s</Device>\n", Device);
    fprintf(Keil, "          <Vendor>%s</Vendor>\n", Vendor);
    fprintf(Keil, "          <Cpu>IRAM(0x08000000,0x10000) IROM(0x20000000,0x10000) CPUTYPE(\"%s\") %s CLOCK(12000000) %s</Cpu>\n", CPU_Type, FPU_Type, Endianness);
    fprintf(Keil, "          <OutputDirectory>.\\Objects\\</OutputDirectory>\n");
    fprintf(Keil, "          <OutputName>%s</OutputName>\n", Target);
    fprintf(Keil, "          <CreateExecutable>1</CreateExecutable>\n");
    fprintf(Keil, "          <CreateHexFile>1</CreateHexFile>\n");
    fprintf(Keil, "          <DebugInformation>1</DebugInformation>\n");
    fprintf(Keil, "          <BrowseInformation>1</BrowseInformation>\n");
    fprintf(Keil, "          <ListingPath>.\\Listings\\</ListingPath>\n");
    fprintf(Keil, "          <HexFormatSelection>1</HexFormatSelection>\n");
    fprintf(Keil, "          <AfterMake>\n");
    fprintf(Keil, "            <RunUserProg1>0</RunUserProg1>\n");
    fprintf(Keil, "            <RunUserProg2>0</RunUserProg2>\n");
    fprintf(Keil, "            <UserProg1Name></UserProg1Name>\n");
    fprintf(Keil, "            <UserProg2Name></UserProg2Name>\n");
    fprintf(Keil, "            <UserProg1Dos16Mode>0</UserProg1Dos16Mode>\n");
    fprintf(Keil, "            <UserProg2Dos16Mode>0</UserProg2Dos16Mode>\n");
    fprintf(Keil, "            <nStopA1X>0</nStopA1X>\n");
    fprintf(Keil, "            <nStopA2X>0</nStopA2X>\n");
    fprintf(Keil, "          </AfterMake>\n");
    fprintf(Keil, "        </TargetCommonOption>\n");
    fprintf(Keil, "        <CommonProperty>\n");
    fprintf(Keil, "          <UseCPPCompiler>0</UseCPPCompiler>\n");
    fprintf(Keil, "          <RVCTCodeConst>0</RVCTCodeConst>\n");
    fprintf(Keil, "          <RVCTZI>0</RVCTZI>\n");
    fprintf(Keil, "          <RVCTOtherData>0</RVCTOtherData>\n");
    fprintf(Keil, "          <ModuleSelection>0</ModuleSelection>\n");
    fprintf(Keil, "          <IncludeInBuild>1</IncludeInBuild>\n");
    fprintf(Keil, "          <AlwaysBuild>0</AlwaysBuild>\n");
    fprintf(Keil, "          <GenerateAssemblyFile>0</GenerateAssemblyFile>\n");
    fprintf(Keil, "          <AssembleAssemblyFile>0</AssembleAssemblyFile>\n");
    fprintf(Keil, "          <PublicsOnly>0</PublicsOnly>\n");
    fprintf(Keil, "          <StopOnExitCode>3</StopOnExitCode>\n");
    fprintf(Keil, "          <CustomArgument></CustomArgument>\n");
    fprintf(Keil, "          <IncludeLibraryModules></IncludeLibraryModules>\n");
    fprintf(Keil, "          <ComprImg>1</ComprImg>\n");
    fprintf(Keil, "        </CommonProperty>\n");
    fprintf(Keil, "        <DllOption>\n");
    fprintf(Keil, "          <SimDllName>SARMCM3.DLL</SimDllName>\n");
    fprintf(Keil, "          <SimDllArguments> -REMAP -MPU</SimDllArguments>\n");
    fprintf(Keil, "          <SimDlgDll>DCM.DLL</SimDlgDll>\n");
    fprintf(Keil, "          <SimDlgDllArguments>%s</SimDlgDllArguments>\n", Dlloption);
    fprintf(Keil, "          <TargetDllName>SARMCM3.DLL</TargetDllName>\n");
    fprintf(Keil, "          <TargetDllArguments> -MPU</TargetDllArguments>\n");
    fprintf(Keil, "          <TargetDlgDll>TCM.DLL</TargetDlgDll>\n");
    fprintf(Keil, "          <TargetDlgDllArguments>%s</TargetDlgDllArguments>\n", Dlloption);
    fprintf(Keil, "        </DllOption>\n");
    fprintf(Keil, "        <TargetArmAds>\n");
    fprintf(Keil, "          <ArmAdsMisc>\n");
    fprintf(Keil, "            <useUlib>1</useUlib>\n");
    fprintf(Keil, "            <OptFeed>0</OptFeed>\n");
    fprintf(Keil, "          </ArmAdsMisc>\n");
    fprintf(Keil, "          <Cads>\n");
    fprintf(Keil, "            <interw>1</interw>\n");
    fprintf(Keil, "            <Optim>%lld</Optim>\n", Opt);
    fprintf(Keil, "            <oTime>%d</oTime>\n", (Timeopt!=0));
    fprintf(Keil, "            <SplitLS>0</SplitLS>\n");
    fprintf(Keil, "            <OneElfS>1</OneElfS>\n");
    fprintf(Keil, "            <Strict>0</Strict>\n");
    fprintf(Keil, "            <EnumInt>1</EnumInt>\n");
    fprintf(Keil, "            <PlainCh>1</PlainCh>\n");
    fprintf(Keil, "            <Ropi>0</Ropi>\n");
    fprintf(Keil, "            <Rwpi>0</Rwpi>\n");
    fprintf(Keil, "            <wLevel>2</wLevel>\n");
    fprintf(Keil, "            <uThumb>0</uThumb>\n");
    fprintf(Keil, "            <uSurpInc>0</uSurpInc>\n");
    fprintf(Keil, "            <uC99>1</uC99>\n");
    fprintf(Keil, "            <uGnu>1</uGnu>\n");
    fprintf(Keil, "            <useXO>0</useXO>\n");
    fprintf(Keil, "            <v6Lang>1</v6Lang>\n");
    fprintf(Keil, "            <v6LangP>1</v6LangP>\n");
    fprintf(Keil, "            <vShortEn>1</vShortEn>\n");
    fprintf(Keil, "            <vShortWch>1</vShortWch>\n");
    fprintf(Keil, "            <v6Lto>0</v6Lto>\n");
    fprintf(Keil, "            <v6WtE>0</v6WtE>\n");
    fprintf(Keil, "            <v6Rtti>0</v6Rtti>\n");
    fprintf(Keil, "            <VariousControls>\n");
    fprintf(Keil, "              <MiscControls></MiscControls>\n");
    fprintf(Keil, "              <Define></Define>\n");
    fprintf(Keil, "              <Undefine></Undefine>\n");
    fprintf(Keil, "              <IncludePath>");
    /* Print all include paths for C */
    for(Include_Cnt=0;Include_Cnt<Include_Num;Include_Cnt++)
    {
        fprintf(Keil, "%s", Includes[Include_Cnt]);
        if(Include_Cnt<(Include_Num-1))
            fprintf(Keil, ";");
    }
    fprintf(Keil, "</IncludePath>\n");
    fprintf(Keil, "            </VariousControls>\n");
    fprintf(Keil, "          </Cads>\n");
    fprintf(Keil, "          <Aads>\n");
    fprintf(Keil, "            <interw>1</interw>\n");
    fprintf(Keil, "            <Ropi>0</Ropi>\n");
    fprintf(Keil, "            <Rwpi>0</Rwpi>\n");
    fprintf(Keil, "            <thumb>0</thumb>\n");
    fprintf(Keil, "            <SplitLS>0</SplitLS>\n");
    fprintf(Keil, "            <SwStkChk>0</SwStkChk>\n");
    fprintf(Keil, "            <NoWarn>0</NoWarn>\n");
    fprintf(Keil, "            <uSurpInc>0</uSurpInc>\n");
    fprintf(Keil, "            <useXO>0</useXO>\n");
    fprintf(Keil, "            <uClangAs>0</uClangAs>\n");
    fprintf(Keil, "            <VariousControls>\n");
    fprintf(Keil, "              <MiscControls></MiscControls>\n");
    fprintf(Keil, "              <Define></Define>\n");
    fprintf(Keil, "              <Undefine></Undefine>\n");
    fprintf(Keil, "              <IncludePath>");
    /* Print all include paths for assembly */
    for(Include_Cnt=0;Include_Cnt<Include_Num;Include_Cnt++)
    {
        fprintf(Keil, "%s", Includes[Include_Cnt]);
        if(Include_Cnt<(Include_Num-1))
            fprintf(Keil, ";");
    }
    fprintf(Keil, "</IncludePath>\n");
    fprintf(Keil, "            </VariousControls>\n");
    fprintf(Keil, "          </Aads>\n");
    fprintf(Keil, "          <LDads>\n");
    fprintf(Keil, "            <umfTarg>0</umfTarg>\n");
    fprintf(Keil, "            <Ropi>0</Ropi>\n");
    fprintf(Keil, "            <Rwpi>0</Rwpi>\n");
    fprintf(Keil, "            <noStLib>0</noStLib>\n");
    fprintf(Keil, "            <RepFail>1</RepFail>\n");
    fprintf(Keil, "            <useFile>0</useFile>\n");
    fprintf(Keil, "            <TextAddressRange>0x08000000</TextAddressRange>\n");
    fprintf(Keil, "            <DataAddressRange>0x20000000</DataAddressRange>\n");
    fprintf(Keil, "            <pXoBase></pXoBase>\n");
    fprintf(Keil, "            <ScatterFile>.\\Objects\\%s.sct</ScatterFile>\n", Target);
    fprintf(Keil, "            <IncludeLibs></IncludeLibs>\n");
    fprintf(Keil, "            <IncludeLibsPath></IncludeLibsPath>\n");
    fprintf(Keil, "            <Misc></Misc>\n");
    fprintf(Keil, "            <LinkerInputFile></LinkerInputFile>\n");
    fprintf(Keil, "            <DisabledWarnings></DisabledWarnings>\n");
    fprintf(Keil, "          </LDads>\n");
    fprintf(Keil, "        </TargetArmAds>\n");
    fprintf(Keil, "      </TargetOption>\n");
    fprintf(Keil, "      <Groups>\n");
    /* Print all files. We only have two groups in all cases. */
    fprintf(Keil, "        <Group>\n");
    fprintf(Keil, "          <GroupName>%s</GroupName>\n", Target);
    fprintf(Keil, "          <Files>\n");
    for(File_Cnt=0;File_Cnt<File_Num;File_Cnt++)
    {
        fprintf(Keil, "            <File>\n");
        fprintf(Keil, "              <FileName>%s</FileName>\n", Files[File_Cnt]);
        if(Files[File_Cnt][strlen(Files[File_Cnt])-1]=='c')
            fprintf(Keil, "              <FileType>1</FileType>\n");
        else
            fprintf(Keil, "              <FileType>2</FileType>\n");
        fprintf(Keil, "              <FilePath>%s/%s</FilePath>\n", Paths[File_Cnt], Files[File_Cnt]);
        fprintf(Keil, "            </File>\n");
    }
    fprintf(Keil, "          </Files>\n");
    fprintf(Keil, "        </Group>\n");
    fprintf(Keil, "        <Group>\n");
    fprintf(Keil, "          <GroupName>User</GroupName>\n");
    fprintf(Keil, "        </Group>\n");
    fprintf(Keil, "      </Groups>\n");
    fprintf(Keil, "    </Target>\n");
    fprintf(Keil, "  </Targets>\n");
    fprintf(Keil, "</Project>\n");
}
/* End Function:A7M_Gen_Keil_Proj ********************************************/

/* Begin Function:A7M_Gen_Keil_RME ********************************************
Description : Generate the RME files for keil uvision. 
              This includes the platform-specific assembly file and the scatter.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The port specific structure.
              s8_t* RME_Path - The RME root folder path.
              s8_t* Output_Path - The output folder path.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Keil_RME(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M, 
                      s8_t* RME_Path, s8_t* Output_Path)
{
    s8_t* Buf1;
    s8_t* Buf2;

    /* Allocate the buffer */
    Buf1=Malloc(sizeof(s8)*4096);
    if(Buf1==0)
        EXIT_FAIL("Buffer allocation failed");
    Buf2=Malloc(sizeof(s8)*4096);
    if(Buf2==0)
        EXIT_FAIL("Buffer allocation failed");

    /* The toolchain specific assembler file */
    if(A7M->CPU_Type==A7M_CPU_CM0P)
    {
        sprintf(Buf1,"%s/M7M1_MuEukaron/MEukaron/Platform/A7M/rme_platform_a7m0p_asm.s", Output_Path);
        sprintf(Buf2,"%s/MEukaron/Platform/A7M/rme_platform_a7m0p_asm.s", RME_Path);
    }
    else
    {
        sprintf(Buf1,"%s/M7M1_MuEukaron/MEukaron/Platform/A7M/rme_platform_a7m_asm.s", Output_Path);
        sprintf(Buf2,"%s/MEukaron/Platform/A7M/rme_platform_a7m_asm.s", RME_Path);
    }
    if(Copy_File(Buf1, Buf2)!=0)
        EXIT_FAIL("File copying failed.");
 
    /* The linker script */

    Free(Buf1);
    Free(Buf2);
}
/* End Function:A7M_Gen_Keil_RME *********************************************/

/* Begin Function:A7M_Gen_Keil_RVM ********************************************
Description : Generate the RVM files for keil uvision. 
              This includes the platform-specific assembly file and the scatter.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The port specific structure.
              s8_t* RVM_Path - The RVM root folder path.
              s8_t* Output_Path - The output folder path.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Keil_RVM(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M, 
                      s8_t* RVM_Path, s8_t* Output_Path)
{

}
/* End Function:A7M_Gen_Keil_RVM *********************************************/

/* Begin Function:A7M_Gen_Keil_Proc *******************************************
Description : Generate the process files for keil uvision. 
              This includes the platform-specific assembly file and the scatter.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The port specific structure.
              s8_t* RME_Path - The RME root folder path.
              s8_t* RVM_Path - The RVM root folder path.
              s8_t* Output_Path - The output folder path.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Keil_Proc(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M, 
                       s8_t* RME_Path, s8_t* RVM_Path, s8_t* Output_Path)
{

}
/* End Function:A7M_Gen_Keil_Proc ********************************************/

/* Begin Function:A7M_Gen_Keil ************************************************
Description : Generate the keil project for ARMv7-M. 
              Keil projects include three parts: 
              .uvmpw (the outside workspace)
              .uvprojx (the specific project files)
              .uvoptx (the options for some unimportant stuff)
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The port specific structure.
              s8_t* RME_Path - The RME root folder path.
              s8_t* RVM_Path - The RVM root folder path.
              s8_t* Output_Path - The output folder path.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Keil(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M,
                  s8_t* RME_Path, s8_t* RVM_Path, s8_t* Output_Path)
{
    /* Common for all projects */
    s8_t* Device;
    s8_t* Vendor;
    s8_t* CPU_Type;
    s8_t* FPU_Type;
    s8_t* Endianness;
    /* Project specific */
    FILE* Keil;
    s8_t* Proj_Path;
    s8_t* Target;
    ptr_t Opt;
    ptr_t Timeopt;
    /* Include path and project file buffer - we never exceed 8 */
    s8_t* Includes[8];
    ptr_t Include_Num;
    s8_t* Paths[8];
    s8_t* Files[8];
    ptr_t File_Num;

    /* Allocate project path buffer */
    Proj_Path=Malloc(4096);
    if(Proj_Path==0)
        EXIT_FAIL("Project path allocation failed");

    /* Decide process specific macros - only STM32 is like this */
    if((strncmp(Proj->Chip,"STM32", 5)==0))
    {
        if(strncmp(Proj->Chip,"STM32F1", 7)==0)
            Device=Proj->Chip;
        else
        {
            Device=Malloc(((ptr_t)strlen(Proj->Fullname))+1);
            strcpy(Device, Proj->Fullname);
            /* Except for STM32F1, all chips end with suffix, and the last number is substituted with 'x' */
            Device[strlen(Proj->Fullname)-1]='x';
        }
    }
    else
        Device=Proj->Chip;

    Vendor=Chip->Vendor;

    switch(A7M->CPU_Type)
    {
        case A7M_CPU_CM0P:CPU_Type="Cortex-M0+";break;
        case A7M_CPU_CM3:CPU_Type="Cortex-M3";break;
        case A7M_CPU_CM4:CPU_Type="Cortex-M4";break;
        case A7M_CPU_CM7:CPU_Type="Cortex-M7";break;
        default:EXIT_FAIL("Wrong CPU type selected.");break;
    }

    switch(A7M->FPU_Type)
    {
        case A7M_FPU_NONE:FPU_Type="";break;
        case A7M_FPU_FPV4:FPU_Type="FPU2";break;
        case A7M_FPU_FPV5_SP:FPU_Type="FPU3(SFPU)";break;
        case A7M_FPU_FPV5_DP:FPU_Type="FPU3(DFPU)";break;
        default:EXIT_FAIL("Wrong FPU type selected.");break;
    }

    if(A7M->Endianness==A7M_END_LITTLE)
        Endianness="ELITTLE";
    else if(A7M->Endianness==A7M_END_BIG)
        Endianness="EBIG";
    else
        EXIT_FAIL("Wrong endianness specified.");

    /* Generate the RME keil project first */
    sprintf(Proj_Path, "%s/M7M1_MuEukaron/Project/%s_RME.uvprojx", Output_Path, Proj->Name);
    Keil=fopen(Proj_Path, "wb");
    Target="RME";
    Opt=Proj->RME.Comp.Opt+1;
    Timeopt=Proj->RME.Comp.Prio;
    /* Allocate the include list */
    Includes[0]=".";
    Includes[1]="./Source";
    Includes[2]="./Include";
    Includes[3]="../MEukaron/Include";
    Include_Num=4;
    /* Allocate the file list */
    Paths[0]="./Source";
    Files[0]="rme_boot.c";
    Paths[1]="./Source";
    Files[1]="rme_user.c";
    Paths[2]="../MEukaron/Kernel";
    Files[2]="rme_kernel.c";
    Paths[3]="../MEukaron/Platform/A7M";
    Files[3]="rme_platform_a7m.c";
    if(A7M->CPU_Type==A7M_CPU_CM0P)
    {
        Paths[4]="../MEukaron/Platform/A7M";
        Files[4]="rme_platform_a7m0p_asm.s";
    }
    else
    {
        Paths[4]="../MEukaron/Platform/A7M";
        Files[4]="rme_platform_a7m_asm.s";
    }
    File_Num=5;
    /* Generate whatever files that are keil specific */
    A7M_Gen_Keil_RME(Proj, Chip, A7M, RME_Path, Output_Path);
    /* Actually generate the project */
    A7M_Gen_Keil_Proj(Keil, Target, Device, Vendor, CPU_Type, FPU_Type, Endianness,
                      Timeopt, Opt,
                      Includes, Include_Num,
                      Paths, Files, File_Num);
    fclose(Keil);
    /* Project files needs to be generated as well */
    A7M_Gen_Keil_RME(Proj, Chip, A7M, RME_Path, Output_Path);

    /* Generate the RVM keil project then */

    /* Generate all other projects one by one */

    /* Finally, generate the uvmpw at the root folder */
    
    /* Free project buffer */
    Free(Proj_Path);
}
/* End Function:A7M_Gen_Keil *************************************************/

/* Begin Function:A7M_Gen_Makefile ********************************************
Description : Generate the makefile project for ARMv7-M. 
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              struct A7M_Info* A7M - The port specific structure.
              ptr_t Output_Type - The output type.
              s8_t* Output_Path - The output folder path.
              s8_t* RME_Path - The RME root folder path.
              s8_t* RVM_Path - The RVM root folder path.
Output      : None.
Return      : struct A7M_Pgtbl* - The page table structure returned.
******************************************************************************/
void A7M_Gen_Makefile(struct Proj_Info* Proj, struct Chip_Info* Chip, struct A7M_Info* A7M,
                      ptr_t Output_Type, s8_t* Output_Path, s8_t* RME_Path, s8_t* RVM_Path)
{
    /* Generate the makefile project */
}
/* End Function:A7M_Gen_Makefile *********************************************/

/* Begin Function:A7M_Gen_Check ***********************************************
Description : Check if the input is really valid for the ARMv7-M port.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Check(struct Proj_Info* Proj, struct Chip_Info* Chip)
{
    ptr_t Proc_Cnt;
    ptr_t Mem_Cnt;

    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        /* Check memory blocks - they must be at least readable */
        for(Mem_Cnt=0;Mem_Cnt<Proj->Proc[Proc_Cnt].Mem_Num;Mem_Cnt++)
        {
            if((Proj->Proc[Proc_Cnt].Mem[Mem_Cnt].Attr&MEM_READ)==0)
                EXIT_FAIL("All memory allocated must be readable on A7M.");
        }
        /* Check if the capability table of processes, plus the extra captbl space
         * required, exceeds 128 - this is simply not allowed for processes. */
        if((Proj->Proc[Proc_Cnt].Captbl_Front+Proj->Proc[Proc_Cnt].Extra_Captbl)>A7M_PROC_CAPTBL_LIMIT)
            EXIT_FAIL("One of the processes have more capabilities in its capability table than allowed.");
    }
}
/* End Function:A7M_Gen_Check ************************************************/

/* Begin Function:A7M_Gen_Proj ************************************************
Description : Generate the project for Cortex-M based processors.
Input       : struct Proj_Info* Proj - The project structure.
              struct Chip_Info* Chip - The chip structure.
              s8_t* RME_Path - The RME root folder path.
              s8_t* RVM_Path - The RVM root folder path.
              s8_t* Output_Path - The output folder path.
              s8_t* Format - The output format.
Output      : None.
Return      : None.
******************************************************************************/
void A7M_Gen_Proj(struct Proj_Info* Proj, struct Chip_Info* Chip,
                  s8_t* RME_Path, s8_t* RVM_Path, s8_t* Output_Path, s8_t* Format)
{
    ptr_t Proc_Cnt;
    struct A7M_Info* A7M;

    /* Allocate architecture-specific project structure */
    A7M=Malloc(sizeof(struct A7M_Info));
    if(A7M==0)
        EXIT_FAIL("Platform specific project struct allocation failed.");
    /* Parse A7M-specific options */
    A7M_Parse_Options(Proj, Chip, A7M);
    /* Check if we can really create such a system */
    A7M_Gen_Check(Proj, Chip);
    /* Allocate page table global captbl - it is unlikely that this number gets exceeded */
    A7M->Pgtbl_Captbl=Malloc(sizeof(struct RVM_Cap_Info)*A7M_PGTBL_CAPTBL_SIZE);
    /* Allocate page tables for all processes */
    A7M->Pgtbl=Malloc(sizeof(struct A7M_Pgtbl*)*Proj->Proc_Num);
    if(A7M->Pgtbl==0)
        EXIT_FAIL("Project page table allocation failed.");
    for(Proc_Cnt=0;Proc_Cnt<Proj->Proc_Num;Proc_Cnt++)
    {
        A7M->Pgtbl[Proc_Cnt]=A7M_Gen_Pgtbl(&(Proj->Proc[Proc_Cnt]),A7M,
                                           Proj->Proc[Proc_Cnt].Mem,Proj->Proc[Proc_Cnt].Mem_Num,32);
        if(A7M->Pgtbl[Proc_Cnt]==0)
            EXIT_FAIL("Page table generation failed.");
    }

    /* Set up the folder structures, and copy all files to where they should be */
    A7M_Copy_Files(Proj, Chip, A7M, RME_Path, RVM_Path, Output_Path);
    /* Write the files that are tool-independent */
    //A7M_Gen_Scripts(Proj, Chip, A7M, RME_Path, RVM_Path, Output_Path);

	/* Create the folder first and copy all necessary files into whatever possible */
    if(strcmp(Format, "keil")==0)
        A7M_Gen_Keil(Proj, Chip, A7M, RME_Path, RVM_Path, Output_Path);
    else
        EXIT_FAIL("Other projects not supported at the moment.");
}
/* End Function:A7M_Gen_Proj *************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
