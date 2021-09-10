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

#include <string.h>
#include <stdint.h>
#include "intel_fpga_api.h"

#include "constants.h"
#include "st_dbg_ip_driver.h"
#include "st_dbg_ip_allocator.h"

ST_DBG_IP_DESIGN_INFO g_std_dbg_ip_info;
static char g_dbg_info_set = 0;
static FPGA_MMIO_INTERFACE_HANDLE g_mmio_handle = FPGA_MMIO_INTERFACE_INVALID_HANDLE;

// Descriptor tracking
static unsigned short g_h2t_descriptor_slots_available = 0;
static unsigned short g_mgmt_descriptor_slots_available = 0;
static unsigned short g_h2t_descriptor_chain[MAX_H2T_DESCRIPTOR_DEPTH] = { 0 };
static unsigned short g_mgmt_descriptor_chain[MAX_MGMT_DESCRIPTOR_DEPTH] = { 0 };
static unsigned short g_h2t_descriptor_write_idx = 0;
static unsigned short g_h2t_descriptor_read_idx = 0;
static unsigned short g_mgmt_descriptor_write_idx = 0;
static unsigned short g_mgmt_descriptor_read_idx = 0;

// SOP tracking
static unsigned char g_t2h_sop = 1;
static unsigned char g_mgmt_rsp_sop = 1;

// Memory tracking
static CIRCLE_BUFF g_h2t_rx_cbuff;
static CIRCLE_BUFF g_mgmt_rx_cbuff;

int init_driver() {
    int ret = 0;
    int num_interface = fpga_get_num_of_interfaces();
    if (num_interface == 1)
    {
        if (g_mmio_handle == FPGA_MMIO_INTERFACE_INVALID_HANDLE)
        {
            g_mmio_handle = fpga_open(0);
            if (g_mmio_handle == FPGA_MMIO_INTERFACE_INVALID_HANDLE)
            {
                fpga_throw_runtime_exception(__func__, __FILE__, __LINE__, "MMIO handle cannot be created.");
            }
        }

#ifdef MMIO_LOG
    g_mmio_log_f = fopen("mmlink_mmio_log.csv", "w");
    
    ::setbuf(g_mmio_log_f, NULL);
    ::fprintf(g_mmio_log_f, "CSR base_addr:64'h%llx\n"
                            "H2T base_addr:64'h%llx\n"
                            "T2H base_addr:64'h%llx\n"
                            "line_no,function,type,base_addr,offset,value\n",
                            g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR, g_std_dbg_ip_info.H2T_MEM_BASE_ADDR, g_std_dbg_ip_info.T2H_MEM_BASE_ADDR);
    #endif
    
    if (!g_dbg_info_set) {
        return INIT_ERROR_CODE_MISSING_INFO;
    }
    if (check_version_and_type() != 0) {
        return INIT_ERROR_CODE_INCOMPATIBLE_IP;
    }
    
    assert_h2t_t2h_reset();
    g_h2t_descriptor_write_idx = 0;
    g_h2t_descriptor_read_idx = 0;
    g_mgmt_descriptor_write_idx = 0;
    g_mgmt_descriptor_read_idx = 0;
    g_h2t_descriptor_slots_available = (unsigned short)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_H2T_AVAILABLE_SLOTS);
    g_mgmt_descriptor_slots_available = (unsigned short)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_AVAILABLE_SLOTS);
    g_t2h_sop = 1;
    g_mgmt_rsp_sop = 1;
    cbuff_init(&g_h2t_rx_cbuff, g_std_dbg_ip_info.H2T_MEM_BASE_ADDR, g_std_dbg_ip_info.H2T_MEM_SZ);
    cbuff_init(&g_mgmt_rx_cbuff, g_std_dbg_ip_info.MGMT_MEM_BASE_ADDR, g_std_dbg_ip_info.MGMT_MEM_SZ);
    }
    else
    {
        fpga_msg_printf(FPGA_MSG_PRINTF_ERROR,
                        "Improper ProtoDriver platform initialization!\n");
        ret = -1;
    }

    return ret;
}
// This should be called one time prior to any driver function calls
void set_design_info(ST_DBG_IP_DESIGN_INFO info)
{
    g_std_dbg_ip_info = info;
    g_dbg_info_set = 1;
}

