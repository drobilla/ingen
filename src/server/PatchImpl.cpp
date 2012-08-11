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

#include <cassert>
#include <string>

#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "raul/log.hpp"

#include "EdgeImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PatchPlugin.hpp"
#include "PortImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

PatchImpl::PatchImpl(Engine&             engine,
                     const Raul::Symbol& symbol,
                     uint32_t            poly,
                     PatchImpl*          parent,
                     SampleRate          srate,
                     uint32_t            internal_poly)
	: NodeImpl(new PatchPlugin(engine.world()->uris(),
	                           engine.world()->uris().ingen_Patch.c_str(),
	                           "patch", "Ingen Patch"),
	           symbol, poly, parent, srate)
	, _engine(engine)
	, _poly_pre(internal_poly)
	, _poly_process(internal_poly)
	, _compiled_patch(NULL)
	, _process(false)
{
	assert(internal_poly >= 1);
}

PatchImpl::~PatchImpl()
{
	assert(!_activated);

	delete _compiled_patch;
	delete _plugin;
}

void
PatchImpl::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);

	for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->activate(bufs);

	assert(_activated);
}

void
PatchImpl::deactivate()
{
	if (_activated) {
		NodeImpl::deactivate();

		for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!_activated);
}

void
PatchImpl::disable(ProcessContext& context)
{
	_process = false;
	for (Ports::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
		(*i)->clear_buffers();
	}
}

bool
PatchImpl::prepare_internal_poly(BufferFactory& bufs, uint32_t poly)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// TODO: Subpatch dynamic polyphony (i.e. changing port polyphony)

	for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->prepare_poly(bufs, poly);

	_poly_pre = poly;
	return true;
}

bool
PatchImpl::apply_internal_poly(ProcessContext& context,
                               BufferFactory&  bufs,
                               Raul::Maid&     maid,
                               uint32_t        poly)
{
	// TODO: Subpatch dynamic polyphony (i.e. changing port polyphony)

	for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->apply_poly(context, maid, poly);

	for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		for (uint32_t j = 0; j < (*i)->num_ports(); ++j) {
			PortImpl* const port = (*i)->port_impl(j);
			if (port->is_input() && dynamic_cast<InputPort*>(port)->direct_connect())
				port->setup_buffers(context, bufs, port->poly());
			port->connect_buffers();
		}
	}

	const bool polyphonic = parent_patch() && (poly == parent_patch()->internal_poly_process());
	for (Ports::iterator i = _outputs.begin(); i != _outputs.end(); ++i)
		(*i)->setup_buffers(context, bufs, polyphonic ? poly : 1);

	_poly_process = poly;
	return true;
}

/** Run the patch for the specified number of frames.
 *
 * Calls all Nodes in (roughly, if parallel) the order _compiled_patch specifies.
 */
void
PatchImpl::process(ProcessContext& context)
{
	if (!_process)
		return;

	NodeImpl::pre_process(context);

	if (_compiled_patch && _compiled_patch->size() > 0) {
		// Run all nodes
		for (size_t i = 0; i < _compiled_patch->size(); ++i) {
			(*_compiled_patch)[i].node()->process(context);
		}

		// Queue any cross-context edges
		for (CompiledPatch::QueuedEdges::iterator i = _compiled_patch->queued_edges.begin();
		     i != _compiled_patch->queued_edges.end(); ++i) {
			(*i)->queue(context);
		}
	}

	NodeImpl::post_process(context);
}

void
PatchImpl::set_buffer_size(Context&       context,
                           BufferFactory& bufs,
                           LV2_URID       type,
                           uint32_t       size)
{
	NodeImpl::set_buffer_size(context, bufs, type, size);

	for (size_t i = 0; i < _compiled_patch->size(); ++i)
		(*_compiled_patch)[i].node()->set_buffer_size(context, bufs, type, size);
}

// Patch specific stuff

/** Add a node.
 * Preprocessing thread only.
 */
void
PatchImpl::add_node(Nodes::Node* ln)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);

	_nodes.push_back(ln);
}

/** Remove a node.
 * Preprocessing thread only.
 */
PatchImpl::Nodes::Node*
PatchImpl::remove_node(const Raul::Symbol& symbol)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	for (Nodes::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		if ((*i)->symbol() == symbol)
			return _nodes.erase(i);

	return NULL;
}

void
PatchImpl::add_edge(SharedPtr<EdgeImpl> c)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	_edges.insert(make_pair(make_pair(c->tail(), c->head()), c));
}

/** Remove a edge.
 * Preprocessing thread only.
 */
