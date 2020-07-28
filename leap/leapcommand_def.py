# -*- coding: utf-8 -*-
from ctypes import *

GET_EMM_SECURITY_CONTEXT = 2
GET_EMM_PROC_COMMON_GET_ARGS = 3
SET_AND_SEND_EMM_ATTACH_REJECT = 4
SET_AND_SEND_AUTHENTICATION_REJECT = 5
SEND_NETWORK_INITIATED_DETACH_REQUEST = 6
NAS_ITTI_DL_DATA = 7
NAS_ITTI_PLAIN_MSG = 8
NAS_PROC_ESTABLISH_IND = 9
NAS_INITIAL_ATTACH_PROC = 10

class leap_command_t(Structure):
	def __str__(self):
		s=[]

		for k in self._fields_:
			if type(k[1]) == type(Structure) or type(k[1]) == type(Union):
				s.append("\"%s\":%s"%(k[0],getattr(self,k[0])))
			else:
				s.append("\"%s\":\"%s\""%(k[0],getattr(self,k[0])))
	
		return '{%s}'%(','.join([i for i in s]))

	_fields_ = [
				('command_id',c_ubyte),
				('ue_id',c_ubyte),
				('cause',c_ubyte),
				('length',c_ubyte),
				]




