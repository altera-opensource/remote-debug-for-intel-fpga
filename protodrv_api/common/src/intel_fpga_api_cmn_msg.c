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

static int s_common_fpga_platform_default_printf(FPGA_MSG_PRINTF_TYPE type, const char *format, va_list args);
static void s_common_fpga_platform_default_runtime_exception_handler(const char *function, const char *file, int lineno, const char *format, va_list args);

FPGA_MSG_PRINTF                 g_common_fpga_platform_printf = s_common_fpga_platform_default_printf;
FPGA_RUNTIME_EXCEPTION_HANDLER  g_common_fpga_platform_runtime_exception_handler = s_common_fpga_platform_default_runtime_exception_handler;

int g_common_show_dbg_msg = 0;


int fpga_msg_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, ...)
{
    int     ret;
    
    va_list args;
    va_start(args, format);
    
    ret = g_common_fpga_platform_printf(type, format, args);
    
    va_end(args);


    return ret;
}

void fpga_throw_runtime_exception(const char *function, const char *file, int lineno, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    
    g_common_fpga_platform_runtime_exception_handler(function, file, lineno, format, args);
    
    va_end(args);
}

void fpga_platform_register_printf(FPGA_MSG_PRINTF msg_printf)
{
    g_common_fpga_platform_printf = msg_printf;
}

void fpga_platform_register_runtime_exception_handler(FPGA_RUNTIME_EXCEPTION_HANDLER handler)
{
    g_common_fpga_platform_runtime_exception_handler = handler;
}

int s_common_fpga_platform_default_printf(FPGA_MSG_PRINTF_TYPE type, const char *format, va_list args)
{
    int ret = 0;

#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE
    switch (type)
    {
    case FPGA_MSG_PRINTF_INFO:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_INFO
        fputs("INFO: ", stdout);
        ret = vprintf(format, args);
        fputs("\n", stdout);
#endif
        break;
    case FPGA_MSG_PRINTF_WARNING:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_WARNING
        fputs("WARNING: ", stdout);
        ret = vprintf(format, args);
        fputs("\n", stdout);
#endif
        break;
    case FPGA_MSG_PRINTF_ERROR:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_ERROR
        fputs("ERROR: ", stdout);
        ret = vprintf(format, args);
        fputs("\n", stdout);
#endif
        break;
    case FPGA_MSG_PRINTF_DEBUG:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_DEBUG
        if (g_common_show_dbg_msg)
        {
            fputs("DEBUG: ", stdout);
            ret = vprintf(format, args);
            fputs("\n", stdout);
        }
#endif
        break;
    default:
        fpga_throw_runtime_exception(__FUNCTION__, __FILE__, __LINE__, "invalid message print type is specified.");
    }

#endif // INTEL_FPGA_MSG_PRINTF_ENABLE

    return ret;
}

void s_common_fpga_platform_default_runtime_exception_handler(const char *function, const char *file, int lineno, const char *format, va_list args)
{
    printf("Exception occured in %s() at line %d in %s due to ", function, lineno, file);
    vprintf(format, args);
    printf("\n\nApplication is terminated.\n");

    exit(1);
}

