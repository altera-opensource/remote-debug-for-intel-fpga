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


typedef void (*FPGA_ISR) ( void *isr_context );
int fpga_register_isr(FPGA_INTERRUPT_HANDLE handle, FPGA_ISR isr, void *isr_context);
int fpga_enable_interrupt(FPGA_INTERRUPT_HANDLE handle);
int fpga_disable_interrupt(FPGA_INTERRUPT_HANDLE handle);
