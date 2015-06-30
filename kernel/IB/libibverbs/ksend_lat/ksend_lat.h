#ifndef _KSEND_LAT_H_
#define _KSEND_LAT_H_


//
// Definitions and types
// 

#define MOD_NAME		"ksend_lat"
#define IP_STR_LENGTH		15
#define MAX_HCA_ID_LEN		256


typedef enum _cmd_t
{
	INIT,
	CONNECT,
	TEST_SEND_LAT,
	CLEANUP
} cmd_t;


typedef enum _transport_t {
	TRANSPORT_RC = 0,
	TRANSPORT_UD
} transport_t;

typedef enum _test_type_t
{
	SEND_LAT_POLL,
	SEND_LAT_INT_NOTIF,
	SEND_LAT_INT_RAW
} test_type_t;


struct _params_t {
	// Test parameters
	uint32_t transport;
	uint32_t payload_size;
	uint32_t iter;
	uint32_t tx_depth;
	uint32_t deamon;
	uint32_t mtu;
	uint32_t all;

	// HCA addressing
	char hca_id[MAX_HCA_ID_LEN];
	uint8_t ib_port;
	uint32_t lid;
	uint32_t qpn;

	// Deamon IP addressing
	char deamon_ip[IP_STR_LENGTH];
	uint32_t deamon_tcp_port;

	// Local base memory address and key (for remote memory access)
	uint32_t rkey;
	uint64_t vaddr;

	// Command related fields
	uint32_t cmd;
	uint32_t type;
	uint64_t timing_ptr;
	uint32_t send_inline;

	// Remote side parameters
	uint32_t r_lid;
	uint32_t r_qpn;
	uint64_t r_vaddr;
	uint32_t r_rkey;
    struct _params_t* user_params;
} __attribute__ ((packed));

typedef struct _params_t params_t;


#endif // _KSEND_LAT_H_