// Returns a non-NULL buffer if there is both space in the H2T memory & H2T descriptor memory.
// Checks to see if any descriptors have been processed by the ST Debug IP, and if so frees
// the associated memory.
uint32_t get_h2t_buffer(size_t sz) {
    // First update available descriptor slots, and free space in the buffer
    unsigned short freed_descriptor_slots = (unsigned short)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_H2T_AVAILABLE_SLOTS) - g_h2t_descriptor_slots_available;
    if (freed_descriptor_slots > 0) {
        g_h2t_descriptor_slots_available += freed_descriptor_slots;
        size_t bytes_freed = 0;
        for (char i = 0; i < freed_descriptor_slots; ++i) {
            bytes_freed += g_h2t_descriptor_chain[(g_h2t_descriptor_read_idx + i) % MAX_H2T_DESCRIPTOR_DEPTH];
        }
        g_h2t_descriptor_read_idx = (g_h2t_descriptor_read_idx + freed_descriptor_slots) % MAX_H2T_DESCRIPTOR_DEPTH;

        // Update the cbuff, freeing up space
        cbuff_free(&g_h2t_rx_cbuff, bytes_freed);
    }

    // Make sure we have space in descriptor mem
    if (g_h2t_descriptor_slots_available > 0) {
        // Make sure we have space in cbuff
        const size_t aligned_sz = GET_ALIGNED_SZ(sz);
        if (g_h2t_rx_cbuff.space_available >= aligned_sz) {
            g_h2t_descriptor_chain[g_h2t_descriptor_write_idx++ % MAX_H2T_DESCRIPTOR_DEPTH] = aligned_sz;
            return cbuff_alloc(&g_h2t_rx_cbuff, aligned_sz);
        }
    }

    return 0;
}

// Assumes there is space in both the buffer and descriptor memory
int push_h2t_data(H2T_PACKET_HEADER *header, uint32_t payload) {
    --g_h2t_descriptor_slots_available;
    unsigned long last_howlong = (header->DATA_LEN_BYTES & ST_DBG_IP_HOW_LONG_MASK);
    if (header->SOP_EOP & H2T_PACKET_HEADER_MASK_EOP) {
        last_howlong |= ST_DBG_IP_LAST_DESCRIPTOR_MASK;
    }
    uint64_t howlong_where = last_howlong | (((uintptr_t)payload) << 32);
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_H2T_HOW_LONG, howlong_where);
    uint64_t connid_channelpush = header->CONN_ID | ((uint64_t)header->CHANNEL << 32);
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_H2T_CONNECTION_ID, connid_channelpush);
    return 0;
}

// Returns a non-NULL buffer if there is both space in the MGMT memory & MGMT descriptor memory.
// Checks to see if any descriptors have been processed by the ST Debug IP, and if so frees
// the associated memory.
uint32_t get_mgmt_buffer(size_t sz) {
    // First update available descriptor slots, and free space in the buffer
    unsigned short freed_descriptor_slots = (unsigned short)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_AVAILABLE_SLOTS) - g_mgmt_descriptor_slots_available;
    if (freed_descriptor_slots > 0) {
        g_mgmt_descriptor_slots_available += freed_descriptor_slots;
        size_t bytes_freed = 0;
        for (char i = 0; i < freed_descriptor_slots; ++i) {
            bytes_freed += g_mgmt_descriptor_chain[(g_mgmt_descriptor_read_idx + i) % MAX_MGMT_DESCRIPTOR_DEPTH];
        }
        g_mgmt_descriptor_read_idx = (g_mgmt_descriptor_read_idx + freed_descriptor_slots) % MAX_MGMT_DESCRIPTOR_DEPTH;

        // Update the cbuff, freeing up space
        cbuff_free(&g_mgmt_rx_cbuff, bytes_freed);
    }

    // Make sure we have space in descriptor mem
    if (g_mgmt_descriptor_slots_available > 0) {
        // Make sure we have space in cbuff
        const size_t aligned_sz = GET_ALIGNED_SZ(sz);
        if (g_mgmt_rx_cbuff.space_available >= aligned_sz) {
            g_mgmt_descriptor_chain[g_mgmt_descriptor_write_idx++ % MAX_MGMT_DESCRIPTOR_DEPTH] = aligned_sz;
            return cbuff_alloc(&g_mgmt_rx_cbuff, aligned_sz);
        }
    }

    return 0;
}

