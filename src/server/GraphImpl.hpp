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

#include "BlockImpl.hpp"
#include "DuplexPort.hpp"
#include "ThreadManager.hpp"
#include "types.hpp"

#include "ingen/Node.hpp"
#include "lv2/urid/urid.h"
#include "raul/Maid.hpp"

#include <boost/intrusive/slist.hpp>

#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>

namespace raul {
class Symbol;
} // namespace raul

namespace boost {
namespace intrusive {

template <bool Enabled> struct constant_time_size;

} // namespace intrusive
} // namespace boost

namespace ingen {
namespace server {

class ArcImpl;
class BufferFactory;
class CompiledGraph;
class Engine;
class PortImpl;
class RunContext;

/** A group of blocks in a graph, possibly polyphonic.
 *
 * Note that this is also a Block, just one which contains Blocks.
 * Therefore infinite subgraphing is possible, of polyphonic
 * graphs of polyphonic blocks etc. etc.
 *
 * \ingroup engine
 */
class GraphImpl final : public BlockImpl
{
public:
	GraphImpl(Engine&             engine,
	          const raul::Symbol& symbol,
	          uint32_t            poly,
	          GraphImpl*          parent,
	          SampleRate          srate,
	          uint32_t            internal_poly);

	~GraphImpl() override;

	GraphType graph_type() const override { return GraphType::GRAPH; }

	BlockImpl* duplicate(Engine&             engine,
	                     const raul::Symbol& symbol,
	                     GraphImpl*          parent) override;

	void activate(BufferFactory& bufs) override;
	void deactivate() override;

	void pre_process(RunContext& ctx) override;
	void process(RunContext& ctx) override;
	void run(RunContext& ctx) override;

	void set_buffer_size(RunContext&    ctx,
	                     BufferFactory& bufs,
	                     LV2_URID       type,
	                     uint32_t       size) override;

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
	 * \param ctx  Process context
	 * \param bufs New set of buffers
	 * \param poly Must be < the most recent value passed to prepare_internal_poly.
	 * \param maid Any objects no longer needed will be pushed to this
	 */
	bool apply_internal_poly(RunContext&    ctx,
	                         BufferFactory& bufs,
	                         raul::Maid&    maid,
	                         uint32_t       poly);

	// Graph specific stuff not inherited from Block

	using Blocks =
	        boost::intrusive::slist<BlockImpl,
	                                boost::intrusive::constant_time_size<true>>;

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
	bool     has_port_with_index(uint32_t index) const;

	using PortList =
	        boost::intrusive::slist<DuplexPort,
	                                boost::intrusive::constant_time_size<true>>;

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
	void add_arc(const std::shared_ptr<ArcImpl>& a);

	/** Remove an arc from this graph.
	 * Pre-processing thread only.
	 */
	std::shared_ptr<ArcImpl>
	remove_arc(const PortImpl* tail, const PortImpl* dst_port);

	bool has_arc(const PortImpl* tail, const PortImpl* dst_port) const;

	/** Set a new compiled graph to run, and return the old one. */
	void set_compiled_graph(raul::managed_ptr<CompiledGraph>&& cg);

	const raul::managed_ptr<Ports>& external_ports() { return _ports; }

	void set_external_ports(raul::managed_ptr<Ports>&& pa) { _ports = std::move(pa); }

	raul::managed_ptr<Ports> build_ports_array(raul::Maid& maid);

	/** Whether to run this graph's DSP bits in the audio thread */
	bool enabled() const { return _process; }
	void enable() { _process = true; }
	void disable(RunContext& ctx);

	uint32_t internal_poly()         const { return _poly_pre; }
	uint32_t internal_poly_process() const { return _poly_process; }

	Engine& engine() { return _engine; }

private:
	Engine&                          _engine;
	uint32_t                         _poly_pre;     ///< Pre-process thread only
	uint32_t                         _poly_process; ///< Process thread only
	raul::managed_ptr<CompiledGraph> _compiled_graph; ///< Process thread only
	PortList                         _inputs;  ///< Pre-process thread only
	PortList                         _outputs; ///< Pre-process thread only
	Blocks                           _blocks;  ///< Pre-process thread only
	bool                             _process; ///< True iff graph is enabled
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_GRAPHIMPL_HPP
