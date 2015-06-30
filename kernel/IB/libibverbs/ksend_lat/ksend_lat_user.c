#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <byteswap.h>
#include <errno.h>
#include <stdlib.h>

#include <mlxkvl/kvl_usr.h>
#include <vl.h>

typedef u_int64_t cycles_t;
#include "ksend_lat.h"


//
// Definitions and macros
// 

#define DEFAULT_HCA_ID		"mlx4_0"
#define DEFAULT_TCP_PORT	19875
#define DEFAULT_ITERS		1000
#define DEFAULT_TX_DEPTH	50
#define DEFAULT_PAYLOAD_SIZE	1

#define MSGFD			stderr
#define PRINT_DONE_FAILED(ok)	fprintf(MSGFD, "%s\n", ok ? "done" : "failed")

typedef struct _report_options_t {
	int unsorted;
	int histogram;
	int cycles;
} report_options_t;


//
// Prototypes
//

static void params_cpu_to_be(params_t*);
static void params_be_to_cpu(params_t*);
static void print_params(void);
static int connect_tcp (struct VL_sock_t *);
static int process_arg (
		       IN const int opt_index,
		       IN const char *equ_ptr,
		       IN const int arr_size,
		       IN const struct VL_usage_descriptor_t
		       *usage_desc_arr);
static void print_report(report_options_t *options, unsigned int iters, cycles_t *tstamp, int size);


//
// Usage descriptors
//

enum {
	OPT_HELP,
	OPT_DEV,
	OPT_ITR,
	OPT_IP,
	OPT_PORT,
	OPT_SIZE,
	OPT_CON,
	OPT_IB_PORT,
	OPT_TX,
	OPT_EVENT,
	OPT_CYCLES,
	OPT_HIST,
	OPT_UNSORTED,
	OPT_MTU,
	OPT_ALL,
	OPT_INLINE
};


struct VL_usage_descriptor_t usage_descriptor[] = {
	{
		'h', "help", "",
		"Print this message and exit",
		OPT_HELP
	},
	{
		'd', "ib-dev", "<dev>",
		"HCA to use (default:mlx4_0)",
		OPT_DEV
	},
	{
		'n', "iters", "<iters>",
		"number of iterations (default:1000)",
		OPT_ITR
	},
	{
		'a', "all", "",
		"run for payload sizes from 1 to 2^12",
		OPT_ALL
	},
	{
		'i', "ip", "<deamon ip>",
		"IP address of daemon (required for client)",
		OPT_IP
	},
	{
		'p', "port", "<port>",
		"tcp port to use (default:19875)",
		OPT_PORT
	},
	{
		's', "size", "<size>",
		"payload size in bytes (default:1)",
		OPT_SIZE
	},
	{
		'c', "connection", "<RC/UD>",
		"connection transport type (default:RC)",
		OPT_CON
	},
	{
		'm', "mtu", "<mtu>",
		"mtu size (default:2048)",
		OPT_MTU
	},
	{
		'b', "ib_port", "<port>",
		"ib port (default 1)",
		OPT_IB_PORT
	},
	{
		't', "tx-depth", "<depth>",
		"outstanding send descriptors (default:50)",
		OPT_TX
	},
	{
		'e', "events", "<0/1/2>",
		"recieve context: 0-polling, 1-blocking, 2-interrupt (default:0)",
		OPT_EVENT
	},
	{
		'C', "report-cycles", "",
		"report times in cpu cycles (default: microseconds)",
		OPT_CYCLES
	},
	{
		'H', "report-histogram", "",
		"print all results",
		OPT_HIST
	},
	{
		'U', "report-unsorted", "",
		"print sorted results (implies -H)",
		OPT_UNSORTED
	},
	{
                'I', "inline", "",
                "use inline send (default: No)",
                OPT_INLINE
	 },
};



//
// global variables
//

