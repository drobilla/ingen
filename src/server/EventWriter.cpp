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

#include <boost/variant.hpp>

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
EventWriter::message(const Message& msg)
{
	boost::apply_visitor(*this, msg);
}

void
EventWriter::operator()(const BundleBegin&)
{
	_engine.enqueue_event(
		new Events::Mark(_engine, _respondee, _request_id, now(),
		                 Events::Mark::Type::BUNDLE_START),
		_event_mode);
}

void
EventWriter::operator()(const BundleEnd&)
{
	_engine.enqueue_event(
		new Events::Mark(_engine, _respondee, _request_id, now(),
		                 Events::Mark::Type::BUNDLE_END),
		_event_mode);
}

void
EventWriter::operator()(const Put& msg)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::PUT, msg.ctx, msg.uri, msg.properties),
		_event_mode);
}

void
EventWriter::operator()(const Delta& msg)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::PATCH, msg.ctx, msg.uri, msg.add, msg.remove),
		_event_mode);
}

void
EventWriter::operator()(const Copy& msg)
{
	_engine.enqueue_event(
		new Events::Copy(_engine, _respondee, _request_id, now(),
		                 msg.old_uri, msg.new_uri),
		_event_mode);
}

void
EventWriter::operator()(const Move& msg)
{
	_engine.enqueue_event(
		new Events::Move(_engine, _respondee, _request_id, now(),
		                 msg.old_path, msg.new_path),
		_event_mode);
}

void
EventWriter::operator()(const Del& msg)
{
	_engine.enqueue_event(
		new Events::Delete(_engine, _respondee, _request_id, now(), msg.uri),
		_event_mode);
}

void
EventWriter::operator()(const Connect& msg)
{
	_engine.enqueue_event(
		new Events::Connect(_engine, _respondee, _request_id, now(),
		                    msg.tail, msg.head),
		_event_mode);

}

void
EventWriter::operator()(const Disconnect& msg)
{
	_engine.enqueue_event(
		new Events::Disconnect(_engine, _respondee, _request_id, now(),
		                       msg.tail, msg.head),
		_event_mode);
}

void
EventWriter::operator()(const DisconnectAll& msg)
{
	_engine.enqueue_event(
		new Events::DisconnectAll(_engine, _respondee, _request_id, now(),
		                          msg.graph, msg.path),
		_event_mode);
}

void
EventWriter::operator()(const SetProperty& msg)
{
	_engine.enqueue_event(
		new Events::Delta(_engine, _respondee, _request_id, now(),
		                  Events::Delta::Type::SET, msg.ctx,
		                  msg.subject, {{msg.predicate, msg.value}}, {}),
		_event_mode);
}

void
EventWriter::operator()(const Undo&)
{
	_engine.enqueue_event(
		new Events::Undo(_engine, _respondee, _request_id, now(), false),
		_event_mode);
}

void
EventWriter::operator()(const Redo&)
{
	_engine.enqueue_event(
		new Events::Undo(_engine, _respondee, _request_id, now(), true),
		_event_mode);
}

void
EventWriter::operator()(const Get& msg)
{
	_engine.enqueue_event(
		new Events::Get(_engine, _respondee, _request_id, now(), msg.subject),
		_event_mode);
}

} // namespace Server
} // namespace Ingen
