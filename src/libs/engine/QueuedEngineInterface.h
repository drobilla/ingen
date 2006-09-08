/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "util/CountedPtr.h"
#include "interface/EngineInterface.h"
#include "interface/ClientInterface.h"
#include "interface/ClientKey.h"
#include "QueuedEventSource.h"
#include "Responder.h"
using std::string;

namespace Ingen {

using Shared::ClientKey;
using Shared::ClientInterface;
using Shared::EngineInterface;
class Engine;


/** A queued (preprocessed) event source / interface.
 *
 * This is the bridge between the EngineInterface presented to the client, and
 * the EventSource that needs to be presented to the AudioDriver.
 *
 * This is sort of a state machine, \ref set_responder sets the Responder that
 * will be used to send the response from all future function calls.  Stateless
 * protocols like UDP/OSC can use this to set a different response address for
 * each event (eg incoming UDP port), but engine/client interfaces that don't
 * need to change an 'address' constantly can just set it once on initialisation.
 * Blocking control interfaces can be made by setting a Responder which signals
 * the caller when the 'response' is 'sent'.
 *
 * If you do not register a responder, you have no way of knowing if your calls
 * are successful.
 *
 * FIXME: this isn't really "queued" entirely, since some events aren't queued
 * events and get pushed directly into the realtime event queue.  Should that
 * be separated into a different interface/client?
 */
class QueuedEngineInterface : public QueuedEventSource, public EngineInterface
{
public:
	QueuedEngineInterface(Engine& engine, size_t queued_size, size_t stamped_size);
	virtual ~QueuedEngineInterface() {}
	
	virtual void set_responder(CountedPtr<Responder> responder);
	virtual void disable_responses();

	// Client registration
	virtual void register_client(ClientKey key, CountedPtr<ClientInterface> client);
	virtual void unregister_client(ClientKey key);
	

	// Engine commands
	virtual void load_plugins();
	virtual void activate();
	virtual void deactivate();
	virtual void quit();
			
	// Object commands
	
	virtual void create_patch(const string& path,
	                          uint32_t      poly);

	virtual void create_port(const string& path,
	                         const string& data_type,
	                         bool          direction);

	virtual void create_node(const string& path,
	                         const string& plugin_type,
	                         const string& plugin_uri,
				        	 bool          polyphonic);
	
	/** FIXME: DEPRECATED, REMOVE */
	virtual void create_node(const string& path,
	                         const string& plugin_type,
	                         const string& lib_path,
	                         const string& plug_label,
				        	 bool          polyphonic);

	virtual void rename(const string& old_path,
	                    const string& new_name);

	virtual void destroy(const string& path);

	virtual void clear_patch(const string& patch_path);

	virtual void enable_patch(const string& patch_path);

	virtual void disable_patch(const string& patch_path);

	virtual void connect(const string& src_port_path,
	                     const string& dst_port_path);

	virtual void disconnect(const string& src_port_path,
	                        const string& dst_port_path);

	virtual void disconnect_all(const string& node_path);

	virtual void set_port_value(const string& port_path,
	                            float         val);

	virtual void set_port_value(const string& port_path,
	                            uint32_t      voice,
	                            float         val);

	virtual void set_port_value_queued(const string& port_path,
	                                   float         val);

	virtual void set_program(const string& node_path,
	                         uint32_t      bank,
	                         uint32_t      program);

	virtual void midi_learn(const string& node_path);

	virtual void set_metadata(const string& path,
	                          const string& predicate,
	                          const string& value);
	
	// Requests //
	
	virtual void ping();

	virtual void request_port_value(const string& port_path);

	virtual void request_plugins();

	virtual void request_all_objects();

protected:

	/** Where responses to current messages will go. */
	CountedPtr<Responder> _responder;

private:
	SampleCount now() const;

	Engine& _engine;
};


} // namespace Ingen

#endif // QUEUEDENGINEINTERFACE_H

