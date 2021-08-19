#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "intel_fpga_platform_uio.h"


#ifdef __cplusplus
extern "C" {
#endif

unsigned int fpga_get_num_of_interfaces();
bool fpga_get_interface_at(unsigned int index, FPGA_INTERFACE_INFO *info);
FPGA_MMIO_INTERFACE_HANDLE fpga_open(unsigned int index);
void fpga_close(unsigned int index);
FPGA_INTERRUPT_HANDLE fpga_interrupt_open(unsigned int index);
void fpga_interrupt_close(unsigned int index);

static inline void *fpga_uio_get_base_address(FPGA_MMIO_INTERFACE_HANDLE handle)
{
    return g_uio_fpga_interface_info_vec[handle].base_address;
}

static inline uint8_t fpga_read_8(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset)
{
    return *((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset);
}

static inline void fpga_write_8(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint8_t value)
{
    *((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset) = value;
}

static inline uint16_t fpga_read_16(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset)
{
    return *((volatile uint16_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset));
}

static inline void fpga_write_16(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint16_t value)
{
    *((volatile uint16_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset)) = value;
}

static inline uint32_t fpga_read_32(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset)
{
    return *((volatile uint32_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset));
}

static inline void fpga_write_32(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint32_t value)
{
    *((volatile uint32_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset)) = value;
}

static inline uint64_t fpga_read_64(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset)
{
    return *((volatile uint64_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset));
}

static inline void fpga_write_64(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint64_t value)
{
    *((volatile uint64_t *)((volatile uint8_t *)fpga_uio_get_base_address(handle) + offset)) = value;
}

static inline void fpga_read_512(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint8_t *value)
{
    int     i;
    for(i = 0; i < (512/64); ++i)
    {
        *((volatile uint64_t *)value) = fpga_read_64(handle, offset);
        value += 64/8;
        offset += 64/8;
    }
}

static inline void fpga_write_512(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t offset, uint8_t *value)
{
    int     i;
    for(i = 0; i < (512/64); ++i)
    {
        fpga_write_64(handle, offset, *((volatile uint64_t *)value));
        value += 64/8;        
        offset += 64/8;
    }
}

void *fpga_malloc(FPGA_MMIO_INTERFACE_HANDLE handle, uint32_t size);
void fpga_free(FPGA_MMIO_INTERFACE_HANDLE handle, void *address);
FPGA_PLATFORM_PHYSICAL_MEM_ADDR_TYPE fpga_get_physical_address(void *address);
typedef void (*FPGA_ISR) ( void *isr_context );
int fpga_register_isr(FPGA_INTERRUPT_HANDLE handle, FPGA_ISR isr, void *isr_context);
int fpga_enable_interrupt(FPGA_INTERRUPT_HANDLE handle);
int fpga_disable_interrupt(FPGA_INTERRUPT_HANDLE handle);

int fpga_msg_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, ...);
void fpga_throw_runtime_exception(const char *function, const char *file, int lineno, const char * format, ...);

#ifdef __cplusplus
}
#endif