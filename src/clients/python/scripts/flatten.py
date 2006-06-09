#!/usr/bin/python

###############################################################################
#
#    flatten.py - a python script that merges all subpatches in an Om patch
#                 into the parent patch
#
#    Copyright (C) 2005 Lars Luthman
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###############################################################################

import omsynth
import os,time,sys


def getPatchBounds(patch):
    """Returns the smallest rectangle that contains all modules in the patch."""
    min_x = None
    min_y = None
    max_x = None
    max_y = None
    for node in patch.getNodes():
        x = node.metadata['module-x']
        y = node.metadata['module-y']
        if (x != None):
            if (min_x == None or float(x) < min_x):
                min_x = float(x)
            if (max_x == None or float(x) > max_x):
                max_x = float(x)
            if (y != None):
                if (min_y == None or float(y) < min_y):
                    min_y = float(y)
                if (max_y == None or float(y) > max_y):
                    max_y = float(y)
    if min_x == None:
        min_x = 0
        max_x = 0
    if min_y == None:
        min_y = 0
        max_y = 0

    return (min_x, min_y, max_x, max_y)


def cloneNode(om, node, patch):
    """Copy a node into a patch, return the new node's name."""

    # create a node with a free name in the parent
    names = []
    for node2 in patch.getNodes():
        names.append(node2.getName())
    for patch2 in patch.getPatches():
        names.append(patch2.getName())
    names.sort()
    name = node.getName()
    for name2 in names:
        if name2 == name:
            name = name + '_'
    om.synth.create_node.async(patch.getPath() + '/' + name,
                               node.plugintype, node.libname,
                               node.pluginlabel, node.polyphonic)

    # copy port values
    for port in node.getPorts():
        path = '%s/%s/%s' % (patch.getPath(), name, port.getName())
        om.synth.set_port_value_slow.async(path, port.value)
        om.metadata.set.async(path, 'user-min', '%f' % port.minvalue)
        om.metadata.set.async(path, 'user-max', '%f' % port.maxvalue)
    return name


def flatten(om, patch):
    """Merge all subpatches into the parent patch."""

    # something is wrong, we don't have a patch
    if patch == None:
        return

    # iterate over all subpatches
    for subpatch in patch.getPatches():
        flatten(om, subpatch)
        lookup = {}

        # copy all nodes from the subpatch to the parent patch
        for node in subpatch.getNodes():
            lookup[node.getName()] = cloneNode(om, node, patch)

        # copy all connections
        for node in subpatch.getNodes():
            for port in node.getPorts():
                if port.direction == 'OUTPUT':
                    for target in port.getConnections().keys():
                        targetname = '%s/%s' % (lookup[target.getParent().getName()], target.getName())
                        om.synth.connect.async(patch.getPath() + '/' +
                                               lookup[node.getName()] + '/' +
                                               port.getName(),
                                               patch.getPath() + '/' +
                                               targetname)

        # make external connections
        for node in subpatch.getNodes():
            if node.libname == '':
                lbl = node.pluginlabel

                if lbl == 'audio_input' or lbl == 'control_input':
                    port1 = node.getPort('in')
                    for port2 in port1.getConnections().keys():
                        dst = '%s/%s/%s' % (patch.getPath(), lookup[port2.getParent().getName()], port2.getName())
                        port4 = subpatch.getPort(node.getName())
                        conns = port4.getConnections().keys()
                        if len(conns) == 0:
                            portValue = port4.value
                            om.synth.set_port_value_slow.async(dst, portValue)
                        else:
                            for port3 in port4.getConnections().keys():
                                src = port3.getPath()
                                om.synth.connect.async(src, dst)
                
                if lbl == 'audio_output' or lbl == 'control_output':
                    port2 = node.getPort('out', True)
                    for port1 in port2.getConnections().keys():
                        src = '%s/%s/%s' % (patch.getPath(), lookup[port1.getParent().getName()], port1.getName())
                        port3 = subpatch.getPort(node.getName())
                        for port4 in port3.getConnections().keys():
                            dst = port4.getPath()
                            om.synth.connect.async(src, dst)

        # destroy all input and output nodes from the subpatch
        for node in subpatch.getNodes():
            if node.libname == '':
                lbl = node.pluginlabel
                if (lbl == 'audio_input' or lbl == 'control_input' or
                    lbl == 'audio_output' or lbl == 'control_output'):
                    om.synth.destroy_node.async('%s/%s' % (patch.getPath(),
                                                           lookup[node.getName()]))
        
        # calculate where to move all the new nodes
        (min_x, min_y, max_x, max_y) = getPatchBounds(subpatch)
        sub_x = subpatch.metadata['module-x']
        if sub_x == None:
            sub_x = 0
        sub_y = subpatch.metadata['module-y']
        if sub_y == None:
            sub_y = 0
        x_offset = float(sub_x)
        if min_x != None:
            x_offset = float(sub_x) - min_x
        y_offset = float(sub_y)
        if min_y != None:
            y_offset = float(sub_y) - min_y

        # move the new nodes
        for node in subpatch.getNodes():
            x = float(node.metadata['module-x'])
            if x == None:
                x = 0
            om.metadata.set.async('%s/%s' % (patch.getPath(),
                                             lookup[node.getName()]),
                                  'module-x', '%f' % (x + x_offset))
            y = float(node.metadata['module-y'])
            if y == None:
                y = 0
            om.metadata.set.async('%s/%s' % (patch.getPath(),
                                             lookup[node.getName()]),
                                  'module-y', '%f' % (y + y_offset))

        # move the old nodes in the patch
        x_offset = 0
        if min_x != None and max_x != None:
            x_offset = max_x - min_x
        y_offset = 0
        if min_y != None and max_y != None:
            y_offset = max_y - min_y
        for node in patch.getNodes():
            if node.getName() not in lookup.values():
                x = node.metadata['module-x']
                if x != None and float(x) > float(sub_x):
                    om.metadata.set.async(node.getPath(), 'module-x',
                                          '%f' % (float(x) + x_offset))
                y = node.metadata['module-y']
                if y != None and float(y) > float(sub_y):
                    om.metadata.set.async(node.getPath(), 'module-y',
                                          '%f' % (float(y) + y_offset))
        # destroy the subpatch
        om.synth.destroy_patch(subpatch.getPath())


def main(om):
    om.setEnvironment(omsynth.Environment())
    om.engine.activate.async()
    om.engine.load_plugins.async()
    om.request.all_objects(om.getAddressAsString())

    # wait for all the data to arrive (there should be a cleaner way)
    time.sleep(3)

    patch = om.getEnvironment().getPatch(sys.argv[1], True);
    flatten(om, patch)
    
    os._exit(0)


if len(sys.argv) > 1:
    if sys.argv[1] == "--name":
        print "Flatten patch"
        os._exit(0)
    elif sys.argv[1] == "--shortdesc":
        print "Merge the contents of all subpatches into the parent patch";
        os._exit(0)
    elif sys.argv[1] == "--signature":
        print "%p";
        os._exit(0)
    else:
        omsynth.startClient(main)
    
else:
    print "Which patch do you want to flatten?"
    os._exit(0)
