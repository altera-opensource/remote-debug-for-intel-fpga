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
#include <sys/mman.h>
#include <new>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>


#include "intel_fpga_platform_api.h"
#include "remote_dbg.h"
#include "stream_dbg.h"

// mmlink Command line struct
struct  MMLinkCommandLine
{
	int      port;
	char     ip[16];
};

struct MMLinkCommandLine mmlinkCmdLine = { 0, { 0, } };

// mmlink Command line input help
void MMLinkAppShowHelp()
{
	printf("Usage:\n");
	printf("etherlink\n");
	printf("<UIO Driver Path>     --uio-path=<PATH>                "
		"OR  -U <PATH>\n");
	printf("<TCP PORT>            --port=<PORT>                "
		"OR  -P <PORT>\n");
	printf("<IP ADDRESS>          --ip=<IP ADDRESS>            "
		"OR  -I <IP ADDRESS>\n");
	printf("<Version>             -v,--version Print version and exit\n");
	printf("\n");

}

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)                    \
		do {                                       \
			if ((res) != FPGA_OK) {            \
				print_err((desc), (res));  \
				goto label;                \
			}                                  \
		} while (0)

void print_err(const char *s)
{
	fprintf(stderr, "Error: %s\n", s);
}

remote_dbg *srv = nullptr;

static int parse_cmd_args(struct MMLinkCommandLine *mmlinkCmdLine,
		int argc,
		char *argv[]);
static long parse_integer_arg(const char *name);
static int run_mmlink(const struct MMLinkCommandLine *mmlinkCmdLine);

int main( int argc, char** argv )
{
	int res;

	// Parse command line
	if ( argc < 2 ) {
		MMLinkAppShowHelp();
		return 1;
	}


    int rc = parse_cmd_args(&mmlinkCmdLine, argc, argv);
	if ( rc ) {
		printf("Error scanning command line.\n");
		goto out_exit;
	}

	if ('\0' == mmlinkCmdLine.ip[0]) {
		strncpy(mmlinkCmdLine.ip, "0.0.0.0", 8);
		mmlinkCmdLine.ip[7] = '\0';
	}
 
 	printf(" Port             : %d\n", mmlinkCmdLine.port);
	printf(" IP address       : %s\n", mmlinkCmdLine.ip);

	fpga_platform_init(argc, (const char **)argv);

	fflush(stdout);


	// Signal Handler
	//signal(SIGINT, mmlink_sig_handler);
	//struct sigaction act_old, act_new;
	//act_new.sa_handler = mmlink_sig_handler;
	//sigaction(SIGINT, &act_new, &act_old);

	if( run_mmlink(&mmlinkCmdLine) != 0) {
		printf("Failed to connect MMLINK  \n.");
		rc = 3;
	}

out_exit:
	fpga_platform_cleanup();

	return rc;
}

int run_mmlink(const struct MMLinkCommandLine *mmlinkCmdLine )
{
	int res                        = 0;

    auto  srv = new stream_dbg();
	if (srv) {
		res = srv->run(mmlinkCmdLine->ip, mmlinkCmdLine->port);
		delete srv;
		srv = 0;
	}

	return res;
}

// parse Input command line
int parse_cmd_args(struct MMLinkCommandLine *mmlinkCmdLine, int argc, char *argv[])
{
	int getopt_ret   = 0;
    int option_index = 0;
    int c;

    const char *GETOPT_STRING = "hp:i:v";

struct option longopts[] = {
	{ "help",        no_argument,       NULL, 'h' },
	{ "version",     no_argument,       NULL, 'v' },
	{ "port",        required_argument, NULL, 'p' },
	{ "ip",          required_argument, NULL, 'i' },
	{ 0,             0,                 0,    0   }
};

    opterr = 0; // Suppress stderr output from getopt_long upon unrecognized options
    optind = 0;     // Reset getopt_long position.
    
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
				MMLinkAppShowHelp();
				return -2;
				break;

			case 'p':
				// TCP Port
				mmlinkCmdLine->port = parse_integer_arg("Port");
				break;

			case 'i':
				// Ip address
				strncpy(mmlinkCmdLine->ip, optarg, 15);
				mmlinkCmdLine->ip[15] = '\0';
				break;
		}
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
