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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>


#include "intel_fpga_platform_api.h"
#include "remote_dbg.h"
#include "stream_dbg.h"

// etherlink Command line input help
static void show_help(const char *program)
{
    printf(
        "Usage:\n"
        " %s [--uio-driver-path=<path>] [--start-address] [--h2t-t2h-mem-size] [--port=<port>] [--ip=<ip address>]\n"
        " %s --version\n"
        " %s --help\n\n"
        "Optional arguments:\n"
        " --uio-driver-path=<path>, -u <path>       UIO driver path (default: /dev/uio0)\n"
        " --start-address=<address>, -s <address>   JTAG-Over-Protocol interface starting address within this UIO driver (default: 0)\n"
        " --h2t-t2h-mem-size=<size>, -m <size>      JTAG-Over-Protocol H2T/T2H Memory Size in bytes (default: 4096)\n"
        " --port=<port>, -p <port>                  listening port (default: 0)\n"
        " --ip=<ip address>, -i <ip address>        IP address to bind (default: 0.0.0.0)\n"
        " --version, -v                             print version and exit\n"
        " --help, -h                                print the usage description\n"
        "\n"
        "Note:\n"
        " In the device tree, the address span of the whole JTAP over protocol should be binded into the specified UIO driver.\n"
        " Typically, the base address starts at 0x0.\n\n",
        program, program, program);
}

static IRemoteDebug *s_etherlink_server = nullptr;

// Streaming debug command line struct
enum
{
    IP_MAX_STR_LEN = 15
};

struct  EtherlinkCommandLine
{
    size_t  h2t_t2h_mem_size;
    int     port;
    char    ip[IP_MAX_STR_LEN+1];
};

static int parse_cmd_args(EtherlinkCommandLine *etherlink_cmdline, int argc, char *argv[]);
static long parse_integer_arg(const char *name);
static int run_etherlink(const struct EtherlinkCommandLine *etherlink_cmdline);
static void install_sigint_handler();

class StreamingDebug : public IRemoteDebug
{
public:
    StreamingDebug(){}
    virtual ~StreamingDebug(){}
    int run(size_t h2t_t2h_mem_size, const char * /*unused*/, int port) override
    {
        return start_st_dbg_transport_server_over_tcpip(h2t_t2h_mem_size, port);
    }
    void terminate() override
    {
        terminate_st_dbg_transport_server_over_tcpip();
    }

};

int main( int argc, char** argv )
{
    EtherlinkCommandLine etherlink_cmdline = {4096, 0, {0,}};
    int rc = parse_cmd_args(&etherlink_cmdline, argc, argv);
    if ( rc ) {
        printf("ERROR: Error scanning command line; exiting\n");
        show_help(argv[0]);
        goto out_exit;
    }

    printf("INFO: Etherlink Server Configuration:\n");
    printf("INFO:    H2T/T2H Memory Size  : %ld\n", etherlink_cmdline.h2t_t2h_mem_size);
    printf("INFO:    Listening Port       : %d\n", etherlink_cmdline.port);
    printf("INFO:    IP Address           : %s\n", etherlink_cmdline.ip);

    if(fpga_platform_init(argc, (const char **)argv) == false)
    {
        printf("ERROR: Platform failed to initilize; exiting\n");
        rc = -1;
        goto out_exit;
    }

    // Install SIGINT handler
    install_sigint_handler();

    if( run_etherlink(&etherlink_cmdline) != 0) {
        printf("ERROR: Etherlink server failed to start successfully; exiting.\n");
        rc = 3;
    }

out_exit:
    fpga_platform_cleanup();

    return rc;
}

int run_etherlink(const struct EtherlinkCommandLine *etherlink_cmdline )
{
    int res = 0;

    s_etherlink_server = new StreamingDebug();
    if (s_etherlink_server) {
        res = s_etherlink_server->run(etherlink_cmdline->h2t_t2h_mem_size, etherlink_cmdline->ip, etherlink_cmdline->port);
        delete s_etherlink_server;
        s_etherlink_server = nullptr;
    }

    return res;
}

