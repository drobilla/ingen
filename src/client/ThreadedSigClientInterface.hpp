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

#ifndef INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP

#include <inttypes.h>
#include <string>
#include <sigc++/sigc++.h>
#include <glibmm/thread.h>
#include "raul/Atom.hpp"
#include "interface/ClientInterface.hpp"
#include "interface/MessageType.hpp"
#include "SigClientInterface.hpp"
#include "raul/SRSWQueue.hpp"

/** Returns nothing and takes no parameters (because they have all been bound) */
typedef sigc::slot<void> Closure;

namespace Ingen {
namespace Shared { class EngineInterface; }
namespace Client {


/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This emits signals (possibly) in a different thread than the ClientInterface
 * functions are called.  It must be explicitly driven with the emit_signals()
 * function, which fires all enqueued signals up until the present.  You can
 * use this in a GTK idle callback for receiving thread safe engine signals.
 */
class ThreadedSigClientInterface : public SigClientInterface
{
public:
	ThreadedSigClientInterface(uint32_t queue_size)
	: _sigs(queue_size)
	, response_ok_slot(signal_response_ok.make_slot())
	, response_error_slot(signal_response_error.make_slot())
	, error_slot(signal_error.make_slot())
	, new_port_slot(signal_new_port.make_slot())
	, put_slot(signal_put.make_slot())
	, connection_slot(signal_connection.make_slot())
	, object_deleted_slot(signal_object_deleted.make_slot())
	, object_moved_slot(signal_object_moved.make_slot())
	, disconnection_slot(signal_disconnection.make_slot())
	, variable_change_slot(signal_variable_change.make_slot())
	, property_change_slot(signal_property_change.make_slot())
	, port_value_slot(signal_port_value.make_slot())
	, activity_slot(signal_activity.make_slot())
	, binding_slot(signal_binding.make_slot())
	{
	}

	virtual Raul::URI uri() const { return "ingen:internal"; }

	void bundle_begin()
		{ push_sig(bundle_begin_slot); }

	void bundle_end()
		{ push_sig(bundle_end_slot); }

	void transfer_begin() {}
	void transfer_end()   {}

	void response_ok(int32_t id)
		{ push_sig(sigc::bind(response_ok_slot, id)); }

	void response_error(int32_t id, const std::string& msg)
		{ push_sig(sigc::bind(response_error_slot, id, msg)); }

	void error(const std::string& msg)
		{ push_sig(sigc::bind(error_slot, msg)); }

	void put(const Raul::URI& path, const Shared::Resource::Properties& properties)
		{ push_sig(sigc::bind(put_slot, path, properties)); }

	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path)
		{ push_sig(sigc::bind(connection_slot, src_port_path, dst_port_path)); }

	void del(const Raul::Path& path)
		{ push_sig(sigc::bind(object_deleted_slot, path)); }

	void move(const Raul::Path& old_path, const Raul::Path& new_path)
		{ push_sig(sigc::bind(object_moved_slot, old_path, new_path)); }

	void disconnect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path)
		{ push_sig(sigc::bind(disconnection_slot, src_port_path, dst_port_path)); }

	void set_property(const Raul::URI& subject, const Raul::URI& key, const Raul::Atom& value)
		{ push_sig(sigc::bind(property_change_slot, subject, key, value)); }

	void set_voice_value(const Raul::Path& port_path, uint32_t voice, const Raul::Atom& value)
		{ push_sig(sigc::bind(voice_value_slot, port_path, voice, value)); }

	void activity(const Raul::Path& port_path)
		{ push_sig(sigc::bind(activity_slot, port_path)); }

	void binding(const Raul::Path& path, const Shared::MessageType& type)
		{ push_sig(sigc::bind(binding_slot, path, type)); }

	/** Process all queued events - Called from GTK thread to emit signals. */
	bool emit_signals();

private:
	void push_sig(Closure ev);

	Glib::Mutex _mutex;
	Glib::Cond  _cond;

	Raul::SRSWQueue<Closure> _sigs;
	bool                     _attached;

	sigc::slot<void>                                              bundle_begin_slot;
	sigc::slot<void>                                              bundle_end_slot;
	sigc::slot<void, int32_t>                                     response_ok_slot;
	sigc::slot<void, int32_t, std::string>                        response_error_slot;
	sigc::slot<void, std::string>                                 error_slot;
	sigc::slot<void, Raul::URI, Raul::URI, Raul::Symbol>          new_plugin_slot;
	sigc::slot<void, Raul::Path, Raul::URI, uint32_t, bool>       new_port_slot;
	sigc::slot<void, Raul::URI, Shared::Resource::Properties>     put_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                      connection_slot;
	sigc::slot<void, Raul::Path>                                  object_deleted_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                      object_moved_slot;
	sigc::slot<void, Raul::Path, Raul::Path>                      disconnection_slot;
	sigc::slot<void, Raul::URI, Raul::URI, Raul::Atom>            variable_change_slot;
	sigc::slot<void, Raul::URI, Raul::URI, Raul::Atom>            property_change_slot;
	sigc::slot<void, Raul::Path, Raul::Atom>                      port_value_slot;
	sigc::slot<void, Raul::Path, uint32_t, Raul::Atom>            voice_value_slot;
	sigc::slot<void, Raul::Path>                                  activity_slot;
	sigc::slot<void, Raul::Path, Shared::MessageType>             binding_slot;
};


} // namespace Client
} // namespace Ingen

#endif
