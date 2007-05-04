// TODO:
// * Keep track of established connections.
Ingen : Model {
	classvar <>program = "om", <>patchLoader = "om_patch_loader";
	classvar <>oscURL, <nodeTypeMap, <>uiClass;
	var <addr;
	var <>loadIntoJack = true;
	var allocator, requestResponders, requestHandlers, notificationResponders;
	var creatingNode, newNodeEnd;
	var <registered = false, <booting = false;
	var <root;
	var <plugins, pluginParentEvent;
	var onNewPatch;
	*initClass {
		Class.initClassTree(Event);
		Class.initClassTree(NetAddr);
		Event.parentEvents.default[\omcmd] = \noteOn;
		Event.parentEvents.default[\omEventFunctions] = (
			noteOn:  #{ arg midinote=60, amp=0.1;
				[midinote, asInteger((amp * 127).clip(0, 127)) ] },
			noteOff: #{ arg midinote=60; [ midinote ] },
			setPortValue: #{|portValue=0| [portValue] }
		);
		Event.parentEvents.default.eventTypes[\om]=#{|server|
			var freqs, lag, dur, sustain, strum, target, bndl, omcmd;
			freqs = ~freq = ~freq.value + ~detune;
			if (freqs.isKindOf(Symbol).not) {
				~amp = ~amp.value;
				strum = ~strum;
				lag = ~lag + server.latency;
				sustain = ~sustain = ~sustain.value;
				omcmd = ~omcmd;
				target = ~target;
				~midinote = ~midinote.value;
				~portValue = ~portValue.value;
				bndl = ~omEventFunctions[omcmd].valueEnvir.asCollection;
				bndl = bndl.flop;
				bndl.do {|msgArgs, i|
					var latency;
					latency = i * strum + lag;
					if(latency == 0.0) {
						target.performList(omcmd, msgArgs)
					} {
						thisThread.clock.sched(latency, {
							target.performList(omcmd, msgArgs);
						})
					};
					if(omcmd === \noteOn) {
						thisThread.clock.sched(sustain + latency, { 
							target.noteOff(msgArgs[0])
						})
					}
				}
			}
		};
		oscURL="osc.udp://"++NetAddr.localAddr.ip++$:++NetAddr.localAddr.port;
		nodeTypeMap = IdentityDictionary[
			\Internal -> IngenInternalNode,
			\LADSPA   -> IngenLADSPANode,
			\DSSI     -> IngenDSSINode
		];
		uiClass = IngenEmacsUI
	}
	*new { | netaddr |
		^super.new.init(netaddr)
	}
	gui { ^uiClass.new(this) }
	init { |netaddr|
		addr = netaddr ? NetAddr("127.0.0.1", 16180);
		onNewPatch = IdentityDictionary.new;
		allocator = StackNumberAllocator(0,1024);
		requestHandlers = IdentityDictionary.new;
		requestResponders = [
			"response/ok" -> {|id|
				requestHandlers.removeAt(id).value; allocator.free(id) },
			"response/error" -> {|id,text|
				requestHandlers.removeAt(id);
				allocator.free(id);
				("Ingen"+text).error }
		].collect({|a|
			var func = a.value;
			OSCresponder(addr, "/om/"++a.key, {|time,resp,msg|
				func.value(*msg[1..])
			})
		});
		notificationResponders = [
			"new_patch" -> {|path,poly|
				var func = onNewPatch.removeAt(path);
				if (func.notNil) {
					func.value(this.getPatch(path,false).prSetPoly(poly))
				}
			},
			"metadata/update" -> {|path,key,value|
				this.getObject(path).metadata.prSetMetadata(key, value) },
			"new_node" -> {|path,poly,type,lib,label|
				var patchPath, nodeName, patch, node;
				var lastSlash = path.asString.inject(nil,{|last,char,i|
					if(char==$/,i,last)
				});
				if (lastSlash.notNil) {
					patchPath = path.asString.copyFromStart(lastSlash-1);
					nodeName = path.asString.copyToEnd(lastSlash+1);
					patch = this.getPatch(patchPath);
					if (patch.notNil) {
						if (patch.hasNode(nodeName).not) {
							node = nodeTypeMap[type].new
							(nodeName, patch, poly, label, lib);
							creatingNode = node;
							patch.nodes[nodeName.asSymbol] = node;
							patch.changed(\newNode, node);
						} {
							if (patch.getNode(nodeName).class != nodeTypeMap[type]) {
								("Ingen sent an existng node with differing type"+path).warn
							}
						}
					} {
						("Ingen tried to create node in non-existing patch"+patchPath).warn
					}
				} {
					("Invalid path in node creation"+path).warn
				}
			},
			"new_node_end" -> {
				newNodeEnd.value(creatingNode);
				newNodeEnd = nil;
				creatingNode = nil },
			"new_port" -> {|path,type,dir,hint,def,min,max|
				var basePath, portName, parent, port;
				var lastSlash = path.asString.inject(nil,{|last,char,i|
					if(char==$/,i,last)
				});
				if (lastSlash.notNil) {
					basePath = path.asString.copyFromStart(lastSlash-1);
					portName = path.asString.copyToEnd(lastSlash+1);
					parent = this.getNode(basePath) ? this.getPatch(basePath);
					if (parent.notNil) {
						if (parent.hasPort(portName).not) {
							port = IngenPort.new(portName, parent, type, dir, hint, def, min, max);
							parent.ports[portName.asSymbol] = port;
							parent.changed(\newPort, port)
						} {
							if (parent.getPort(portName).porttype != type) {
								("Ingen tried to create an already existing port with differing type"
									+path).warn
							}
						}
					} {
						("Ingen tried to create port on non-existing object"+basePath).warn
					}
				} {
					("Invalid path in port creation"+path).warn
				}
			},
			"control_change" -> {|path,value|
				this.getPort(path).prSetValue(value) },
			"patch_enabled" -> {|path| this.getPatch(path).prSetEnabled(true) },
			"patch_disabled" -> {|path| this.getPatch(path).prSetEnabled(false) },
			"plugin" -> {|lib,label,name,type|
				plugins.add(Event.new(4,nil,pluginParentEvent).putAll(
					(type:type, lib:lib, label:label, name:name))) },
			"node_removal" -> {|path|
				var node = this.getNode(path);
				if (node.notNil) {
					node.parent.nodes.removeAt(node.name.asSymbol).free
				} {
					("Ingen attempting to remove non-existing node"+path).warn
				}
			},
			"port_removal" -> {|path|
				var port = this.getPort(path);
				if (port.notNil) {
					port.parent.ports.removeAt(port.name.asSymbol).free
				} {
					("Ingen attempting to remove non-existing port"+path).warn
				}
			},
			"patch_destruction" -> {|path|
				var patch = this.getPatch(path);
				if (patch.notNil) {
					patch.parent.patches.removeAt(patch.name.asSymbol).free
				} {
					("Ingen attempting to remove non-existing patch"+path).warn
				}
			},
			"program_add" -> {|path,bank,program,name|
				var node = this.getNode(path);
				if (node.respondsTo(\prProgramAdd)) {
					node.prProgramAdd(bank,program,name)
				} {
					("Ingen tried to add program info to"+node).warn
				}
			}
		].collect({|a|
			var func = a.value;
			OSCresponder(addr, "/om/"++a.key, {|time,resp,msg|
				func.value(*msg[1..])
			})
		});
		pluginParentEvent = Event.new(2,nil,nil).putAll((
			engine:this,
			new:{|self,path,poly=1,handler|self.engine.createNode(path?("/"++self.name),self.type,self.lib,self.label,poly,created:handler)}
		));
	}
	*waitForBoot {|func| ^this.new.waitForBoot(func) }
	waitForBoot {|func|
		var r, id = 727;
		requestHandlers[id] = {
			r.stop;
			booting=false;
			this.changed(\running, true);
			func.value(this)
		};
		if (booting.not) {this.boot};
		r = Routine.run {
			50.do {
				0.1.wait;
				addr.sendMsg("/om/ping", id)
			};
			requestHandlers.removeAt(id);
			"Ingen engine boot failed".error;
		}
	}
	getPatch {|path, mustExist=true|
		var elements, currentPatch;
		if (path.class == Array) { elements = path
		} { elements = path.asString.split($/) };
		elements.do{|elem|
			if (elem=="") {
				currentPatch = root
			} {
				currentPatch = currentPatch.getPatch(elem,mustExist);
				if (currentPatch.isNil) { ^nil }
			}
		};
		^currentPatch;
	}
	getNode {|path|
		var basePath, nodeName, patch;
		if (path.class == Array) { basePath = path
		} { basePath = path.asString.split($/) };
		nodeName = basePath.pop;
		patch = this.getPatch(basePath,true);
		if (patch.notNil) {
			^patch.getNode(nodeName)
		};
		^nil
	}
	getPort {|path|
		var basePath, portName, node, patch;
		basePath = path.asString.split($/);
		portName = basePath.pop;
		node = this.getNode(basePath.copy);
		if (node.notNil) { ^node.getPort(portName) };
		patch = this.getPatch(basePath,true);
		if (patch.notNil) { ^patch.getPort(portName) };
		^nil
	}
	getObject {|path|
		var patch,node,port;
		patch = this.getPatch(path,true);
		if (patch.notNil) { ^patch };
		node = this.getNode(path);
		if (node.notNil) { ^node };
		port = this.getPort(path,true);
		if (port.notNil) { ^port };
		^nil
	}
	at {|path|^this.getObject(path.asString)}
	*boot {|func|
		^Ingen.new.waitForBoot {|e|
			e.activate {
				e.register {
					e.loadPlugins {
						e.requestPlugins {
							e.requestAllObjects {
								func.value(e)
							}
						}
					}
				}
			}
		}
	}
	boot {
		requestResponders.do({|resp| resp.add});
		booting = true;
		if (addr.addr == 2130706433) {
			if (loadIntoJack) {
				("jack_load"+"-i"+addr.port+"Ingen"+"om").unixCmd
			} {
				(program+"-p"+addr.port).unixCmd
			}
		} {
			"You have to manually boot Ingen now".postln
		}
	}
	loadPatch {|patchPath| (patchLoader + patchPath).unixCmd }
	activate { | handler |
		this.sendReq("engine/activate", {
			root = IngenPatch("",nil,this);
			this.changed(\newPatch, root);
			handler.value
		})
	}
	register { | handler |
		this.sendReq("engine/register_client", {
			registered=true;
			notificationResponders.do({|resp| resp.add});
			this.changed(\registered, registered);
			handler.value(this)
		})
	}
	unregister { | handler |
		this.sendReq("engine/unregister_client", {
			registered=false;
			notificationResponders.do({|resp| resp.remove});
			this.changed(\registered, registered);
			handler.value(this)
		})
	}
	registered_ {|flag|
		if (flag and: registered.not) {
			this.register
		} {
			if (flag.not and: registered) {
				this.unregister
			}
		}
	}
	loadPlugins { | handler | this.sendReq("engine/load_plugins", handler) }
	requestPlugins {|handler|
		var startTime = Main.elapsedTime;
		plugins = Set.new;
		this.sendReq("request/plugins", {
			("Received info about"+plugins.size+"plugins in"+(Main.elapsedTime-startTime)+"seconds").postln;
			this.changed(\plugins, plugins);
			handler.value(this);
		})
	}
	requestAllObjects { |handler|
		this.sendReq("request/all_objects", handler)
	}
	createPatch { | path, poly=1, handler |
		onNewPatch[path.asSymbol] = handler;
		this.sendReq("synth/create_patch", nil, path.asString, poly.asInteger)
	}
	createNode { | path, type='LADSPA', lib, label, poly=1, created, handler |
		newNodeEnd = created;
		this.sendReq("synth/create_node",handler,path,type,lib,label,poly)
	}
	createAudioInput { | path, handler |
		this.createNode(path,"Internal","","audio_input",0,handler)
	}
	createAudioOutput {|path,handler|
		this.createNode(path,"Internal","","audio_output",0,handler)
	}
	createMIDIInput {|path,handler|
		this.createNode(path,"Internal","","midi_input",1,handler)
	}
	createMIDIOutput {|path,handler|
		this.createNode(path,"Internal","","midi_output",1,handler)
	}
	createNoteIn {|path| this.createNode(path,"Internal","","note_in") }
	connect {|fromPath,toPath,handler|
		this.sendReq("synth/connect",handler,fromPath.asString,toPath.asString)
	}
	disconnect { | fromPath, toPath, handler |
		this.sendReq("synth/disconnect",handler,fromPath.asString,toPath.asString)
	}
	disconnectAll { | path, handler |
		this.sendReq("synth/disconnect_all",handler,path);
	}
	sendReq { | path, handler...args |
		var id = allocator.alloc;
		requestHandlers[id] = handler;
		addr.sendMsg("/om/"++path, id, *args)
	}
	quit {
		if (loadIntoJack) {
			("jack_unload"+"Ingen").unixCmd;
			booting=false;
			requestResponders.do(_.remove);
			notificationResponders.do(_.remove);
			this.changed(\running, false);
		} {
			this.sendReq("engine/quit", {
				booting=false;
				requestResponders.do(_.remove);
				notificationResponders.do(_.remove);
				this.changed(\running, false);
			})
		}
	}
	ping {| n=1, func |
		var id, result, start;
		id = allocator.alloc;
		result = 0;
		requestHandlers[id] = {
			var end;
			end = Main.elapsedTime;
			result=max((end-start).postln,result);
			n=n-1;
			if (n > 0) {
				start = Main.elapsedTime;
				addr.sendMsg("/om/ping", id)
			} {
				allocator.free(id);
				func.value(result)
			}
		};
		start = Main.elapsedTime;
		addr.sendMsg("/om/ping", id)
	}
	setPortValue {|path, val| this.getPort(path.asString).value=val	}
	jackConnect {|path, jackPort|
		this.getPort(path).jackConnect(jackPort)
	}
	noteOn {|path, note, vel|
		var patch,node;
		patch = this.getPatch(path,true);
		if (patch.notNil) { patch.noteOn(note,vel) };
		node = this.getNode(path);
		if (node.notNil) { node.noteOn(note,vel) };
	}
	noteOff {|path, note|
		var patch,node;
		patch = this.getPatch(path,true);
		if (patch.notNil) { patch.noteOff(note) };
		node = this.getNode(path);
		if (node.notNil) { node.noteOff(note) };
	}
	matchPlugins{ | label, lib, name, type |
		^plugins.select{ |p|
			label.matchItem(p.label) and: {
				lib.matchItem(p.lib) and: {
					name.matchItem(p.name) and: {
						type.matchItem(p.type)
					}
				}
			}
		}
	}
	dssiMsg {|path,reqType="program" ...args|
		addr.sendMsg("/dssi"++path++$/++reqType,*args)
	}
}

