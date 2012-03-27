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


#ifndef INGEN_ENGINE_PATCHIMPL_HPP
#define INGEN_ENGINE_PATCHIMPL_HPP

#include <cstdlib>
#include <string>

#include "ingen/Patch.hpp"
#include "raul/List.hpp"

#include "CompiledPatch.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"
#include "PortType.hpp"
#include "ThreadManager.hpp"

namespace Ingen {

class Connection;

namespace Server {

class CompiledPatch;
class ConnectionImpl;
class Context;
class Engine;
class ProcessContext;

/** A group of nodes in a graph, possibly polyphonic.
 *
 * Note that this is also a Node, just one which contains Nodes.
 * Therefore infinite subpatching is possible, of polyphonic
 * patches of polyphonic nodes etc. etc.
 *
 * \ingroup engine
 */
class PatchImpl : public NodeImpl, public Patch
{
public:
	PatchImpl(Engine&             engine,
	          const Raul::Symbol& symbol,
	          uint32_t            poly,
	          PatchImpl*          parent,
	          SampleRate          srate,
	          uint32_t            local_poly);

	virtual ~PatchImpl();

	void activate(BufferFactory& bufs);
	void deactivate();

	void process(ProcessContext& context);

	void set_buffer_size(Context&       context,
	                     BufferFactory& bufs,
	                     LV2_URID       type,
	                     uint32_t       size);

	/** Prepare for a new (internal) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_internal_poly.
	 * \return true on success.
	 */
	bool prepare_internal_poly(BufferFactory& bufs, uint32_t poly);

	/** Apply a new (internal) polyphony value.
	 *
	 * Audio thread.
	 *
	 * \param context Process context
	 * \param bufs New set of buffers
	 * \param poly Must be < the most recent value passed to prepare_internal_poly.
	 * \param maid Any objects no longer needed will be pushed to this
	 */
	bool apply_internal_poly(ProcessContext& context,
	                         BufferFactory&  bufs,
	                         Raul::Maid&     maid,
	                         uint32_t        poly);

	// Patch specific stuff not inherited from Node

	typedef Raul::List<NodeImpl*> Nodes;

	void         add_node(Nodes::Node* tn);
	Nodes::Node* remove_node(const Raul::Symbol& symbol);

	Nodes&       nodes()       { return _nodes; }
	Connections& connections() { return _connections; }

	const Nodes&       nodes()       const { return _nodes; }
	const Connections& connections() const { return _connections; }

	uint32_t num_ports() const;

	PortImpl* create_port(BufferFactory&     bufs,
	                      const std::string& name,
	                      PortType           type,
	                      LV2_URID           buffer_type,
	                      uint32_t           buffer_size,
	                      bool               is_output,
	                      bool               polyphonic);

	typedef Raul::List<PortImpl*> Ports;

	void add_input(Ports::Node* port) {
		ThreadManager::assert_thread(THREAD_PRE_PROCESS);
		_inputs.push_back(port);
	}

	void add_output(Ports::Node* port) {
		ThreadManager::assert_thread(THREAD_PRE_PROCESS);
		_outputs.push_back(port);
	}

	Ports::Node* remove_port(const std::string& name);
	void         clear_ports();

	void add_connection(SharedPtr<ConnectionImpl> c);

	SharedPtr<ConnectionImpl> remove_connection(const PortImpl* src_port,
	                                            const PortImpl* dst_port);

	bool has_connection(const PortImpl* src_port, const PortImpl* dst_port) const;

	CompiledPatch* compiled_patch()                  { return _compiled_patch; }
	void           compiled_patch(CompiledPatch* cp) { _compiled_patch = cp; }

	Raul::Array<PortImpl*>* external_ports()                           { return _ports; }
	void                    external_ports(Raul::Array<PortImpl*>* pa) { _ports = pa; }

	CompiledPatch*          compile() const;
	Raul::Array<PortImpl*>* build_ports_array() const;

	/** Whether to run this patch's DSP bits in the audio thread */
	bool enabled() const { return _process; }
	void enable() { _process = true; }
	void disable();

	uint32_t internal_poly() const { return _internal_poly; }

private:
	inline void compile_recursive(NodeImpl* n, CompiledPatch* output) const;
	void process_parallel(ProcessContext& context);
	void process_single(ProcessContext& context);

	Engine&        _engine;
	uint32_t       _internal_poly;
	CompiledPatch* _compiled_patch;  ///< Process thread only
	Connections    _connections;     ///< Pre-process thread only
	Ports          _inputs;          ///< Pre-process thread only
	Ports          _outputs;         ///< Pre-process thread only
	Nodes          _nodes;           ///< Pre-process thread only
	bool           _process;
};

/** Private helper for compile */
inline void
PatchImpl::compile_recursive(NodeImpl* n, CompiledPatch* output) const
{
	if (n == NULL || n->traversed())
		return;

	n->traversed(true);
	assert(output != NULL);

	for (std::list<NodeImpl*>::iterator i = n->providers().begin();
	     i != n->providers().end(); ++i)
		if (!(*i)->traversed())
			compile_recursive(*i, output);

	output->push_back(CompiledNode(n, n->providers().size(), n->dependants()));
}

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PATCHIMPL_HPP
