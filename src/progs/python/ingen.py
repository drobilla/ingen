#!/usr/bin/python
#
# Python bindings for Ingen
# Copyright (C) 2005 Leonard Ritter
# Copyright (C) 2006 Dave Robillard

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

import os,sys,thread,time

from OSC import OSCMessage, decodeOSC

from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor

INGEN_CALL_TIMEOUT = 5
INGEN_CALL_POLLTIME = 0.1

class TreeElement:
	def __init__(self,environment,parent,name):
		self.environment = environment
		self.parent = parent
		self.name = name
		self.metadata = {}
		
	def __del__(self):
		print "'%s': gone" % self.name
		
	def removeChild(self, child):
		pass

	def remove(self):
		self.parent.removeChild(self.name)
		self.parent = None
		del self
		
	def getParent(self):
		return self.parent
		
	def getName(self):
		return self.name

	def getPath(self):
		if self.parent:
			return self.parent.getPath() + "/" + self.name
		else:
			return self.name
			
	def getDepth(self):
		if self.parent:
			return self.parent.getDepth() + 1
		else:
			return 0
			
	def setMetaData(self,key,value):
		if (not self.metadata.has_key(value)) or (self.metadata[key] != value):
			print "||| '%s': '%s' = '%s'" % (self.getPath(), key, value)
			self.metadata[key] = value

class Port(TreeElement):
	def __init__(self,environment,parent,name):
		TreeElement.__init__(self,environment,parent,name)
		self.porttype = ""
		self.direction = ""
		self.hint = ""
		self.defaultvalue = 0.0
		self.minvalue = 0.0
		self.maxvalue = 0.0
		self.value = 0.0
		self.connections = {}
		print "*** '%s': new port" % self.getPath()
		
	def remove(self):
		for connection in self.getConnections():
			connection.remove()
		TreeElement.remove(self)
		
	def getConnections(self):
		return self.connections
		
	def addConnection(self,target,connection):
		self.connections[target] = connection
		
	def removeConnection(self,target):
		del self.connections[target]

	def setPortType(self,porttype):
		if self.porttype != porttype:
			print "*** '%s': changing porttype from '%s' to '%s'" % (self.getPath(), self.porttype, porttype)
			self.porttype = porttype
		
	def setDirection(self,direction):
		if self.direction != direction:
			print "*** '%s': changing direction from '%s' to '%s'" % (self.getPath(), self.direction, direction)
			self.direction = direction
		
	def setHint(self,hint):
		if self.hint != hint:
			print "*** '%s': changing hint from '%s' to '%s'" % (self.getPath(), self.hint, hint)
			self.hint = hint
		
	def setDefaultValue(self,defaultvalue):
		if self.defaultvalue != defaultvalue:
			print "*** '%s': changing defaultvalue from '%.3f' to '%.3f'" % (self.getPath(), self.defaultvalue, defaultvalue)
			self.defaultvalue = defaultvalue
		
	def setMinValue(self,minvalue):
		if self.minvalue != minvalue:
			print "*** '%s': changing minvalue from '%.3f' to '%.3f'" % (self.getPath(), self.minvalue, minvalue)
			self.minvalue = minvalue
		
	def setMaxValue(self,maxvalue):
		if self.maxvalue != maxvalue:
			print "*** '%s': changing maxvalue from '%.3f' to '%.3f'" % (self.getPath(), self.maxvalue, maxvalue)
			self.maxvalue = maxvalue
			
	def setValue(self,value):
		if self.value != value:
			print "*** '%s': changing value from '%.3f' to '%.3f'" % (self.getPath(), self.value, value)
			self.value = value

