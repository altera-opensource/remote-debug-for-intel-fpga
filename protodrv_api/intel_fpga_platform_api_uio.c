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

#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "intel_fpga_api_uio.h"
#include "intel_fpga_platform_uio.h"
#include "intel_fpga_platform_api_uio.h"


FPGA_INTERFACE_INFO     *g_uio_fpga_interface_info_vec = NULL;
size_t                  g_uio_fpga_interface_info_vec_size = 0;

static int uio_fpga_platform_default_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, va_list args);
static void uio_fpga_platform_default_runtime_exception_handler(const char *function, const char *file, int lineno, const char * format, va_list args);

FPGA_MSG_PRINTF                 g_uio_fpga_platform_printf = uio_fpga_platform_default_printf;
FPGA_RUNTIME_EXCEPTION_HANDLER  g_uio_fpga_platform_runtime_exception_handler = uio_fpga_platform_default_runtime_exception_handler;

static char *s_uio_drv_path = NULL;
static size_t s_uio_addr_span = 0;
static int s_uio_single_component_mode = 0;
static size_t s_uio_start_addr = 0;

static int  s_uio_drv_handle = -1;
static void *s_uio_mmap_ptr = NULL;

static void uio_parse_args(unsigned int argc, const char *argv[]);
static long uio_parse_integer_arg(const char *name);
static bool uio_validate_args();
static void uio_print_configuration();
static bool uio_open_driver();
static bool uio_map_mmio();
static bool uio_scan_interfaces();
static bool uio_create_unit_test_sw_model();

bool fpga_platform_init(unsigned int argc, const char *argv[])
{
    bool        ret = false;
    bool        is_args_valid;

    uio_parse_args(argc, argv);
    is_args_valid = uio_validate_args();

    if (is_args_valid)
    {
        uio_print_configuration();
#ifndef UIO_UNIT_TEST_SW_MODEL_MODE
        ret = uio_open_driver();
        ret &= uio_map_mmio();
        ret &= uio_scan_interfaces();
#else
        ret = uio_create_unit_test_sw_model();
#endif
    }
        
    return ret;
}


void fpga_platform_cleanup()
{
    if (s_uio_drv_handle>=0)
    {
        close(s_uio_drv_handle);
    }
    
    // Re-initialize local variables.
    s_uio_drv_path = NULL;
    s_uio_start_addr = 0;
    s_uio_addr_span = 0;
    s_uio_single_component_mode = 0;
    
    s_uio_drv_handle = -1;
    s_uio_mmap_ptr = NULL;
    
    if ( g_uio_fpga_interface_info_vec != NULL )
    {
#ifdef UIO_UNIT_TEST_SW_MODEL_MODE
        free(g_uio_fpga_interface_info_vec[0].base_address);
#endif 
        free(g_uio_fpga_interface_info_vec);
        g_uio_fpga_interface_info_vec = NULL;
    }
    
    g_uio_fpga_interface_info_vec_size = 0;
    
    errno = -1;  //  -1 is a valid return on success.  Some function, such as strtol(), doesn't set errno upon successful return.
}

void uio_parse_args(unsigned int argc, const char *argv[])
{
    static struct option long_options[] =
    {
        {"uio-driver-path",  required_argument, 0, 'p'},
        {"start-address",  required_argument, 0, 'a'},
        {"address-span",  required_argument, 0, 's'},
        {"single-component-mode", no_argument,  &s_uio_single_component_mode, 'c'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;

    opterr = 0; // Suppress stderr output from getopt_long upon unrecognized options
    optind = 0;     // Reset getopt_long position.
    
    while(1)
    {
        c = getopt_long(argc, (char * const*)argv, "p:a:s:c", long_options, &option_index);
      
        if (c == -1)
        {
            break;
        }
        
        switch(c)
        {
            case 'p':
                s_uio_drv_path = optarg;
                break;
                
            case 's':
                s_uio_addr_span = uio_parse_integer_arg("Address span");
                break;
                
            case 'a':
                s_uio_start_addr = uio_parse_integer_arg("Start address");
                break;
        }
    }        
}

long uio_parse_integer_arg( const char *name )
{
    long ret = 0;
    
    bool is_all_digit = true;
    char *p;
    typedef int (*DIGIT_TEST_FN) ( int c );
    DIGIT_TEST_FN is_acceptabl_digit;
    if(optarg[0] == '0' && (optarg[1] == 'x' || optarg[1] == 'X'))
    {
        is_acceptabl_digit = isxdigit;
        optarg += 2;    // trim the "0x" portion
    }
    else
    {
        is_acceptabl_digit = isdigit;
    }
    
    for(p = optarg; (*p) != '\0'; ++p)
    {
        if(!is_acceptabl_digit(*p))
        {
            is_all_digit = false;
            break;
        }
    }
    
    if ( is_acceptabl_digit == isxdigit )
    {
        optarg -= 2;  // restore the "0x" portion
    }
    
    if (is_all_digit)
    {
        if (sizeof(size_t) <= sizeof(long))
        {
            ret = (size_t)strtol(optarg, NULL, 0);
            if (errno == ERANGE)
            {
                ret = 0;
                fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, LONG_MAX );
            }
        }
        else
        {
            long span, span_c;
            span = strtol(optarg, NULL, 0);
            if (errno == ERANGE)
            {
                ret = 0;
                fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, LONG_MAX );
            }
            else
            {
                ret = (size_t)span;
                span_c = ret;
                if (span != span_c)
                {
                    fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, (size_t)-1 );
                    ret = 0;
                }
            }
        }
    }
    else
    {
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Invalid argument value type is provided. A integer value is expected. %s is provided.", optarg );
    }
    
    return ret;
}

