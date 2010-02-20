/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_NODEBASE_HPP
#define INGEN_ENGINE_NODEBASE_HPP

#include "types.hpp"
#include <string>
#include <cstdlib>
#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/AtomicInt.hpp"
#include "raul/IntrusivePtr.hpp"
#include "raul/Semaphore.hpp"
#include "contexts.lv2/contexts.h"
#include "interface/Port.hpp"
#include "interface/PortType.hpp"
#include "NodeImpl.hpp"

namespace Ingen {

class Context;
class EngineStore;
class PatchImpl;
class PluginImpl;
class BufferFactory;

namespace Shared { class ClientInterface; }


/** Common implementation stuff for Node.
 *
 * \ingroup engine
 */
class NodeBase : public NodeImpl
{
public:
	NodeBase(PluginImpl*         plugin,
	         const Raul::Symbol& symbol,
	         bool                poly,
	         PatchImpl*          parent,
	         SampleRate          rate);

	virtual ~NodeBase();

	virtual void activate(BufferFactory& bufs);
	virtual void deactivate();
	bool activated() { return _activated; }

	/** Flag node as polyphonic.
	 *
	 * Note this will not actually allocate voices etc., prepare_poly
	 * and apply_poly must be called after this function to truly make
	 * a node polyphonic.
	 */
	virtual void set_polyphonic(bool p) { _polyphonic = p; }

	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);

	virtual void     reset_input_ready();
	virtual bool     process_lock();
	virtual void     process_unlock();
	virtual void     wait_for_input(size_t num_providers);
	virtual unsigned n_inputs_ready() const { return _n_inputs_ready.get(); }

	virtual void learn() {}

	virtual void message_run(MessageContext& context) {}

	virtual void set_port_valid(uint32_t port_index);

	virtual void* valid_ports();
	virtual void  reset_valid_ports();

	virtual void pre_process(Context& context);
	virtual void process(ProcessContext& context) = 0;
	virtual void post_process(Context& context);

	virtual void set_port_buffer(uint32_t voice, uint32_t port_num, IntrusivePtr<Buffer> buf);

	virtual void set_buffer_size(Context& context, BufferFactory& bufs,
			Shared::PortType type, size_t size);

	SampleRate sample_rate() const { return _srate; }
	uint32_t   num_ports()   const { return _ports ? _ports->size() : 0; }
	uint32_t   polyphony()   const { return _polyphony; }
	bool       traversed()   const { return _traversed; }
	void       traversed(bool b)   { _traversed = b; }

	virtual Shared::Port* port(uint32_t index) const;
	virtual PortImpl*     port_impl(uint32_t index) const { return (*_ports)[index]; }

	/* These are NOT to be used in the audio thread!
	 * The providers and dependants in CompiledNode are for that
	 */

	Raul::List<NodeImpl*>* providers()                         { return _providers; }
	void                   providers(Raul::List<NodeImpl*>* l) { _providers = l; }

	Raul::List<NodeImpl*>* dependants()                         { return _dependants; }
	void                   dependants(Raul::List<NodeImpl*>* l) { _dependants = l; }

	virtual const Shared::Plugin* plugin() const;
	virtual PluginImpl*           plugin_impl() const    { return _plugin; }
	virtual void                  plugin(PluginImpl* pi) { _plugin = pi; }

	/** A node's parent is always a patch, so static cast should be safe */
	inline PatchImpl* parent_patch() const { return (PatchImpl*)_parent; }

protected:
	virtual void signal_input_ready();

	PluginImpl* _plugin;

	bool       _polyphonic;
	uint32_t   _polyphony;
	SampleRate _srate;

	void* _valid_ports; ///< Valid port flags for message context

	Raul::Semaphore         _input_ready;    ///< Parallelism: input ready signal
	Raul::AtomicInt         _process_lock;   ///< Parallelism: Waiting on inputs 'lock'
	Raul::AtomicInt         _n_inputs_ready; ///< Parallelism: # input ready signals this cycle
	Raul::Array<PortImpl*>* _ports;          ///< Access in audio thread only
	Raul::List<NodeImpl*>*  _providers;      ///< Nodes connected to this one's input ports
	Raul::List<NodeImpl*>*  _dependants;     ///< Nodes this one's output ports are connected to

	bool _activated;
	bool _traversed; ///< Flag for process order algorithm
};


} // namespace Ingen

#endif // INGEN_ENGINE_NODEBASE_HPP