class Node(TreeElement):
	def __init__(self,environment,parent,name):
		TreeElement.__init__(self,environment,parent,name)
		self.ports = {}
		self.polyphonic = 0
		self.plugintype = ""
		self.libname = ""
		self.pluginlabel = ""
		print "+++ '%s': new node" % self.getPath()
		
	def remove(self):		
		for port in self.getPorts():
			port.remove()
		TreeElement.remove(self)		
		
	def removeChild(self, child):
		del self.ports[child]

	def getPorts(self):
		return self.ports.values()
		
	def setPluginLabel(self,pluginlabel):
		if pluginlabel != self.pluginlabel:
			print "+++ '%s': changing pluginlabel from '%s' to '%s'" % (self.getPath(), self.pluginlabel, pluginlabel)
			self.pluginlabel = pluginlabel			

	def setLibName(self,libname):
		if libname != self.libname:
			print "+++ '%s': changing libname from '%s' to '%s'" % (self.getPath(), self.libname, libname)
			self.libname = libname			
		
	def setPluginType(self,plugintype):
		if plugintype != self.plugintype:
			print "+++ '%s': changing plugintype from '%s' to '%s'" % (self.getPath(), self.plugintype, plugintype)
			self.plugintype = plugintype
		
	def setPolyphonic(self,polyphonic):
		if polyphonic != self.polyphonic:
			print "+++ '%s': changing polyphony from %i to %i" % (self.getPath(), self.polyphonic, polyphonic)
			self.polyphonic = polyphonic

	def hasPort(self,name):
		return self.ports.has_key(name)
		
	def getPort(self,name,mustexist=False):
		if not self.hasPort(name):
			if mustexist:
				return None
			self.ports[name] = self.environment.getPortClass()(self.environment,self,name)
		return self.ports[name]

class Patch(Node):
	def __init__(self,environment,parent,name):
		Node.__init__(self,environment,parent,name)
		self.nodes = {}
		self.patches = {}
		self.poly = 0
		self.enabled = False
		print "### '%s': new patch" % self.getPath()
		
	def remove(self):		
		for patch in self.getPatches():
			patch.remove()
		for node in self.getNodes():
			node.remove()
		Node.remove(self)		
		
	def removeChild(self, child):
		if self.hasNode(child):
			del self.nodes[child]
		elif self.hasPatch(child):
			del self.patches[child]
		else:
			Node.removeChild(self,child)
		
	def getPatches(self):
		return self.patches.values()
		
	def getNodes(self):
		return self.nodes.values()
		
	def getEnabled(self):
		return self.enabled
		
	def setEnabled(self,enabled):
		if enabled != self.enabled:
			print "### '%s': changing enabled from %s to %s" % (self.getPath(), str(self.enabled), str(enabled))
			enabled = self.enabled
		
	def getPoly(self):
		return self.poly
		
	def setPoly(self,poly):
		if poly != self.poly:
			print "### '%s': changing polyphony from %i to %i" % (self.getPath(), self.poly, poly)
			self.poly = poly
		
	def hasNode(self,name):
		return self.nodes.has_key(name)	

	def getNode(self,name,mustexist=False):
		if not self.hasNode(name):
			if mustexist:
				return None
			self.nodes[name] = self.environment.getNodeClass()(self.environment,self,name)
		return self.nodes[name]

	def hasPatch(self,name):
		return self.patches.has_key(name)
	
	def getPatch(self,name,mustexist=False):
		if not self.hasPatch(name):
			if mustexist:
				return None
			self.patches[name] = self.environment.getPatchClass()(self.environment,self,name)
		return self.patches[name]

class Connection:
	def __init__(self,environment,srcport,dstport):
		self.environment = environment
		self.srcport = srcport
		self.dstport = dstport
		self.srcport.addConnection(self.dstport,self)
		self.dstport.addConnection(self.srcport,self)
		print ">>> '%s'->'%s': new connection" % (self.srcport.getPath(),self.dstport.getPath())
		
	def __del__(self):
		print "connection gone"
		
	def remove(self):
		self.srcport.removeConnection(self.dstport)
		self.dstport.removeConnection(self.srcport)
		del self
		
	def getSrcPort(self):
		return self.srcport
		
	def getDstPort(self):
		return self.dstport
		
	def getPortPair(self):
		return self.srcport, self.dstport