IngenMetadata {
	var object, dict;
	*new {|obj|^super.new.metadataInit(obj)}
	metadataInit {|obj|
		dict=Dictionary.new;
		object=obj
	}
	put {|key,val|
		object.engine.sendReq("metadata/set", nil,
			object.path, key.asString, val.asString)
	}
	at {|key|^dict.at(key.asSymbol)}
	prSetMetadata {|key,val|
		dict.put(key,val);
		object.changed(\metadata, key, val)
	}
}

IngenObject : Model {
	var <name, <parent, <metadata;
	*new {|name, parent|
		^super.new.initIngenObject(name,parent);
	}
	initIngenObject {|argName, argParent|
		name = argName;
		parent = argParent;
		metadata=IngenMetadata(this)
	}
	path { ^parent.notNil.if({ parent.path ++ $/ ++ name }, name).asString }
	depth { ^parent.notNil.if({ parent.depth + 1 }, 0) }
	engine { ^parent.engine }
}

IngenPort : IngenObject {
	var <porttype, <direction, <spec, <value, <connections;
	*new {|name,parent,type,dir,hint,def,min,max|
		^super.new(name,parent).initPort(type,dir,hint,def,min,max)
	}
	initPort {|type,dir,hint,def,min,max|
		porttype = type;
		direction = dir;
		spec = ControlSpec(min, max,
			if (hint == 'LOGARITHMIC', 'exp', 'lin'),
			if (hint == 'INTEGER', 1, 0),
			def);
		connections = IdentityDictionary.new;
	}
	jackConnect {|jackPort|
		if (porttype.asSymbol != \AUDIO
			|| direction.asSymbol!=\OUTPUT) {
			Error("Not a audio output port").throw
		};
		("jack_connect" + "Ingen:"++(this.path) + jackPort).unixCmd
	}
	value_ {|val|
		if (porttype == \CONTROL and: {direction == \INPUT}) {
			this.engine.sendReq("synth/set_port_value", nil, this.path, val)
		} {
			Error("Not a input control port"+this.path).throw
		}
	}
	connectTo {|destPort|
		if (this.direction!=destPort.direction) {
			this.engine.connect(this,destPort)
		} {
			Error("Unable to connect ports with same direction").throw
		}
	}

	// Setters for OSC responders
	prSetValue {|argValue|
		if (value != argValue) {
			value = argValue;
			this.changed(\value, value)
		}
	}
}

