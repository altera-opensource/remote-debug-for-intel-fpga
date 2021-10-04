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

#include "intel_fpga_platform_api_uio.h"

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
            //FAIL("invalid message print type is specified.");
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

class Argument : public ::testing::Test  
{
public:
    void SetUp()
    {
        s_uio_msg_oss = &m_uio_msg_oss;
        fpga_platform_register_printf(s_uio_utst_printf);
	fpga_platform_register_runtime_exception_handler(s_uio_utst_exception_handler);
    }

    void TearDown()
    {
    }
   

protected:

    ostringstream             m_uio_msg_oss;
};

TEST_F(Argument, should_deal_with_null_argument)
{
    bool rc = fpga_platform_init(0, NULL);
    EXPECT_FALSE(rc);
    
    EXPECT_STREQ( 
        "ERROR: No valid address span value is provided using the argument, --address-span."
        "ERROR: UIO driver path is not provided using the argument, --uio-driver-path.", 
        m_uio_msg_oss.str().c_str() );
    
    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_no_argument)
{
    const char *argv_invalid[] =
    {
        "program"
    };
    
    bool rc = fpga_platform_init(1, argv_invalid);
    EXPECT_FALSE(rc);
    
    EXPECT_STREQ( 
        "ERROR: No valid address span value is provided using the argument, --address-span."
        "ERROR: UIO driver path is not provided using the argument, --uio-driver-path.", 
        m_uio_msg_oss.str().c_str() );
    
    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_random_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "blah",
        "-c",
        "foo",
        "--x=y"
    };
    
    bool rc = fpga_platform_init(5, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ( 
        "ERROR: No valid address span value is provided using the argument, --address-span."
        "ERROR: UIO driver path is not provided using the argument, --uio-driver-path.", 
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_one_valid_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0"
    };
    
    bool rc = fpga_platform_init(2, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ( 
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_invalid_span_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0",
        "--address-span=0"
    };
    
    bool rc = fpga_platform_init(3, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ( 
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_invalid_span_1_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--address-span=012abc",
        "--uio-driver-path=/dev/uio0"       
    };
    
    bool rc = fpga_platform_init(3, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ(
        "ERROR: Invalid argument value type is provided. A integer value is expected. 012abc is provided."
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_invalid_span_2_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--address-span=0xabcdefg",
        "--uio-driver-path=/dev/uio0"       
    };
    
    bool rc = fpga_platform_init(3, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ(
        "ERROR: Invalid argument value type is provided. A integer value is expected. 0xabcdefg is provided."
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_invalid_span_3_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0",        
        "--address-span=9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999"
    };
    
    bool rc = fpga_platform_init(3, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ( 
        "ERROR: Address span value is too big. 9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999 is provided; maximum accepted is 9223372036854775807"
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_invalid_span_4_argument)
{
    const char *argv_invalid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0",        
        "--address-span=0xffffffffffffffffffffffffffffffffffffffff"
    };
    
    bool rc = fpga_platform_init(3, argv_invalid);
    EXPECT_FALSE(rc);   
    
    EXPECT_STREQ( 
        "ERROR: Address span value is too big. 0xffffffffffffffffffffffffffffffffffffffff is provided; maximum accepted is 9223372036854775807"
        "ERROR: No valid address span value is provided using the argument, --address-span.",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_valid_argument_dec_span)
{
    const char *argv_valid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0",        
        "--address-span=12345678"
    };
    
    bool rc = fpga_platform_init(3, argv_valid);
    EXPECT_TRUE(rc);
    
    EXPECT_STREQ( 
        "INFO: UIO Platform Configuration:"
        "INFO:    Driver Path: /dev/uio0"
        "INFO:    Address Span: 12345678"
        "INFO:    Start Address: 0x0",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_valid_argument_hex_span)
{
    const char *argv_valid[] =
    {
        "program",
        "--uio-driver-path=/dev/uio0",        
        "--address-span=0x12345678"
    };
    
    bool rc = fpga_platform_init(3, argv_valid);
    EXPECT_TRUE(rc);
    
    EXPECT_STREQ( 
        "INFO: UIO Platform Configuration:"
        "INFO:    Driver Path: /dev/uio0"
        "INFO:    Address Span: 305419896"
        "INFO:    Start Address: 0x0",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}


TEST_F(Argument, should_deal_with_valid_argument_with_optional_single_mode)
{
    const char *argv_valid[] =
    {
        "program",
        "--single-component-mode",        
        "--uio-driver-path=/dev/uio0",        
        "--address-span=0x12345678"
    };
    
    bool rc = fpga_platform_init(4, argv_valid);
    EXPECT_TRUE(rc);
    
    EXPECT_STREQ( 
        "INFO: UIO Platform Configuration:"
        "INFO:    Driver Path: /dev/uio0"
        "INFO:    Address Span: 305419896"
        "INFO:    Start Address: 0x0",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}

TEST_F(Argument, should_deal_with_valid_argument_with_optional_start_addr)
{
    const char *argv_valid[] =
    {
        "program",
        "--start-address=0x1234",
        "--single-component-mode",        
        "--uio-driver-path=/dev/uio0",        
        "--address-span=0x12345678"
    };
    
    bool rc = fpga_platform_init(5, argv_valid);
    EXPECT_TRUE(rc);
    
    EXPECT_STREQ( 
        "INFO: UIO Platform Configuration:"
        "INFO:    Driver Path: /dev/uio0"
        "INFO:    Address Span: 305419896"
        "INFO:    Start Address: 0x1234",
        m_uio_msg_oss.str().c_str() );

    fpga_platform_cleanup();
}