bool uio_validate_args()
{
    bool ret = s_uio_addr_span > 0 &&
               s_uio_drv_path != NULL;
               
    if (!ret)
    {
        if ( s_uio_addr_span == 0 )
        {
            fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "No valid address span value is provided using the argument, --address-span." );
        }
        if ( s_uio_drv_path == NULL )
        {
            fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "UIO driver path is not provided using the argument, --uio-driver-path." );
        }
    }
    return ret;
}

void uio_print_configuration()
{
    fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "UIO Platform Configuration:" );
    fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "   Driver Path: %s", s_uio_drv_path );
    fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "   Address Span: %ld", s_uio_addr_span );
    fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "   Start Address: 0x%lX", s_uio_start_addr );
    fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "   Single Component Operation Model: %s", s_uio_single_component_mode ? "Yes" : "No" );
    
}

bool uio_open_driver()
{
    bool ret = true;
    
    s_uio_drv_handle = open(s_uio_drv_path, O_RDWR);
    if (s_uio_drv_handle == -1)
    {
#ifdef _BSD_SOURCE
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Failed to open %s. (Error code %d: %s)", s_uio_drv_path, sys_nerr, sys_errlist[sys_nerr] );
#else
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Failed to open %s. (Error code %d)", errno );
#endif    
        ret = false;
    }
    
    return ret;
}

bool uio_map_mmio()
{
    bool ret = true;
    
    s_uio_mmap_ptr = mmap(0, s_uio_addr_span, PROT_READ | PROT_WRITE, MAP_SHARED, s_uio_drv_handle, 0);
    if (s_uio_mmap_ptr == MAP_FAILED)
    {
#ifdef _BSD_SOURCE
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Failed to map the mmio inteface to the UIO driver provided.  (Error code %d: %s)", sys_nerr, sys_errlist[sys_nerr] );
#else
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Failed to map the mmio inteface to the UIO driver provided.  (Error code %d)", errno );
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_DEBUG
        perror("MMAP error");
#endif
#endif    
        ret = false;
    }
    
    return ret;
}

bool uio_scan_interfaces()
{
    bool ret = true;
    
    if(s_uio_single_component_mode)
    {
        g_uio_fpga_interface_info_vec = (FPGA_INTERFACE_INFO *)calloc(1, sizeof(FPGA_INTERFACE_INFO));
        g_uio_fpga_interface_info_vec_size = 1;

        g_uio_fpga_interface_info_vec[0].base_address = (void *)((char *)s_uio_mmap_ptr + s_uio_start_addr);
        g_uio_fpga_interface_info_vec[0].is_mmio_opened = false;
        g_uio_fpga_interface_info_vec[0].is_interrupt_opened = false;
    }

    return ret;
}


bool uio_create_unit_test_sw_model()
{
    g_uio_fpga_interface_info_vec = (FPGA_INTERFACE_INFO *)calloc(1, sizeof(FPGA_INTERFACE_INFO));
    g_uio_fpga_interface_info_vec_size = 1;
    
    g_uio_fpga_interface_info_vec[0].base_address = malloc(s_uio_addr_span);
    // Preset mem with all 1s
    memset(g_uio_fpga_interface_info_vec[0].base_address, 0xFF, s_uio_addr_span);
    
    g_uio_fpga_interface_info_vec[0].is_mmio_opened = false;
    g_uio_fpga_interface_info_vec[0].is_interrupt_opened = false;
    
    return true;
}

void fpga_platform_register_printf(FPGA_MSG_PRINTF msg_printf)
{
    g_uio_fpga_platform_printf = msg_printf;
}

void fpga_platform_register_runtime_exception_handler(FPGA_RUNTIME_EXCEPTION_HANDLER handler)
{
    g_uio_fpga_platform_runtime_exception_handler = handler;
}


int uio_fpga_platform_default_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, va_list args)
{
    int ret = 0;
    
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE
    switch(type)
    {
        case FPGA_MSG_PRINTF_INFO:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_INFO
            fputs( "INFO: ", stdout );
            ret = vprintf( format, args );
            fputs( "\n", stdout );
#endif            
            break;
        case FPGA_MSG_PRINTF_WARNING:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_WARNING
            fputs( "WARNING: ", stdout );
            ret = vprintf( format, args );
            fputs( "\n", stdout );
#endif            
            break;
        case FPGA_MSG_PRINTF_ERROR:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_ERROR
            fputs( "ERROR: ", stdout );
            ret = vprintf( format, args );
            fputs( "\n", stdout );
#endif            
            break;
        case FPGA_MSG_PRINTF_DEBUG:
#ifdef INTEL_FPGA_MSG_PRINTF_ENABLE_DEBUG
            fputs( "DEBUG: ", stdout );
            ret = vprintf( format, args );
            fputs( "\n", stdout );
#endif            
            break;
        default:
            fpga_throw_runtime_exception( __FUNCTION__, __FILE__, __LINE__, "invalid message print type is specified." );
    }
    
#endif //INTEL_FPGA_MSG_PRINTF_ENABLE

    return ret;
}


void uio_fpga_platform_default_runtime_exception_handler(const char *function, const char *file, int lineno, const char * format, va_list args)
{
    printf( "Exception occured in %s() at line %d in %s due to ", function, lineno, file );
    vprintf( format, args );
    printf( "\n\nApplication is terminated.\n" );
    
    exit(1);
}


