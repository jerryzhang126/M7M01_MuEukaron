/******************************************************************************
Filename    : rme_genproc.cpp
Author      : pry
Date        : 16/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The project folder generation class.
******************************************************************************/

/* Includes ******************************************************************/
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

extern "C"
{
#include "xml.h"
#include "pbfs.h"
}

#include "list"
#include "string"
#include "memory"
#include "vector"
#include "stdexcept"

#define __HDR_DEFS__
#include "Main/rme_mcu.hpp"
#include "Main/rme_fsys.hpp"
#include "Main/rme_chip.hpp"
#include "Main/rme_comp.hpp"
#include "Main/rme_raw.hpp"
#include "Main/rme_mem.hpp"

#include "Kobj/rme_kobj.hpp"
#include "Kobj/rme_captbl.hpp"
#include "Kobj/rme_pgtbl.hpp"
#include "Kobj/rme_thd.hpp"
#include "Kobj/rme_inv.hpp"
#include "Kobj/rme_port.hpp"
#include "Kobj/rme_recv.hpp"
#include "Kobj/rme_send.hpp"
#include "Kobj/rme_vect.hpp"
#include "Kobj/rme_proc.hpp"

#include "Main/rme_proj.hpp"

#include "Gen/rme_doc.hpp"
#include "Gen/rme_genproc.hpp"
#undef __HDR_DEFS__

#define __HDR_CLASSES__
#include "Main/rme_mcu.hpp"
#include "Main/rme_fsys.hpp"
#include "Main/rme_chip.hpp"
#include "Main/rme_comp.hpp"
#include "Main/rme_raw.hpp"
#include "Main/rme_mem.hpp"

#include "Kobj/rme_kobj.hpp"
#include "Kobj/rme_captbl.hpp"
#include "Kobj/rme_pgtbl.hpp"
#include "Kobj/rme_thd.hpp"
#include "Kobj/rme_inv.hpp"
#include "Kobj/rme_port.hpp"
#include "Kobj/rme_recv.hpp"
#include "Kobj/rme_send.hpp"
#include "Kobj/rme_vect.hpp"
#include "Kobj/rme_proc.hpp"

#include "Main/rme_proj.hpp"

