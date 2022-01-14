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
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "intel_fpga_api_uio.h"

/*
 * This testcase is showing:
 * -> How user's ISR can be registered.
 * -> How UIO interrupt can be enabled and disabled.
 * -> How interrupt Thread will be created and destroyed.
 */

static int isr_count = 0;

static void my_isr(){

       printf("UIO interrupt triggered by user.\n");
       isr_count++;
}

int main(void){

    const char *argv_valid[] =
    {
        "program",
        "--single-component-mode",
        "--uio-driver-path=/dev/uio0",
        "--address-span=4096"
    };
    printf("############################\n");
    printf("###Interrupt test started###\n");
    printf("############################\n");

    printf("Fpga platform init\n");
    bool rc = fpga_platform_init(4, argv_valid);

    if (rc != true){
        printf("Test Error: fpga_platform_init\n");
        return -1;
    }

    FPGA_MMIO_INTERFACE_HANDLE m_handle = fpga_open(0);
    if (m_handle == FPGA_MMIO_INTERFACE_INVALID_HANDLE){
        printf("Test Error: fpga_open\n");
        return -1;
    }

    //Register user ISR
    fpga_register_isr(m_handle, &my_isr, NULL);

    //Enable UIO interrupt
    fpga_enable_interrupt(m_handle);
    printf("UIO interrupt enabled. Please trigger at least 1 interrupt within 20s.\n");
    sleep(20);

    //Disable UIO interrupt
    fpga_disable_interrupt(m_handle);
    printf("UIO interrupt disabled\n");
    sleep(3);

    fpga_close(0);

    //Interrupt Thread should exit successfully
    fpga_platform_cleanup();

    if(isr_count > 0){
       printf("Interrupt test PASS. %d interrupt was triggered.\n", isr_count);
    }else{
        printf("Interrupt test FAIL. No interrupt was triggered.\n");
    }

    printf("############################\n");
    printf("###End of interrupt test####\n");
    printf("############################\n");
    return 0;
}