class Environment:
	def __init__(self):
		self.omPatch = self.getPatchClass()(self,None,"")
		self.enabled = False
		self.connections = {}		
		
	def getConnectionClass(self):
		return Connection
		
	def getPatchClass(self):
		return Patch
		
	def getNodeClass(self):
		return Node
		
	def getPortClass(self):
		return Port
		
	def getConnection(self,srcportpath,dstportpath,mustexist=False):
		srcport = self.getPort(srcportpath,True)
		if not srcport:
			return None
		dstport = self.getPort(dstportpath,True)
		if not dstport:
			return None
		if not self.connections.has_key((srcport,dstport)):
			if mustexist:
				return None
			self.connections[(srcport,dstport)] = self.getConnectionClass()(self,srcport,dstport)
		return self.connections[(srcport,dstport)]
		
	def getConnections(self):
		return self.connections.values()
		
	def getPatch(self,path,mustexist=False):
		elements = path.split("/")
		currentPatch = None
		for element in elements:
			if element == "":
				currentPatch = self.omPatch
			else:
				currentPatch = currentPatch.getPatch(element,mustexist)
				if not currentPatch:
					break
		return currentPatch

	def getNode(self,path,mustexist=False):
		elements = path.split("/")
		basepath = "/".join(elements[:-1])
		nodename = elements[-1]		
		patch = self.getPatch(basepath,True)
		if patch:
			return patch.getNode(nodename,mustexist)
		return None

	def getPort(self,path,mustexist=False):		
		elements = path.split("/")
		basepath = "/".join(elements[:-1])
		portname = elements[-1]
		node = self.getNode(basepath,True)
		if node:
			return node.getPort(portname,mustexist)
		patch = self.getPatch(basepath,True)
		if patch:
			return patch.getPort(portname,mustexist)
		return None
		
	def getObject(self,path):
		patch = self.getPatch(path,True)
		if patch:
			return patch
		node = self.getNode(path,True)
		if node:
			return node
		return self.getPort(path,True)		

	def printPatch(self,patch=None):
		if not patch:
			patch = self.omPatch
		print patch.getDepth()*'   ' + "### " + patch.getPath()		
		for node in patch.getNodes():
			print node.getDepth()*'   ' + "+++ " + node.getPath()
			for port in node.getPorts():
				print port.getDepth()*'   ' + "*** " + port.getPath()
		for port in patch.getPorts():
			print port.getDepth()*'   ' + "*** " + port.getPath()
		for subpatch in patch.getPatches():
			self.printPatch(subpatch)
			
	def printConnections(self):
		for connection in self.getConnections():
			print ">>> %s -> %s" % (connection.getSrcPort().getPath(), connection.getDstPort().getPath())

	#~ /om/engine_enabled - Notification engine's DSP has been enabled.
	def __ingen__engine_enabled(self):
		self.enabled = True
	
	#~ /om/engine_disabled - Notification engine's DSP has been disabled.
	def __ingen__engine_disabled(self):
		self.enabled = False
		
	#~ /om/new_node - Notification of a new node's creation.
	#~ * path (string) - Path of the new node
	#~ * polyphonic (integer-boolean) - Node is polyphonic (1 for yes, 0 for no)
	#~ * type (string) - Type of plugin (LADSPA, DSSI, Internal, Patch)
	#~ * lib-name (string) - Name of library if a plugin (ie cmt.so)
	#~ * plug-label (string) - Label of plugin in library (ie adsr_env)

	#~ * New nodes are sent as a blob. The first message in the blob will be this one (/om/new_node), followed by a series of /om/new_port commands, followed by /om/new_node_end.
	def __ingen__new_node(self,path,polyphonic,plugintype,libname,pluginlabel):
		node = self.getNode(path)
		node.setPolyphonic(polyphonic)
		node.setPluginType(plugintype)
		node.setLibName(libname)
		node.setPluginLabel(pluginlabel)
		
	def __ingen__new_node_end(self):
		pass
		
	#~ /om/node_removal - Notification of a node's destruction.
	#~ * path (string) - Path of node (which no longer exists)
	def __ingen__node_removal(self,path):
		node = self.getNode(path)
		node.remove()

	#~ /om/new_port - Notification of a node's destruction.

	#~ * path (string) - Path of new port
	#~ * type (string) - Type of port (CONTROL or AUDIO)
	#~ * direction (string) - Direction of data flow (INPUT or OUTPUT)
	#~ * hint (string) - Hint (INTEGER, LOGARITHMIC, TOGGLE, or NONE)
	#~ * default-value (float) - Default (initial) value
	#~ * min-value (float) - Suggested minimum value
	#~ * min-value (float) - Suggested maximum value

	#~ * Note that in the event of loading a patch, this message could be followed immediately by a control change, meaning the default-value is not actually the current value of the port (ahem, Lachlan).
	#~ * The minimum and maximum values are suggestions only, they are not enforced in any way, and going outside them is perfectly fine. Also note that the port ranges in om_gtk are not these ones! Those ranges are set as metadata.
	def __ingen__new_port(self,path,porttype,direction,hint,defaultvalue,minvalue,maxvalue):
		port = self.getPort(path)
		port.setPortType(porttype)
		port.setDirection(direction)
		port.setHint(hint)
		port.setDefaultValue(defaultvalue)
		port.setMinValue(minvalue)
		port.setMaxValue(maxvalue)

	#~ /om/port_removal - Notification of a port's destruction.
	#~ * path (string) - Path of port (which no longer exists)
	def __ingen__port_removal(self,path):
		port = self.getPort(path)
		port.remove()

	#~ /om/patch_destruction - Notification of a patch's destruction.
	#~ * path (string) - Path of patch (which no longer exists)
	def __ingen__patch_destruction(self,path):
		patch = self.getPatch(path)
		patch.remove()

	#~ /om/patch_enabled - Notification a patch's DSP processing has been enabled.
	#~ * path (string) - Path of enabled patch
	def __ingen__patch_enabled(self,path):
		patch = self.getPatch(path)
		patch.setEnabled(True)

	#~ /om/patch_disabled - Notification a patch's DSP processing has been disabled.
	#~ * path (string) - Path of disabled patch
	def __ingen__patch_disabled(self,path):
		patch = self.getPatch(path)
		patch.setEnabled(False)

	#~ /om/new_connection - Notification a new connection has been made.
	#~ * src-path (string) - Path of the source port
	#~ * dst-path (string) - Path of the destination port
	def __ingen__new_connection(self,srcpath,dstpath):
		self.getConnection(srcpath,dstpath)

	#~ /om/disconnection - Notification a connection has been unmade.
	#~ * src-path (string) - Path of the source port
	#~ * dst-path (string) - Path of the destination port
	def __ingen__disconnection(self,srcpath,dstpath):
		connection = self.getConnection(srcpath,dstpath)
		portpair = connection.getPortPair()
		connection.remove()
		del self.connections[portpair]

	#~ /om/metadata/update - Notification of a piece of metadata.
	#~ * path (string) - Path of the object associated with metadata (can be a node, patch, or port)
	#~ * key (string)
	#~ * value (string)
	def __ingen__metadata__update(self,path,key,value):
		object = self.getObject(path)
		object.setMetaData(key,value)

	#~ /om/control_change - Notification the value of a port has changed
	#~ * path (string) - Path of port
	#~ * value (float) - New value of port
	#~ * This will only send updates for values set by clients of course - not values changing because of connections to other ports!
	def __ingen__control_change(self,path,value):
		port = self.getPort(path)
		port.setValue(value)

	#~ /om/new_patch - Notification of a new patch
	#~ * path (string) - Path of new patch
	#~ * poly (int) - Polyphony of new patch (not a boolean like new_node)
	def __ingen__new_patch(self,path,poly):				
		patch = self.getPatch(path)
		patch.setPoly(poly)

