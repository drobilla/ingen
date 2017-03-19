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

#include "ingen/URIs.hpp"

#include "Engine.hpp"
#include "EventWriter.hpp"
#include "events.hpp"

using namespace std;

namespace Ingen {
namespace Server {

EventWriter::EventWriter(Engine& engine)
	: _engine(engine)
	, _request_id(0)
	, _event_mode(Event::Mode::NORMAL)
{
}

EventWriter::~EventWriter()
{
}

SampleCount
EventWriter::now() const
{
	return _engine.event_time();
}

void
EventWriter::set_response_id(int32_t id)
{
	_request_id = id;
}

void
EventWriter::bundle_begin()
{
	_engine.enqueue_event(
		new Events::Mark(_engine, _respondee, _request_id, now(),
		                 Events::Mark::Type::BUNDLE_START),
		_event_mode);
}

void
EventWriter::bundle_end()
{
	_engine.enqueue_event(
		new Events::Mark(_engine, _respondee, _request_id, now(),
		                 Events::Mark::Type::BUNDLE_END),
		_event_mode);
}

void
EventWriter::put(const Raul::URI&      uri,
                 const Properties&     properties,
                 const Resource::Graph ctx)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::PUT, ctx, uri, properties),
		_event_mode);
}

void
EventWriter::delta(const Raul::URI&      uri,
                   const Properties&     remove,
                   const Properties&     add,
                   const Resource::Graph ctx)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::PATCH, ctx, uri, add, remove),
		_event_mode);
}

void
EventWriter::copy(const Raul::URI& old_uri,
                  const Raul::URI& new_uri)
{
	_engine.enqueue_event(
		new Events::Copy(_engine, _respondee, _request_id, now(),
		                 old_uri, new_uri),
		_event_mode);
}

void
EventWriter::move(const Raul::Path& old_path,
                  const Raul::Path& new_path)
{
	_engine.enqueue_event(
		new Events::Move(_engine, _respondee, _request_id, now(),
		                 old_path, new_path),
		_event_mode);
}

void
EventWriter::del(const Raul::URI& uri)
{
	_engine.enqueue_event(
		new Events::Delete(_engine, _respondee, _request_id, now(), uri),
		_event_mode);
}

void
EventWriter::connect(const Raul::Path& tail_path,
                     const Raul::Path& head_path)
{
	_engine.enqueue_event(
		new Events::Connect(_engine, _respondee, _request_id, now(),
		                    tail_path, head_path),
		_event_mode);

}

void
EventWriter::disconnect(const Raul::Path& src,
                        const Raul::Path& dst)
{
	_engine.enqueue_event(
		new Events::Disconnect(_engine, _respondee, _request_id, now(),
		                       src, dst),
		_event_mode);
}

void
EventWriter::disconnect_all(const Raul::Path& graph,
                            const Raul::Path& path)
{
	_engine.enqueue_event(
		new Events::DisconnectAll(_engine, _respondee, _request_id, now(),
		                          graph, path),
		_event_mode);
}

void
EventWriter::set_property(const Raul::URI&      uri,
                          const Raul::URI&      predicate,
                          const Atom&           value,
                          const Resource::Graph ctx)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::SET, ctx,
		                  uri, {{predicate, value}}, {}),
		_event_mode);
}

void
EventWriter::undo()
{
	_engine.enqueue_event(
		new Events::Undo(_engine, _respondee, _request_id, now(), false),
		_event_mode);
}

void
EventWriter::redo()
{
	_engine.enqueue_event(
		new Events::Undo(_engine, _respondee, _request_id, now(), true),
		_event_mode);
}

void
EventWriter::get(const Raul::URI& uri)
{
	_engine.enqueue_event(
		new Events::Get(_engine, _respondee, _request_id, now(), uri),
		_event_mode);
}

} // namespace Server
} // namespace Ingen
