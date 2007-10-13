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

#include <cassert>
#include <cmath>
#include <iostream>
#include "ThreadManager.hpp"
#include "NodeImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ConnectionImpl.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "ProcessSlave.hpp"

using namespace std;

namespace Ingen {


PatchImpl::PatchImpl(Engine& engine, const string& path, uint32_t poly, PatchImpl* parent, SampleRate srate, size_t buffer_size, uint32_t internal_poly) 
: NodeBase(new PluginImpl(Plugin::Patch, "ingen:patch"), path, poly, parent, srate, buffer_size),
  _engine(engine),
  _internal_poly(internal_poly),
  _compiled_patch(NULL),
  _process(false)
{
	assert(internal_poly >= 1);

	//_plugin->plug_label("om_patch");
	//_plugin->name("Ingen Patch");

	//std::cerr << "Creating patch " << _name << ", poly = " << poly
	//	<< ", internal poly = " << internal_poly << std::endl;
}


PatchImpl::~PatchImpl()
{
	assert(!_activated);
	
	delete _compiled_patch;
}


void
PatchImpl::activate()
{
	NodeBase::activate();

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->activate();
	
	assert(_activated);
}


void
PatchImpl::deactivate()
{
	if (_activated) {
	
		NodeBase::deactivate();
	
		for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!_activated);
}


void
PatchImpl::disable()
{
	_process = false;

	for (List<PortImpl*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->clear_buffers();
}

	
bool
PatchImpl::prepare_internal_poly(uint32_t poly)
{
	/* TODO: ports?  internal/external poly? */

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->prepare_poly(poly);
	
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i)
		((ConnectionImpl*)i->get())->prepare_poly(poly);

	/* FIXME: Deal with failure */

	return true;
}


bool
PatchImpl::apply_internal_poly(Raul::Maid& maid, uint32_t poly)
{
	/* TODO: ports?  internal/external poly? */

	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->apply_poly(maid, poly);
	
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i)
		PtrCast<ConnectionImpl>(*i)->apply_poly(maid, poly);

	_internal_poly = poly;
	
	return true;
}


/** Run the patch for the specified number of frames.
 * 
 * Calls all Nodes in (roughly, if parallel) the order _compiled_patch specifies.
 */
void
PatchImpl::process(ProcessContext& context)
{
	if (_compiled_patch == NULL || _compiled_patch->size() == 0 || !_process)
		return;
	
	NodeBase::pre_process(context);
	
	/*if (_ports)
		for (size_t i=0; i < _ports->size(); ++i)
			if (_ports->at(i)->is_input() && _ports->at(i)->type() == DataType::MIDI)
				cerr << _ports->at(i)->path() << " "
					<< _ports->at(i)->buffer(0) << " # events: "
					<< ((MidiBuffer*)_ports->at(i)->buffer(0))->event_count() << endl;*/

	/* Run */
	if (_engine.process_slaves().size() > 0)
		process_parallel(context);
	else
		process_single(context);

	NodeBase::post_process(context);
}


void
PatchImpl::process_parallel(ProcessContext& context)
{
	size_t n_slaves = _engine.process_slaves().size();

	CompiledPatch* const cp = _compiled_patch;

	/* Start p-1 slaves */

	if (n_slaves >= cp->size())
		n_slaves = cp->size()-1;

	if (n_slaves > 0) {
		for (size_t i=0; i < cp->size(); ++i)
			(*cp)[i].node()->reset_input_ready();

		for (size_t i=0; i < n_slaves; ++i)
			_engine.process_slaves()[i]->whip(cp, i+1, context);
	}
	

	/* Process ourself until everything is done
	 * This is analogous to ProcessSlave::_whipped(), but this is the master
	 * (i.e. what the main Jack process thread calls).  Where ProcessSlave
	 * waits on input, this just skips the node and tries the next, to avoid
	 * waiting in the Jack thread which pisses Jack off.
	 */

	size_t index = 0;
	size_t num_finished = 0; // Number of consecutive finished nodes hit

	while (num_finished < cp->size()) {

		CompiledNode& n = (*cp)[index];

		if (n.node()->process_lock()) {
			if (n.node()->n_inputs_ready() == n.n_providers()) {
				n.node()->process(context);

				/* Signal dependants their input is ready */
				for (size_t i=0; i < n.dependants().size(); ++i)
					n.dependants()[i]->signal_input_ready();

				++num_finished;
			} else {
				n.node()->process_unlock();
				num_finished = 0;
			}
		} else {
			if (n.node()->n_inputs_ready() == n.n_providers())
				++num_finished;
			else
				num_finished = 0;
		}

		index = (index + 1) % cp->size();
	}
	
	/* Tell slaves we're done in case we beat them, and pray they're
	 * really done by the start of next cycle.
	 * FIXME: This probably breaks (race) at extremely small nframes where
	 * ingen is the majority of the DSP load.
	 */
	for (size_t i=0; i < n_slaves; ++i)
		_engine.process_slaves()[i]->finish();
}

	
void
PatchImpl::process_single(ProcessContext& context)
{
	CompiledPatch* const cp = _compiled_patch;

	for (size_t i=0; i < cp->size(); ++i)
		(*cp)[i].node()->process(context);
}
	