// Assumes there is space in both the buffer and descriptor memory
int push_mgmt_data(MGMT_PACKET_HEADER *header, uint32_t payload) {
    --g_mgmt_descriptor_slots_available;
    unsigned long last_howlong = (header->DATA_LEN_BYTES & ST_DBG_IP_HOW_LONG_MASK);
    if (header->SOP_EOP & MGMT_PACKET_HEADER_MASK_EOP) {
        last_howlong |= ST_DBG_IP_LAST_DESCRIPTOR_MASK;
    }
    uint64_t howlong_where = last_howlong | (((uintptr_t)payload) << 32);
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_HOW_LONG, howlong_where);
    uint64_t channel_id_push = (uint64_t)header->CHANNEL << 32;
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_CHANNEL_ID_PUSH - 0x4, channel_id_push);
    return 0;
}

// Reads out the next T2H data if non-empty
int get_t2h_data(H2T_PACKET_HEADER *header, uint32_t *payload) {
    uint64_t howlong_where = fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_T2H_HOW_LONG);
    uint32_t last_howlong = (uint32_t)howlong_where;
    uint32_t where = (uint32_t)(howlong_where >> 32);

    // Early return no need to do more work if there is no data
    if ((header->DATA_LEN_BYTES = (unsigned short)(last_howlong & ST_DBG_IP_HOW_LONG_MASK)) == 0) {
        return 0;
    }
    *payload = where + g_std_dbg_ip_info.T2H_MEM_BASE_ADDR;
    header->SOP_EOP = 0; // Be sure to clear this!
    if (g_t2h_sop) {
        header->SOP_EOP |= H2T_PACKET_HEADER_MASK_SOP;
    }
    if (last_howlong & ST_DBG_IP_LAST_DESCRIPTOR_MASK) {
        header->SOP_EOP |= H2T_PACKET_HEADER_MASK_EOP;
        g_t2h_sop = 1;
    } else {
        g_t2h_sop = 0;
    }
    uint64_t connid_channelid = fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_T2H_CONNECTION_ID);
    header->CONN_ID = (unsigned char)(connid_channelid);
    header->CHANNEL = (uint16_t)(connid_channelid >> 32);
    return 0;
}

inline void t2h_data_complete()
{
    fpga_write_32(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_T2H_DESCRIPTORS_DONE, 1);
}

// Reads out the next MGMT RSP data if non-empty
int get_mgmt_rsp_data(MGMT_PACKET_HEADER *header, uint32_t *payload) {
    uint64_t howlong_where = fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_RSP_HOW_LONG);
    uint32_t last_howlong = (uint32_t)howlong_where;
    uint32_t where = (uint32_t)(howlong_where >> 32);

    // Early return no need to do more work if there is no data
    header->DATA_LEN_BYTES = last_howlong & ST_DBG_IP_HOW_LONG_MASK;
    if (header->DATA_LEN_BYTES == 0)
    {
        return 0;
    }
    *payload = where + g_std_dbg_ip_info.MGMT_RSP_MEM_BASE_ADDR;
    header->SOP_EOP = 0; // Be sure to clear this!
    if (g_mgmt_rsp_sop) {
        header->SOP_EOP |= H2T_PACKET_HEADER_MASK_SOP;
    }
    if (last_howlong & ST_DBG_IP_LAST_DESCRIPTOR_MASK) {
        header->SOP_EOP |= H2T_PACKET_HEADER_MASK_EOP;
        g_mgmt_rsp_sop = 1;
    } else {
        g_mgmt_rsp_sop = 0;
    }

    header->CHANNEL = (uint32_t)(fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_RSP_CHANNEL_ID_ADVANCE - 0x4) >> 32);
    return 0;
}