SharedPtr<EdgeImpl>
PatchImpl::remove_edge(const PortImpl* tail, const PortImpl* dst_port)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Edges::iterator i = _edges.find(make_pair(tail, dst_port));
	if (i != _edges.end()) {
		SharedPtr<EdgeImpl> c = PtrCast<EdgeImpl>(i->second);
		_edges.erase(i);
		return c;
	} else {
		Raul::error << "[PatchImpl::remove_edge] Edge not found" << endl;
		return SharedPtr<EdgeImpl>();
	}
}

bool
PatchImpl::has_edge(const PortImpl* tail, const PortImpl* dst_port) const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	Edges::const_iterator i = _edges.find(make_pair(tail, dst_port));
	return (i != _edges.end());
}

uint32_t
PatchImpl::num_ports_non_rt() const
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	return _inputs.size() + _outputs.size();
}

/** Create a port.  Not realtime safe.
 */
PortImpl*
PatchImpl::create_port(BufferFactory& bufs,
                       const string&  name,
                       PortType       type,
                       LV2_URID       buffer_type,
                       uint32_t       buffer_size,
                       bool           is_output,
                       bool           polyphonic)
{
	if (type == PortType::UNKNOWN) {
		Raul::error << "[PatchImpl::create_port] Unknown port type " << type.uri() << endl;
		return NULL;
	}

	Raul::Atom value;
	if (type == PortType::CONTROL || type == PortType::CV)
		value = bufs.forge().make(0.0f);

	return new DuplexPort(bufs, this, name, num_ports_non_rt(), polyphonic, _polyphony,
	                      type, buffer_type, value, buffer_size, is_output);
}

/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
PatchImpl::Ports::Node*
PatchImpl::remove_port(const string& symbol)
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	bool found = false;
	Ports::Node* ret = NULL;
	for (Ports::iterator i = _inputs.begin(); i != _inputs.end(); ++i) {
		if ((*i)->symbol() == symbol) {
			ret = _inputs.erase(i);
			found = true;
			break;
		}
	}

	if (!found) {
		for (Ports::iterator i = _outputs.begin(); i != _outputs.end(); ++i) {
			if ((*i)->symbol() == symbol) {
				ret = _outputs.erase(i);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		Raul::error << "[PatchImpl::remove_port] Port not found!" << endl;
	}

	return ret;
}

/** Remove all ports from ports list used in pre-processing thread.
 *
 * Ports are not removed from ports array for process thread (which could be
 * simultaneously running).  Returned is a (inputs, outputs) pair.
 *
 * Realtime safe.  Preprocessing thread only.
 */
void
PatchImpl::clear_ports()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	_inputs.clear();
	_outputs.clear();
}

Raul::Array<PortImpl*>*
PatchImpl::build_ports_array() const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	const size_t n = _inputs.size() + _outputs.size();
	Raul::Array<PortImpl*>* const result = new Raul::Array<PortImpl*>(n);

	size_t i = 0;

	for (Ports::const_iterator p = _inputs.begin(); p != _inputs.end(); ++p, ++i)
		result->at(i) = *p;

	for (Ports::const_iterator p = _outputs.begin(); p != _outputs.end(); ++p, ++i)
		result->at(i) = *p;

	assert(i == n);

	return result;
}

static inline void
compile_recursive(NodeImpl* n, CompiledPatch* output)
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

/** Find the process order for this Patch.
 *
 * The process order is a flat list that the patch will execute in order
 * when its run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * Not realtime safe.
 */
CompiledPatch*
PatchImpl::compile() const
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	CompiledPatch* const compiled_patch = new CompiledPatch();//_nodes.size());

	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->traversed(false);

	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		// Either a sink or connected to our output ports:
		if (!node->traversed() && node->dependants().empty())
			compile_recursive(node, compiled_patch);
	}

	// Traverse any nodes we didn't hit yet
	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		if ( ! node->traversed())
			compile_recursive(node, compiled_patch);
	}

	// Add any queued edges that must be run after a cycle
	for (Edges::const_iterator i = _edges.begin();
	     i != _edges.end(); ++i) {
		SharedPtr<EdgeImpl> c = PtrCast<EdgeImpl>(i->second);
		if (c->tail()->parent_node()->context() == Context::AUDIO &&
		    c->head()->parent_node()->context() == Context::MESSAGE) {
			compiled_patch->queued_edges.push_back(c.get());
		}
	}

	assert(compiled_patch->size() == _nodes.size());

	return compiled_patch;
}

} // namespace Server
} // namespace Ingen
