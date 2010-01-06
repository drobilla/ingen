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

#include <cassert>
#include <unistd.h>
#include "raul/log.hpp"
#include "raul/AtomLiblo.hpp"
#include "interface/ClientInterface.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "OSCClientSender.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


/*! \page client_osc_namespace Client OSC Namespace Documentation
 *
 * <p>These are the commands the client recognizes.  All monitoring of
 * changes in the engine happens via these commands.</p>
 */


/** \page client_osc_namespace
 * <h2>/ingen/ok</h2>
 * \arg \b response-id (int) - Request ID this is a response to
 *
 * Successful response to some command.
 */
void
OSCClientSender::response_ok(int32_t id)
{
	if (!_enabled)
		return;

	if (lo_send(_address, "/ingen/ok", "i", id) < 0) {
		Raul::error << "Unable to send OK " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}


/** \page client_osc_namespace
 * <h2>/ingen/error</h2>
 * \arg \b response-id (int) - Request ID this is a response to
 * \arg \b message (string) - Error message (natural language text)
 *
 * Unsuccessful response to some command.
 */
void
OSCClientSender::response_error(int32_t id, const std::string& msg)
{
	if (!_enabled)
		return;

	if (lo_send(_address, "/ingen/error", "is", id, msg.c_str()) < 0) {
		Raul::error << "Unable to send error " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}


/** \page client_osc_namespace
 * <h2>/ingen/error</h2>
 * \arg \b message (string) - Error message (natural language text)
 *
 * Notification that an error has occurred.
 * This is for notification of errors that aren't a direct response to a
 * user command, ie "unexpected" errors.
 */
void
OSCClientSender::error(const std::string& msg)
{
	send("/ingen/error", "s", msg.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <h2>/ingen/put</h2>
 * \arg \b path (string) - Path of object
 * \arg \b predicate
 * \arg \b value
 * \arg \b ...
 *
 * PUT a set of properties to a path (see \ref methods).
 */
void
OSCClientSender::put(const Raul::URI&                    path,
                     const Shared::Resource::Properties& properties)
{
	typedef Shared::Resource::Properties::const_iterator iterator;
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
	}
	send_message("/ingen/put", m);
}


/** \page client_osc_namespace
 * <h2>/ingen/move</h2>
 * \arg \b old-path (string) - Old path of object
 * \arg \b new-path (string) - New path of object
 *
 * MOVE an object to a new path (see \ref methods).
 * The new path will have the same parent as the old path.
 */
void
OSCClientSender::move(const Path& old_path, const Path& new_path)
{
	send("/ingen/move", "ss", old_path.c_str(), new_path.c_str(), LO_ARGS_END);
}



/** \page client_osc_namespace
 * <h2>/ingen/delete</h2>
 * \arg \b path (string) - Path of object (which no longer exists)
 *
 * DELETE an object (see \ref methods).
 */
void
OSCClientSender::del(const Path& path)
{
	send("/ingen/delete", "s", path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <h2>/ingen/new_connection</h2>
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port
 *
 * Notification a new connection has been made.
 */
void
OSCClientSender::connect(const Path& src_port_path, const Path& dst_port_path)
{
	send("/ingen/new_connection", "ss", src_port_path.c_str(), dst_port_path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <h2>/ingen/disconnection</h2>
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port
 *
 * Notification a connection has been unmade.
 */
void
OSCClientSender::disconnect(const Path& src_port_path, const Path& dst_port_path)
{
	send("/ingen/disconnection", "ss", src_port_path.c_str(), dst_port_path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <h2>/ingen/set_property</h2>
 * \arg \b path (string) - Path of the object associated with property (node, patch, or port)
 * \arg \b key (string)
 * \arg \b value (string)
 *
 * Notification of a property.
 */
void
OSCClientSender::set_property(const URI& path, const URI& key, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_property", m);
}


/** \page client_osc_namespace
 * <h2>/ingen/set_port_value</h2>
 * \arg \b path (string) - Path of port
 * \arg \b value (any) - New value of port
 *
 * Notification the value of a port has changed.
 */
void
OSCClientSender::set_port_value(const Path& port_path, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


/** \page client_osc_namespace
 * <h2>/ingen/set_port_value</h2>
 * \arg \b path (string) - Path of port
 * \arg \b voice (int) - Voice which is set to this value
 * \arg \b value (any) - New value of port
 *
 * Notification the value of a port has changed.
 */
void
OSCClientSender::set_voice_value(const Path& port_path, uint32_t voice, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


/** \page client_osc_namespace
 * <h2>/ingen/activity</h2>
 * \arg \b path (string) - Path of object
 *
 * Notification of "activity" (e.g. port message blinkenlights).
 */
void
OSCClientSender::activity(const Path& path)
{
	if (!_enabled)
		return;

	lo_send(_address, "/ingen/activity", "s", path.c_str(), LO_ARGS_END);
}


} // namespace Ingen