void mgmt_rsp_data_complete()
{
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_MGMT_RSP_DESCRIPTORS_DONE, 1);
}

void set_loopback_mode(int val) {
    unsigned long rd = (unsigned long)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK);
    if (val == 1) {
        fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK, rd | ST_DBG_IP_CONFIG_LOOPBACK_FIELD);
    } else {
        fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK, rd & ~ST_DBG_IP_CONFIG_LOOPBACK_FIELD);
    }
}

int get_loopback_mode() {
    unsigned long rd = (unsigned long)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK);
    if ((rd & ST_DBG_IP_CONFIG_LOOPBACK_FIELD) > 0) {
        return 1;
    } else {
        return 0;
    }
}

void enable_interrupts(int val) {
    unsigned long rd = (unsigned long)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK);
    if (val == 1) {
        fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK, rd | ST_DBG_IP_CONFIG_ENABLE_INT_FIELD);
    } else {
        fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK, rd & ~ST_DBG_IP_CONFIG_ENABLE_INT_FIELD);
    }
}

int get_mgmt_support() {
    unsigned long rd = (unsigned long)fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_MGMT_MGMT_RSP_DESC_DEPTH);
    if (rd > 0) {
        return 1;
    } else {
        return 0;
    }
}

int check_version_and_type() {
    uint64_t type_version = fpga_read_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_TYPE);
    unsigned long type = (unsigned long)type_version;
    unsigned long version = (unsigned long)(type_version >> 32);
    if ((type != SUPPORTED_TYPE) || (version != SUPPORTED_VERSION)) {
        return -1;
    } else {
        return 0;
    }
}

void assert_h2t_t2h_reset()
{
    fpga_write_64(g_mmio_handle, g_std_dbg_ip_info.ST_DBG_IP_CSR_BASE_ADDR + ST_DBG_IP_CONFIG_RESET_AND_LOOPBACK, ST_DBG_IP_CONFIG_H2T_T2H_RESET_FIELD);
}

void memcpy64_fpga2host(int32_t fpga_buff, uint64_t *host_buff, size_t len)
{
    size_t transfers = (len + 7) / 8;
    for (size_t i = 0; i < transfers; ++i)
    {
        *host_buff++ = fpga_read_64(g_mmio_handle, fpga_buff);
        fpga_buff += 8;
    }
}

void memcpy64_host2fpga(uint64_t *host_buff, int32_t fpga_buff, size_t len)
{
    size_t transfers = (len + 7) / 8;
    for (size_t i = 0; i < transfers; ++i)
    {
        fpga_write_64(g_mmio_handle, fpga_buff, *host_buff++);
        fpga_buff += 8;
    }
}

int set_driver_param(const char *param, const char *val)
{
    if (strncmp(param, HW_LOOPBACK_PARAM, HW_LOOPBACK_PARAM_LEN) == 0) {
        if (strncmp(val, "1", 1) == 0) {
            set_loopback_mode(1);
        } else {
            set_loopback_mode(0);
        }
    }

    return -1;
}

char *get_driver_param(const char *param) {
    if (strncmp(param, HW_LOOPBACK_PARAM, HW_LOOPBACK_PARAM_LEN) == 0) {
        if (get_loopback_mode() == 0) {
            return "0";
        } else {
            return "1";
        }
    } else if (strncmp(param, MGMT_SUPPORT_PARAM, MGMT_SUPPORT_PARAM_LEN) == 0) {
        if (get_mgmt_support() == 1) {
            return "1";
        } else {
            return "0";
        }
    }

    return NULL;
}