#include "Gen/rme_doc.hpp"
#include "Gen/rme_genproc.hpp"
#undef __HDR_CLASSES__
/* End Includes **************************************************************/
namespace rme_mcu
{
/* Begin Function:Proc_Gen::Folder ********************************************
Description : Setup the generic folder contents for process.
Input       : class Proc* Proc - The process to generate for.
Output      : None.
Return      : None.
******************************************************************************/
void Proc_Gen::Folder(class Proc* Proc)
{
    /* RME directory */
    this->Main->Dstfs->Make_Dir("Proc_%s", Proc->Name->c_str());
    this->Main->Dstfs->Make_Dir("Proc_%s/Include");
    this->Main->Dstfs->Make_Dir("Proc_%s/Include/%s",this->Main->Proj->Plat_Name->c_str());
    this->Main->Dstfs->Make_Dir("Proc_%s/Include/%s",this->Main->Proj->Plat_Name->c_str());
    this->Main->Dstfs->Make_Dir("Proc_%s/Include/%s",
                                this->Main->Proj->Plat_Name->c_str(),this->Main->Chip->Chip_Class->c_str());
    this->Main->Dstfs->Make_Dir("Proc_%s/Source/%s",this->Main->Proj->Plat_Name->c_str());
    this->Main->Dstfs->Make_Dir("Proc_%s/Source");
    this->Main->Dstfs->Make_Dir("Proc_%s/Include");
    
    /* Copy kernel file, kernel header, platform file, platform header, and chip headers - do we need to? */
    this->Main->Srcfs->Copy_File("M7M2_MuAmmonite/MAmmonite/Kernel/rme_kernel.c");
    this->Main->Srcfs->Copy_File("M7M2_MuAmmonite/MAmmonite/Include/rme.h");
    this->Main->Srcfs->Copy_File("M7M2_MuAmmonite/MAmmonite/Include/Kernel/rme_kernel.h");

    /* The toolchain specific assembly be created when we are playing with toolchains */
    this->Main->Srcfs->Copy_File("M7M2_MuAmmonite/MAmmonite/Platform/%s/rme_platform_%s.c",
                                 this->Main->Proj->Plat_Name->c_str(),this->Main->Proj->Plat_Lower->c_str());
    this->Main->Srcfs->Copy_File("M7M2_MuAmmonite/MAmmonite/Include/Platform/%s/rme_platform_%s.h",
                                 this->Main->Proj->Plat_Name->c_str(),this->Main->Proj->Plat_Lower->c_str());
}
/* End Function:Proc_Gen::Folder *********************************************/

/* Begin Function:Proc_Gen::Proc_Hdr ******************************************
Description : Generate the proc_xxx.h. These files are mainly responsible for
              setting up interrupt endpoints.
Input       : class Proc* Proc - The process to generate for.
Output      : None.
Return      : None.
******************************************************************************/
void Proc_Gen::Proc_Hdr(class Proc* Proc)
{
    s8_t Buf[512];
    FILE* File;
    ptr_t Obj_Cnt;
    ret_t Cap_Front;
    ptr_t Capacity;
    class Vect* Vect;
    std::unique_ptr<class Doc> Doc;
    std::unique_ptr<class Para> Para;
    std::unique_ptr<std::string> Line;

    Doc=std::make_unique<class Doc>();

    Doc->Csrc_Desc("rme_boot.h", "The boot-time initialization file header.");

    Para=std::make_unique<class Para>("Doc:Defines");
    Para->Add("/* Defines *******************************************************************/");
    Para->Add("/* Vector endpoint capability tables */");

    /* Vector capability tables */
    Cap_Front=this->Main->Proj->RME->Map->Vect_Cap_Front;
    Capacity=this->Main->Plat->Capacity;
    for(Obj_Cnt=0;Obj_Cnt<this->Main->Proj->RVM->Vect.size();Obj_Cnt+=Capacity)
    {
        sprintf(Buf, "RME_BOOT_CTVECT%lld",Obj_Cnt/Capacity);
        Para->Cdef(std::make_unique<std::string>(Buf), Cap_Front++);
    }
    Para->Add(std::make_unique<std::string>(""));

    /* Vector endpoints */
    Para->Add("/* Vector endpoints */");
    Obj_Cnt=0;
    for(std::unique_ptr<class Cap>& Info:this->Main->Proj->RVM->Vect)
    {
        Vect=static_cast<class Vect*>(Info->Kobj);
        sprintf(Buf, "RME_CAPID(RME_BOOT_CTVECT%lld,%lld)", Obj_Cnt/Capacity, Obj_Cnt%Capacity);
        Para->Cdef(Vect->RME_Macro, std::make_unique<std::string>(Buf));
        Obj_Cnt++;
    }

    Para->Add("/* End Defines ***************************************************************/");

    Doc->Csrc_Foot();

    /* Generate rme_boot.h */
    File=this->Main->Dstfs->Open_File("M7M1_MuEukaron/Project/Include/rme_boot.h");
    Doc->Write(File);
    fclose(File);
}
/* End Function:Proc_Gen::Proc_Hdr *******************************************/

/* Begin Function:Proc_Gen::Proc_Src ******************************************
Description : Generate the proc_xxx.c. These files are mainly responsible for
              setting up interrupt endpoints.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void Proc_Gen::Proc_Src(class Proc* Proc)
{
    FILE* File;
    ptr_t Obj_Cnt;
    class Vect* Vect;
    ptr_t Capacity;
    ptr_t Captbl_Size;
    std::unique_ptr<class Doc> Doc;
    std::unique_ptr<class Para> Para;
    std::unique_ptr<std::string> Line;
    std::vector<std::unique_ptr<std::string>> Input;
    std::vector<std::unique_ptr<std::string>> Output;

    Capacity=this->Main->Plat->Capacity;

    Doc=std::make_unique<class Doc>();

    Doc->Csrc_Desc("rme_boot.c", "The boot-time initialization file.");

    /* Print all header includes */
    Para=std::make_unique<class Para>("Doc:Includes");
    Para->Add("/* Includes ******************************************************************/");
    //Include(Para);
    Para->Add("#include \"rme_boot.h\"");
    Para->Add("/* End Includes **************************************************************/");
    Doc->Add(std::move(Para));

