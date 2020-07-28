# -*- coding: utf-8 -*-
from ctypes import *

LEAP_ITTI_MSG = 0
LEAP_INITIAL_NAS_DATA = 1
LEAP_EMM_SAP_MSG = 2
LEAP_SEC_MODE_COMMAND_NAS_DATA = 3
LEAP_SECURITY_MODE_COMMAND = 4
LEAP_EMM_SECURITY_CTX  = 5
LEAP_EMM_PROC_COMMON_GET_ARGS = 6
LEAP_WAIT_COMMAND = 7

class leap_message_t(Structure):
	def __str__(self):
		s=[]

		for k in self._fields_:
			if type(k[1]) == type(Structure) or type(k[1]) == type(Union):
				s.append("\"%s\":%s"%(k[0],getattr(self,k[0])))
			else:
				s.append("\"%s\":\"%s\""%(k[0],getattr(self,k[0])))
	
		return '{%s}'%(','.join([i for i in s]))

	_fields_ = [
				('message_id',c_ubyte),
				('ue_id',c_ubyte),
				('length',c_ubyte),
				('message',c_char),
				]