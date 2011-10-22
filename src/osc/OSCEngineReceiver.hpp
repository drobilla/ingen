/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_OSCENGINERECEIVER_HPP
#define INGEN_ENGINE_OSCENGINERECEIVER_HPP

#include <stdint.h>

#include <lo/lo.h>

#include "raul/Thread.hpp"
#include "raul/URI.hpp"

#include "ingen/Resource.hpp"

#include "ingen-config.h"
#include "macros.h"

namespace Ingen {

class ServerInterface;

namespace Server {

class Engine;

/* FIXME: Make this receive and preprocess in the same thread? */

/** Receive OSC messages and call interface functions.
 *
 * \ingroup engine
 */
class OSCEngineReceiver
{
public:
	OSCEngineReceiver(Engine&                    engine,
	                  SharedPtr<ServerInterface> interface,
	                  uint16_t                   port);

	~OSCEngineReceiver();

private:
	struct ReceiveThread : public Raul::Thread {
		explicit ReceiveThread(OSCEngineReceiver& receiver) : _receiver(receiver) {}
		virtual void _run();
	private:
		OSCEngineReceiver& _receiver;
	};

	friend struct ReceiveThread;

	ReceiveThread* _receive_thread;

	Raul::URI            _delta_uri;
	Resource::Properties _delta_remove;
	Resource::Properties _delta_add;

#ifdef LIBLO_BUNDLES
	static int bundle_start_cb(lo_timetag time, void* myself) {
		return ((OSCEngineReceiver*)myself)->_bundle_start_cb(time);
	}
	static int bundle_end_cb(void* myself) {
		return ((OSCEngineReceiver*)myself)->_bundle_end_cb();
	}

	int _bundle_start_cb(lo_timetag time);
	int _bundle_end_cb();
#endif

	static void error_cb(int num, const char* msg, const char* path);
	static int  set_response_address_cb(LO_HANDLER_ARGS, void* myself);
	static int  generic_cb(LO_HANDLER_ARGS, void* myself);
	static int  unknown_cb(LO_HANDLER_ARGS, void* myself);

	LO_HANDLER(OSCEngineReceiver, quit);
	LO_HANDLER(OSCEngineReceiver, ping);
	LO_HANDLER(OSCEngineReceiver, ping_slow);
	LO_HANDLER(OSCEngineReceiver, register_client);
	LO_HANDLER(OSCEngineReceiver, unregister_client);
	LO_HANDLER(OSCEngineReceiver, get);
	LO_HANDLER(OSCEngineReceiver, put);
	LO_HANDLER(OSCEngineReceiver, delta_begin);
	LO_HANDLER(OSCEngineReceiver, delta_remove);
	LO_HANDLER(OSCEngineReceiver, delta_add);
	LO_HANDLER(OSCEngineReceiver, delta_end);
	LO_HANDLER(OSCEngineReceiver, move);
	LO_HANDLER(OSCEngineReceiver, del);
	LO_HANDLER(OSCEngineReceiver, connect);
	LO_HANDLER(OSCEngineReceiver, disconnect);
	LO_HANDLER(OSCEngineReceiver, disconnect_all);
	LO_HANDLER(OSCEngineReceiver, note_on);
	LO_HANDLER(OSCEngineReceiver, note_off);
	LO_HANDLER(OSCEngineReceiver, all_notes_off);
	LO_HANDLER(OSCEngineReceiver, learn);
	LO_HANDLER(OSCEngineReceiver, set_property);
	LO_HANDLER(OSCEngineReceiver, property_set);

	Engine&                    _engine;
	SharedPtr<ServerInterface> _interface;
	lo_server                  _server;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_OSCENGINERECEIVER_HPP