class SocketError:
	pass

class Call:
	pass

class ClientProxy:
	def __init__(self, om, name, is_async = False):
		self.name = name
		self.om = om
		self.is_async = is_async
		
	def __call__(self, *args):
		if (self.is_async):
			self.om.sendMsg(self.name, *args)
			return True
		
		result = self.om.sendMsgBlocking(self.name, *args)
		if not result:
			return None
		if result[0] == "/om/response/ok":
			return True
		print "ERROR: %s" % result[1][1]
		return False
		
	def __getattr__(self, name):
		if (name[:2] == "__") and (name[-2:] == "__"): # special function
			raise AttributeError, name
		if name in self.__dict__:
			raise AttributeError, name
		if name == 'async':
			return ClientProxy(self.om, self.name, True)
		else:
			return ClientProxy(self.om, self.name + '/' + name)

class Client(DatagramProtocol, ClientProxy):
	def __init__(self):
		ClientProxy.__init__(self, self, "/om")
	
	def startProtocol(self):		
		self.transport.connect("127.0.0.1", 16180)
		host = self.transport.getHost()
		self.host = host.host
		self.port = host.port
		self.environment = None
		self.handlers = {}
		self.calls = {}
		#print "opened port at %s" % (str((self.host,self.port)))
		self.nextPacketNumber = 1
		
	def setEnvironment(self, environment):
		self.handlers = {}
		self.environment = environment
		for name in dir(self.environment):			
			element = getattr(self.environment,name)
			if (type(element).__name__ == 'instancemethod') and (element.__name__[:6] == "__ingen__"):
				handlername = element.__name__.replace("__","/")
				print "registering handler for '%s'" % handlername				
				self.handlers[handlername] = element
		
	def getEnvironment(self):
		return self.environment

	def connectionRefused(self):
		print "Noone listening, aborting."
		os._exit(-1)

	def messageReceived(self, (msg, args)):
		if msg == "/om/error":
			print "ERROR: %r" % args
			return
		if msg == "/om/response/ok":
			omcall = self.calls[args[0]]
			omcall.result = (msg,args)
			omcall.done = True
			return
 		if msg == "/om/response/error":
 			omcall = self.calls[args[0]]
 			omcall.result = (msg,args)
 			omcall.done = True
 			return
		if msg == "#bundle":
			for arg in args:
				self.messageReceived(arg)
			return
		if self.handlers.has_key(msg):
			try:
				self.handlers[msg](*args)
			except:
				a,b,c = sys.exc_info()
				sys.excepthook(a,b,c)
				print "with '%s'" % repr((msg,args))
			return
		print "no handler for '%s'" % repr((msg,args))

	def datagramReceived(self, data, (host, port)):
		self.messageReceived(decodeOSC(data))

	def getPacketNumber(self):
		packetNumber = self.nextPacketNumber
		self.nextPacketNumber = (self.nextPacketNumber + 1)
		return packetNumber
		
	def sendMsg(self,msg,*args):
		packetNumber = self.getPacketNumber()
		print "Sending %r (#%i)..." % (msg,packetNumber)
		omcall = Call()
		omcall.result = None
		omcall.done = False
		self.calls[packetNumber] = omcall
		message = OSCMessage()
		message.setAddress(msg)
		message.append(packetNumber)
		for arg in args:
			message.append(arg)
		self.transport.write(message.getBinary())
		time.sleep(0.01)
		return True

	def sendMsgBlocking(self,msg,*args):
		packetNumber = self.getPacketNumber()
		print "Sending %r (#%i)..." % (msg,packetNumber)
		omcall = Call()
		omcall.result = None
		omcall.done = False
		self.calls[packetNumber] = omcall
		message = OSCMessage()
		message.setAddress(msg)
		message.append(packetNumber)
		for arg in args:
			message.append(arg)
		self.transport.write(message.getBinary())
		now = time.time()
		while not omcall.done:
			time.sleep(INGEN_CALL_POLLTIME)			
			distance = time.time() - now
			if distance > INGEN_CALL_TIMEOUT:
				print "timeout"
				break			
		del self.calls[packetNumber]
		return omcall.result

 	def getAddressAsString(self):
 		return "osc.udp://%s:%d" % (self.host, self.port)
 
		

def startClient(func):
	om = Client()
	reactor.listenUDP(0, om)
	thread.start_new_thread(func,(om,))
	reactor.run()
