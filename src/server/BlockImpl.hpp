/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_NODEIMPL_HPP
#define INGEN_ENGINE_NODEIMPL_HPP

#include <list>

#include <boost/intrusive/slist.hpp>

#include "raul/Array.hpp"
#include "raul/AtomicInt.hpp"

#include "BufferRef.hpp"
#include "Context.hpp"
#include "GraphObjectImpl.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Raul {
class Maid;
}

namespace Ingen {

class Plugin;

namespace Server {

class Buffer;
class BufferFactory;
class Context;
class GraphImpl;
class PluginImpl;
class PortImpl;
class ProcessContext;

/** A Block in a Graph (which is also a Block).
 *
 * This is what is often called a "Module" in modular synthesizers.  A Block is
 * a unit with input/output ports, a process() method, and some other things.
 *
 * \ingroup engine
 */
class BlockImpl : public GraphObjectImpl
                , public boost::intrusive::slist_base_hook<>  // In GraphImpl
{
public:
	BlockImpl(PluginImpl*         plugin,
	          const Raul::Symbol& symbol,
	          bool                poly,
	          GraphImpl*          parent,
	          SampleRate          rate);

	virtual ~BlockImpl();

	virtual GraphType graph_type() const { return BLOCK; }

	/** Activate this Block.
	 *
	 * This function must be called in a non-realtime thread before it is
	 * inserted in to a graph.  Any non-realtime actions that need to be
	 * done before the Block is ready for use should be done here.
	 */
	virtual void activate(BufferFactory& bufs);

	/** Deactivate this Block.
	 *
	 * This function must be called in a non-realtime thread after the
	 * block has been removed from its graph (i.e. processing is finished).
	 */
	virtual void deactivate();

	/** Return true iff this block is activated */
	bool activated() { return _activated; }

	/** Learn the next incoming MIDI event (for internals) */
	virtual void learn() {}

	/** Do whatever needs doing in the process thread before process() is called */
	virtual void pre_process(ProcessContext& context);

	/** Run the block for @a nframes input/output.
	 *
	 * @a start and @a end are transport times: end is not redundant in the case
	 * of varispeed, where end-start != nframes.
	 */
	virtual void process(ProcessContext& context) = 0;

	/** Do whatever needs doing in the process thread after process() is called */
	virtual void post_process(ProcessContext& context);

	/** Set the buffer of a port to a given buffer (e.g. connect plugin to buffer) */
	virtual void set_port_buffer(uint32_t  voice,
	                             uint32_t  port_num,
	                             BufferRef buf);

	virtual GraphObject* port(uint32_t index)      const;
	virtual PortImpl*    port_impl(uint32_t index) const { return (*_ports)[index]; }

	/** Blocks that are connected to this Block's inputs. */
	std::list<BlockImpl*>& providers() { return _providers; }

	/** Blocks are are connected to this Block's outputs. */
	std::list<BlockImpl*>& dependants() { return _dependants; }

	/** Flag block as polyphonic.
	 *
	 * Note this will not actually allocate voices etc., prepare_poly
	 * and apply_poly must be called after this function to truly make
	 * a block polyphonic.
	 */
	virtual void set_polyphonic(bool p) { _polyphonic = p; }

	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	virtual bool apply_poly(
		ProcessContext& context, Raul::Maid& maid, uint32_t poly);

	/** Information about the Plugin this Block is an instance of.
	 * Not the best name - not all blocks come from plugins (e.g. Graph)
	 */
	virtual PluginImpl* plugin_impl() const { return _plugin; }

	/** Information about the Plugin this Block is an instance of.
	 * Not the best name - not all blocks come from plugins (ie Graph)
	 */
	virtual const Plugin* plugin() const;

	virtual void plugin(PluginImpl* pi) { _plugin = pi; }

	virtual void set_buffer_size(Context&       context,
	                             BufferFactory& bufs,
	                             LV2_URID       type,
	                             uint32_t       size);

	/** The Graph this Block belongs to. */
	inline GraphImpl* parent_graph() const { return (GraphImpl*)_parent; }

	Context::ID      context()   const { return _context; }
	uint32_t         num_ports() const { return _ports ? _ports->size() : 0; }
	virtual uint32_t polyphony() const { return _polyphony; }

	/** Used by the process order finding algorithm (ie during connections) */
	bool traversed() const { return _traversed; }
	void traversed(bool b) { _traversed = b; }

protected:
	PluginImpl*             _plugin;
	Raul::Array<PortImpl*>* _ports; ///< Access in audio thread only
	Context::ID             _context; ///< Context this block runs in
	uint32_t                _polyphony;
	std::list<BlockImpl*>   _providers; ///< Blocks connected to this one's input ports
	std::list<BlockImpl*>   _dependants; ///< Blocks this one's output ports are connected to
	bool                    _polyphonic;
	bool                    _activated;
	bool                    _traversed; ///< Flag for process order algorithm
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BLOCKIMPL_HPP
