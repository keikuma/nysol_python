# -*- coding: utf-8 -*-
import nysol._nysolshell_core as n_core
from nysol.mod.nysollib.nysolmodcore import NysolMOD_CORE
from nysol.mod.nysollib import nysolutil as nutil

class Nysol_Muniq(NysolMOD_CORE):
	kwd = n_core.getparalist("muniq")
	def __init__(self,*args, **kw_args) :
		super(Nysol_Muniq,self).__init__("muniq",nutil.args2dict(args,kw_args,Nysol_Muniq.kwd))
