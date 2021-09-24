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
#include <pthread.h>
#include <poll.h>
#include <semaphore.h>

#include "intel_fpga_api_uio.h"
#include "intel_fpga_platform_uio.h"
#include "intel_fpga_platform_api_uio.h"


FPGA_INTERFACE_INFO     *g_uio_fpga_interface_info_vec = NULL;
size_t                  g_uio_fpga_interface_info_vec_size = 0;
sem_t g_intSem;

static int uio_fpga_platform_default_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, va_list args);
static void uio_fpga_platform_default_runtime_exception_handler(const char *function, const char *file, int lineno, const char * format, va_list args);

FPGA_MSG_PRINTF                 g_uio_fpga_platform_printf = uio_fpga_platform_default_printf;
FPGA_RUNTIME_EXCEPTION_HANDLER  g_uio_fpga_platform_runtime_exception_handler = uio_fpga_platform_default_runtime_exception_handler;

int g_uio_show_dbg_msg = 0;

static char *s_uio_drv_path = "/dev/uio0";
static size_t s_uio_addr_span = 0;
static int s_uio_single_component_mode = 1;
static size_t s_uio_start_addr = 0;
static size_t s_uio_inThread_timeout = 0;

static int  s_uio_drv_handle = -1;
static void *s_uio_mmap_ptr = NULL;
static pthread_t s_intThread_id = 0;
static pthread_rwlock_t s_intLock;
static int s_intFlags = 0;

static void uio_parse_args(unsigned int argc, const char *argv[]);
static long uio_parse_integer_arg(const char *name);
static void uio_update_based_on_sysfs();
static void uio_get_sysfs_map_path(char *path, int path_buf_size);
static uint64_t uio_get_sysfs_map_file_to_uint64(const char *path);
static bool uio_validate_args();
static void uio_print_configuration();
static bool uio_open_driver();
static bool uio_map_mmio();
static bool uio_scan_interfaces();
static bool uio_create_unit_test_sw_model();

static void *uio_interrupt_thread();

void *uio_interrupt_thread()
{
    int fd = 0;

    while(1)
    {
        pthread_rwlock_rdlock(&s_intLock);
        uint16_t flags = s_intFlags;
        pthread_rwlock_unlock(&s_intLock);

        if(!(flags & FPGA_PLATFORM_INT_THREAD_EXIT))
        {
            // Loop for uio with interrupt enabled
            // Current implementaion support 1 vector
            if(g_uio_fpga_interface_info_vec[0].interrupt_enable)
            {
               fd = open(s_uio_drv_path, O_RDWR);
               if(fd < 0)
                {
                   fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "InterruptThread failed to open UIO device" );
                    break;
                }

               struct pollfd fds = {
                   .fd = fd,
                   .events = POLLIN,
                   };

                uint32_t info = 1;
                int ret =write(fd, &info, sizeof(info));
                if(ret < 0)
                {
                   fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "InterruptThread failed to re-Arm UIO interrupt" );
                   break;
                }

                // Polling with timeout to check thread exit flag
                // s_uio_inThread_timeout init in uio_parse_args()
                ret = poll(&fds, 1, s_uio_inThread_timeout);

                if(ret > 0){
                    if(g_uio_fpga_interface_info_vec[0].isr_callback != NULL){
                        (*g_uio_fpga_interface_info_vec[0].isr_callback)();
                    } else {
                        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "InterruptThread ISR is NULL ptr" );
			break;
                    }
                }

                if(fd > 0)
	            close(fd);

            } else {

                // Interrupt Thread blocked until sem_post
               sem_wait(&g_intSem);
           }
       }
       else
       {
            // Interrupt Thread exit
            fpga_msg_printf( FPGA_MSG_PRINTF_DEBUG, "InterruptThread exit" );
            pthread_exit(NULL);
       }
    }

    // Interrupt Thread exit for break condition
    fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "InterruptThread exit with error" );
    pthread_exit(NULL);
}

bool fpga_platform_init(unsigned int argc, const char *argv[])
{
    bool        is_args_valid;
    bool        ret = false;

    uio_parse_args(argc, argv);
    uio_update_based_on_sysfs();
    is_args_valid = uio_validate_args();

    // Interrupt Timeout is configured to 100ms
    s_uio_inThread_timeout = 100;

    if (is_args_valid)
    {
        uio_print_configuration();
#ifndef UIO_UNIT_TEST_SW_MODEL_MODE
        if(uio_open_driver() == false)
            goto err_open;

        if(uio_map_mmio() == false)
            goto err_map;

        if(uio_scan_interfaces() == false)
            goto err_scan;
#else
        if(uio_create_unit_test_sw_model() == false)
            goto err_scan;
#endif
        ret = true;
    }
    else
    {
        goto err_open;
    }

    if(s_uio_single_component_mode)
    {
        // Interrupt Thread creation and sync init
        sem_init(&g_intSem, 0, 0);
        pthread_rwlock_init(&s_intLock,0);
        pthread_rwlock_wrlock(&s_intLock);
        s_intFlags = 0;
        pthread_rwlock_unlock(&s_intLock);
        pthread_create(&s_intThread_id, NULL, uio_interrupt_thread, NULL);
    }

    return ret;

err_scan:
    munmap(s_uio_mmap_ptr,s_uio_addr_span);

err_map:
    close(s_uio_drv_handle);

err_open:
    return ret;
}


