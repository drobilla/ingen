/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_GRAPHIMPL_HPP
#define INGEN_ENGINE_GRAPHIMPL_HPP

#include <cstdlib>

#include "ingen/ingen.h"

#include "BlockImpl.hpp"
#include "CompiledGraph.hpp"
#include "DuplexPort.hpp"
#include "PluginImpl.hpp"
#include "PortType.hpp"
#include "ThreadManager.hpp"

namespace Ingen {

class Arc;

namespace Server {

class ArcImpl;
class CompiledGraph;
class Engine;
class RunContext;

/** A group of blocks in a graph, possibly polyphonic.
 *
 * Note that this is also a Block, just one which contains Blocks.
 * Therefore infinite subgraphing is possible, of polyphonic
 * graphs of polyphonic blocks etc. etc.
 *
 * \ingroup engine
 */
class GraphImpl : public BlockImpl
{
public:
	GraphImpl(Engine&             engine,
	          const Raul::Symbol& symbol,
	          uint32_t            poly,
	          GraphImpl*          parent,
	          SampleRate          srate,
	          uint32_t            local_poly);

	virtual ~GraphImpl();

	virtual GraphType graph_type() const { return GraphType::GRAPH; }

	BlockImpl* duplicate(Engine&             engine,
	                     const Raul::Symbol& symbol,
	                     GraphImpl*          parent);

	void activate(BufferFactory& bufs);
	void deactivate();

	void pre_process(RunContext& context);
	void process(RunContext& context);
	void run(RunContext& context);

	void set_buffer_size(RunContext&    context,
	                     BufferFactory& bufs,
	                     LV2_URID       type,
	                     uint32_t       size);

	/** Prepare for a new (internal) polyphony value.
	 *
	 * Pre-process thread, poly is actually applied by apply_internal_poly.
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
	bool apply_internal_poly(RunContext&    context,
	                         BufferFactory& bufs,
	                         Raul::Maid&    maid,
	                         uint32_t       poly);

	// Graph specific stuff not inherited from Block

	typedef boost::intrusive::slist<
		BlockImpl, boost::intrusive::constant_time_size<true> > Blocks;

	/** Add a block to this graph.
	 * Pre-process thread only.
	 */
	void add_block(BlockImpl& block);

	/** Remove a block from this graph.
	 * Pre-process thread only.
	 */
	void remove_block(BlockImpl& block);

	Blocks&       blocks()       { return _blocks; }
	const Blocks& blocks() const { return _blocks; }

	uint32_t num_ports_non_rt() const;

	typedef boost::intrusive::slist<
		DuplexPort, boost::intrusive::constant_time_size<true> > PortList;

	void add_input(DuplexPort& port) {
		ThreadManager::assert_thread(THREAD_PRE_PROCESS);
		assert(port.is_input());
		_inputs.push_front(port);
	}

	void add_output(DuplexPort& port) {
		ThreadManager::assert_thread(THREAD_PRE_PROCESS);
		assert(port.is_output());
		_outputs.push_front(port);
	}

	/** Remove port from ports list used in pre-processing thread.
	 *
	 * Port is not removed from ports array for process thread (which could be
	 * simultaneously running).
	 *
	 * Pre-processing thread or situations that won't cause races with it only.
	 */
	void remove_port(DuplexPort& port);

	/** Remove all ports from ports list used in pre-processing thread.
	 *
	 * Ports are not removed from ports array for process thread (which could be
	 * simultaneously running).  Returned is a (inputs, outputs) pair.
	 *
	 * Pre-processing thread or situations that won't cause races with it only.
	 */
	void clear_ports();

	/** Add an arc to this graph.
	 * Pre-processing thread only.
	 */
	void add_arc(SPtr<ArcImpl> arc);

	/** Remove an arc from this graph.
	 * Pre-processing thread only.
	 */
	SPtr<ArcImpl> remove_arc(const PortImpl* tail, const PortImpl* head);

	bool has_arc(const PortImpl* tail, const PortImpl* head) const;

	/** Set a new compiled graph to run, and return the old one. */
	void set_compiled_graph(MPtr<CompiledGraph>&& cg);

	const MPtr<Ports>& external_ports() { return _ports; }

	void set_external_ports(MPtr<Ports>&& pa) { _ports = std::move(pa); }

	MPtr<Ports> build_ports_array(Raul::Maid& maid);

	/** Whether to run this graph's DSP bits in the audio thread */
	bool enabled() const { return _process; }
	void enable() { _process = true; }
	void disable(RunContext& context);

	uint32_t internal_poly()         const { return _poly_pre; }
	uint32_t internal_poly_process() const { return _poly_process; }

	Engine& engine() { return _engine; }

private:
	Engine&             _engine;
	uint32_t            _poly_pre;        ///< Pre-process thread only
	uint32_t            _poly_process;    ///< Process thread only
	MPtr<CompiledGraph> _compiled_graph;  ///< Process thread only
	PortList            _inputs;          ///< Pre-process thread only
	PortList            _outputs;         ///< Pre-process thread only
	Blocks              _blocks;          ///< Pre-process thread only
	bool                _process;         ///< True iff graph is enabled
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_GRAPHIMPL_HPP
