// Copyright(c) 2020, Intel Corporation
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
#include "stream_dbg.h"
#include "server.h"
#include "st_dbg_ip_driver.h"
#include "platform.h"
#include "intel_fpga_api.h"

#define SERVER_PORT_FILE ".intel_reserved_debug_server.port"
enum {
  CTRL_RX_BUFF_SZ = 512,
  CTRL_TX_BUFF_SZ = 512
};
static char g_ctrl_rx_buff[CTRL_RX_BUFF_SZ] = {0};
static char g_ctrl_tx_buff[CTRL_TX_BUFF_SZ] = {0};

SERVER_HW_CALLBACKS get_hw_callbacks() {
  SERVER_HW_CALLBACKS result = SERVER_HW_CALLBACKS_default;
  result.init_driver = init_driver;
  result.has_mgmt_support = get_mgmt_support;
  result.set_param = set_driver_param;
  result.get_param = get_driver_param;
  result.get_h2t_buffer = get_h2t_buffer;
  result.h2t_data_received = push_h2t_data;
  result.acquire_t2h_data = get_t2h_data;
  result.t2h_data_complete = t2h_data_complete;
#if ENABLE_MGMT != 0
  result.get_mgmt_buffer = get_mgmt_buffer;
  result.mgmt_data_received = push_mgmt_data;
  result.acquire_mgmt_rsp_data = get_mgmt_rsp_data;
  result.mgmt_rsp_data_complete = mgmt_rsp_data_complete;
#endif
  return result;
}

// These addresses are of those that should be used with FPGA MMIO to gain access
// to H2T, T2H, etc memories attached to ST DBG IP
ST_DBG_IP_DESIGN_INFO get_design_info() {
  ST_DBG_IP_DESIGN_INFO result;
  result.ST_DBG_IP_CSR_BASE_ADDR = ST_DBG_IF_BASE;
  result.H2T_MEM_BASE_ADDR = H2T_MEM_BASE;
  result.H2T_MEM_SZ = H2T_MEM_SPAN;
  result.T2H_MEM_BASE_ADDR = T2H_MEM_BASE;
  result.T2H_MEM_SZ = T2H_MEM_SPAN;
#if ENABLE_MGMT != 0
  result.MGMT_MEM_BASE_ADDR = MGMT_MEM_BASE;
  result.MGMT_MEM_SZ = MGMT_MEM_SPAN;
  result.MGMT_RSP_MEM_BASE_ADDR = MGMT_RSP_MEM_BASE;
  result.MGMT_RSP_MEM_SZ = MGMT_RSP_MEM_SPAN;
#else
  result.MGMT_MEM_BASE_ADDR = 0;
  result.MGMT_MEM_SZ = 0;
  result.MGMT_RSP_MEM_BASE_ADDR = 0;
  result.MGMT_RSP_MEM_SZ = 0;
#endif
  return result;
}


int stream_dbg::run(const char *address, int port)
{
  int ret = 0;

  (void)address;
  // Implement Nathan server here...
  ST_DBG_IP_DESIGN_INFO design_info = get_design_info();
  set_design_info(design_info);

  SERVER_BUFFERS buffers = SERVER_BUFFERS_default;
  buffers.use_wrapping_data_buffers = 1;
  buffers.ctrl_rx_buff = g_ctrl_rx_buff;
  buffers.ctrl_rx_buff_sz = CTRL_RX_BUFF_SZ;
  buffers.ctrl_tx_buff = g_ctrl_tx_buff;
  buffers.ctrl_tx_buff_sz = CTRL_TX_BUFF_SZ;
  buffers.h2t_rx_buff = design_info.H2T_MEM_BASE_ADDR;
  buffers.h2t_rx_buff_sz = design_info.H2T_MEM_SZ;
  buffers.t2h_tx_buff = design_info.T2H_MEM_BASE_ADDR;
  buffers.t2h_tx_buff_sz = design_info.T2H_MEM_SZ;
  buffers.mgmt_rx_buff = design_info.MGMT_MEM_BASE_ADDR;
  buffers.mgmt_rx_buff_sz = design_info.MGMT_MEM_SZ;
  buffers.mgmt_rsp_tx_buff = design_info.MGMT_RSP_MEM_BASE_ADDR;
  buffers.mgmt_rsp_tx_buff_sz = design_info.MGMT_RSP_MEM_SZ;

  SERVER_CONN server_conn = SERVER_CONN_default;
  server_conn.buff = &buffers;
  server_conn.hw_callbacks = get_hw_callbacks();

    if (initialize_server((unsigned short)port, &server_conn, SERVER_PORT_FILE) == OK)
    {
      server_main(MULTIPLE_CLIENTS, &server_conn);
    }
    else
    {
      fpga_msg_printf(FPGA_MSG_PRINTF_ERROR,
                      "Server failed to initialize, no further attempts will be made!\n");
      ret = -1;
    }

  return ret;
}

void stream_dbg::terminate()
{
  fpga_close(0);
  server_terminate();
}

