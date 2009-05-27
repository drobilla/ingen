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

#ifndef QUEUEDENGINEINTERFACE_H
#define QUEUEDENGINEINTERFACE_H

#include <inttypes.h>
#include <string>
#include <memory>
#include "raul/SharedPtr.hpp"
#include "interface/ClientInterface.hpp"
#include "interface/EngineInterface.hpp"
#include "interface/Resource.hpp"
#include "QueuedEventSource.hpp"
#include "Responder.hpp"
#include "tuning.hpp"
#include "types.hpp"

namespace Ingen {

class Engine;


/** A queued (preprocessed) event source / interface.
 *
 * This is the bridge between the EngineInterface presented to the client, and
 * the EventSource that needs to be presented to the AudioDriver.
 *
 * Responses occur through the event mechanism (which notified clients in
 * event post_process methods) and are related to an event by an integer ID.
 * If you do not register a responder, you have no way of knowing if your calls
 * are successful.
 */
class QueuedEngineInterface : public QueuedEventSource, public Shared::EngineInterface
{
public:
	QueuedEngineInterface(Engine& engine, size_t queue_size);
	virtual ~QueuedEngineInterface() {}

	Raul::URI uri() const { return "ingen:internal"; }

	void set_next_response_id(int32_t id);

	// Client registration
	virtual void register_client(Shared::ClientInterface* client);
	virtual void unregister_client(const Raul::URI& uri);

	// Engine commands
	virtual void load_plugins();
	virtual void activate();
	virtual void deactivate();
	virtual void quit();

	// Bundles
	virtual void bundle_begin();
	virtual void bundle_end();

	// CommonInterface object commands

	virtual void put(const Raul::Path&                   path,
	                 const Shared::Resource::Properties& properties);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);

	virtual void set_variable(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_port_value(const Raul::Path&  port_path,
	                            const Raul::Atom&  value);

	virtual void set_voice_value(const Raul::Path& port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);

	virtual void del(const Raul::Path& path);

	virtual void clear_patch(const Raul::Path& patch_path);

	// EngineInterface object commands

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void midi_learn(const Raul::Path& node_path);

	// Requests //

	virtual void ping();
	virtual void request_plugin(const Raul::URI& uri);
	virtual void request_object(const Raul::Path& path);
	virtual void request_variable(const Raul::URI& object_path, const Raul::URI& key);
	virtual void request_property(const Raul::URI& object_path, const Raul::URI& key);
	virtual void request_plugins();
	virtual void request_all_objects();

protected:

	virtual void disable_responses();

	SharedPtr<Responder> _responder; ///< NULL if responding disabled
	Engine&              _engine;
	bool                 _in_bundle; ///< True iff a bundle is currently being received

private:
	SampleCount now() const;
};


} // namespace Ingen

extern "C" {
	extern Ingen::QueuedEngineInterface* new_queued_interface(Ingen::Engine& engine);
}

#endif // QUEUEDENGINEINTERFACE_H

