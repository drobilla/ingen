#!/usr/bin/env python
# Ingen Python Interface
# Copyright 2012 David Robillard <http://drobilla.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import StringIO
import os
import rdflib
import socket
import sys

class NS:
    ingen = rdflib.Namespace('http://drobilla.net/ns/ingen#')
    lv2   = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    patch = rdflib.Namespace('http://lv2plug.in/ns/ext/patch#')
    rdf   = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
    xsd   = rdflib.Namespace('http://www.w3.org/2001/XMLSchema#')

class Interface:
    'The core Ingen interface'
    def put(self, path, body):
        pass

    def set(self, path, body):
        pass

    def connect(self, tail, head):
        pass

    def disconnect(self, tail, head):
        pass

    def delete(self, path):
        pass

class Error(Exception):
    def __init__(self, code, cause):
        Exception.__init__(self, 'error %d, cause: %s' % (code, cause))

class Remote(Interface):
    def __init__(self, uri='unix:///tmp/ingen.sock'):
        self.msg_id      = 1
        self.server_base = uri + '/'
        self.model       = rdflib.Graph()
        self.ns_manager  = rdflib.syntax.NamespaceManager.NamespaceManager(self.model)
        self.ns_manager.bind('server', self.server_base)
        for (k, v) in NS.__dict__.items():
            self.ns_manager.bind(k, v)
        if uri.startswith('unix://'):
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.connect(uri[len('unix://'):])
        elif uri.startswith('tcp://'):
            self.sock = Socket(socket.AF_INET, socket.SOCK_STREAM)
            parsed = re.split('[:/]', uri[len('tcp://'):])
            addr = (parsed[0], int(parsed[1]))
            self.sock.connect(addr)
        else:
            raise Exception('Unsupported server URI `%s' % uri)

    def __del__(self):
        self.sock.close()

    def msgencode(self, msg):
        if sys.version_info[0] == 3:
            return bytes(msg, 'utf-8')
        else:
            return msg

    def update_model(self, update):
        for i in update.triples([None, NS.rdf.type, NS.patch.Put]):
            put     = i[0]
            subject = update.value(put, NS.patch.subject, None)
            body    = update.value(put, NS.patch.body, None)
            desc    = {}
            for i in update.triples([body, None, None]):
                self.model.add([subject, i[1], i[2]])
        return update

    def uri_to_path(self, uri):
        if uri.startswith(self.server_base):
            return uri[len(self.server_base):]
        return uri

    def recv(self):
        'Read from socket until a NULL terminator is received'
        msg = ''
        while True:
            c = self.sock.recv(1, 0)
            if not c or ord(c[0]) == 0:  # End of transmission
                break
            else:
                msg += c[0]
        return msg

    def blank_closure(self, graph, node):
        def blank_walk(node, g):
            for i in g.triples([node, None, None]):
                if type(i[2]) == rdflib.BNode and i[2] != node:
                    yield i[2]
                    blank_walk(i[2], g)

        closure = [node]
        for b in graph.transitiveClosure(blank_walk, node):
            closure += [b]

        return closure
        
    def send(self, msg):
        # Send message to server
        payload = msg
        if sys.version_info[0] == 3:
            payload = bytes(msg, 'utf-8')
        self.sock.send(self.msgencode(msg))
        
        # Receive response and parse into a model
        response_str = self.recv()
        response_model = rdflib.Graph()
        response_model.namespace_manager = self.ns_manager
        response_model.parse(StringIO.StringIO(response_str), self.server_base, format='n3')

        # Handle response (though there should be only one)
        blanks        = []
        response_desc = []
        for i in response_model.triples([None, NS.rdf.type, NS.patch.Response]):
            response = i[0]
            subject  = response_model.value(response, NS.patch.subject, None)
            body     = response_model.value(response, NS.patch.body, None)

            response_desc += [i]
            blanks        += [response]
            if body != 0:
                raise Error(int(body), msg)  # Raise exception on server error

        # Find the blank node closure of all responses
        blank_closure = []
        for b in blanks:
            blank_closure += self.blank_closure(response_model, b)

        # Remove response descriptions from model
        for b in blank_closure:
            for t in response_model.triples([b, None, None]):
                response_model.remove(t)
            
        # Remove triples describing responses from response model
        for i in response_desc:
            response_model.remove(i)

        # Update model with remaining information, e.g. patch:Put updates
        return self.update_model(response_model)

    def get(self, path):
        return self.send('''
[]
 	a patch:Get ;
 	patch:subject <ingen:root%s> .
''' % path)
        
    def put(self, path, body):
        return self.send('''
[]
 	a patch:Put ;
 	patch:subject <ingen:root%s> ;
 	patch:body [
%s
] .
''' % (path, body))

    def set(self, path, body):
        return self.send('''
[]
	a patch:Set ;
	patch:subject <ingen:root%s> ;
	patch:body [
%s
	] .
''' % (path, body))

    def connect(self, tail, head):
        return self.send('''
[]
	a patch:Put ;
	patch:subject <ingen:root%s> ;
	patch:body [
		a ingen:Edge ;
		ingen:tail <%s> ;
		ingen:head <%s> ;
	] .
''' % (os.path.commonprefix([tail, head]), tail, head))

    def disconnect(self, tail, head):
        return self.send('''
[]
	a patch:Delete ;
	patch:body [
		a ingen:Edge ;
		ingen:tail <%s> ;
		ingen:head <%s> ;
	] .
''' % (tail, head))

    def delete(self, path):
        return self.send('''
[]
	a patch:Delete ;
	patch:subject <ingen:root%s> .
''' % path)

