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
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_CLIENT_SIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_SIGCLIENTINTERFACE_HPP

#include <inttypes.h>
#include <sigc++/sigc++.h>
#include "raul/Path.hpp"
#include "interface/ClientInterface.hpp"

namespace Ingen {
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This simply emits an sigc signal for every event (eg OSC message) coming from
 * the engine.  Use Store (which extends this) if you want a nice client-side
 * model of the engine.
 *
 * The signals here match the calls to ClientInterface exactly.  See the
 * documentation for ClientInterface for meanings of signal parameters.
 */
class SigClientInterface : public Ingen::Shared::ClientInterface, public sigc::trackable
{
public:
	SigClientInterface() {}

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#internal"; }

	sigc::signal<void, int32_t>                                     signal_response_ok;
	sigc::signal<void, int32_t, std::string>                        signal_response_error;
	sigc::signal<void>                                              signal_bundle_begin;
	sigc::signal<void>                                              signal_bundle_end;
	sigc::signal<void, std::string>                                 signal_error;
	sigc::signal<void, Raul::Path, uint32_t>                        signal_new_patch;
	sigc::signal<void, Raul::Path, Raul::URI, uint32_t, bool>       signal_new_port;
	sigc::signal<void, Raul::URI, Shared::Resource::Properties>     signal_put;
	sigc::signal<void, Raul::URI, Shared::Resource::Properties,
	                              Shared::Resource::Properties>     signal_delta;
	sigc::signal<void, Raul::Path, Raul::Path>                      signal_object_moved;
	sigc::signal<void, Raul::Path>                                  signal_object_deleted;
	sigc::signal<void, Raul::Path, Raul::Path>                      signal_connection;
	sigc::signal<void, Raul::Path, Raul::Path>                      signal_disconnection;
	sigc::signal<void, Raul::URI, Raul::URI, Raul::Atom>            signal_variable_change;
	sigc::signal<void, Raul::URI, Raul::URI, Raul::Atom>            signal_property_change;
	sigc::signal<void, Raul::Path, Raul::Atom>                      signal_port_value;
	sigc::signal<void, Raul::Path, uint32_t, Raul::Atom>            signal_voice_value;
	sigc::signal<void, Raul::Path>                                  signal_activity;

	/** Fire pending signals.  Only does anything on derived classes (that may queue) */
	virtual bool emit_signals() { return false; }

protected:

	// ClientInterface hooks that fire the above signals

#define EMIT(name, ...) { signal_ ## name (__VA_ARGS__); }

	void bundle_begin()
		{ EMIT(bundle_begin); }

	void bundle_end()
		{ EMIT(bundle_end); }

	void transfer_begin() {}
	void transfer_end()   {}

	void response_ok(int32_t id)
		{ EMIT(response_ok, id); }

	void response_error(int32_t id, const std::string& msg)
		{ EMIT(response_error, id, msg); }

	void error(const std::string& msg)
		{ EMIT(error, msg); }

	void put(const Raul::URI& uri, const Shared::Resource::Properties& properties)
		{ EMIT(put, uri, properties); }

	void delta(const Raul::URI& uri,
			const Shared::Resource::Properties& remove, const Shared::Resource::Properties& add)
		{ EMIT(delta, uri, remove, add); }

	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path)
		{ EMIT(connection, src_port_path, dst_port_path); }

	void del(const Raul::Path& path)
		{ EMIT(object_deleted, path); }

	void move(const Raul::Path& old_path, const Raul::Path& new_path)
		{ EMIT(object_moved, old_path, new_path); }

	void disconnect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path)
		{ EMIT(disconnection, src_port_path, dst_port_path); }

	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value)
		{ EMIT(property_change, subject, key, value); }

	void set_voice_value(const Raul::Path& port_path, uint32_t voice, const Raul::Atom& value)
		{ EMIT(voice_value, port_path, voice, value); }

	void activity(const Raul::Path& port_path)
		{ EMIT(activity, port_path); }
};


} // namespace Client
} // namespace Ingen

#endif
