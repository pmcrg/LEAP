# -*- coding: utf-8 -*-
import socket
import sctp
import time
import pprint
from ctypes import *
from .variable_def import *
from .message_def import *

from .leapcommand_def import *
from .leapmessage_def import *
buf_size = 4096

CDLL("/absolute/path/to/leap/build/libLFDS.so", mode=RTLD_GLOBAL)
CDLL("/absolute/path/to/leap/build/libBSTR.so", mode=RTLD_GLOBAL)
CDLL("/lib/x86_64-linux-gnu/libpthread.so.0", mode=RTLD_GLOBAL)  #pthread
CDLL("/lib/x86_64-linux-gnu/librt.so.1", mode=RTLD_GLOBAL)   #timer
CDLL("/lib/x86_64-linux-gnu/libc.so.6", mode=RTLD_GLOBAL)
CDLL("/lib64/ld-linux-x86-64.so.2", mode=RTLD_GLOBAL)
CDLL("/usr/lib/x86_64-linux-gnu/libstdc++.so.6", mode=RTLD_GLOBAL)
CDLL("/absolute/path/to/leap/build/libfreewrapper.so", mode=RTLD_GLOBAL)   #free_wrapper
CDLL("/absolute/path/to/leap/build/libHASHTABLE.so", mode=RTLD_GLOBAL)
CDLL("/absolute/path/to/leap/build/libITTI.so", mode=RTLD_GLOBAL)
CDLL("/absolute/path/to/leap/build/libCN_UTILS.so", mode=RTLD_GLOBAL)  #util
CDLL("/usr/lib/x86_64-linux-gnu/libconfig.so.9", mode=RTLD_GLOBAL)   #config_init
CDLL("/usr/lib/x86_64-linux-gnu/libnettle.so.6", mode=RTLD_GLOBAL)
CDLL("/lib/x86_64-linux-gnu/libcrypto.so.1.0.0", mode=RTLD_GLOBAL)
lib_bstr = CDLL("/absolute/path/to/leap/build/libBSTR.so")
lib_nas = CDLL("/absolute/path/to/leap/build/libnas_api.so")

class leap(object):
	"""docstring for leap"""
	def __init__(self, ip, port):
		self.s = sctp.sctpsocket_tcp(socket.AF_INET)
		self.s.connect((ip, eval(port)))

	def send(self, buf):
		return self.s.sctp_send(buf)

	def recv(self, size):
		fromaddr, flags, buf, notif = self.s.sctp_recv(size)
		msg_proc_p = point_str(buf, leap_message_t)
		message_id = msg_proc_p.contents.message_id
		ue_id = msg_proc_p.contents.ue_id
		pbuf = create_string_buffer(buf, len(buf))
		length = msg_proc_p.contents.length
		msg = string_at(addressof(pbuf)+3*sizeof(c_ubyte), len(buf)-3*sizeof(c_ubyte))
		return message_id, ue_id, length, msg

	def hold(self, decision):
		if decision == True:
			command = leap_command_t()
			command.command_id = 1
			command_msg = ""
			self.send_command(command, command_msg)
		else:
			command = leap_command_t()
			command.command_id = 0
			command_msg = ""
			self.send_command(command, command_msg)
		return

	def send_command(self, command, command_msg):
		command.length = len(command_msg)
		buf = string_at(addressof(command), sizeof(command)) + command_msg
		return self.s.sctp_send(buf)

	def nas_message_decode(self, undecoded_nas_msg):
		lib_nas.nas_message_decode.argtypes = (POINTER(c_ubyte), POINTER(nas_message_t), c_size_t, c_void_p, POINTER(nas_message_decode_status_t))
		lib_nas.nas_message_decode.restype = c_int
		decoded_nas_msg = nas_message_t()
		decode_status = nas_message_decode_status_t()
		p_undecoded_nas_msg = point_str(undecoded_nas_msg, c_ubyte)
		start = time.time()
		command = leap_command_t()
		command.command_id = GET_EMM_SECURITY_CONTEXT
		command_msg = ""
		self.send_command(command, command_msg)

		messsage_id, ue_id, length, emm_ctx = self.recv(buf_size)
		end = time.time()
		print("retrieving emm_ctx: " + str(end-start))

		if emm_ctx == "-1\x00":
			emm_ctx = c_void_p()

		start = time.time()
		bytes = lib_nas.nas_message_decode(p_undecoded_nas_msg, byref(decoded_nas_msg), len(undecoded_nas_msg), emm_ctx, byref(decode_status))
		end = time.time()
		print("lib_nas_msg_decode time : " + str(end-start))
		p_emm_msg = pointer_convert(decoded_nas_msg, EMM_msg)
		return p_emm_msg

	def emm_sap_send_attach_reject(self, ue_id, emm_cause):

		command = leap_command_t()
		command.command_id = SET_AND_SEND_EMM_ATTACH_REJECT
		command.ue_id = ue_id
		command.cause = emm_cause
		command_msg = ""
		self.send_command(command, command_msg)
		return 1
	
	def send_authentication_reject(self, ue_id):
		command = leap_command_t()
		command.command_id = SET_AND_SEND_AUTHENTICATION_REJECT
		command.ue_id = ue_id
		command_msg = ""
		self.send_command(command, command_msg)

	def send_detach_request(self, ue_id):
		command = leap_command_t()
		command.command_id = SEND_NETWORK_INITIATED_DETACH_REQUEST
		command.ue_id = ue_id
		command_msg = ""
		self.send_command(command, command_msg)

	def exit_loop(self):
		command = leap_command_t()
		command.command_id = -2
		command_msg = ""
		self.send_command(command, command_msg)

	def send_itti_dl_data(self, ue_id, nas_msg):

		command = leap_command_t()
		command.command_id = NAS_ITTI_DL_DATA
		command.ue_id = ue_id
		command_msg = nas_msg
		self.send_command(command, command_msg)

def str_to_hex(s):
    return r"/x"+r'/x'.join([hex(ord(c)).replace('0x', '') for c in s])

def pointer_convert(original_variable, new_type):
	original_pointer = pointer(original_variable)
	return cast(original_pointer, POINTER(new_type))

def point_with(variable, pointer_type):
	p = POINTER(pointer_type)
	return p(variable)

def point_str(str_buf, pointer_type):
	str_buf = create_string_buffer(str_buf, len(str_buf))
	return point_with(str_buf, pointer_type)

def ppstruct(stucture_to_print):
	pprint.pprint(eval(str(stucture_to_print.contents)))