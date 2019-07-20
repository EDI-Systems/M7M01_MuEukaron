/******************************************************************************
Filename    : rme_fsys.cpp
Author      : pry
Date        : 16/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The filesystem interface class.
******************************************************************************/

/* Includes ******************************************************************/
/* Kill CRT warnings for MS. This also relies on Shlwapi.lib, remember to add it */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

extern "C"
{
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
}

#include "string"
#include "memory"
#include "vector"
#include "algorithm"
#include "stdexcept"

#define __HDR_DEFS__
#include "Core/rme_mcu.hpp"
#include "Core/rme_fsys.hpp"
#undef __HDR_DEFS__

#define __HDR_CLASSES__
#include "Core/rme_fsys.hpp"
#undef __HDR_CLASSES__
/* End Includes **************************************************************/
namespace rme_mcu
{
/* Begin Function:Memcpy ******************************************************
Description : Memcpy wrapper for 64-bit XML library.
Input       : void* Src - The source string.
              xml_ptr_t Num - The number to copy.
Output      : void* Dst - The destination string.
Return      : void* - The destination is returned.
******************************************************************************/
extern "C" void* Memcpy(void* Dst, void* Src, xml_ptr_t Num)
{
    return memcpy(Dst, Src, (size_t)Num);
}
/* End Function:Memcpy *******************************************************/

/* Begin Function:Strncmp *****************************************************
Description : Strncmp wrapper for 64-bit XML library.
Input       : s8_t* Str1 - The first string.
              s8_t* Str2 - The second string.
              ptr_t Num - The number of characters to compare.
Output      : None.
Return      : ret_t - If Str1 is bigger, positive; if equal, 0; if Str2 is bigger,
                      negative.
******************************************************************************/
extern "C" ret_t Strncmp(s8_t* Str1, s8_t* Str2, ptr_t Num)
{
    return strncmp(Str1,Str2,(size_t)Num);
}
/* End Function:Strncmp ******************************************************/

/* Begin Function:Strlen ******************************************************
Description : Strlen wrapper for 64-bit XML library.
Input       : s8_t* Str - The Input string.
Output      : None.
Return      : ptr_t - The length of the string.
******************************************************************************/
extern "C" ptr_t Strlen(s8_t* Str)
{
    return strlen(Str);
}
/* End Function:Strlen *******************************************************/

/* Begin Function:Fsys::Dir_Present *******************************************
Description : Figure out whether the directory is present.
Input       : std::unique_ptr<std::string>& Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for present, -1 for non-present.
******************************************************************************/
ret_t Fsys::Dir_Present(std::unique_ptr<std::string>& Path)
{
#ifdef _MSC_VER
    u32_t Attr;
    Attr=PathIsDirectory((*Path).c_str());
    if(Attr!=0)
        return 0;
    else
        return -1;
#else
    DIR* Dir;
    Dir=opendir((*Path).c_str());
    if(Dir!=0)
    {
        closedir(Dir);
        return 0;
    }
    else
        return -1;
#endif
}
/* End Function:Fsys::Dir_Present ********************************************/

/* Begin Function:Fsys::Dir_Empty *********************************************
Description : Figure out whether the directory is empty. When using this function,
              the directory must be present.
Input       : std::unique_ptr<std::string>& Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for empty, -1 for non-empty.
******************************************************************************/
ret_t Fsys::Dir_Empty(std::unique_ptr<std::string>& Path)
{
#ifdef _MSC_VER
    u32_t Attr;
    Attr=PathIsDirectoryEmpty((*Path).c_str());
    if(Attr!=0)
        return 0;
    else
        return -1;
#else
    ptr_t Num;
    DIR* Dir;

    Dir=opendir((*Path).c_str());
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
/* End Function:Fsys::Dir_Empty **********************************************/

/* Begin Function:Fsys::Make_Dir **********************************************
Description : Create a directory if it does not exist.
Input       : std::unique_ptr<std::string>& Path - The path to the directory.
Output      : None.
Return      : ret_t - 0 for successful, -1 for failure.
******************************************************************************/
void Fsys::Make_Dir(std::unique_ptr<std::string>& Path)
{
    if(Dir_Present(Path)==0)
        return;

#ifdef _WIN32
    if(CreateDirectory((*Path).c_str(), NULL)!=0)
        return;
#else
    if(mkdir((*Path).c_str(), S_IRWXU)==0)
        return;
#endif

    throw std::runtime_error("Folder creation failed.");
}
/* End Function:Fsys::Make_Dir ***********************************************/

/* Begin Function:Sysfs::Sysfs ************************************************
Description : Constructor for Sysfs class.
Input       : std::unique_ptr<std::string>& Root - The root folder containing everything.
              std::unique_ptr<std::string>& Output - The output folder.
Output      : None.
Return      : None.
******************************************************************************/
/* void */ Sysfs::Sysfs(std::unique_ptr<std::string>& Root, std::unique_ptr<std::string>& Output)
{
    try
    {
        /* Root */
        this->Root=std::move(Root);
        if((*this->Root).back()!='/')
            (*this->Root)+="/";

        /* Output folder */
        this->Output=std::move(Output);
        if((*this->Output).back()!='/')
            (*this->Output)+="/";
    }
    catch(std::exception& Exc)
    {
        throw std::runtime_error(std::string("System file storage:\n")+Exc.what());
    }
}
/* End Function:Sysfs::Sysfs *************************************************/

/* Begin Function:Sysfs::File_Size ********************************************
Description : Get the size of the file. The file is known to be present somewhere.
Input       : std::unique_ptr<std::string>& Path - The path of the file.
Output      : None.
Return      : ptr_t - The size of the file.
******************************************************************************/
ptr_t Sysfs::File_Size(std::unique_ptr<std::string>& Path)
{
    struct stat Buf;
    if(stat((*Path).c_str(),&Buf)!=0)
        throw std::runtime_error("System file storage:\nWindows/Linux stat failed.");
    return Buf.st_size;
}
/* End Function:Sysfs::File_Size *********************************************/

/* Begin Function:Sysfs::Copy_File ********************************************
Description : Copy a file from some position to another position. This function
              only need a path input, and will automatically copy stuff to the
              correct location.
Input       : s8_t* Dst - The destination path.
              s8_t* Src - The source path.
Output      : None.
Return      : ret_t - 0 for successful, -1 for failure.
******************************************************************************/
void Sysfs::Copy_File(std::unique_ptr<std::string>& Path)
{
    FILE* Dst_File;
    FILE* Src_File;
    s8_t Buf[128];
    ptr_t Size;

    std::unique_ptr<std::string> Src;
    std::unique_ptr<std::string> Dst;

    try
    {
        Src=std::make_unique<std::string>(*(this->Root)+*Path);
        Src_File=fopen((*Src).c_str(), "rb");
        if(Src_File==0)
            throw std::runtime_error("Copy file:\nCannot open source file.");

        /* This will wipe the contents of the file */
        Dst=std::make_unique<std::string>(*(this->Output)+*Path);
        Dst_File=fopen((*Dst).c_str(), "wb");
        if(Dst_File==0)
            throw std::runtime_error("Copy file:\nCannot open destination file.");

        Size=fread(Buf, 1, 128, Src_File);
        while(Size!=0)
        {
            fwrite(Buf, 1, (size_t)Size, Dst_File);
            Size=fread(Buf, 1, 128, Src_File);
        }

        fclose(Src_File);
        fclose(Dst_File);
    }
    catch(std::exception& Exc)
    {
        throw std::runtime_error(std::string("System file storage:\n")+Exc.what());
    }
}
/* End Function:Sysfs::Copy_File *********************************************/

/* Begin Function:Sysfs::Read_Proj ********************************************
Description : Read the project XML content into a buffer. This only works for text
              files; binaries are not allowed.
Input       : std::unique_ptr<std::string>& Path - The path to the file.
Output      : None.
Return      : u8_t* - The buffer returned.
******************************************************************************/
std::unique_ptr<std::string> Sysfs::Read_Proj(std::unique_ptr<std::string>& Path)
{
    ptr_t Size;
    FILE* File;
    s8_t* Buf;
    std::unique_ptr<std::string> Str;

    try
    {
        Str=std::make_unique<std::string>(*(this->Root)+*Path);
        Size=File_Size(Path);
        Buf=new s8_t[(unsigned int)(Size+1)];

        File=fopen((*Path).c_str(), "rb");
        if(File==0)
            throw std::runtime_error("Read text file:\nCannot read file.");

        fread(Buf, 1, (size_t)Size, File);
        Buf[Size]='\0';

        Str=std::make_unique<std::string>(Buf);
        delete[] Buf;
        return Str;
    }
    catch(std::exception& Exc)
    {
        throw std::runtime_error(std::string("System file storage:\n")+Exc.what());
        return nullptr;
    }
}
/* End Function:Sysfs::Read_Proj *********************************************/

/* Begin Function:Sysfs::Read_Chip ********************************************
Description : Read the chip configuration file into a buffer. This only works
              for text files; binaries are not allowed.
Input       : std::unique_ptr<std::string>& Path - The path to the file.
Output      : None.
Return      : u8_t* - The buffer returned.
******************************************************************************/
std::unique_ptr<std::string> Sysfs::Read_Chip(std::unique_ptr<std::string>& Path)
{
    ptr_t Size;
    FILE* File;
    s8_t* Buf;
    std::unique_ptr<std::string> Str;

    try
    {
        Str=std::make_unique<std::string>(*(this->Root)+*Path);
        Size=File_Size(Path);
        Buf=new s8_t[(unsigned int)(Size+1)];

        File=fopen((*Path).c_str(), "rb");
        if(File==0)
            throw std::runtime_error("Read text file:\nCannot read file.");

        fread(Buf, 1, (size_t)Size, File);
        Buf[Size]='\0';

        Str=std::make_unique<std::string>(Buf);
        delete[] Buf;
        return Str;
    }
    catch(std::exception& Exc)
    {
        throw std::runtime_error(std::string("System file storage:\n")+Exc.what());
        return nullptr;
    }
}
/* End Function:Sysfs::Read_Chip *********************************************/
}
/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
