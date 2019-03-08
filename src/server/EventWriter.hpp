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

#ifndef INGEN_ENGINE_EVENTWRITER_HPP
#define INGEN_ENGINE_EVENTWRITER_HPP

#include "Event.hpp"
#include "types.hpp"

#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/URI.hpp"
#include "ingen/types.hpp"

namespace ingen {
namespace server {

class Engine;

/** An Interface that creates and enqueues Events for the Engine to execute.
 */
class EventWriter : public Interface
{
public:
	explicit EventWriter(Engine& engine);

	URI uri() const override { return URI("ingen:/clients/event_writer"); }

	SPtr<Interface> respondee() const override {
		return _respondee;
	}

	void set_respondee(SPtr<Interface> respondee) override {
		_respondee = respondee;
	}

	void message(const Message& msg) override;

	void        set_event_mode(Event::Mode mode) { _event_mode = mode; }
	Event::Mode get_event_mode()                 { return _event_mode; }

	void operator()(const BundleBegin&);
	void operator()(const BundleEnd&);
	void operator()(const Connect&);
	void operator()(const Copy&);
	void operator()(const Del&);
	void operator()(const Delta&);
	void operator()(const Disconnect&);
	void operator()(const DisconnectAll&);
	void operator()(const Error&) {}
	void operator()(const Get&);
	void operator()(const Move&);
	void operator()(const Put&);
	void operator()(const Redo&);
	void operator()(const Response&) {}
	void operator()(const SetProperty&);
	void operator()(const Undo&);

protected:
	Engine&         _engine;
	SPtr<Interface> _respondee;
	Event::Mode     _event_mode;

private:
	SampleCount now() const;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_EVENTWRITER_HPP
