# -*- coding: utf-8 -*-
import nysol._nysolshell_core as n_core
from nysol.mod.nysollib.nysolmodcore import NysolMOD_CORE
from nysol.mod.nysollib import nysolutil as nutil

class Nysol_Mduprec(NysolMOD_CORE):
	kwd = n_core.getparalist("mduprec")
	def __init__(self,*args, **kw_args) :
		super(Nysol_Mduprec,self).__init__("mduprec",nutil.args2dict(args,kw_args,Nysol_Mduprec.kwd))
