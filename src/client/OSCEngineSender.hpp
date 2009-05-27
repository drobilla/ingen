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

#ifndef OSCENGINESENDER_H
#define OSCENGINESENDER_H

#include <inttypes.h>
#include <string>
#include <lo/lo.h>
#include "interface/EngineInterface.hpp"
#include "shared/OSCSender.hpp"

namespace Ingen {
namespace Client {


/* OSC (via liblo) interface to the engine.
 *
 * Clients can use this opaquely as an EngineInterface* to control the engine
 * over OSC (whether over a network or not, etc).
 *
 * \ingroup IngenClient
 */
class OSCEngineSender : public Shared::EngineInterface, public Shared::OSCSender {
public:
	OSCEngineSender(const Raul::URI& engine_url);
	~OSCEngineSender();

	static OSCEngineSender* create(const Raul::URI& engine_url) {
		return new OSCEngineSender(engine_url);
	}

	Raul::URI uri() const { return _engine_url; }

	inline int32_t next_id()
	{ int32_t ret = (_id == -1) ? -1 : _id++; return ret; }

	void set_next_response_id(int32_t id) { _id = id; }
	void disable_responses() { _id = -1; }

	void attach(int32_t ping_id, bool block);


	/* *** EngineInterface implementation below here *** */

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin()   { OSCSender::bundle_begin(); }
	void bundle_end()     { OSCSender::bundle_end(); }
	void transfer_begin() { OSCSender::transfer_begin(); }
	void transfer_end()   { OSCSender::transfer_end(); }

	// Client registration
	void register_client(Shared::ClientInterface* client);
	void unregister_client(const Raul::URI& uri);

	// Engine commands
	void load_plugins();
	void activate();
	void deactivate();
	void quit();

	// Object commands

	virtual void put(const Raul::URI&                    path,
	                 const Shared::Resource::Properties& properties);

	virtual void clear_patch(const Raul::Path& path);

	virtual void del(const Raul::Path& path);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_port_value(const Raul::Path& port_path,
	                            const Raul::Atom& value);

	virtual void set_voice_value(const Raul::Path& port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);

	virtual void midi_learn(const Raul::Path& node_path);


	// Requests //
	void ping();
	void request_plugin(const Raul::URI& uri);
	void request_object(const Raul::Path& path);
	void request_variable(const Raul::URI& path, const Raul::URI& key);
	void request_property(const Raul::URI& path, const Raul::URI& key);
	void request_plugins();
	void request_all_objects();

protected:
	const Raul::URI _engine_url;
	int             _client_port;
	int32_t         _id;
};


} // namespace Client
} // namespace Ingen

#endif // OSCENGINESENDER_H

