//command id
#define EXIT_LEAP_LOOP -2
#define HOLD_FALSE 0
#define HOLD_TRUE 1
#define GET_EMM_SECURITY_CONTEXT 2
#define GET_EMM_PROC_COMMON_GET_ARGS 3
#define SET_AND_SEND_EMM_ATTACH_REJECT 4
#define SET_AND_SEND_AUTHENTICATION_REJECT 5
#define SEND_NETWORK_INITIATED_DETACH_REQUEST 6
#define NAS_ITTI_DL_DATA 7
#define NAS_ITTI_PLAIN_MSG 8
#define NAS_PROC_ESTABLISH_IND 9
#define NAS_INITIAL_ATTACH_PROC 10

#define buf_size 4096

//message id
#define LEAP_ITTI_MSG 0
#define LEAP_INITIAL_NAS_DATA 1
#define LEAP_EMM_SECURITY_CTX  2
#define LEAP_EMM_PROC_COMMON_GET_ARGS 3
#define LEAP_EMM_SAP_MSG 4
#define LEAP_SEC_MODE_COMMAND_NAS_DATA 5
#define LEAP_SECURITY_MODE_COMMAND 6
#define LEAP_WAIT_COMMAND 7

int leap_send_only(int assocfd, uint8_t message_id, uint8_t ue_id, char *sendBuf, int size);
int leap_send(int assocfd, uint8_t message_id, uint8_t ue_id, char *sendBuf, int size);
int leap_recv_only(int assocfd, char *recvBuf, int size, int flag);
int leap_recv(int assocfd, char *recvBuf, int size, int flag);
int get_emm_security_context(int ue_id);
int get_emm_proc_common_get_args(int ue_id);
int leap_wait_command();
void * _leap_wait_command();
int tcp_init();
int *tcpproc();


typedef struct 
{
	int8_t command_id;
	uint8_t ue_id;
	uint8_t cause;
	uint8_t length;
	char message[0];
} leap_command_t;

typedef struct
{
	uint8_t message_id;
	uint8_t ue_id;
	uint8_t length;
	char message[0];
} leap_message_t;