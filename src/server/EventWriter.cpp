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

#include <string>

#include "raul/log.hpp"

#include "ingen/shared/URIs.hpp"

#include "Broadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EventWriter.hpp"
#include "events.hpp"

#define LOG(s) s << "[EventWriter] "

using namespace std;

namespace Ingen {
namespace Server {

EventWriter::EventWriter(Engine& engine)
	: _engine(engine)
	, _request_id(-1)
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
EventWriter::put(const Raul::URI&            uri,
                 const Resource::Properties& properties,
                 const Resource::Graph       ctx)
{
	_engine.enqueue_event(
		new Events::SetMetadata(_engine, _respondee.get(), _request_id, now(),
		                        true, ctx, uri, properties));
}

void
EventWriter::delta(const Raul::URI&            uri,
                   const Resource::Properties& remove,
                   const Resource::Properties& add)
{
	_engine.enqueue_event(
		new Events::SetMetadata(_engine, _respondee.get(), _request_id, now(),
		                        false, Resource::DEFAULT, uri, add, remove));
}

void
EventWriter::move(const Raul::Path& old_path,
                  const Raul::Path& new_path)
{
	_engine.enqueue_event(
		new Events::Move(_engine, _respondee.get(), _request_id, now(),
		                 old_path, new_path));
}

void
EventWriter::del(const Raul::URI& uri)
{
	_engine.enqueue_event(
		new Events::Delete(_engine, _respondee.get(), _request_id, now(), uri));
}

void
EventWriter::connect(const Raul::Path& tail_path,
                     const Raul::Path& head_path)
{
	_engine.enqueue_event(
		new Events::Connect(_engine, _respondee.get(), _request_id, now(),
		                    tail_path, head_path));

}

void
EventWriter::disconnect(const Raul::Path& src,
                        const Raul::Path& dst)
{
	_engine.enqueue_event(
		new Events::Disconnect(_engine, _respondee.get(), _request_id, now(),
		                       src, dst));
}

void
EventWriter::disconnect_all(const Raul::Path& patch_path,
                            const Raul::Path& path)
{
	_engine.enqueue_event(
		new Events::DisconnectAll(_engine, _respondee.get(), _request_id, now(),
		                          patch_path, path));
}

void
EventWriter::set_property(const Raul::URI&  uri,
                          const Raul::URI&  predicate,
                          const Raul::Atom& value)
{
	Resource::Properties remove;
	remove.insert(make_pair(predicate, _engine.world()->uris().wildcard));
	Resource::Properties add;
	add.insert(make_pair(predicate, value));
	_engine.enqueue_event(
		new Events::SetMetadata(_engine, _respondee.get(), _request_id, now(),
		                        false, Resource::DEFAULT, uri, add, remove));
}

void
EventWriter::get(const Raul::URI& uri)
{
	_engine.enqueue_event(
		new Events::Get(_engine, _respondee.get(), _request_id, now(), uri));
}

} // namespace Server
} // namespace Ingen
