#!/usr/bin/env python

import ingen
import time

world = ingen.World()

class PythonClient(ingen.Client):
    def error(self, msg):
        print "*** Received error:", msg

    def bundle_begin(self):
        print "*** Receiving Bundle {"
    
    def bundle_end(self):
        print "*** }"

    def new_port(self, path, data_type, is_output):
        print "*** Received Port:", path, data_type, is_output

    def new_node(self, plugin_uri, path, polyphonic, nports):
        print "*** Received Node:", plugin_uri, path, polyphonic, nports

c = PythonClient()
c.enable()

e = world.engine
e.activate()

c.subscribe(e)

e.create_port("/I", "ingen:midi", False)
e.create_port("/made", "ingen:audio", False)
e.create_port("/these", "ingen:audio", False)
e.create_port("/in", "ingen:midi", True)
e.create_port("/a", "ingen:audio", True)
e.create_port("/script", "ingen:audio", True)

while True:
    world.iteration()
    time.sleep(0.1)

