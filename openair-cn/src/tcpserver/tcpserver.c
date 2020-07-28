#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#include <pthread.h>
#include <stdint.h>
#include "log.h"
#include "msc.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "nas_defs.h"
#include "nas_network.h"
#include "nas_proc.h"
#include "emm_main.h"
#include "nas_timer.h"
#include "bstrlib.h"
#include "nas_proc.h"


#include "tcpserver.h"
#include "nas_itti_messaging.h"
#include "as_message.h"
#include "Attach.c"
#include "Authentication.c"
#include "emm_proc.h"  //detach request

//tcp global variables
extern int socketfd,assocfd;
extern struct sockaddr_in s_addr,r_addr;
extern socklen_t len; 

extern char recvBuf[buf_size];
extern char sendBuf[buf_size];

#define PORT 7897
#define LEAP_MSG_TOTAL_LENGTH 500

//sctp
#include <netinet/in.h>
#include <netinet/sctp.h>

int leap_send_only(int assocfd, uint8_t message_id, uint8_t ue_id, char *sendBuf, int size){
  leap_message_t * send_message = (leap_message_t *)malloc(size+3);
  send_message->message_id = message_id;
  send_message->ue_id = ue_id;
  send_message->length = size;
  memcpy(send_message->message, sendBuf, size);
  sctp_sendmsg(assocfd, send_message, size+3, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  return 1;
}

int leap_send(int assocfd, uint8_t message_id, uint8_t ue_id, char *sendBuf, int size){
  leap_message_t * send_message = (leap_message_t *)malloc(size+3);
  send_message->message_id = message_id;
  send_message->ue_id = ue_id;
  send_message->length = size;
  memcpy(send_message->message, sendBuf, size);
  sctp_sendmsg(assocfd, send_message, size+3, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	return leap_loop();
}

int leap_recv_only(int assocfd, char *recvBuf, int size, int flag){
  int recv_size = sctp_recvmsg(assocfd, recvBuf, size, NULL, NULL, NULL, NULL);
  return 0;
}

int leap_recv(int assocfd, char *recvBuf, int size, int flag){
  //rc = -2 force exit loop
  //rc = -1 normal exit; normally process msg and return from leap_recv, should still in while
  //rc = 0 hold false
  //rc = 1 hold current process
  
  int recv_size = sctp_recvmsg(assocfd, recvBuf, size, NULL, NULL, NULL, NULL);

  leap_command_t * msg_proc_t;
  
  msg_proc_t = recvBuf;
  int8_t command_id = msg_proc_t->command_id;
	switch(command_id){
		case GET_EMM_SECURITY_CONTEXT:
			{
        get_emm_security_context(1);
        break;
      }

		case GET_EMM_PROC_COMMON_GET_ARGS:
      {
        get_emm_proc_common_get_args(1);
  			break;
      }

    case SET_AND_SEND_EMM_ATTACH_REJECT:
      {
        clock_t start, end;
        start = clock();
        uint32_t ue_id;
        int emm_cause;
        ue_id = msg_proc_t->ue_id;
        emm_cause = msg_proc_t->cause;
        printf("ue_id: %d\n", ue_id);
        printf("emm_cause: %d\n", emm_cause);
        emm_data_context_t                      ue_ctx;
        memset (&ue_ctx, 0, sizeof (emm_data_context_t));
        ue_ctx.is_dynamic = false;
        ue_ctx.ue_id = ue_id;
        ue_ctx.emm_cause = emm_cause;
        _emm_attach_reject(&ue_ctx);
        end = clock();
        
        printf("total time: %lf\n", (double)(end-start)/CLOCKS_PER_SEC);
        break;
      }
    case EXIT_LEAP_LOOP:
      {
        printf("exiting\n");
        return -2;  
        break;
      }
		case HOLD_TRUE:
			{
        printf("hold signal detected\n");
        return 1;
        break;
      }
    case HOLD_FALSE:
      {
        printf("hold false signal detected\n");
        return 0;
        break;
      }
    case NAS_ITTI_DL_DATA:
      {
        printf("sending nas itti downlink data...\n");
        as_message_t                            as_msg = {0};
        as_msg.msg.dl_info_transfer_req.ue_id = msg_proc_t->ue_id;
        char * nas_msg = msg_proc_t->message;
        as_msg.msg.dl_info_transfer_req.nas_msg = blk2bstr(nas_msg, msg_proc_t->length);
        printf("ready\n");
        nas_itti_dl_data_req (as_msg.msg.dl_info_transfer_req.ue_id, as_msg.msg.dl_info_transfer_req.nas_msg,as_msg.msg.dl_info_transfer_req.err_code);
        printf("send\n");
        return -2;
        break;
      }
    case NAS_INITIAL_ATTACH_PROC:
      {
        break;
      }
    case SET_AND_SEND_AUTHENTICATION_REJECT:
      {
        int                                     rc = RETURNerror;
        emm_sap_t                               emm_sap = {0};
        emm_sap.primitive = EMMAS_SECURITY_REJ;
        emm_sap.u.emm_as.u.security.guti = NULL;
        emm_sap.u.emm_as.u.security.ue_id = msg_proc_t->ue_id;
        emm_sap.u.emm_as.u.security.msg_type = EMM_AS_MSG_TYPE_AUTH;
        emm_as_set_security_data (&emm_sap.u.emm_as.u.security.sctx, NULL, false, true);
        rc = emm_sap_send (&emm_sap);
      }
    case SEND_NETWORK_INITIATED_DETACH_REQUEST:
      {
        emm_proc_detach_request (msg_proc_t->ue_id, EMM_DETACH_TYPE_EPS, 1 /*switch_off */ , 0 /*native_ksi */ , 0 /*ksi */ ,
                           NULL /*guti */ , NULL /*imsi */ , NULL /*imei */ );
      }
	}

	return -1;
}

int leap_loop(){
  //rc = -2 force exit loop
  //rc = -1 normal exit
  //rc = 0 hold false
  //rc = 1 hold current process
  int rc = -1;
  while (rc == -1){
    rc = leap_recv(assocfd, recvBuf, buf_size, 0);
  }
  return rc;
}

int get_emm_proc_common_get_args(int ue_id){
	typedef struct attach_data_s {
	  unsigned int                            ue_id; /* UE identifier        */
	#define ATTACH_COUNTER_MAX  5
	  unsigned int                            retransmission_count; /* Retransmission counter   */
	  bstring                                 esm_msg;      /* ESM message to be sent within
	                                                         * the Attach Accept message    */
	} attach_data_t;
	attach_data_t                          *data_p = NULL;
	data_p = (attach_data_t *) emm_proc_common_get_args (ue_id);
  leap_send(assocfd, LEAP_EMM_PROC_COMMON_GET_ARGS, ue_id, data_p->esm_msg, strlen(data_p->esm_msg));

}

int get_emm_security_context(int ue_id){
    emm_data_context_t                     *emm_ctx = emm_data_context_get (&_emm_data, ue_id);
    emm_security_context_t                 *emm_security_context = NULL;

    if (emm_ctx) {
      OAILOG_INFO (LOG_NAS_EMM, "EMMAS-SAP - got context %p\n", emm_ctx);
      if (IS_EMM_CTXT_PRESENT_SECURITY(emm_ctx)) {
        emm_security_context = &emm_ctx->_security;
      }
    }

    if (emm_security_context){
    	printf("emm_security_context exists, sending...\n");
        leap_send_only(assocfd, LEAP_EMM_SECURITY_CTX, ue_id, emm_security_context, sizeof(emm_security_context_t));
    }
    else
    {
    	printf("emm_security_context doesn't exists, sending NULL\n");
      leap_send_only(assocfd, LEAP_EMM_SECURITY_CTX, ue_id, "-1", sizeof("-1"));

    }
}

int leap_wait_command(){
  pthread_t wait_command_tid;
  int ret = pthread_create(&wait_command_tid, NULL, _leap_wait_command, NULL);
  return ret;
}

void * _leap_wait_command(){

  leap_send(assocfd, 0, LEAP_WAIT_COMMAND, "", 0);
  return NULL;
}

int tcp_init(){
	pthread_t tcp_tid;
  pthread_create(&tcp_tid,NULL,tcpproc,NULL);
}

int *tcpproc()
{
  if(-1 == (socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP))){
          printf("fail to create SCTP socket!\n");       
  };
  printf("SCTP socket create success!\n"); 

  memset(&s_addr,0x00,sizeof(s_addr));
  s_addr.sin_family = PF_INET;
  s_addr.sin_port = htons(PORT);
  s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if(-1 == bind(socketfd,(struct sockaddr*)&s_addr,sizeof(s_addr))){
      printf("bind failed!\n");            
  }
  printf("bind success!\n");
  
  struct sctp_initmsg initmsg;
  memset( &initmsg, 0, sizeof(initmsg) );  
  initmsg.sinit_num_ostreams = 5;  
  initmsg.sinit_max_instreams = 5;  
  initmsg.sinit_max_attempts = 4;  
  setsockopt( socketfd, IPPROTO_SCTP, SCTP_INITMSG,   
                     &initmsg, sizeof(initmsg) );  
  listen( socketfd, 5 );  
  printf("listen success!\n");
  
  len = sizeof(struct sockaddr);
  assocfd = accept(socketfd,(struct sockaddr *)&r_addr,&len);
  if(-1 == assocfd){
    printf("accept failed!\n");
  }
  printf("accept success!\n");
  
  printf("waiting\n");

  int a;
  scanf("%d",&a);

  close(assocfd);
  close(socketfd);

  return 0;
}