    /* Global variables */
    Para=std::make_unique<class Para>("Doc:Private Global Variables");
    Para->Add("/* Private Global Variables **************************************************/");
    for(std::unique_ptr<class Cap>& Info:this->Main->Proj->RVM->Vect)
    {
        Vect=static_cast<class Vect*>(Info->Kobj);
        Para->Add("static struct RME_Sig_Struct* %s_Vect_Sig;\n", Vect->Name->c_str());
    }
    Para->Add("/* End Private Global Variables **********************************************/");
    Doc->Add(std::move(Para));

    /* Private prototypes */
    Para=std::make_unique<class Para>("Doc:Private C Function Prototypes");
    Para->Add("/* Private C Function Prototypes *********************************************/");
    for(std::unique_ptr<class Cap>& Info:this->Main->Proj->RVM->Vect)
    {
        Vect=static_cast<class Vect*>(Info->Kobj);
        Para->Add("static rme_ptr_t RME_Vect_%s_User(rme_ptr_t Int_Num);\n", Vect->Name->c_str());
    }
    Para->Add("/* End Private C Function Prototypes *****************************************/");
    Doc->Add(std::move(Para));

    /* Public prototypes */
    Para=std::make_unique<class Para>("Doc:Public C Function Prototypes");
    Para->Add("/* Public C Function Prototypes **********************************************/");
    Para->Add("void RME_Boot_Vect_Init(struct RME_Cap_Captbl* Captbl, rme_ptr_t Cap_Front, rme_ptr_t Kmem_Front);");
    Para->Add("rme_ptr_t RME_Boot_Vect_Handler(rme_ptr_t Vect_Num);");
    Para->Add("/* End Public C Function Prototypes ******************************************/");
    Doc->Add(std::move(Para));

