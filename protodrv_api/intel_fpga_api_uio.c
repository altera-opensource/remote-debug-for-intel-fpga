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
#include "intel_fpga_platform_uio.h"

unsigned int fpga_get_num_of_interfaces()
{
    return (unsigned int)g_uio_fpga_interface_info_vec_size;
}

bool fpga_get_interface_at(unsigned int index, FPGA_INTERFACE_INFO *info)
{
    bool ret = false;
    if (index < g_uio_fpga_interface_info_vec_size)
    {
        *info = g_uio_fpga_interface_info_vec[index];
        
        ret = true;
    }
    
    return ret;
}

FPGA_MMIO_INTERFACE_HANDLE fpga_open(unsigned int index)
{
    FPGA_MMIO_INTERFACE_HANDLE  ret = FPGA_MMIO_INTERFACE_INVALID_HANDLE;
    
    if (index < g_uio_fpga_interface_info_vec_size &&
        !g_uio_fpga_interface_info_vec[index].is_mmio_opened )
    {
        ret = index;
        g_uio_fpga_interface_info_vec[index].is_mmio_opened = true;
    }
    
    return ret;
}

void fpga_close(unsigned int index)
{
    if (index < g_uio_fpga_interface_info_vec_size )
    {
        g_uio_fpga_interface_info_vec[index].is_mmio_opened = false;
    }
}

FPGA_INTERRUPT_HANDLE fpga_interrupt_open(unsigned int index)
{
    FPGA_INTERRUPT_HANDLE  ret = FPGA_INTERRUPT_INVALID_HANDLE;
    
    if (index < g_uio_fpga_interface_info_vec_size &&
        !g_uio_fpga_interface_info_vec[index].is_interrupt_opened )
    {
        ret = index;
    }
    
    return ret;
}

void fpga_interrupt_close(unsigned int index)
{
    if (index < g_uio_fpga_interface_info_vec_size )
    {
        g_uio_fpga_interface_info_vec[index].is_interrupt_opened = false;
    }
}

int fpga_msg_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, ...)
{
    int     ret;
    
    va_list args;
    va_start(args, format);
    
    ret = g_uio_fpga_platform_printf(type, format, args);
    
    va_end(args);


    return ret;
}

void fpga_throw_runtime_exception(const char *function, const char *file, int lineno, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    
    g_uio_fpga_platform_runtime_exception_handler(function, file, lineno, format, args);
    
    va_end(args);
}

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
    if (handle < g_uio_fpga_interface_info_vec_size )
    {
	ret = 0;
        if(g_uio_fpga_interface_info_vec[handle].isr_callback != NULL)
            ret = 1;

         g_uio_fpga_interface_info_vec[handle].isr_callback = isr;
    }

    return ret;
}

int fpga_enable_interrupt(FPGA_INTERRUPT_HANDLE handle)
{
    int ret    = -1;
    int retVal = 0;
    int status = 0;
    if (handle < g_uio_fpga_interface_info_vec_size )
    {
        g_uio_fpga_interface_info_vec[handle].interrupt_enable = true;

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
    if (handle < g_uio_fpga_interface_info_vec_size )
    {
        g_uio_fpga_interface_info_vec[handle].interrupt_enable = false;
        ret = 0;
    }
    return ret;
}
