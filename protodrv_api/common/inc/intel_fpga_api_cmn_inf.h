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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "intel_fpga_platform.h"


#ifdef __cplusplus
extern "C" {
#endif

unsigned int fpga_get_num_of_interfaces();
bool fpga_get_interface_at(unsigned int index, FPGA_INTERFACE_INFO *info);
FPGA_MMIO_INTERFACE_HANDLE fpga_open(unsigned int index);
void fpga_close(unsigned int index);
FPGA_INTERRUPT_HANDLE fpga_interrupt_open(unsigned int index);
void fpga_interrupt_close(unsigned int index);


// This API with pre-fix common_fpga_interface_info_vec implements C++ vector semantics without exception handling.
// Used internally to access g_common_fpga_interface_info_vec, which shouldn't be used directly outside of the scope of this file.
extern FPGA_INTERFACE_INFO     *g_common_fpga_interface_info_vec;
extern size_t                  g_common_fpga_interface_info_vec_reserved;
extern size_t                  g_common_fpga_interface_info_vec_size;
static inline size_t common_fpga_interface_info_vec_size()
{
    return g_common_fpga_interface_info_vec_size;
}
static inline FPGA_INTERFACE_INFO *common_fpga_interface_info_vec_at(size_t index)
{
    return g_common_fpga_interface_info_vec + index;
}
void common_fpga_interface_info_vec_resize(size_t size);
void common_fpga_interface_info_vec_reserve(size_t size);

#ifdef __cplusplus
}
#endif