// parse Input command line
int parse_cmd_args(EtherlinkCommandLine *etherlink_cmdline, int argc, char *argv[])
{
    int option_index = 0;
    int c;

    const char *GETOPT_STRING = "hp:i:v";

    struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"h2t-t2h-mem-size", required_argument, NULL, 'm'},
        {"port", required_argument, NULL, 'p'},
        {"ip", required_argument, NULL, 'i'},
        {0, 0, 0, 0}};

    opterr = 0; // Suppress stderr output from getopt_long upon unrecognized options
    optind = 0; // Reset getopt_long position.
    
    while(1)
    {
        c = getopt_long(argc, (char * const*)argv, GETOPT_STRING, longopts, &option_index);
      
        if (c == -1)
        {
            break;
        }
        
        switch(c)
        {
            case 'v':
                
                break;
                
            case 'h':
                // Command line help
                show_help(argv[0]);
                return -2;
                break;

            case 'm':
                // H2T/T2H Mem Size
                etherlink_cmdline->h2t_t2h_mem_size = parse_integer_arg("h2t-t2h-mem-size");
                break;

            case 'p':
                // TCP Port
                etherlink_cmdline->port = parse_integer_arg("port");
                break;

            case 'i':
                // Ip address
                strncpy(etherlink_cmdline->ip, optarg, 15);
                etherlink_cmdline->ip[15] = '\0';
                break;
        }
    }

    if (etherlink_cmdline->ip[0] == '\0')
    {
        strncpy(etherlink_cmdline->ip, "0.0.0.0", sizeof(etherlink_cmdline->ip));
    }

    return 0;
}

long parse_integer_arg(const char *name)
{
    long ret = 0;

    bool is_all_digit = true;
    char *p;
    typedef int (*DIGIT_TEST_FN)(int c);
    DIGIT_TEST_FN is_acceptabl_digit;
    if (optarg[0] == '0' && (optarg[1] == 'x' || optarg[1] == 'X'))
    {
        is_acceptabl_digit = isxdigit;
        optarg += 2; // trim the "0x" portion
    }
    else
    {
        is_acceptabl_digit = isdigit;
    }

    for (p = optarg; (*p) != '\0'; ++p)
    {
        if (!is_acceptabl_digit(*p))
        {
            is_all_digit = false;
            break;
        }
    }

    if (is_acceptabl_digit == isxdigit)
    {
        optarg -= 2; // restore the "0x" portion
    }

    if (is_all_digit)
    {
        if (sizeof(size_t) <= sizeof(long))
        {
            ret = (size_t)strtol(optarg, NULL, 0);
            if (errno == ERANGE)
            {
                ret = 0;
                printf("%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, LONG_MAX);
            }
        }
        else
        {
            long span, span_c;
            span = strtol(optarg, NULL, 0);
            if (errno == ERANGE)
            {
                ret = 0;
                printf("%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, LONG_MAX);
            }
            else
            {
                ret = (size_t)span;
                span_c = ret;
                if (span != span_c)
                {
                    printf("%s value is too big. %s is provided; maximum accepted is %ld", name, optarg, (size_t)-1);
                    ret = 0;
                }
            }
        }
    }
    else
    {
        printf("Invalid argument value type is provided. A integer value is expected. %s is provided.", optarg);
    }

    return ret;
}

void etherlink_sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("\nINFO: Signal, SIGINT, was triggered; the program is terminating.\n");
        fpga_platform_cleanup();
        if (s_etherlink_server != nullptr)
        {
            delete s_etherlink_server;
            s_etherlink_server = nullptr;
        }
        exit(0);
    }
    else
    {
        printf("WARNING: Unexpected signal, %d, triggered; it is ignored\n", signo);
    }
}

void install_sigint_handler()
{
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = &etherlink_sig_handler;
    sig_action.sa_flags = 0;

    if (sigaction(SIGINT, &sig_action, NULL) != 0)
    {
        printf("WARNING: SIGINT handler installment failed; this program will not terminate gracefully.\n");
    }
}