params_t ksl_params = {
	TRANSPORT_RC,
	1,
	DEFAULT_ITERS, // iter
	50,            // tx depth
	1,             // deamon
	2048,          // MTU
	0,

	DEFAULT_HCA_ID,
	1,
	0,
	0,

	"",
	DEFAULT_TCP_PORT,

	0,
	0,

	INIT,
	SEND_LAT_POLL,
	0,
	0,

	0,
	0,
	0,
	0,
	NULL
};

report_options_t report_options = { 0, 0, 0 };



//
// Entry point
// 

int
main (int argc, char *argv[])
{
	struct VL_sock_t sock;
	int fd = -1;
	int rc;
	int rc_cleanup;
	int i;
	int size;
	params_t r_ksl_params; // remote parameters

	// Parse args
	rc = VL_parse_argv (argc, argv, (sizeof (usage_descriptor) / sizeof (struct VL_usage_descriptor_t)),
			    usage_descriptor, process_arg);
	if (rc < 0) goto exit;

	if (ksl_params.all)
		ksl_params.payload_size = 1 << 12;
	// self pointer so kernel can copy data to user
	ksl_params.user_params = &ksl_params;

	// Connect tcp
	fprintf(MSGFD, "Trying to connect over TCP...");
	rc = connect_tcp(&sock);
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto exit;

	// Open dev
	fprintf(MSGFD, "Opening device...");
	//rc = VL_udriver_generic_open(MOD_NAME, &fd);
	fd = kvl_dev_open(MOD_NAME);
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto exit;

	// Client sends to deamon configuration parameters
	params_cpu_to_be(&ksl_params);
	fprintf(MSGFD, "Exchanging test parameters...");
	rc = VL_sock_sync_data (&sock, sizeof (params_t), &ksl_params, &r_ksl_params);
	params_be_to_cpu(&ksl_params);
	params_be_to_cpu(&r_ksl_params);
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto exit;

	// If we are the deamon, update additional configuration parameters provided to us
	// by the client
	if (ksl_params.deamon) {
		ksl_params.iter = r_ksl_params.iter;
		ksl_params.payload_size = r_ksl_params.payload_size;
		ksl_params.transport = r_ksl_params.transport;
		ksl_params.tx_depth = r_ksl_params.tx_depth;
		ksl_params.type = r_ksl_params.type;
		ksl_params.all = r_ksl_params.all;
		ksl_params.send_inline = r_ksl_params.send_inline;
	}
	print_params();

	// Initialize IB resources
	fprintf(MSGFD, "Initializing IB resources...");
	ksl_params.cmd = INIT;
	rc = write(fd, &ksl_params, sizeof(params_t)); // Although this is a little wierd, the write syscall
						      // updates 'ksl_params'...
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto exit;

	// Exchange IB parameters
	fprintf(MSGFD, "Exchanging IB parameters...");
	params_cpu_to_be(&ksl_params);
	rc = VL_sock_sync_data (&sock, sizeof (params_t), &ksl_params, &r_ksl_params);
	params_be_to_cpu(&ksl_params);
	params_be_to_cpu(&r_ksl_params);
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto exit;


	// Update peer IB connection details and buffers
	ksl_params.r_lid = r_ksl_params.lid;
	ksl_params.r_qpn = r_ksl_params.qpn;
	ksl_params.r_rkey = r_ksl_params.rkey;
	ksl_params.r_vaddr = r_ksl_params.vaddr;

	// Bring up IB connecttion
	fprintf(MSGFD, "Connecting IB...");
	ksl_params.cmd = CONNECT;
	rc = write(fd, &ksl_params, sizeof(params_t));
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto clean_exit;

	// Synch before test
	fprintf(MSGFD, "Synchronizing before test...");
	rc = VL_sock_sync_ready (&sock);
	PRINT_DONE_FAILED(rc >= 0);
	if (rc < 0) goto clean_exit;

	//
	// The actual test
	//

	if (!ksl_params.deamon)
		sleep(1);

	ksl_params.cmd = TEST_SEND_LAT;

	ksl_params.timing_ptr = (uintptr_t) malloc(sizeof(cycles_t) * ksl_params.iter);
	if (!ksl_params.timing_ptr) {
		fprintf(MSGFD, "Failed allocating timing array\n");
		rc = -ENOMEM;
		goto clean_exit;
	}

	printf("------------------------------------------------------------------\n");
	printf(" #bytes #iterations    t_min[usec]    t_max[usec]  t_typical[usec]\n");


	if (ksl_params.all) {
		for (i = 0; i <= 12; i++) {
			size = 1 << i;
			if (TRANSPORT_UD == ksl_params.transport && size > ksl_params.mtu)
				break;

			ksl_params.payload_size = size;
			rc = write(fd, &ksl_params, sizeof(params_t));
			if (rc < 0)
				goto clean_exit;
			print_report(&report_options, ksl_params.iter, (cycles_t*)(uintptr_t)ksl_params.timing_ptr, ksl_params.payload_size);
		}
	}
	else
	{
		rc = write(fd, &ksl_params, sizeof(params_t));
		if (rc < 0)
			goto clean_exit;
		else
			rc = 0; // A successful write returned the 'number of bytes written' rather than our 0 success code
		print_report(&report_options, ksl_params.iter, (cycles_t*)(uintptr_t)ksl_params.timing_ptr, ksl_params.payload_size);
	}

	printf("------------------------------------------------------------------\n");

	// Synch after test
	VL_sock_sync_ready (&sock);


	// Fall through...

clean_exit:
	if (ksl_params.timing_ptr) {
		free((void*)(uintptr_t)ksl_params.timing_ptr);
	}

	// Cleanup kernel resources
	fprintf(MSGFD, "Cleaning up kernel resources...");
	ksl_params.cmd = CLEANUP;
	rc_cleanup = write(fd, &ksl_params, sizeof(params_t));
	PRINT_DONE_FAILED(rc_cleanup >= 0);

exit:
	if (fd != -1)
		kvl_dev_close(fd);
	VL_sock_close(&sock);
	if (rc < 0) {
		printf("TEST FAILED\n");
		return 1;
	} 
	printf("TEST PASSED\n");
	return 0;
}


