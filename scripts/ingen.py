#!/usr/bin/env python
# Ingen Python Interface
# Copyright 2012-2015 David Robillard <http://drobilla.net>
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

import os
import rdflib
import re
import socket
import sys

try:
    import StringIO.StringIO as StringIO
except ImportError:
    from io import StringIO as StringIO


class NS:
    atom   = rdflib.Namespace('http://lv2plug.in/ns/ext/atom#')
    ingen  = rdflib.Namespace('http://drobilla.net/ns/ingen#')
    ingerr = rdflib.Namespace('http://drobilla.net/ns/ingen/errors#')
    lv2    = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    patch  = rdflib.Namespace('http://lv2plug.in/ns/ext/patch#')
    rdf    = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
    rsz    = rdflib.Namespace('http://lv2plug.in/ns/ext/resize-port#')
    xsd    = rdflib.Namespace('http://www.w3.org/2001/XMLSchema#')


class Interface:
    'The core Ingen interface'
    def put(self, subject, body):
        pass

    def patch(self, subject, remove, add):
        pass

    def get(self, subject):
        pass

    def set(self, subject, key, value):
        pass

    def connect(self, tail, head):
        pass

    def disconnect(self, tail, head):
        pass

    def delete(self, subject):
        pass


class Error(Exception):
    def __init__(self, msg, cause):
        Exception.__init__(self, '%s; cause: %s' % (msg, cause))


def lv2_path():
    path = os.getenv('LV2_PATH')
    if path:
        return path
    elif sys.platform == 'darwin':
        return os.pathsep.join(['~/Library/Audio/Plug-Ins/LV2',
                                '~/.lv2',
                                '/usr/local/lib/lv2',
                                '/usr/lib/lv2',
                                '/Library/Audio/Plug-Ins/LV2'])
    elif sys.platform == 'haiku':
        return os.pathsep.join(['~/.lv2',
                                '/boot/common/add-ons/lv2'])
    elif sys.platform == 'win32':
        return os.pathsep.join([
            os.path.join(os.getenv('APPDATA'), 'LV2'),
            os.path.join(os.getenv('COMMONPROGRAMFILES'), 'LV2')])
    else:
        return os.pathsep.join(['~/.lv2',
                                '/usr/lib/lv2',
                                '/usr/local/lib/lv2'])


def ingen_bundle_path():
    for d in lv2_path().split(os.pathsep):
        bundle = os.path.abspath(os.path.join(d, 'ingen.lv2'))
        if os.path.exists(bundle):
            return bundle
    return None


class Remote(Interface):
    def __init__(self, uri='unix:///tmp/ingen.sock'):
        self.msg_id      = 1
        self.server_base = uri + '/'
        self.model       = rdflib.Graph()
        self.ns_manager  = rdflib.namespace.NamespaceManager(self.model)
        self.ns_manager.bind('server', self.server_base)
        for (k, v) in NS.__dict__.items():
            if not k.startswith('_'):
                self.ns_manager.bind(k, v)
        if uri.startswith('unix://'):
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.connect(uri[len('unix://'):])
        elif uri.startswith('tcp://'):
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            parsed = re.split('[:/]', uri[len('tcp://'):])
            addr = (parsed[0], int(parsed[1]))
            self.sock.connect(addr)
        else:
            raise Exception('Unsupported server URI `%s' % uri)

        # Parse error description from Ingen bundle for pretty printing
        bundle = ingen_bundle_path()
        if bundle:
            self.model.parse(os.path.join(bundle, 'errors.ttl'), format='n3')

    def __del__(self):
        self.sock.close()

    def _get_prefixes_string(self):
        s = ''
        for k, v in self.ns_manager.namespaces():
            s += '@prefix %s: <%s> .\n' % (k, v)
        return s

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
            for i in update.triples([body, None, None]):
                self.model.add([subject, i[1], i[2]])
        return update

    def uri_to_path(self, uri):
        if uri.startswith(self.server_base):
            return uri[len(self.server_base) - 1:]
        return uri

    def recv(self):
        'Read from socket until a null terminator is received'
        msg = u''
        while True:
            c = self.sock.recv(1, 0).decode('utf-8')
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

    def raise_error(self, code, cause):
        klass = self.model.value(None, NS.ingerr.errorCode, rdflib.Literal(code))
        if not klass:
            raise Error('error %d' % code, cause)

        fmt = self.model.value(klass, NS.ingerr.formatString, None)
        if not fmt:
            raise Error('%s' % klass, cause)

        raise Error(fmt, cause)

    def send(self, msg):
        if type(msg) == list:
            msg = '\n'.join(msg)

        # Send message to server
        self.sock.send(self.msgencode(msg))

        # Receive response and parse into a model
        response_str = self._get_prefixes_string() + self.recv()
        response_model = rdflib.Graph(namespace_manager=self.ns_manager)

        # Because rdflib has embarrassingly broken base URI resolution that
        # just drops path components from the base URI entirely (seriously),
        # unfortunate the real server base URI can not be used here.  Use
        # <ingen:/> instead to at least avoid complete nonsense
        response_model.parse(StringIO(response_str), 'ingen:/', format='n3')

        # Add new prefixes to prepend to future responses because rdflib sucks
        for line in response_str.split('\n'):
            if line.startswith('@prefix'):
                match = re.search('@prefix ([^:]*): <(.*)> *\\.', line)
                if match:
                    name = match.group(1)
                    uri  = match.group(2)
                    self.ns_manager.bind(name, uri)

        # Handle response (though there should be only one)
        blanks        = []
        response_desc = []
        for i in response_model.triples([None, NS.rdf.type, NS.patch.Response]):
            response = i[0]
            body     = response_model.value(response, NS.patch.body, None)

            response_desc += [i]
            blanks        += [response]
            if body != 0:
                self.raise_error(int(body), msg)  # Raise exception on server error

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

    def get(self, subject):
        return self.send(['[]',
                          '	a patch:Get ;',
                          '	patch:subject <%s> .' % subject])

    def put(self, subject, body):
        return self.send(['[]',
                          '	a patch:Put ;',
                          '	patch:subject <%s> ;' % subject,
                          '	patch:body [',
                          '	' + body,
                          '	] .'])

    def patch(self, subject, remove, add):
        return self.send(['[]',
                          '	a patch:Patch ;',
                          '	patch:subject <%s> ;' % subject,
                          '	patch:remove [',
                          remove,
                          '	] ;',
                          '	patch:add [',
                          add,
                          '	] .'])

    def set(self, subject, key, value):
        return self.send(['[]',
                          '	a patch:Set ;',
                          '	patch:subject <%s> ;' % subject,
                          '	patch:property <%s> ;' % key,
                          ' patch:value %s .' % value])

    def connect(self, tail, head):
        subject = os.path.commonprefix([tail, head])
        return self.send(['[]',
                          '	a patch:Put ;',
                          '	patch:subject <%s> ;' % subject,
                          '	patch:body [',
                          '		a ingen:Arc ;',
                          '		ingen:tail <%s> ;' % tail,
                          '		ingen:head <%s> ;' % head,
                          '	] .'])

    def disconnect(self, tail, head):
        return self.send(['[]',
                          '	a patch:Delete ;',
                          '	patch:body [',
                          '		a ingen:Arc ;',
                          '		ingen:tail <%s> ;' % tail,
                          '		ingen:head <%s> ;' % head,
                          '	] .'])

    def delete(self, subject):
        return self.send(['[]',
                          '	a patch:Delete ;',
                          '	patch:subject <%s> .' % subject])
