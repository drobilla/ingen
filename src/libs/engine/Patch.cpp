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
#include "Node.hpp"
#include "Patch.hpp"
#include "Plugin.hpp"
#include "Port.hpp"
#include "Connection.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "ProcessSlave.hpp"

using namespace std;

namespace Ingen {


Patch::Patch(Engine& engine, const string& path, uint32_t poly, Patch* parent, SampleRate srate, size_t buffer_size, uint32_t internal_poly) 
: NodeBase(new Plugin(Plugin::Patch, "ingen:patch"), path, poly, parent, srate, buffer_size),
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


Patch::~Patch()
{
	assert(!_activated);
	
	for (Raul::List<Connection*>::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		delete (*i);
		delete _connections.erase(i);
	}

	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		assert(!(*i)->activated());
		delete (*i);
		delete _nodes.erase(i);
	}

	delete _compiled_patch;
}


void
Patch::activate()
{
	NodeBase::activate();

	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->activate();
	
	assert(_activated);
}


void
Patch::deactivate()
{
	if (_activated) {
	
		NodeBase::deactivate();
	
		for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!_activated);
}


void
Patch::disable()
{
	_process = false;

	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->clear_buffers();
}


/** Run the patch for the specified number of frames.
 * 
 * Calls all Nodes in (roughly, if parallel) the order _compiled_patch specifies.
 */
void
Patch::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	if (_compiled_patch == NULL || _compiled_patch->size() == 0 || !_process)
		return;
	
	/* Prepare input ports */

	// This breaks MIDI input, somehow (?)
	//for (Raul::List<Port*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i)
	//	(*i)->pre_process(nframes, start, end);
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->pre_process(nframes, start, end);


	if (_engine.process_slaves().size() > 0)
		process_parallel(nframes, start, end);
	else
		process_single(nframes, start, end);

	
	/* Write output ports */

	for (Raul::List<Port*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i)
		(*i)->post_process(nframes, start, end);
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->post_process(nframes, start, end);
}


void
Patch::process_parallel(SampleCount nframes, FrameTime start, FrameTime end)
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
			_engine.process_slaves()[i]->whip(cp, i+1, nframes, start, end);
	}
	

	/* Process ourself until everything is done
	 * This is analogous to ProcessSlave::_whipped(), but this is the master
	 * (i.e. what the main Jack process thread calls).  Where ProcessSlave
	 * waits on input, this just skips the node and tries the next, to avoid
	 * waiting in the Jack thread which pisses Jack off.
	 */

	size_t index = 0;
	//size_t run_count = 0;
	size_t num_finished = 0; // Number of consecutive finished nodes hit

	while (num_finished < cp->size()) {

		CompiledNode& n = (*cp)[index];

		if (n.node()->process_lock()) {
			if (n.node()->n_inputs_ready() == n.n_providers()) {
				//cout << "************ Main running " << n.node()->path() << " at index " << index << endl;
				n.node()->process(nframes, start, end);

				//cerr << n.node()->path() << " @ " << &n << " dependants: " << n.dependants().size() << endl;

				/* Signal dependants their input is ready */
				for (size_t i=0; i < n.dependants().size(); ++i)
					n.dependants()[i]->signal_input_ready();

				//++run_count;
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
	
	//cout << "Main Thread ran \t" << run_count << " nodes this cycle." << endl;

	/* Tell slaves we're done in case we beat them, and pray they're
	 * really done by the start of next cycle.
	 * FIXME: This probably breaks (race) at extremely small nframes.
	 */
	for (size_t i=0; i < n_slaves; ++i)
		_engine.process_slaves()[i]->finish();
}

	
void
Patch::process_single(SampleCount nframes, FrameTime start, FrameTime end)
{
	CompiledPatch* const cp = _compiled_patch;

	for (size_t i=0; i < cp->size(); ++i)
		(*cp)[i].node()->process(nframes, start, end);
}
	

void
Patch::set_buffer_size(size_t size)
{
	NodeBase::set_buffer_size(size);
	assert(_buffer_size == size);
	
	for (Raul::List<Node*>::iterator j = _nodes.begin(); j != _nodes.end(); ++j)
		(*j)->set_buffer_size(size);
}


// Patch specific stuff


void
Patch::add_node(Raul::ListNode<Node*>* ln)
{
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);
	assert(ln->elem()->poly() == _internal_poly || ln->elem()->poly() == 1);
	
	_nodes.push_back(ln);
}


/** Remove a node.
 * Realtime Safe.  Preprocessing thread.
 */
Raul::ListNode<Node*>*
Patch::remove_node(const string& name)
{
	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		if ((*i)->name() == name)
			return _nodes.erase(i);
	
	return NULL;
}


/** Remove a connection.  Realtime safe.
 */
Raul::ListNode<Connection*>*
Patch::remove_connection(const Port* src_port, const Port* dst_port)
{
	bool found = false;
	Raul::ListNode<Connection*>* connection = NULL;
	for (Raul::List<Connection*>::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		if ((*i)->src_port() == src_port && (*i)->dst_port() == dst_port) {
			connection = _connections.erase(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_connection] Connection not found !" << endl;

	return connection;
}


uint32_t
Patch::num_ports() const
{
	ThreadID context = ThreadManager::current_thread_id();

	if (context == THREAD_PROCESS)
		return NodeBase::num_ports();
	else
		return _input_ports.size() + _output_ports.size();
}


/** Create a port.  Not realtime safe.
 */
Port*
Patch::create_port(const string& name, DataType type, size_t buffer_size, bool is_output)
{
	if (type == DataType::UNKNOWN) {
		cerr << "[Patch::create_port] Unknown port type " << type.uri() << endl;
		return NULL;
	}

	assert( !(type == DataType::UNKNOWN) );

	return new DuplexPort(this, name, 0, _poly, type, buffer_size, is_output);
}


/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
Raul::ListNode<Port*>*
Patch::remove_port(const string& name)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	bool found = false;
	Raul::ListNode<Port*>* ret = NULL;
	for (Raul::List<Port*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _input_ports.erase(i);
			found = true;
		}
	}

	if (!found)
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _output_ports.erase(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_port] Port not found !" << endl;

	return ret;
}


Raul::Array<Port*>*
Patch::build_ports_array() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	Raul::Array<Port*>* const result = new Raul::Array<Port*>(_input_ports.size() + _output_ports.size());

	size_t i = 0;
	
	for (Raul::List<Port*>::const_iterator p = _input_ports.begin(); p != _input_ports.end(); ++p,++i)
		result->at(i) = *p;
	
	for (Raul::List<Port*>::const_iterator p = _output_ports.begin(); p != _output_ports.end(); ++p,++i)
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
Patch::compile() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	//cerr << "*********** Building process order for " << path() << endl;

	CompiledPatch* const compiled_patch = new CompiledPatch();//_nodes.size());
	
	for (Raul::List<Node*>::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->traversed(false);
		
	for (Raul::List<Node*>::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		Node* const node = (*i);
		// Either a sink or connected to our output ports:
		if ( ( ! node->traversed()) && node->dependants()->size() == 0)
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
