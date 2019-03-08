/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "events/Copy.hpp"

#include "BlockImpl.hpp"
#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PreProcessContext.hpp"

#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/World.hpp"
#include "raul/Path.hpp"

#include <mutex>
#include <string>
#include <utility>

namespace ingen {
namespace server {
namespace events {

Copy::Copy(Engine&            engine,
           SPtr<Interface>    client,
           SampleCount        timestamp,
           const ingen::Copy& msg)
	: Event(engine, client, msg.seq, timestamp)
	, _msg(msg)
	, _old_block(nullptr)
	, _parent(nullptr)
	, _block(nullptr)
{}

bool
Copy::pre_process(PreProcessContext& ctx)
{
	std::lock_guard<Store::Mutex> lock(_engine.store()->mutex());

	if (uri_is_path(_msg.old_uri)) {
		// Old URI is a path within the engine
		const Raul::Path old_path = uri_to_path(_msg.old_uri);

		// Find the old node
		const Store::iterator i = _engine.store()->find(old_path);
		if (i == _engine.store()->end()) {
			return Event::pre_process_done(Status::NOT_FOUND, old_path);
		}

		// Ensure it is a block (ports are not supported for now)
		if (!(_old_block = dynamic_ptr_cast<BlockImpl>(i->second))) {
			return Event::pre_process_done(Status::BAD_OBJECT_TYPE, old_path);
		}

		if (uri_is_path(_msg.new_uri)) {
			// Copy to path within the engine
			return engine_to_engine(ctx);
		} else if (_msg.new_uri.scheme() == "file") {
			// Copy to filesystem path (i.e. save)
			return engine_to_filesystem(ctx);
		} else {
			return Event::pre_process_done(Status::BAD_REQUEST);
		}
	} else if (_msg.old_uri.scheme() == "file") {
		if (uri_is_path(_msg.new_uri)) {
			return filesystem_to_engine(ctx);
		} else {
			// Ingen is not your file manager
			return Event::pre_process_done(Status::BAD_REQUEST);
		}
	}

	return Event::pre_process_done(Status::BAD_URI);
}

bool
Copy::engine_to_engine(PreProcessContext& ctx)
{
	// Only support a single source for now
	const Raul::Path new_path = uri_to_path(_msg.new_uri);
	if (!Raul::Symbol::is_valid(new_path.symbol())) {
		return Event::pre_process_done(Status::BAD_REQUEST);
	}

	// Ensure the new node doesn't already exist
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
	_compiled_graph = ctx.maybe_compile(*_engine.maid(), *_parent);

	return Event::pre_process_done(Status::SUCCESS);
}

static bool
ends_with(const std::string& str, const std::string& end)
{
    if (str.length() >= end.length()) {
        return !str.compare(str.length() - end.length(), end.length(), end);
    }
    return false;
}

bool
Copy::engine_to_filesystem(PreProcessContext& ctx)
{
	// Ensure source is a graph
	SPtr<GraphImpl> graph = dynamic_ptr_cast<GraphImpl>(_old_block);
	if (!graph) {
		return Event::pre_process_done(Status::BAD_OBJECT_TYPE, _msg.old_uri);
	}

	if (!_engine.world().serialiser()) {
		return Event::pre_process_done(Status::INTERNAL_ERROR);
	}

	std::lock_guard<std::mutex> lock(_engine.world().rdf_mutex());

	if (ends_with(_msg.new_uri, ".ingen") || ends_with(_msg.new_uri, ".ingen/")) {
		_engine.world().serialiser()->write_bundle(graph, URI(_msg.new_uri));
	} else {
		_engine.world().serialiser()->start_to_file(graph->path(), _msg.new_uri);
		_engine.world().serialiser()->serialise(graph);
		_engine.world().serialiser()->finish();
	}

	return Event::pre_process_done(Status::SUCCESS);
}

bool
Copy::filesystem_to_engine(PreProcessContext& ctx)
{
	if (!_engine.world().parser()) {
		return Event::pre_process_done(Status::INTERNAL_ERROR);
	}

	std::lock_guard<std::mutex> lock(_engine.world().rdf_mutex());

	// Old URI is a filesystem path and new URI is a path within the engine
	const std::string             src_path(_msg.old_uri.path());
	const Raul::Path              dst_path = uri_to_path(_msg.new_uri);
	boost::optional<Raul::Path>   dst_parent;
	boost::optional<Raul::Symbol> dst_symbol;
	if (!dst_path.is_root()) {
		dst_parent = dst_path.parent();
		dst_symbol = Raul::Symbol(dst_path.symbol());
	}

	_engine.world().parser()->parse_file(
		_engine.world(), *_engine.world().interface(), src_path,
		dst_parent, dst_symbol);

	return Event::pre_process_done(Status::SUCCESS);
}

void
Copy::execute(RunContext& context)
{
	if (_block && _compiled_graph) {
		_parent->set_compiled_graph(std::move(_compiled_graph));
	}
}

void
Copy::post_process()
{
	Broadcaster::Transfer t(*_engine.broadcaster());
	if (respond() == Status::SUCCESS) {
		_engine.broadcaster()->message(_msg);
	}
}

void
Copy::undo(Interface& target)
{
	if (uri_is_path(_msg.new_uri)) {
		target.del(_msg.new_uri);
	}
}

} // namespace events
} // namespace server
} // namespace ingen
