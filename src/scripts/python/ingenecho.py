#!/usr/bin/python
#
# Python bindings for Om
# Copyright (C) 2005 Leonard Ritter
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import ingen
import os,time,sys

def main(om):
	om.setEnvironment(ingen.Environment())
	om.engine.activate()
 	om.engine.register_client(om.getAddressAsString())
 	om.request.all_objects(om.getAddressAsString())
	
	om.request.all_objects()
	time.sleep(3)	
	om.getEnvironment().printPatch()
	om.getEnvironment().printConnections()
	print "ingenecho will now monitor and mirror changes in the structure"
	print "hit return to exit when youre done"
	sys.stdin.readline()
 	om.engine.unregister_client(om.getAddressAsString())
	os._exit(0)

if __name__ == "__main__":
	ingen.startClient(main)
