// Copyright(c) 2021, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "intel_fpga_api_cmn_msg.h"
#include "intel_fpga_api_cmn_inf.h"

FPGA_INTERFACE_INFO     *g_common_fpga_interface_info_vec = NULL;
size_t                  g_common_fpga_interface_info_vec_size = 0;
size_t                  g_common_fpga_interface_info_vec_reserved = 0;


unsigned int fpga_get_num_of_interfaces()
{
    return (unsigned int)common_fpga_interface_info_vec_size();
}

bool fpga_get_interface_at(unsigned int index, FPGA_INTERFACE_INFO *info)
{
    bool ret = false;
    if (index < common_fpga_interface_info_vec_size() && info != NULL)
    {
        memcpy(info, common_fpga_interface_info_vec_at(index), sizeof(FPGA_INTERFACE_INFO));
        
        ret = true;
    }
    
    return ret;
}

FPGA_MMIO_INTERFACE_HANDLE fpga_open(unsigned int index)
{
    FPGA_MMIO_INTERFACE_HANDLE  ret = FPGA_MMIO_INTERFACE_INVALID_HANDLE;
    
    if (index < common_fpga_interface_info_vec_size() &&
        !common_fpga_interface_info_vec_at(index)->is_mmio_opened )
    {
        ret = index;
        common_fpga_interface_info_vec_at(index)->is_mmio_opened = true;
    }
    
    return ret;
}

void fpga_close(unsigned int index)
{
    if (index < common_fpga_interface_info_vec_size() )
    {
        common_fpga_interface_info_vec_at(index)->is_mmio_opened = false;
    }
}

FPGA_INTERRUPT_HANDLE fpga_interrupt_open(unsigned int index)
{
    FPGA_INTERRUPT_HANDLE  ret = FPGA_INTERRUPT_INVALID_HANDLE;
    
    if (index < common_fpga_interface_info_vec_size() &&
        !common_fpga_interface_info_vec_at(index)->is_interrupt_opened )
    {
        ret = index;
        common_fpga_interface_info_vec_at(index)->is_interrupt_opened = true;
    }
    
    return ret;
}

void fpga_interrupt_close(unsigned int index)
{
    if (index < common_fpga_interface_info_vec_size() )
    {
        common_fpga_interface_info_vec_at(index)->is_interrupt_opened = false;
    }
}

void common_fpga_interface_info_vec_resize(size_t size)
{
    common_fpga_interface_info_vec_reserve(size);
    g_common_fpga_interface_info_vec_size = size;
}

void common_fpga_interface_info_vec_reserve(size_t size)
{
    if (size > g_common_fpga_interface_info_vec_reserved)
    {
        g_common_fpga_interface_info_vec = realloc(g_common_fpga_interface_info_vec, size * sizeof(FPGA_INTERFACE_INFO));
        if (g_common_fpga_interface_info_vec == NULL)
        {
            fpga_throw_runtime_exception(__FUNCTION__, __FILE__, __LINE__, "insufficient memory for %ld interfaces.", size);
        }
        else
        {
            memset(g_common_fpga_interface_info_vec + g_common_fpga_interface_info_vec_reserved, 0, (size - g_common_fpga_interface_info_vec_reserved) * sizeof(FPGA_INTERFACE_INFO));
            g_common_fpga_interface_info_vec_reserved = size;
        }
    }
    else if(size == 0)
    {
        if (g_common_fpga_interface_info_vec!=NULL)
        {
            free(g_common_fpga_interface_info_vec);
        }
        g_common_fpga_interface_info_vec = NULL;
        g_common_fpga_interface_info_vec_reserved = 0;
        g_common_fpga_interface_info_vec_size = 0;
    }
}
