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

#ifndef INGEN_ENGINE_PREPROCESSOR_HPP
#define INGEN_ENGINE_PREPROCESSOR_HPP

#include <glibmm/thread.h>

#include "raul/AtomicPtr.hpp"
#include "raul/Slave.hpp"

namespace Ingen {
namespace Server {

class Event;
class PostProcessor;
class ProcessContext;

class PreProcessor : public Raul::Slave
{
public:
	explicit PreProcessor();

	~PreProcessor();

	/** Return true iff no events are enqueued. */
	inline bool empty() const { return !_head.get(); }

	/** Enqueue an event.
	 * This is safe to call from any non-realtime thread (it locks).
	 */
	void event(Event* ev);

	/** Process events for a cycle.
	 * @return The number of events processed.
	 */
	unsigned process(ProcessContext& context,
	                 PostProcessor&  dest,
	                 bool            limit = true);

protected:
	virtual void _whipped();  ///< Prepare 1 event

private:
	Glib::Mutex            _mutex;
	Raul::AtomicPtr<Event> _head;
	Raul::AtomicPtr<Event> _prepared_back;
	Raul::AtomicPtr<Event> _tail;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PREPROCESSOR_HPP

