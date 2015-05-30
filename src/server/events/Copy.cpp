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

#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "raul/Path.hpp"
#include "serd/serd.h"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EnginePort.hpp"
#include "GraphImpl.hpp"
#include "events/Copy.hpp"

namespace Ingen {
namespace Server {
namespace Events {

Copy::Copy(Engine&           engine,
           SPtr<Interface>   client,
           int32_t           id,
           SampleCount       timestamp,
           const Raul::Path& old_path,
           const Raul::URI&  new_uri)
	: Event(engine, client, id, timestamp)
	, _old_path(old_path)
	, _new_uri(new_uri)
	, _old_block(NULL)
	, _parent(NULL)
	, _block(NULL)
	, _compiled_graph(NULL)
{}

bool
Copy::pre_process()
{
	if (_old_path.empty() || _new_uri == Node::root_graph_uri()) {
		return Event::pre_process_done(Status::BAD_REQUEST);
	}

	std::unique_lock<std::mutex> lock(_engine.store()->mutex());

	// Find the old node
	const Store::iterator i = _engine.store()->find(_old_path);
	if (i == _engine.store()->end()) {
		return Event::pre_process_done(Status::NOT_FOUND, _old_path);
	}

	// Ensure it is a block (ports are not supported for now)
	if (!(_old_block = dynamic_ptr_cast<BlockImpl>(i->second))) {
		return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _old_path);
	}

	if (Node::uri_is_path(_new_uri)) {
		// Copy to path within the engine
		return copy_to_engine();
	} else if (_new_uri.scheme() == "file") {
		// Copy to filesystem path (i.e. save)
		return copy_to_filesystem();
	} else {
		return Event::pre_process_done(Status::BAD_REQUEST);
	}
}

bool
Copy::copy_to_engine()
{
	// Only support a single source for now
	const Raul::Path new_path = Node::uri_to_path(_new_uri);
	if (!Raul::Symbol::is_valid(new_path.symbol())) {
		return Event::pre_process_done(Status::BAD_REQUEST);
	}

	// Ensure the new node doesn't already exists
	if (_engine.store()->find(new_path) != _engine.store()->end()) {
		return Event::pre_process_done(Status::EXISTS, new_path);
	}

	// Find new parent graph
	const Raul::Path      parent_path = new_path.parent();
	const Store::iterator p           = _engine.store()->find(parent_path);
	if (p == _engine.store()->end()) {
		return Event::pre_process_done(Status::NOT_FOUND, parent_path);
	}
	if (!(_parent = dynamic_cast<GraphImpl*>(p->second.get()))) {
		return Event::pre_process_done(Status::BAD_OBJECT_TYPE, parent_path);
	}

	// Create new block
	if (!(_block = dynamic_cast<BlockImpl*>(
		      _old_block->duplicate(_engine, Raul::Symbol(new_path.symbol()), _parent)))) {
		return Event::pre_process_done(Status::INTERNAL_ERROR);
	}

	_block->activate(*_engine.buffer_factory());

	// Add block to the store and the graph's pre-processor only block list
	_parent->add_block(*_block);
	_engine.store()->add(_block);

	// Compile graph with new block added for insertion in audio thread
	if (_parent->enabled()) {
		_compiled_graph = _parent->compile();
	}

	return Event::pre_process_done(Status::SUCCESS);
}

bool
Copy::copy_to_filesystem()
{
	// Ensure source is a graph
	SPtr<GraphImpl> graph = dynamic_ptr_cast<GraphImpl>(_old_block);
	if (!graph) {
		return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _old_path);
	}

	// Parse filename from target URI
	const uint8_t*    uri      = (const uint8_t*)_new_uri.c_str();
	uint8_t*          path     = serd_file_uri_parse(uri, NULL);
	const std::string filename = (const char*)path;
	free(path);

	if (!_engine.world()->serialiser()) {
		return Event::pre_process_done(Status::INTERNAL_ERROR);
	}

	std::lock_guard<std::mutex> lock(_engine.world()->rdf_mutex());

	if (filename.find(".ingen") != std::string::npos) {
		_engine.world()->serialiser()->write_bundle(graph, _new_uri/*filename*/);
	} else {
		_engine.world()->serialiser()->start_to_file(graph->path(), _new_uri);
		_engine.world()->serialiser()->serialise(graph);
		_engine.world()->serialiser()->finish();
	}

	return Event::pre_process_done(Status::SUCCESS);
}

void
Copy::execute(ProcessContext& context)
{
	if (_block) {
		_parent->set_compiled_graph(_compiled_graph);
		_compiled_graph = NULL;  // Graph takes ownership
	}
}

void
Copy::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->copy(_old_path, _new_uri);
	}
}

} // namespace Events
} // namespace Server
} // namespace Ingen
