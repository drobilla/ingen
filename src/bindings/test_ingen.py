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

	def put(self, path, properties):
		print "*** Received Object:", path

c = PythonClient()
c.enable()

e = world.engine
e.activate()

c.subscribe(e)

e.create_port("/dynamic_port", "http://lv2plug.in/ns/ext/event#EventPort", False)

while True:
    world.iteration()
    time.sleep(0.1)

