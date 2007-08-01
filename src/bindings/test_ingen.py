#!/usr/bin/env python
import ingen
import time

world = ingen.World()

class PythonClient(ingen.Client):
    #def __init__(self):
     #   ingen.Client(self)
      #  print "Client"

    def bundle_begin():
        print "Bundle {"


    def new_port(self, path, data_type, is_output):
        print "Port:", path, data_type, is_output

c = PythonClient()
c.thisown = 0
print "C OWN", c.thisown
#print c.__base__

e = world.engine
print "E OWN", e.thisown
e.thisown = 0
#print e

e.activate()

#e.register_client("foo", c)
c.subscribe(e)

c.enable()
#c.new_patch("/foo/bar", 1)


e.create_port("/I", "ingen:midi", False)
e.create_port("/made", "ingen:audio", False)
e.create_port("/these", "ingen:audio", False)
e.create_port("/in", "ingen:midi", True)
e.create_port("/a", "ingen:audio", True)
e.create_port("/script", "ingen:audio", True)

while True:
    time.sleep(1)