    /* Boot-time setup routine for the interrupt endpoints */
    Para=std::make_unique<class Para>("Func:RME_Boot_Vect_Init");
    Input.push_back(std::make_unique<std::string>("rme_ptr_t Cap_Front - The current capability table frontier."));
    Input.push_back(std::make_unique<std::string>("rme_ptr_t Kmem_Front - The current kernel absolute memory frontier."));
    Para->Cfunc_Desc("RME_Boot_Vect_Init",
                     "Initialize all the vector endpoints at boot-time.",
                     Input, Output, "None.");
    Input.clear();
    Output.clear();
    Para->Add("void RME_Boot_Vect_Init(struct RME_Cap_Captbl* Captbl, rme_ptr_t Cap_Front, rme_ptr_t Kmem_Front)");
    Para->Add("{");
    Para->Add("    rme_ptr_t Cur_Addr;");
    Para->Add("");
    Para->Add("    /* The address here shall match what is in the generator */");
    Para->Add("    RME_ASSERT(Cap_Front==%lld);", this->Main->Proj->RME->Map->Vect_Cap_Front);
    Para->Add("    RME_ASSERT(Kmem_Front==0x%llX);",
              this->Main->Proj->RME->Map->Vect_Kmem_Front+
              this->Main->Proj->RME->Map->Kmem_Base);
    Para->Add("");
    Para->Add("    Cur_Addr=Kmem_Front;");
    Para->Add("    /* Create all the vector capability tables first */");
    for(Obj_Cnt=0;Obj_Cnt<this->Main->Proj->RVM->Vect.size();Obj_Cnt+=Capacity)
    {
        if(this->Main->Proj->RVM->Vect.size()>=(Obj_Cnt+1)*Capacity)
            Captbl_Size=Capacity;
        else
            Captbl_Size=this->Main->Proj->RVM->Vect.size()%Capacity;

        Para->Add("    RME_ASSERT(_RME_Captbl_Boot_Crt(Captbl, RME_BOOT_CAPTBL, RME_BOOT_CTVECT%lld, Cur_Addr, %lld)==0);\n", 
                  Obj_Cnt/Capacity,Captbl_Size);
        Para->Add("    Cur_Addr+=RME_KOTBL_ROUND(RME_CAPTBL_SIZE(%lld));\n",Captbl_Size);
    }
    Para->Add("");
    Para->Add("    /* Then all the vectors */");
    Obj_Cnt=0;
    for(std::unique_ptr<class Cap>& Info:this->Main->Proj->RVM->Vect)
    {
        Vect=static_cast<class Vect*>(Info->Kobj);
        Para->Add("    %s_Vect_Sig=(struct RME_Sig_Struct*)Cur_Addr;", Vect->Name->c_str());
        Para->Add("    RME_ASSERT(_RME_Sig_Boot_Crt(Captbl, RME_BOOT_CTVECT%lld, %s, Cur_Addr)==0);",
                  Obj_Cnt/Capacity, Vect->RME_Macro->c_str());
        Para->Add("    Cur_Addr+=RME_KOTBL_ROUND(RME_SIG_SIZE);");
        Obj_Cnt++;
    }
    Para->Add("}");
    Para->Cfunc_Foot(std::make_unique<std::string>("RME_Boot_Vect_Init"));
    Doc->Add(std::move(Para));

    /* Print the interrupt relaying function */
    Para=std::make_unique<class Para>("Func:RME_Boot_Vect_Handler");
    Input.push_back(std::make_unique<std::string>("rme_ptr_t Vect_Num - The vector number."));
    Para->Cfunc_Desc("RME_Boot_Vect_Handler",
                     "The interrupt handler entry for all the vectors.",
                     Input, Output, "rme_ptr_t - The number of signals to send to the generic vector endpoint.");
    Input.clear();
    Output.clear();
    Para->Add("rme_ptr_t RME_Boot_Vect_Handler(rme_ptr_t Vect_Num)");
    Para->Add("{");
    Para->Add("    rme_ptr_t Send_Num;");
    Para->Add("");
    Para->Add("    switch(Vect_Num)");
    Para->Add("    {");
    for(std::unique_ptr<class Cap>& Info:this->Main->Proj->RVM->Vect)
    {
        Vect=static_cast<class Vect*>(Info->Kobj);
        Para->Add("        /* %s */", Vect->Name->c_str());
        Para->Add("        case %lld:", Vect->Num);
        Para->Add("        {");
        Para->Add("            extern rme_ptr_t RME_Vect_%s_User(rme_ptr_t Int_Num)\n", Vect->Name->c_str());
        Para->Add("            Send_Num=RME_Vect_%s_User(Vect_Num);", Vect->Name->c_str());
        Para->Add("            _RME_Kern_Snd(%s_Vect_Sig);", Vect->Name->c_str());
        Para->Add("            return Send_Num;");
        Para->Add("        }");
    }
    Para->Add("        default: break;");
    Para->Add("    }");
    Para->Add("    return 1;");
    Para->Add("}");
    Para->Cfunc_Foot(std::make_unique<std::string>("RME_Boot_Vect_Handler"));
    Doc->Add(std::move(Para));
    
    Doc->Csrc_Foot();

    /* Generate rme_boot.c */
    File=this->Main->Dstfs->Open_File("M7M1_MuEukaron/Project/Source/rme_boot.c");
    Doc->Write(File);
    fclose(File);
}
/* End Function:Proc_Gen::Proc_Src *******************************************/
}
/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/