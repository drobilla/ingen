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

#include <unistd.h>

#include <cassert>
#include <string>

#include "raul/log.hpp"
#include "raul/AtomLiblo.hpp"

#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "OSCClientSender.hpp"
#include "PatchImpl.hpp"

#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "ingen/ClientInterface.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

/** @page client_osc_namespace Client OSC Namespace Documentation
 *
 * <p>These are the commands the client recognizes.  All monitoring of
 * changes in the engine happens via these commands.</p>
 */

/** @page client_osc_namespace
 * <h2>/ok</h2>
 * @arg @p response-id :: Integer
 *
 * @par
 * Successful response to some command.
 */
void
OSCClientSender::response_ok(int32_t id)
{
	if (!_enabled)
		return;


	
	if (lo_send(_address, "/ok", "i", id, LO_ARGS_END) < 0) {
		Raul::error << "Unable to send OK " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}

/** @page client_osc_namespace
 * <h2>/error</h2>
 * @arg @p response-id :: Integer
 * @arg @p message :: String
 *
 * @par
 * Unsuccessful response to some command.
 */
void
OSCClientSender::response_error(int32_t id, const std::string& msg)
{
	if (!_enabled)
		return;

	if (lo_send(_address, "/error", "is", id, msg.c_str(), LO_ARGS_END) < 0) {
		Raul::error << "Unable to send error " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}

/** @page client_osc_namespace
 * <h2>/error</h2>
 * @arg @p message :: String
 *
 * @par
 * Notification that an error has occurred.  This is for notification of errors
 * that aren't a direct response to a user command, i.e. "unexpected" errors.
 */
void
OSCClientSender::error(const std::string& msg)
{
	send("/error", "s", msg.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/put</h2>
 * @arg @p path :: String
 * @arg @p predicate :: URI String
 * @arg @p value
 * @arg @p ...
 *
 * @par
 * PUT a set of properties to a path.
 */
void
OSCClientSender::put(const Raul::URI&            path,
                     const Resource::Properties& properties,
                     Resource::Graph             ctx)
{
	typedef Resource::Properties::const_iterator iterator;
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
	}
	send_message("/put", m);
}

void
OSCClientSender::delta(const Raul::URI&            path,
                       const Resource::Properties& remove,
                       const Resource::Properties& add)
{
	typedef Resource::Properties::const_iterator iterator;

	const bool bundle = !_bundle;
	if (bundle)
		bundle_begin();

	send("/delta_begin", "s", path.c_str(), LO_ARGS_END);

	for (iterator i = remove.begin(); i != remove.end(); ++i) {
		lo_message m = lo_message_new();
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
		send_message("/delta_remove", m);
	}

	for (iterator i = add.begin(); i != add.end(); ++i) {
		lo_message m = lo_message_new();
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
		send_message("/delta_add", m);
	}

	send("/delta_end", "", LO_ARGS_END);

	if (bundle)
		bundle_end();
}

/** @page client_osc_namespace
 * <h2>/move</h2>
 * @arg @p old-path :: String
 * @arg @p new-path :: String
 *
 * @par
 * MOVE an object to a new path.
 * The new path will have the same parent as the old path.
 */
void
OSCClientSender::move(const Path& old_path, const Path& new_path)
{
	send("/move", "ss", old_path.c_str(), new_path.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/delete</h2>
 * @arg @p path :: String
 *
 * @par
 * DELETE an object.
 */
void
OSCClientSender::del(const URI& uri)
{
	send("/delete", "s", uri.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/connect</h2>
 * @arg @p src-path :: String
 * @arg @p dst-path :: String
 *
 * @par
 * Notification a new connection has been made.
 */
void
OSCClientSender::connect(const Path& src_port_path,
                         const Path& dst_port_path)
{
	send("/connect", "ss", src_port_path.c_str(), dst_port_path.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/disconnect</h2>
 * @arg @p src-path :: String
 * @arg @p dst-path :: String
 *
 * @par
 * Notification a connection has been unmade.
 */
void
OSCClientSender::disconnect(const URI& src,
                            const URI& dst)
{
	send("/disconnect", "ss", src.c_str(), dst.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/disconnect_all</h2>
 * @arg @p parent-patch-path :: String
 * @arg @p path :: String
 *
 * @par
 * Notification all connections to an object have been disconnected.
 */
void
OSCClientSender::disconnect_all(const Raul::Path& parent_patch_path,
                                const Raul::Path& path)
{
	send("/disconnect_all", "ss", parent_patch_path.c_str(), path.c_str(), LO_ARGS_END);
}

/** @page client_osc_namespace
 * <h2>/set_property</h2>
 * @arg @p path :: String
 * @arg @p key :: URI String
 * @arg @p value
 *
 * @par
 * Notification of a property.
 */
void
OSCClientSender::set_property(const URI& path,
                              const URI& key,
                              const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/set_property", m);
}

/** @page client_osc_namespace
 * <h2>/activity</h2>
 * @arg @p path :: String
 *
 * @par
 * Notification of "activity" (e.g. port message blinkenlights).
 */
void
OSCClientSender::activity(const Path& path)
{
	if (!_enabled)
		return;

	lo_send(_address, "/activity", "s", path.c_str(), LO_ARGS_END);
}

} // namespace Server
} // namespace Ingen