IngenNode : IngenObject { // Abstract class
	var <ports, <polyphonic;
	*new {|name,parent,poly|
		^super.new(name,parent).initNode(poly)
	}
	initNode {|argPoly|
		polyphonic = argPoly;
		ports = IdentityDictionary.new;
	}
	hasPort {|name| ^ports[name.asSymbol].notNil }
	getPort {|name| ^ports[name.asSymbol] }
	portArray {|type=\AUDIO,dir=\OUTPUT|
		var result = Array.new(8);
		ports.do {|port|
			if (port.porttype==type and: {port.direction==dir}) {
				result=result.add(port)
			}
		};
		^result
	}
	audioOut {
		^this.portArray(\AUDIO, \OUTPUT)
	}
	audioIn {
		^this.portArray(\AUDIO, \INPUT)
	}
}

IngenInternalNode : IngenNode {
	var <pluginlabel;
	*new {|name,parent,poly,label|
		^super.new(name,parent,poly).initInternalNode(label)
	}
	initInternalNode {|label|
		pluginlabel = label
	}
	noteOn { |note,vel|
		if (pluginlabel == \note_in or:
			{pluginlabel == \trigger_in and: {this.value == note}}) {
			this.engine.sendReq("synth/note_on", nil, this.path, note, vel)
		} {
			Error("Not a trigger_in or note_in node").throw
		}
	}
	noteOff {|note|
		if (pluginlabel == \note_in or:
			{pluginlabel == \trigger_in and: {this.value == note}}) {
			this.engine.sendReq("synth/note_off", nil, this.path, note)
		} {
			Error("Not a trigger_in or note_in node").throw
		}
	}
}