void fpga_platform_cleanup()
{
    void *ret;
    int retVal = 0;
    int status = 0;

    if(s_intThread_id != 0)
    {
        pthread_rwlock_wrlock(&s_intLock);
        s_intFlags = s_intFlags | FPGA_PLATFORM_INT_THREAD_EXIT;
        pthread_rwlock_unlock(&s_intLock);

        status = sem_getvalue(&g_intSem, &retVal);
        if(status == 0)
        {
            if(retVal <= 0)
                sem_post(&g_intSem);
        }

       if(pthread_join(s_intThread_id, &ret) != 0)
       {
           fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Interrupt Thread join failed" );
       } else {
	   fpga_msg_printf( FPGA_MSG_PRINTF_DEBUG, "Interrupt Thread join successfully" );
       }

    }

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

    fpga_msg_printf( FPGA_MSG_PRINTF_DEBUG, "Completed fpga_platform_cleanup" );
}

void uio_parse_args(unsigned int argc, const char *argv[])
{
    static struct option long_options[] =
        {
            {"uio-driver-path", required_argument, 0, 'p'},
            {"start-address", required_argument, 0, 'a'},
            {"address-span", required_argument, 0, 's'},
            {"show-dbg-msg", no_argument, &g_uio_show_dbg_msg, 'd'},
            {"single-component-mode", no_argument, &s_uio_single_component_mode, 'c'},
            {0, 0, 0, 0}};

    int option_index = 0;
    int c;

    opterr = 0; // Suppress stderr output from getopt_long upon unrecognized options
    optind = 0;     // Reset getopt_long position.
    
    while(1)
    {
        c = getopt_long(argc, (char * const*)argv, "p:a:s:dc", long_options, &option_index);
      
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

void uio_update_based_on_sysfs()
{
#ifndef UIO_UNIT_TEST_SW_MODEL_MODE
    if (s_uio_addr_span == 0)
    {
        enum
        {
            UIO_MAP_PATH_SIZE = 1024
        };

        char map_path[UIO_MAP_PATH_SIZE + 1];

        uio_get_sysfs_map_path(map_path, UIO_MAP_PATH_SIZE);

        strncat(map_path, "size", UIO_MAP_PATH_SIZE);
        s_uio_addr_span = uio_get_sysfs_map_file_to_uint64(map_path);
    }
#endif
}

uint64_t uio_get_sysfs_map_file_to_uint64(const char *path)
{
    uint64_t ret = 0;
    
    FILE *fp;

    fp = fopen(path, "r");
    if (fp)
    {
        if (fscanf(fp, "%lx", &ret) != 1)
        {
            ret = 0;
        }
        fclose(fp);
    }

    return ret;
}
void uio_get_sysfs_map_path(char *path, int path_buf_size)
{
    uint32_t index = 0;
    char *p;
    char *endptr;

    path[0] = '\0';
    // The region index is encoded in the file name component.
    p = strrchr(s_uio_drv_path, '/');
    if (!p)
    {
        return;
    }

    // p + 4 because the string will look like:
    // 01234
    // /dev/uio3
    endptr = NULL;
    p += 4;
    index = strtoul(p, &endptr, 10);
    if (*endptr)
    {
        return;
    }

    // Example map path: /sys/class/uio/uio0/maps/map0
    // Only use map0!
    if (snprintf(path, path_buf_size, "/sys/class/uio/uio%d/maps/map0/", index) < 0)
    {
        path[0] = '\0';
        return;
    }
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
    // TODO: no way to disable "Single Component Operation Model" for now.  Don't print this info.
    // fpga_msg_printf( FPGA_MSG_PRINTF_INFO, "   Single Component Operation Model: %s", s_uio_single_component_mode ? "Yes" : "No" );
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
        fpga_msg_printf( FPGA_MSG_PRINTF_ERROR, "Failed to open %s. (Error code %d)", s_uio_drv_path, errno );
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
            if (g_uio_show_dbg_msg)
            {
                fputs("DEBUG: ", stdout);
                ret = vprintf(format, args);
                fputs("\n", stdout);
            }
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