void
PatchImpl::set_buffer_size(size_t size)
{
	NodeBase::set_buffer_size(size);
	assert(_buffer_size == size);
	
	for (List<NodeImpl*>::iterator j = _nodes.begin(); j != _nodes.end(); ++j)
		(*j)->set_buffer_size(size);
}


// Patch specific stuff


void
PatchImpl::add_node(List<NodeImpl*>::Node* ln)
{
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);
	//assert(ln->elem()->polyphony() == _internal_poly);
	
	_nodes.push_back(ln);
}


/** Remove a node.
 * Realtime Safe.  Preprocessing thread.
 */
PatchImpl::Nodes::Node*
PatchImpl::remove_node(const string& name)
{
	for (List<NodeImpl*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		if ((*i)->name() == name)
			return _nodes.erase(i);
	
	return NULL;
}


/** Remove a connection.  Realtime safe.
 */
PatchImpl::Connections::Node*
PatchImpl::remove_connection(const PortImpl* src_port, const PortImpl* dst_port)
{
	bool found = false;
	Connections::Node* connection = NULL;
	for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		ConnectionImpl* const c = (ConnectionImpl*)i->get();
		if (c->src_port() == src_port && c->dst_port() == dst_port) {
			connection = _connections.erase(i);
			found = true;
			break;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [PatchImpl::remove_connection] Connection not found !" << endl;

	return connection;
}


uint32_t
PatchImpl::num_ports() const
{
	ThreadID context = ThreadManager::current_thread_id();

	if (context == THREAD_PROCESS)
		return NodeBase::num_ports();
	else
		return _input_ports.size() + _output_ports.size();
}


/** Create a port.  Not realtime safe.
 */
PortImpl*
PatchImpl::create_port(const string& name, DataType type, size_t buffer_size, bool is_output)
{
	if (type == DataType::UNKNOWN) {
		cerr << "[PatchImpl::create_port] Unknown port type " << type.uri() << endl;
		return NULL;
	}

	assert( !(type == DataType::UNKNOWN) );

	return new DuplexPort(this, name, 0, _polyphony, type, buffer_size, is_output);
}


/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
List<PortImpl*>::Node*
PatchImpl::remove_port(const string& name)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	bool found = false;
	List<PortImpl*>::Node* ret = NULL;
	for (List<PortImpl*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _input_ports.erase(i);
			found = true;
		}
	}

	if (!found)
	for (List<PortImpl*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _output_ports.erase(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [PatchImpl::remove_port] Port not found !" << endl;

	return ret;
}


Raul::Array<PortImpl*>*
PatchImpl::build_ports_array() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	Raul::Array<PortImpl*>* const result = new Raul::Array<PortImpl*>(_input_ports.size() + _output_ports.size());

	size_t i = 0;
	
	for (List<PortImpl*>::const_iterator p = _input_ports.begin(); p != _input_ports.end(); ++p,++i)
		result->at(i) = *p;
	
	for (List<PortImpl*>::const_iterator p = _output_ports.begin(); p != _output_ports.end(); ++p,++i)
		result->at(i) = *p;

	return result;
}


/** Find the process order for this Patch.
 *
 * The process order is a flat list that the patch will execute in order
 * when it's run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * Not realtime safe.
 */
CompiledPatch*
PatchImpl::compile() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	//cerr << "*********** Building process order for " << path() << endl;

	CompiledPatch* const compiled_patch = new CompiledPatch();//_nodes.size());
	
	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->traversed(false);
		
	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		// Either a sink or connected to our output ports:
		if ( ( ! node->traversed()) && node->dependants()->size() == 0)
			compile_recursive(node, compiled_patch);
	}
	
	// Traverse any nodes we didn't hit yet
	for (Nodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		NodeImpl* const node = (*i);
		if ( ! node->traversed())
			compile_recursive(node, compiled_patch);
	}
	
	/*cerr << "----------------------------------------\n";
	for (size_t i=0; i < process_order->size(); ++i) {
		assert(process_order->at(i));
		cerr << process_order->at(i)->path() << endl;
	}
	cerr << "----------------------------------------\n";*/

	assert(compiled_patch->size() == _nodes.size());

#ifndef NDEBUG
	for (size_t i=0; i < compiled_patch->size(); ++i)
		assert(compiled_patch->at(i).node());
#endif

	return compiled_patch;
}


} // namespace Ingen
