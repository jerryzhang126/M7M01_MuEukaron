/******************************************************************************
Filename    : rme_rvmuser.hpp
Author      : pry
Date        : 16/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The header for the RVM user-modifiable file class.
******************************************************************************/

/* Defines *******************************************************************/
namespace rme_mcu
{
#ifdef __HDR_DEFS__
#ifndef __RME_GENRVM_HPP_DEFS__
#define __RME_GENRVM_HPP_DEFS__
/*****************************************************************************/

/*****************************************************************************/
/* __RME_GENRVM_HPP_DEFS__ */
#endif
/* __HDR_DEFS__ */
#endif
/* End Defines ***************************************************************/

/* Classes *******************************************************************/
#ifdef __HDR_CLASSES__
#ifndef __RME_GENRVM_HPP_CLASSES__
#define __RME_GENRVM_HPP_CLASSES__
/*****************************************************************************/
/* RVM user changeable file */
class RVM_User:public Doc
{
public:
    void Read(FILE* File);
};

class RVM_Gen
{
    void Include(std::unique_ptr<class Para>& Para);
    
    void Macro_Vect(std::unique_ptr<class Para>& Para);
    void Macro_Captbl(std::unique_ptr<class Para>& Para);
    void Macro_Pgtbl(std::unique_ptr<class Para>& Para);
    void Macro_Proc(std::unique_ptr<class Para>& Para);
    void Macro_Thd(std::unique_ptr<class Para>& Para);
    void Macro_Inv(std::unique_ptr<class Para>& Para);
    void Macro_Recv(std::unique_ptr<class Para>& Para);
    
    void Cons_Pgtbl(std::unique_ptr<class Para>& Para, class Pgtbl* Pgtbl);
    void Map_Pgtbl(std::unique_ptr<class Para>& Para, class Pgtbl* Pgtbl, ptr_t Init_Num_Ord);
    void Init_Pgtbl(std::unique_ptr<class Doc>& Doc);
public:
    class Fsys* Fsys;
    class Plat* Plat;
    class Proj* Proj;
    class Chip* Chip;

    virtual ~RVM_Gen(void){};

    void Folder(void);
    void Boot_Hdr(void);
    void Boot_Src(void);
    void User_Src(void);
    
    virtual void Plat_Gen(void)=0;
};
/*****************************************************************************/
/* __RME_GENRVM_HPP_CLASSES__ */
#endif
/* __HDR_CLASSES__ */
#endif
}
/* End Classes ***************************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
