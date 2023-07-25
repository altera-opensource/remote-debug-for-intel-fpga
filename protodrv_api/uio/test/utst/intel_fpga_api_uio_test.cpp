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

#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <iostream>
using namespace std;

#include "gtest/gtest.h"

#include "intel_fpga_api_uio.h"
#include "intel_fpga_api_cmn_msg.h"

extern int optind;

static ostringstream             *s_uio_msg_oss;

static const int MSG_BUFFER_SIZE = 4096;
static char s_msg_buffer[MSG_BUFFER_SIZE];
   
static int s_uio_utst_printf(FPGA_MSG_PRINTF_TYPE type, const char * format, va_list args)
{
    int ret;
    
    switch(type)
    {
        case FPGA_MSG_PRINTF_INFO:
            *s_uio_msg_oss << "INFO: ";
            break;
        case FPGA_MSG_PRINTF_WARNING:
            *s_uio_msg_oss << "WARNING: ";
            break;
        case FPGA_MSG_PRINTF_ERROR:
            *s_uio_msg_oss << "ERROR: ";
            break;
        case FPGA_MSG_PRINTF_DEBUG:
            *s_uio_msg_oss << "DEBUG: ";
            break;
        //default:
            // FAIL() << "invalid message print type is specified.";
    }

    s_msg_buffer[0] = '\0';
    ret = ::vsnprintf( s_msg_buffer, MSG_BUFFER_SIZE, format, args );
    *s_uio_msg_oss << s_msg_buffer;
    
    return ret;
}

static void s_uio_utst_exception_handler(const char *function, const char *file, int lineno, const char * format, va_list args)
{
    s_msg_buffer[0] = '\0';
    ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Exception occured in %s() at line %d in %s due to ", function, lineno, file );
    *s_uio_msg_oss << s_msg_buffer;
    
    s_msg_buffer[0] = '\0';
    ::vsnprintf( s_msg_buffer, MSG_BUFFER_SIZE, format, args );
    *s_uio_msg_oss << s_msg_buffer;
}

class MMIO : public ::testing::Test  
{
public:
    void SetUp()
    {
        optind = 0;     // Reset getopt_long position.
        s_uio_msg_oss = &m_uio_msg_oss;
        fpga_platform_register_printf(s_uio_utst_printf);
        fpga_platform_register_runtime_exception_handler(s_uio_utst_exception_handler); 
        const char *argv_valid[] =
        {
            "program",
            "--single-component-mode",        
            "--uio-driver-path=/dev/uio0",        
            "--address-span=4096"
        };
        
        bool rc = fpga_platform_init(4, argv_valid);
        EXPECT_TRUE(rc);
        
        EXPECT_STREQ( 
            "INFO: UIO Platform Configuration:"
            "INFO:    Driver Path: /dev/uio0"
            "INFO:    Address Span: 4096"
            "INFO:    Start Address: 0x0",
            m_uio_msg_oss.str().c_str() );
        
        unsigned int num_inf = fpga_get_num_of_interfaces();
        EXPECT_EQ(1, (int)num_inf);
        
        m_handle = fpga_open(0);
        EXPECT_TRUE(m_handle != FPGA_MMIO_INTERFACE_INVALID_HANDLE);
    }

    void TearDown()
    {
        fpga_close(0);
        
        fpga_platform_cleanup();
    }
   

protected:

    FPGA_MMIO_INTERFACE_HANDLE  m_handle;
    ostringstream               m_uio_msg_oss;
};

TEST_F(MMIO, should_deal_with_invalid_open)
{
    FPGA_MMIO_INTERFACE_HANDLE handle = fpga_open(0);
    EXPECT_EQ(FPGA_MMIO_INTERFACE_INVALID_HANDLE, handle);
    
    handle = fpga_open(1);
    EXPECT_EQ(FPGA_MMIO_INTERFACE_INVALID_HANDLE, handle);
}