//
// Helper functions
// 

static int process_arg(
		      IN const int opt_index,
		      IN const char *equ_ptr,
		      IN const int arr_size,
		      IN const struct VL_usage_descriptor_t *usage_desc_arr)
{
	switch (usage_descriptor[opt_index].case_code) {
	case OPT_HELP:
		VL_usage (1, arr_size, usage_desc_arr);
		exit (0);

	case OPT_DEV:
		strcpy (ksl_params.hca_id, equ_ptr);
		break;

	case OPT_ITR:
		ksl_params.iter = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_ALL:
		ksl_params.all = 1;
		break;

	case OPT_IP:
		strncpy(ksl_params.deamon_ip, equ_ptr, IP_STR_LENGTH);
		ksl_params.deamon = 0;
		break;

	case OPT_PORT:
		ksl_params.deamon_tcp_port = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_SIZE:
		ksl_params.payload_size = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_CON:
		if (!strcmp(equ_ptr, "UD")) {
			ksl_params.transport = TRANSPORT_UD;
		}
		break;

	case OPT_MTU:
		ksl_params.mtu = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_IB_PORT:
		ksl_params.ib_port = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_TX:
		ksl_params.tx_depth = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_EVENT:
		ksl_params.type = strtoul (equ_ptr, NULL, 0);
		break;

	case OPT_CYCLES:
		report_options.cycles = 1;
		break;

	case OPT_HIST:
		report_options.histogram = 1;
		break;

	case OPT_UNSORTED:
		report_options.unsorted = 1;
		break;
	case OPT_INLINE:
	        ksl_params.send_inline = 1;
	        break;

	default:
		printf ("unknown option: %s\n", equ_ptr);
		exit (2);
	}
	return 0;
}


