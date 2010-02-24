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

#ifndef INGEN_ENGINE_NODEIMPL_HPP
#define INGEN_ENGINE_NODEIMPL_HPP

#include <string>
#include "raul/Array.hpp"
#include "raul/AtomicInt.hpp"
#include "raul/IntrusivePtr.hpp"
#include "raul/Semaphore.hpp"
#include "interface/Node.hpp"
#include "GraphObjectImpl.hpp"
#include "types.hpp"

namespace Raul { template <typename T> class List; class Maid; }

namespace Ingen {

namespace Shared { class Plugin; class Port; }

class Buffer;
class BufferFactory;
class Context;
class MessageContext;
class PatchImpl;
class PluginImpl;
class PortImpl;
class ProcessContext;


/** A Node (or "module") in a Patch (which is also a Node).
 *
 * A Node is a unit with input/output ports, a process() method, and some other
 * things.
 *
 * \ingroup engine
 */
class NodeImpl : public GraphObjectImpl, virtual public Ingen::Shared::Node
{
public:
	NodeImpl(PluginImpl*         plugin,
	         const Raul::Symbol& symbol,
	         bool                poly,
	         PatchImpl*          parent,
	         SampleRate          rate);

	virtual ~NodeImpl();

	/** Activate this Node.
	 *
	 * This function must be called in a non-realtime thread before it is
	 * inserted in to a patch.  Any non-realtime actions that need to be
	 * done before the Node is ready for use should be done here.
	 */
	virtual void activate(BufferFactory& bufs);

	/** Deactivate this Node.
	 *
	 * This function must be called in a non-realtime thread after the
	 * node has been removed from its patch (i.e. processing is finished).
	 */
	virtual void deactivate();

	/** Return true iff this node is activated */
	bool activated() { return _activated; }

	/** Parallelism: Reset flags for start of a new cycle. */
	virtual void reset_input_ready();

	/** Parallelism: Claim this node (to wait on its input).
	 * Only one thread will ever take this lock on a particular Node.
	 * \return true if lock was aquired, false otherwise
	 */
	virtual bool process_lock();

	/** Parallelism: Unclaim this node (let someone else wait on its input).
	 * Only a thread which successfully called process_lock may call this.
	 */
	virtual void process_unlock();

	/** Parallelism: Wait for signal that input is ready.
	 * Only a thread which successfully called process_lock may call this.
	 */
	virtual void wait_for_input(size_t num_providers);

	/** Parallelism: Signal that input is ready.  Realtime safe.
	 * Calling this will wake up the thread which blocked on wait_for_input
	 * if there is one, and otherwise cause it to return true the next call.
	 * \return true if lock was aquired and input is ready, false otherwise
	 */
	virtual void signal_input_ready();

	/** Parallelism: Return the number of providers that have signalled. */
	virtual unsigned n_inputs_ready() const { return _n_inputs_ready.get(); }

	/** Learn the next incoming MIDI event (for internals) */
	virtual void learn() {}

	/** Run the node for one instant in the message thread. */
	virtual void message_run(MessageContext& context) {}

	/** Flag a port as valid (for message context) */
	virtual void set_port_valid(uint32_t index);

	/** Return a bit vector of which ports are valid */
	virtual void* valid_ports();

	/** Clear all bits in valid_ports() */
	virtual void reset_valid_ports();

	/** Do whatever needs doing in the process thread before process() is called */
	virtual void pre_process(Context& context);

	/** Run the node for @a nframes input/output.
	 *
	 * @a start and @a end are transport times: end is not redundant in the case
	 * of varispeed, where end-start != nframes.
	 */
	virtual void process(ProcessContext& context) = 0;

	/** Do whatever needs doing in the process thread after process() is called */
	virtual void post_process(Context& context);

	/** Set the buffer of a port to a given buffer (e.g. connect plugin to buffer) */
	virtual void set_port_buffer(
			uint32_t             voice,
			uint32_t             port_num,
			IntrusivePtr<Buffer> buf,
			SampleCount          offset);

	virtual Shared::Port* port(uint32_t index)      const;
	virtual PortImpl*     port_impl(uint32_t index) const { return (*_ports)[index]; }

	/** Nodes that are connected to this Node's inputs.
	 * (This Node depends on them)
	 */
	Raul::List<NodeImpl*>* providers()                         { return _providers; }
	void                   providers(Raul::List<NodeImpl*>* l) { _providers = l; }

	/** Nodes are are connected to this Node's outputs.
	 * (They depend on this Node)
	 */
	Raul::List<NodeImpl*>* dependants()                         { return _dependants; }
	void                   dependants(Raul::List<NodeImpl*>* l) { _dependants = l; }

	/** Flag node as polyphonic.
	 *
	 * Note this will not actually allocate voices etc., prepare_poly
	 * and apply_poly must be called after this function to truly make
	 * a node polyphonic.
	 */
	virtual void set_polyphonic(bool p) { _polyphonic = p; }

	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);

	/** Information about the Plugin this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual PluginImpl* plugin_impl() const { return _plugin; }

	/** Information about the Plugin this Node is an instance of.
	 * Not the best name - not all nodes come from plugins (ie Patch)
	 */
	virtual const Shared::Plugin* plugin() const;

	virtual void plugin(PluginImpl* pi) { _plugin = pi; }

	virtual void set_buffer_size(Context& context, BufferFactory& bufs,
			Shared::PortType type, size_t size);

	/** The Patch this Node belongs to. */
	inline PatchImpl* parent_patch() const { return (PatchImpl*)_parent; }

	SampleRate         sample_rate() const { return _srate; }
	virtual uint32_t   num_ports()   const { return _ports ? _ports->size() : 0; }
	virtual uint32_t   polyphony()   const { return _polyphony; }

	/** Used by the process order finding algorithm (ie during connections) */
	bool               traversed()   const { return _traversed; }
	void               traversed(bool b)   { _traversed = b; }

protected:
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

#endif // INGEN_ENGINE_NODEIMPL_HPP
