#!/usr/bin/env python
import ingen
import time

world = ingen.World()
e = world.engine

e.activate()
e.create_port("/I", "ingen:midi", False)
e.create_port("/made", "ingen:audio", False)
e.create_port("/these", "ingen:audio", False)
e.create_port("/in", "ingen:midi", True)
e.create_port("/a", "ingen:audio", True)
e.create_port("/script", "ingen:audio", True)

while True:
    time.sleep(1)