static void print_params(void)
{
	char* event_types[] = { "polling", "blocking receive", "interrupt" };

	VL_MISC_TRACE((" %s configuration report", MOD_NAME));
	VL_MISC_TRACE((" ------------------------------------------------"));
	VL_MISC_TRACE((" Current pid                  : " VL_PID_FMT, VL_getpid()));
	VL_MISC_TRACE((" Device name                  : \"%s\"", ksl_params.hca_id));
	VL_MISC_TRACE((" IB port                      : %u", ksl_params.ib_port));
	VL_MISC_TRACE((" Is daemon                    : %s", (ksl_params.deamon) ? "Yes" : "No"));
	VL_MISC_TRACE((" IP                           : %s", ksl_params.deamon_ip));
	VL_MISC_TRACE((" TCP port                     : %u", ksl_params.deamon_tcp_port));
	VL_MISC_TRACE((" Iterations                   : %d", ksl_params.iter));
	VL_MISC_TRACE((" Number of outstanding WR     : %u", ksl_params.tx_depth));
	VL_MISC_TRACE((" QP transport type            : %s", ksl_params.transport ? "UD" : "RC"));
	VL_MISC_TRACE((" Send inline flag             : %s", ksl_params.send_inline ? "ON" : "OFF"));
	VL_MISC_TRACE((" Event type (receive context) : %s", event_types[ksl_params.type]));
	VL_MISC_TRACE((" ------------------------------------------------\n"));
}


static int connect_tcp(struct VL_sock_t *sock_p)
{
	struct VL_sock_props_t sock_props;
	int rc;

	sock_props.is_daemon = ksl_params.deamon;
	strncpy (sock_props.ip, ksl_params.deamon_ip, IP_STR_LENGTH);
	sock_props.port = ksl_params.deamon_tcp_port;
	rc = VL_sock_connect (&sock_props, sock_p);
	return rc;
}

#define KSL_CPU_TO_BE32(x) (x) = htonl(x)
#define KSL_CPU_TO_BE64(x) (x) = VL_htonll(x)
#define KSL_BE32_TO_CPU(x) (x) = ntohl(x)
#define KSL_BE64_TO_CPU(x) (x) = VL_ntohll(x)

static void params_cpu_to_be(params_t* ksl_params_p)
{
	KSL_CPU_TO_BE32(ksl_params_p->transport);
	KSL_CPU_TO_BE32(ksl_params_p->payload_size);
	KSL_CPU_TO_BE32(ksl_params_p->iter);
	KSL_CPU_TO_BE32(ksl_params_p->tx_depth);
	KSL_CPU_TO_BE32(ksl_params_p->deamon);
	KSL_CPU_TO_BE32(ksl_params_p->mtu);
	KSL_CPU_TO_BE32(ksl_params_p->all);
	KSL_CPU_TO_BE32(ksl_params_p->lid);
	KSL_CPU_TO_BE32(ksl_params_p->qpn);
	KSL_CPU_TO_BE32(ksl_params_p->deamon_tcp_port);
	KSL_CPU_TO_BE32(ksl_params_p->rkey);
	KSL_CPU_TO_BE64(ksl_params_p->vaddr);
	KSL_CPU_TO_BE32(ksl_params_p->cmd);
	KSL_CPU_TO_BE32(ksl_params_p->type);
	KSL_CPU_TO_BE32(ksl_params_p->send_inline);
	KSL_CPU_TO_BE32(ksl_params_p->r_lid);
	KSL_CPU_TO_BE32(ksl_params_p->r_qpn);
	KSL_CPU_TO_BE32(ksl_params_p->r_rkey);
	KSL_CPU_TO_BE32(ksl_params_p->r_vaddr);
}