TEST_F(MMIO, should_deal_with_mmio_8)
{
    int i;
    uint32_t addr;
    uint8_t  rdata;
    const unsigned int  TEST_BITS = 8;
    const unsigned int  NUM_UNIT_PER_64_BIT = 64/TEST_BITS;
    const unsigned int  BYTES_PER_64_BIT = 64/8;
    const unsigned int  BYTES_PER_UNIT = TEST_BITS/8;
    const unsigned int  START_OFFSET = 0;
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i;
        rdata = fpga_read_8(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        //EXPECT_TRUE(s_is_expected_mmio_value(0xff, rdata, s_msg_buffer));
        EXPECT_EQ(0xff, rdata);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_64_BIT + i * BYTES_PER_UNIT;
        fpga_write_8(m_handle, addr + START_OFFSET, (uint8_t)i);
    }
    
    for(i = 0; (unsigned int)i < (NUM_UNIT_PER_64_BIT * NUM_UNIT_PER_64_BIT); ++i)
    {
        addr = i * BYTES_PER_UNIT;
        rdata = fpga_read_8(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Read after write iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        if (((addr % BYTES_PER_64_BIT) / BYTES_PER_UNIT) == ((unsigned int)i / BYTES_PER_64_BIT))
        {
            EXPECT_EQ((i/NUM_UNIT_PER_64_BIT), rdata);
        }
        else
        {
            EXPECT_EQ(0xff, rdata);
        }
    }
}


TEST_F(MMIO, should_deal_with_mmio_16)
{
    int i;
    uint32_t addr;
    uint16_t  rdata;
    const int  TEST_BITS = 16;
    const int  NUM_UNIT_PER_64_BIT = 64/TEST_BITS;
    const int  BYTES_PER_64_BIT = 64/8;
    const int  BYTES_PER_UNIT = TEST_BITS/8;
    const int  START_OFFSET = 128;
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i;
        rdata = fpga_read_16(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(0xffff, rdata);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_64_BIT + i * BYTES_PER_UNIT;
        fpga_write_16(m_handle, addr + START_OFFSET, (uint16_t)i);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_UNIT;
        rdata = fpga_read_16(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Read after write iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        if (((addr % BYTES_PER_64_BIT) / BYTES_PER_UNIT) == ((unsigned int)i / NUM_UNIT_PER_64_BIT))
        {
            EXPECT_EQ((i/NUM_UNIT_PER_64_BIT), rdata);
        }
        else
        {
            EXPECT_EQ(0xffff, rdata);
        }
    }
}


TEST_F(MMIO, should_deal_with_mmio_32)
{
    int i;
    uint32_t addr;
    uint32_t  rdata;
    const int  TEST_BITS = 32;
    const int  NUM_UNIT_PER_64_BIT = 64/TEST_BITS;
    const int  BYTES_PER_64_BIT = 64/8;
    const int  BYTES_PER_UNIT = TEST_BITS/8;
    const int  START_OFFSET = 256;
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i;
        rdata = fpga_read_32(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(0xffffffff, rdata);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_64_BIT + i * BYTES_PER_UNIT;
        fpga_write_32(m_handle, addr + START_OFFSET, (uint32_t)i);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_UNIT;
        rdata = fpga_read_32(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Read after write iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        if (((addr % BYTES_PER_64_BIT) / BYTES_PER_UNIT) == ((unsigned int)i / NUM_UNIT_PER_64_BIT))
        {
            EXPECT_EQ((i/NUM_UNIT_PER_64_BIT), (int)rdata);
        }
        else
        {
            EXPECT_EQ(0xffffffff, rdata);
        }
    }
}


TEST_F(MMIO, should_deal_with_mmio_64)
{
    int i;
    uint32_t addr;
    uint64_t  rdata;
    const int  TEST_BITS = 64;
    const int  NUM_UNIT_PER_64_BIT = 64/TEST_BITS;
    const int  BYTES_PER_64_BIT = 64/8;
    const int  BYTES_PER_UNIT = TEST_BITS/8;
    const int  START_OFFSET = 384;
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i;
        rdata = fpga_read_64(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(0xffffffffffffffff, rdata);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_64_BIT + i * BYTES_PER_UNIT;
        fpga_write_64(m_handle, addr + START_OFFSET, (uint64_t)i);
    }
    
    for(i = 0; (unsigned int)i < NUM_UNIT_PER_64_BIT*NUM_UNIT_PER_64_BIT; ++i)
    {
        addr = i * BYTES_PER_UNIT;
        rdata = fpga_read_64(m_handle, addr + START_OFFSET);
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Read after write iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        if (((addr % BYTES_PER_64_BIT) / BYTES_PER_UNIT) == ((unsigned int)i / NUM_UNIT_PER_64_BIT))
        {
            EXPECT_EQ((i/NUM_UNIT_PER_64_BIT), (int)rdata);
        }
        else
        {
            EXPECT_EQ(0xffffffffffffffff, rdata);
        }
    }
}



TEST_F(MMIO, should_deal_with_mmio_512)
{
    int i;
    uint8_t  rdata[512/8];
    const int  TEST_BITS = 512;
    const int  BYTES_PER_UNIT = TEST_BITS/8;
    const int  START_OFFSET = 1024;
    
    fpga_read_512(m_handle, START_OFFSET, rdata);
    for(i = 0; (unsigned int)i < BYTES_PER_UNIT; ++i)
    {
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(0xff, rdata[i]);
    }
    
    for(i = 0; (unsigned int)i < BYTES_PER_UNIT; ++i)
    {
        rdata[i] = i;
        
    }
    fpga_write_512(m_handle, START_OFFSET, rdata);
    
    fpga_read_512(m_handle, START_OFFSET, rdata);
    for(i = 0; (unsigned int)i < BYTES_PER_UNIT; ++i)
    {
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Read after write read iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(i, rdata[i]);
    }    
    
    fpga_read_512(m_handle, START_OFFSET + BYTES_PER_UNIT, rdata);
    for(i = 0; (unsigned int)i < BYTES_PER_UNIT; ++i)
    {
        s_msg_buffer[0] = '\0';
        ::snprintf( s_msg_buffer, MSG_BUFFER_SIZE, "Initial read in the range not written iteration (i): %d", i );
        SCOPED_TRACE(s_msg_buffer);
        EXPECT_EQ(0xff, rdata[i]);
    }
    
}
