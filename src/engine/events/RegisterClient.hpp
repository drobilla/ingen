/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_REGISTERCLIENT_HPP
#define INGEN_EVENTS_REGISTERCLIENT_HPP

#include "raul/URI.hpp"
#include "interface/ClientInterface.hpp"
#include "QueuedEvent.hpp"

namespace Ingen {
namespace Events {


/** Registers a new client with the OSC system, so it can receive updates.
 *
 * \ingroup engine
 */
class RegisterClient : public QueuedEvent
{
public:
	RegisterClient(Engine&                  engine,
	               SharedPtr<Request>       request,
	               SampleCount              timestamp,
	               const Raul::URI&         uri,
	               Shared::ClientInterface* client);

	void pre_process();
	void post_process();

private:
	Raul::URI                _uri;
	Shared::ClientInterface* _client;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_REGISTERCLIENT_HPP
