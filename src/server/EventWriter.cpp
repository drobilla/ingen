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

#include <boost/variant/apply_visitor.hpp>

#include "ingen/URIs.hpp"

#include "Engine.hpp"
#include "EventWriter.hpp"
#include "events.hpp"

using namespace std;

namespace Ingen {
namespace Server {

EventWriter::EventWriter(Engine& engine)
	: _engine(engine)
	, _event_mode(Event::Mode::NORMAL)
{
}

SampleCount
EventWriter::now() const
{
	return _engine.event_time();
}

void
EventWriter::message(const Message& msg)
{
	boost::apply_visitor(*this, msg);
}

void
EventWriter::operator()(const BundleBegin& msg)
{
	_engine.enqueue_event(new Events::Mark(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const BundleEnd& msg)
{
	_engine.enqueue_event(new Events::Mark(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Put& msg)
{
	_engine.enqueue_event(new Events::Delta(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Delta& msg)
{
	_engine.enqueue_event(new Events::Delta(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Copy& msg)
{
	_engine.enqueue_event(new Events::Copy(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Move& msg)
{
	_engine.enqueue_event(new Events::Move(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Del& msg)
{
	_engine.enqueue_event(new Events::Delete(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Connect& msg)
{
	_engine.enqueue_event(new Events::Connect(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Disconnect& msg)
{
	_engine.enqueue_event(
		new Events::Disconnect(_engine, _respondee, now(), msg),
		_event_mode);
}

void
EventWriter::operator()(const DisconnectAll& msg)
{
	_engine.enqueue_event(
		new Events::DisconnectAll(_engine, _respondee, now(), msg),
		_event_mode);
}

void
EventWriter::operator()(const SetProperty& msg)
{
	_engine.enqueue_event(new Events::Delta(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Undo& msg)
{
	_engine.enqueue_event(new Events::Undo(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Redo& msg)
{
	_engine.enqueue_event(new Events::Undo(_engine, _respondee, now(), msg),
	                      _event_mode);
}

void
EventWriter::operator()(const Get& msg)
{
	_engine.enqueue_event(new Events::Get(_engine, _respondee, now(), msg),
	                      _event_mode);
}

} // namespace Server
} // namespace Ingen
