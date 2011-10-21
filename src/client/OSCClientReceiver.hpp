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

#ifndef INGEN_CLIENT_OSCCLIENTRECEIVER_HPP
#define INGEN_CLIENT_OSCCLIENTRECEIVER_HPP

#include <cstdlib>

#include <boost/utility.hpp>
#include <lo/lo.h>

#include "ingen/ClientInterface.hpp"
#include "raul/Deletable.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen {
namespace Client {

/** Arguments to a liblo handler */
#define LO_HANDLER_ARGS const char* path, const char* types, lo_arg** argv, int argc, lo_message msg

/** Define a static handler to be passed to lo_add_method, which is a trivial
 * wrapper around a non-static method that does the real work. */
#define LO_HANDLER(name) \
int _##name##_cb (LO_HANDLER_ARGS);\
inline static int name##_cb(LO_HANDLER_ARGS, void* osc_listener)\
{ return ((OSCClientReceiver*)osc_listener)->_##name##_cb(path, types, argv, argc, msg); }

/** Client-side receiver for OSC messages from the engine.
 *
 * \ingroup IngenClient
 */
class OSCClientReceiver : public boost::noncopyable, public Raul::Deletable
{
public:
	OSCClientReceiver(int listen_port, SharedPtr<ClientInterface> target);
	~OSCClientReceiver();

	std::string uri() const { return lo_server_thread_get_url(_st); }

	void start(bool dump_osc);
	void stop();

	int         listen_port() { return _listen_port; }
	std::string listen_url()  { return lo_server_thread_get_url(_st); }

private:
	void setup_callbacks();

	static void lo_error_cb(int num, const char* msg, const char* path);

	static int  generic_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data);
	static int  unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* osc_receiver);

	SharedPtr<ClientInterface> _target;
	lo_server_thread           _st;
	Raul::URI                  _delta_uri;
	Resource::Properties       _delta_remove;
	Resource::Properties       _delta_add;
	int                        _listen_port;

	LO_HANDLER(error);
	LO_HANDLER(response_ok);
	LO_HANDLER(response_error);
	LO_HANDLER(plugin);
	LO_HANDLER(plugin_list_end);
	LO_HANDLER(new_patch);
	LO_HANDLER(del);
	LO_HANDLER(move);
	LO_HANDLER(connection);
	LO_HANDLER(disconnection);
	LO_HANDLER(put);
	LO_HANDLER(delta_begin);
	LO_HANDLER(delta_remove);
	LO_HANDLER(delta_add);
	LO_HANDLER(delta_end);
	LO_HANDLER(set_property);
	LO_HANDLER(activity);
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_OSCCLIENTRECEIVER_HPP
