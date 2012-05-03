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

#ifndef INGEN_ENGINE_EVENTSOURCE_HPP
#define INGEN_ENGINE_EVENTSOURCE_HPP

namespace Ingen {
namespace Server {

class Event;
class PostProcessor;
class ProcessContext;

/** Source for events to run in the audio thread.
 *
 * The Driver gets events from an EventQueue in the process callback and
 * executes them, then they are sent to the PostProcessor and finalised
 * (post-processing thread).
 */
class EventSource
{
public:
	virtual ~EventSource() {}

	virtual void process(PostProcessor&  dest,
	                     ProcessContext& context,
	                     bool            limit = true) = 0;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_EVENTSOURCE_HPP

