/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NODEBASE_H
#define NODEBASE_H

#include "types.h"
#include <string>
#include <cstdlib>
#include "Node.h"

using std::string;

namespace Ingen {

class Plugin;
class Patch;
class ObjectStore;

namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** Common implementation stuff for Node.
 *
 * Pretty much just attributes and getters/setters are here.
 *
 * \ingroup engine
 */
class NodeBase : public Node
{
public:
	NodeBase(const Plugin* plugin, const string& name, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size);

	virtual ~NodeBase();

	virtual void activate();
	virtual void deactivate();
	bool activated() { return _activated; }

	virtual void process(SampleCount nframes, FrameTime start, FrameTime end);
		
	virtual void set_port_buffer(size_t voice, size_t port_num, void* buf) {}
	
	virtual void set_buffer_size(size_t size);
	
	void add_to_store(ObjectStore* store);
	void remove_from_store();
	
	//void send_creation_messages(ClientInterface* client) const;
	
	SampleRate sample_rate() const { return _srate; }
	size_t     buffer_size() const { return _buffer_size; }
	size_t     num_ports()   const { return _ports ? _ports->size() : 0; }
	size_t     poly()        const { return _poly; }
	bool       traversed()   const { return _traversed; }
	void       traversed(bool b)   { _traversed = b; }
	
	const Array<Port*>& ports() const { return *_ports; }
	
	virtual List<Node*>* providers()               { return _providers; }
	virtual void         providers(List<Node*>* l) { _providers = l; }
	
	virtual List<Node*>* dependants()               { return _dependants; }
	virtual void         dependants(List<Node*>* l) { _dependants = l; }
	
	virtual const Plugin* plugin() const { return _plugin; }
	
	virtual void set_path(const Path& new_path);
	
	/** A node's parent is always a patch, so static cast should be safe */
	Patch* parent_patch() const { return (Patch*)_parent; }
	
protected:	
	const Plugin* _plugin;

	size_t      _poly;

	SampleRate  _srate;
	size_t      _buffer_size;
	bool        _activated;

	Array<Port*>* _ports;     ///< Access in audio thread only

	bool         _traversed;  ///< Flag for process order algorithm
	List<Node*>* _providers;  ///< Nodes connected to this one's input ports
	List<Node*>* _dependants; ///< Nodes this one's output ports are connected to
};


} // namespace Ingen

#endif // NODEBASE_H
