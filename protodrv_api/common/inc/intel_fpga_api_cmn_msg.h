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

#define INTEL_FPGA_MSG_PRINTF_ENABLE  1
#define INTEL_FPGA_MSG_PRINTF_ENABLE_INFO  1
#define INTEL_FPGA_MSG_PRINTF_ENABLE_WARNING  1
#define INTEL_FPGA_MSG_PRINTF_ENABLE_ERROR  1
#define INTEL_FPGA_MSG_PRINTF_ENABLE_DEBUG  1

typedef enum
{
    FPGA_MSG_PRINTF_INFO,         //!< In default printf, this type of message is prefixed with "Info: "
    FPGA_MSG_PRINTF_WARNING,      //!< In default printf, this type of message is prefixed with "Warning: "
    FPGA_MSG_PRINTF_ERROR,        //!< In default printf, this type of message is prefixed with "Error: "
    FPGA_MSG_PRINTF_DEBUG         //!< In default printf, this type of message is prefixed with "Debug: "
} FPGA_MSG_PRINTF_TYPE;
typedef int (*FPGA_MSG_PRINTF) ( FPGA_MSG_PRINTF_TYPE type, const char * format, va_list args);
void fpga_platform_register_printf(FPGA_MSG_PRINTF msg_printf);
typedef void (*FPGA_RUNTIME_EXCEPTION_HANDLER) ( const char *function, const char *file, int lineno, const char * format, va_list args);
void fpga_platform_register_runtime_exception_handler(FPGA_RUNTIME_EXCEPTION_HANDLER handler);

extern FPGA_MSG_PRINTF         g_common_fpga_platform_printf;
extern int                     g_common_show_dbg_msg;
extern FPGA_RUNTIME_EXCEPTION_HANDLER  g_common_fpga_platform_runtime_exception_handler;

int fpga_msg_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, ...);
void fpga_throw_runtime_exception(const char *function, const char *file, int lineno, const char * format, ...);

#ifdef __cplusplus
}
#endif
