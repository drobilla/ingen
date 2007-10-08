/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "types.hpp"
#include <string>
#include <cstdlib>
#include <raul/Semaphore.hpp>
#include <raul/AtomicInt.hpp>
#include "interface/Port.hpp"
#include "NodeImpl.hpp"

using std::string;

namespace Ingen {

class PluginImpl;
class PatchImpl;
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
class NodeBase : public NodeImpl
{
public:
	NodeBase(const PluginImpl* plugin,
	         const string&     name,
	         bool              poly,
	         PatchImpl*        parent,
	         SampleRate        rate,
	         size_t            buffer_size);

	virtual ~NodeBase();

	virtual void activate();
	virtual void deactivate();
	bool activated() { return _activated; }
	
	virtual bool prepare_poly(uint32_t poly);
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);
	
	virtual void     reset_input_ready();
	virtual bool     process_lock();
	virtual void     process_unlock();
	virtual void     wait_for_input(size_t num_providers);
	virtual unsigned n_inputs_ready() const { return _n_inputs_ready.get(); }

	virtual void pre_process(ProcessContext& context);
	virtual void process(ProcessContext& context) = 0;
	virtual void post_process(ProcessContext& context);
		
	virtual void set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf) {}
	
	virtual void set_buffer_size(size_t size);
	
	SampleRate sample_rate() const { return _srate; }
	size_t     buffer_size() const { return _buffer_size; }
	uint32_t   num_ports()   const { return _ports ? _ports->size() : 0; }
	uint32_t   polyphony()   const { return _polyphony; }
	bool       traversed()   const { return _traversed; }
	void       traversed(bool b)   { _traversed = b; }
	
	virtual Port*     port(uint32_t index) const;
	virtual PortImpl* port_impl(uint32_t index) const { return (*_ports)[index]; }
	
	/* These are NOT to be used in the audio thread!
	 * The providers and dependants in CompiledNode are for that
	 */

	Raul::List<NodeImpl*>* providers()                         { return _providers; }
	void                   providers(Raul::List<NodeImpl*>* l) { _providers = l; }
	
	Raul::List<NodeImpl*>* dependants()                         { return _dependants; }
	void                   dependants(Raul::List<NodeImpl*>* l) { _dependants = l; }
	
	virtual const Plugin*     plugin()      const;
	virtual const PluginImpl* plugin_impl() const          { return _plugin; }
	virtual void              plugin(const PluginImpl* pi) { _plugin = pi; }
	
	/** A node's parent is always a patch, so static cast should be safe */
	inline PatchImpl* parent_patch() const { return (PatchImpl*)_parent; }
	
protected:
	virtual void signal_input_ready();
	
	const PluginImpl* _plugin;

	uint32_t   _polyphony;
	SampleRate _srate;
	size_t     _buffer_size;
	bool       _activated;
	
	bool                    _traversed;      ///< Flag for process order algorithm
	Raul::Semaphore         _input_ready;    ///< Parallelism: input ready signal
	Raul::AtomicInt         _process_lock;   ///< Parallelism: Waiting on inputs 'lock'
	Raul::AtomicInt         _n_inputs_ready; ///< Parallelism: # input ready signals this cycle
	Raul::Array<PortImpl*>* _ports;          ///< Access in audio thread only
	Raul::List<NodeImpl*>*  _providers;      ///< Nodes connected to this one's input ports
	Raul::List<NodeImpl*>*  _dependants;     ///< Nodes this one's output ports are connected to
};


} // namespace Ingen

#endif // NODEBASE_H