IngenLADSPANode : IngenNode {
	var <pluginlabel, <libname;
	*new {|name, parent, poly, label, lib|
		^super.new(name,parent,poly).initLADSPANode(label,lib)
	}
	initLADSPANode {|label,lib|
		pluginlabel = label;
		libname = lib
	}
}

IngenDSSINode : IngenLADSPANode {
	var programs;
	*new {|name,parent,poly,label,lib|
		^super.new(name,parent,poly,label,lib).initDSSI
	}
	initDSSI {
		programs = Set.new;
	}
	program {|bank, prog|
		this.engine.dssiMsg(this.path,"program",bank.asInteger,prog.asInteger)
	}
	prProgramAdd {|bank,program,name|
		var info = (bank:bank, program:program, name:name);
		if (programs.includes(info).not) {
			programs.add(info);
			this.changed(\programAdded, info)
		}
	}
}

IngenPatch : IngenNode {
	var <nodes, <patches, <poly, <enabled;
	var om;
	*new {|name,parent,engine|
		^super.new(name,parent).initPatch(engine);
	}
	initPatch {|argEngine|
		nodes = IdentityDictionary.new;
		patches = IdentityDictionary.new;
		om = argEngine
	}
	hasNode {|name|
		^nodes[name.asSymbol].notNil
	}
	getNode {|name|
		^nodes[name.asSymbol]
	}
	hasPatch {|name|
		^patches[name.asSymbol].notNil
	}
	getPatch {|name,mustExist=false|
		if (this.hasPatch(name).not) {
			if (mustExist) { ^nil };
			patches[name.asSymbol] = IngenPatch(name,this);
		};
		^patches[name.asSymbol]
	}
	engine {
		if (om.notNil) { ^om } { ^parent.engine };
	}
	connect {|fromPort, toPort|
		this.engine.connect(this.path++$/++fromPort, this.path++$/++toPort)
	}
	dumpX {
		(String.fill(this.depth,$ )++"*"+this.path).postln;
		nodes.do{|node|
			(String.fill(node.depth,$ )++"**"+node.path).postln;
			node.ports.do{|port|
				(String.fill(port.depth,$ )++"***"+port.path).postln };
		};
		ports.do{|port| (String.fill(port.depth,$ )++"***"+port.path).postln };
		patches.do(_.dump)
	}
	noteOn {|note,vel|
		var targetNode;
		this.nodes.do{|node|
			if (node.class == IngenInternalNode) {
				node.pluginlabel.switch(
					\trigger_in, {
						if (node.ports['Note Number'].value == note) {
							targetNode = node;
						}
					},
					\note_in, {
						targetNode = node;
					}
				)
			}
		};
		if (targetNode.notNil) {
			targetNode.noteOn(note, vel)
		} {
			Error("Unable to find trigger_in for note "++note).throw
		}
	}
	noteOff {|note|
		var targetNode;
		this.nodes.do{|node|
			if (node.class == IngenInternalNode) {
				node.pluginlabel.switch(
					\trigger_in, {
						if (node.ports['Note Number'].value == note) {
							targetNode = node;
						}
					},
					\note_in, {
						targetNode = node;
					}
				)
			}
		};
		if (targetNode.notNil) {
			targetNode.noteOff(note)
		} {
			Error("Unable to find node for note_off "++note).throw
		}
	}
	newPatch {|name, poly=1, handler|
		this.engine.createPatch(this.path++$/++name, poly, handler)
	}
	newNode {|name, poly=1, type, lib, label, fullname, handler|
		var candidates = this.engine.matchPlugins(label,lib,fullname,type);
		if (candidates.size == 1) {
			candidates.pop.new(this.path++"/"++name, poly, handler)
		} {
			if (candidates.size==0) {
				Error("Plugin not found").throw
			} {
				Error("Plugin info not unique").throw
			}
		}
	}

	// Setters for OSC responders
	prSetPoly {|argPoly| poly = argPoly }
	prSetEnabled {|flag|
		if (flag != enabled) {
			enabled = flag;
			this.changed(\enabled, flag)
		}
	}
}


IngenEmacsUI {
	var engine, window, bootBtn;
	*new {|engine| ^super.newCopyArgs(engine).init }
	init {
		window = EmacsBuffer("*Ingen -"+engine.addr.ip++$:++engine.addr.port);
		bootBtn = EmacsButton(window, ["Boot","Quit"], {|value|
			if (value==1) {
				engine.boot
			} {
				engine.quit
			}
		}).value=engine.registered.binaryValue;
		engine.addDependant(this);
		window.front;
	}
	update {|who, what  ... args|
		Emacs.message(who.asString+what+args);
		if (what == \newPatch or: {what == \newNode or: {what == \newPort}}) {
			args[0].addDependant(this)
		};
	}
}
