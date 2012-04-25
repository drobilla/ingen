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

#ifndef INGEN_CLIENT_SIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_SIGCLIENTINTERFACE_HPP

#include <stdint.h>

#include "raul/Path.hpp"

#include "ingen/Interface.hpp"
#include "ingen/client/signal.hpp"

namespace Ingen {
namespace Client {

/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This simply emits a signal for every event that comes from the engine.
 * For a higher level model based view of the engine, use ClientStore.
 *
 * The signals here match the calls to ClientInterface exactly.  See the
 * documentation for ClientInterface for meanings of signal parameters.
 */
class SigClientInterface : public Ingen::Interface,
                           public INGEN_TRACKABLE
{
public:
	SigClientInterface() {}

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	INGEN_SIGNAL(response, void, int32_t, Status)
	INGEN_SIGNAL(bundle_begin, void)
	INGEN_SIGNAL(bundle_end, void)
	INGEN_SIGNAL(error, void, std::string)
	INGEN_SIGNAL(new_patch, void, Raul::Path, uint32_t)
	INGEN_SIGNAL(new_port, void, Raul::Path, Raul::URI, uint32_t, bool)
	INGEN_SIGNAL(put, void, Raul::URI, Resource::Properties, Resource::Graph)
	INGEN_SIGNAL(delta, void, Raul::URI, Resource::Properties, Resource::Properties)
	INGEN_SIGNAL(object_moved, void, Raul::Path, Raul::Path)
	INGEN_SIGNAL(object_deleted, void, Raul::URI)
	INGEN_SIGNAL(connection, void, Raul::Path, Raul::Path)
	INGEN_SIGNAL(disconnection, void, Raul::URI, Raul::URI)
	INGEN_SIGNAL(disconnect_all, void, Raul::Path, Raul::Path)
	INGEN_SIGNAL(variable_change, void, Raul::URI, Raul::URI, Raul::Atom)
	INGEN_SIGNAL(property_change, void, Raul::URI, Raul::URI, Raul::Atom)
	INGEN_SIGNAL(port_value, void, Raul::Path, Raul::Atom)

	/** Fire pending signals.  Only does anything on derived classes (that may queue) */
	virtual bool emit_signals() { return false; }

protected:

	// ClientInterface hooks that fire the above signals

#define EMIT(name, ...) { _signal_ ## name (__VA_ARGS__); }

	void bundle_begin()
		{ EMIT(bundle_begin); }

	void bundle_end()
		{ EMIT(bundle_end); }

	void response(int32_t id, Status status)
		{ EMIT(response, id, status); }

	void error(const std::string& msg)
		{ EMIT(error, msg); }

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx=Resource::DEFAULT)
		{ EMIT(put, uri, properties, ctx); }

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add)
		{ EMIT(delta, uri, remove, add); }

	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path)
		{ EMIT(connection, src_port_path, dst_port_path); }

	void del(const Raul::URI& uri)
		{ EMIT(object_deleted, uri); }

	void move(const Raul::Path& old_path, const Raul::Path& new_path)
		{ EMIT(object_moved, old_path, new_path); }

	void disconnect(const Raul::URI& src, const Raul::URI& dst)
		{ EMIT(disconnection, src, dst); }

	void disconnect_all(const Raul::Path& parent_patch_path, const Raul::Path& path)
		{ EMIT(disconnect_all, parent_patch_path, path); }

	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value)
		{ EMIT(property_change, subject, key, value); }

	void set_response_id(int32_t id) {}
	void get(const Raul::URI& uri) {}
};

} // namespace Client
} // namespace Ingen

#endif