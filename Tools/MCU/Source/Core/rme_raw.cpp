/******************************************************************************
Filename    : rme_raw.cpp
Author      : pry
Date        : 16/07/2019
Licence     : LGPL v3+; see COPYING for details.
Description : The raw information class.
******************************************************************************/

/* Includes ******************************************************************/
#include "string"
#include "memory"
#include "stdexcept"

extern "C"
{
#include "xml.h"
}

#define __HDR_DEFS__
#include "Core/rme_mcu.hpp"
#include "Core/rme_raw.hpp"
#undef __HDR_DEFS__

#define __HDR_CLASSES__
#include "Core/rme_raw.hpp"
#undef __HDR_CLASSES__
/* End Includes **************************************************************/
namespace rme_mcu
{
/* Begin Function:Raw::Raw ****************************************************
Description : Constructor for Raw class.
Input       : xml_node_t* Node - The node containing the raw information.
Output      : None.
Return      : None.
******************************************************************************/
/* void */ Raw::Raw(xml_node_t* Node)
{
    try
    {
        /* Tag/Value */
        if(Node->XML_Tag_Len==0)
            throw std::invalid_argument("Tag section is empty.");
        if(Node->XML_Val_Len==0)
            throw std::invalid_argument("Value section is empty.");
        this->Tag=std::make_unique<std::string>(Node->XML_Tag,(int)Node->XML_Tag_Len);
        this->Val=std::make_unique<std::string>(Node->XML_Val,(int)Node->XML_Val_Len);
    }
    catch(std::exception& Exc)
    {
        throw std::runtime_error(std::string("Raw:\n")+Exc.what());
    }
}
/* End Function:Raw::Raw *****************************************************/
}
/* End Of File ***************************************************************/

/* Copyright (C) Evo-Devo Instrum. All rights reserved ***********************/
