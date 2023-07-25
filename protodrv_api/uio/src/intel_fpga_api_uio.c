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

#include "intel_fpga_api_uio.h"
#include "intel_fpga_api_cmn_msg.h"

void *fpga_malloc(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t size)
{
    fpga_throw_runtime_exception("fpga_malloc", __FILE__, __LINE__, "Current platform doesn't support such feature.");
    
    return NULL;
}

void fpga_free(FPGA_MMIO_INTERFACE_HANDLE handle, void *address)
{
    fpga_throw_runtime_exception("fpga_free", __FILE__, __LINE__, "Current platform doesn't support such feature.");
}

FPGA_PLATFORM_PHYSICAL_MEM_ADDR_TYPE fpga_get_physical_address(void *address)
{
    fpga_throw_runtime_exception("fpga_get_physical_address", __FILE__, __LINE__, "Current platform doesn't support such feature.");
    
    return 0;
}

int fpga_register_isr(FPGA_INTERRUPT_HANDLE handle, FPGA_ISR isr, void *isr_context)
{
    int ret = -1;
    if (handle < common_fpga_interface_info_vec_size() )
    {
        ret = 0;
        if(common_fpga_interface_info_vec_at(handle)->isr_callback != NULL)
            ret = 1;

        common_fpga_interface_info_vec_at(handle)->isr_callback = isr;
        common_fpga_interface_info_vec_at(handle)->isr_context = isr_context;
    }

    return ret;
}

int fpga_enable_interrupt(FPGA_INTERRUPT_HANDLE handle)
{
    int ret    = -1;
    int retVal = 0;
    int status = 0;
    if (handle < common_fpga_interface_info_vec_size() )
    {
        common_fpga_interface_info_vec_at(handle)->interrupt_enable = true;

       status = sem_getvalue(&g_intSem, &retVal);
       if(status == 0)
       {
           if(retVal <= 0)
               sem_post(&g_intSem);
       }
        ret = 0;
    }

    return ret;
}

int fpga_disable_interrupt(FPGA_INTERRUPT_HANDLE handle)
{
    int ret = -1;
    if (handle < common_fpga_interface_info_vec_size() )
    {
        common_fpga_interface_info_vec_at(handle)->interrupt_enable = false;
        ret = 0;
    }
    return ret;
}