static void params_be_to_cpu(params_t* ksl_params_p)
{
	KSL_BE32_TO_CPU(ksl_params_p->transport);
	KSL_BE32_TO_CPU(ksl_params_p->payload_size);
	KSL_BE32_TO_CPU(ksl_params_p->iter);
	KSL_BE32_TO_CPU(ksl_params_p->tx_depth);
	KSL_BE32_TO_CPU(ksl_params_p->deamon);
	KSL_BE32_TO_CPU(ksl_params_p->mtu);
	KSL_BE32_TO_CPU(ksl_params_p->all);
	KSL_BE32_TO_CPU(ksl_params_p->lid);
	KSL_BE32_TO_CPU(ksl_params_p->qpn);
	KSL_BE32_TO_CPU(ksl_params_p->deamon_tcp_port);
	KSL_BE32_TO_CPU(ksl_params_p->rkey);
	KSL_BE64_TO_CPU(ksl_params_p->vaddr);
	KSL_BE32_TO_CPU(ksl_params_p->cmd);
	KSL_BE32_TO_CPU(ksl_params_p->type);
	KSL_BE32_TO_CPU(ksl_params_p->send_inline);
	KSL_BE32_TO_CPU(ksl_params_p->r_lid);
	KSL_BE32_TO_CPU(ksl_params_p->r_qpn);
	KSL_BE32_TO_CPU(ksl_params_p->r_rkey);
	KSL_BE32_TO_CPU(ksl_params_p->r_vaddr);
}


/////////////////////////////////////////////////////////////////////////////////
//
// Reporting functions taken from user-level tests
//


/*****************************************************************************
* Function: get_median
*****************************************************************************/
static cycles_t get_median(int n, cycles_t delta[])
{
	if (n % 2)
		return (delta[n / 2] + delta[n / 2 - 1]) / 2;
	else
		return delta[n / 2];
}

/*****************************************************************************
* Function: cycles_compare
*****************************************************************************/
static int cycles_compare(const void * aptr, const void * bptr)
{
	const cycles_t *a = aptr;
	const cycles_t *b = bptr;
	if (*a < *b) return -1;
	if (*a > *b) return 1;
	return 0;
}

/*****************************************************************************
* Function: get_cpu_mhz
*****************************************************************************/
double get_cpu_mhz(void)
{
	FILE* f;
	char buf[256];
	double mhz = 0.0;

	f = fopen("/proc/cpuinfo","r");
	if (!f)
		return 0.0;
	while(fgets(buf, sizeof(buf), f)) {
		double m;
		int rc;
		rc = sscanf(buf, "cpu MHz : %lf", &m);
		if (rc != 1) {	/* PPC has a different format */
			rc = sscanf(buf, "clock : %lf", &m);
			if (rc != 1)
				continue;
		}
		if (mhz == 0.0) {
			mhz = m;
			continue;
		}
		if (mhz != m) {
			fprintf(stderr,"Conflicting CPU frequency values"
					" detected: %lf != %lf\n",
					mhz, m);
			fclose(f);
			return 0.0;
		}
	}
	fclose(f);
	return mhz;
}

/*****************************************************************************
* Function: print_report
*****************************************************************************/
static void print_report(report_options_t * options, unsigned int iters, cycles_t *tstamp,int size)
{
	double cycles_to_units;
	cycles_t median;
	unsigned int i;
	const char* units;
	cycles_t *delta = malloc(iters * sizeof *delta);

	if (!delta) {
		perror("malloc");
		return;
	}

	for (i = 0; i < iters - 1; ++i)
		delta[i] = tstamp[i + 1] - tstamp[i];


	if (options->cycles) {
		cycles_to_units = 1;
		units = "cycles";
	} else {
		cycles_to_units = get_cpu_mhz();
		units = "usec";
	}

	if (options->unsorted) {
		printf("#, %s\n", units);
		for (i = 0; i < iters - 1; ++i)
			printf("%d, %g\n", i + 1, delta[i] / cycles_to_units / 2);
	}

	qsort(delta, iters - 1, sizeof *delta, cycles_compare);

	if (options->histogram) {
		printf("#, %s\n", units);
		for (i = 0; i < iters - 1; ++i)
			printf("%d, %g\n", i + 1, delta[i] / cycles_to_units / 2);
	}

	median = get_median(iters - 1, delta);
	printf("%7d        %d        %7.2f        %7.2f          %7.2f\n",
	       size, iters,delta[0] / cycles_to_units / 2,
	       delta[iters - 2] / cycles_to_units / 2, median / cycles_to_units / 2);
	free(delta);
}




