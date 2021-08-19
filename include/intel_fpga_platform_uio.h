#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "intel_fpga_platform_api_uio.h"

#ifdef __cplusplus
extern "C" {
#endif


#define FPGA_PLATFORM_MAJOR_VERSION 0
#define FPGA_PLATFORM_MINOR_VERSION 1
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_READ_8
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_WRITE_8
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_READ_16
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_WRITE_16
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_READ_32
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_WRITE_32
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_READ_64
#define FPGA_PLATFORM_HAS_NATIVE_MMIO_WRITE_64
#define FPGA_PLATFORM_IS_ISR_CALLED_IN_THREAD

typedef int FPGA_MMIO_INTERFACE_HANDLE;
#define FPGA_MMIO_INTERFACE_INVALID_HANDLE -1
typedef int FPGA_INTERRUPT_HANDLE;
#define FPGA_INTERRUPT_INVALID_HANDLE -1
typedef struct
{
    uint8_t                      version;       //!< Identify the version of the interface.
    uint8_t                      mfg_id;        //!< Identify the vendor providing.
    uint16_t                     type;          //!< Identify the type of interface.
    uint16_t                     instance;      //!< Identify an instance of interface when this interface is instantiated multiple times in the system.
    uint16_t                     group_id;      //!< Define a group of interfaces that support a high-level function.  One ProtoDriver may be developed using such group of interfaces.
    uint8_t                      subsystem_id;  //!< Define the subsystem scope of the group_id and instance field.
    void                         *base_address;  //!< Define the base address to be used by MMIO functions
    uint16_t                     interrupt;      //!< interrupt assignment
    bool                         is_mmio_opened; 
    bool                         is_interrupt_opened;
} FPGA_INTERFACE_INFO;
typedef void * FPGA_PLATFORM_PHYSICAL_MEM_ADDR_TYPE;



// Platform specific internal API
extern FPGA_INTERFACE_INFO     *g_uio_fpga_interface_info_vec;
extern size_t                  g_uio_fpga_interface_info_vec_size;
extern FPGA_MSG_PRINTF                 g_uio_fpga_platform_printf;
extern FPGA_RUNTIME_EXCEPTION_HANDLER  g_uio_fpga_platform_runtime_exception_handler;

#ifdef __cplusplus
}
#endif
