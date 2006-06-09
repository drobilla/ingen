/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GTKCLIENTHOOKS_H
#define GTKCLIENTHOOKS_H

#include <cassert>
#include <string>
#include <queue>
#include <iostream>
#include <sigc++/sigc++.h>
#include "ControlInterface.h"
#include "OSCListener.h"
#include "util/Queue.h"
#include "util/CountedPtr.h"
#include "ModelClientInterface.h"
#include "SigClientInterface.h"
using std::string;
using std::cerr; using std::endl;

namespace LibOmClient { class PluginModel; }
using namespace LibOmClient;

namespace OmGtk {

/** Returns nothing and takes no parameters (because they have been bound) */
typedef sigc::slot<void> Closure;

#if 0
/** ModelClientInterface implementation for the Gtk client.
 *
 * This is a threadsafe interface to OmGtk.  It provides the same interface as
 * @ref ControlInterface, except all public functions may be called in a thread
 * other than the GTK thread (a closure will be created and pushed through a
 * queue for the GTK thread to execute).
 *
 * It is also a ModelClientInterface, which is how it is driven by the engine.
 * This is redundant, "ControlInterface" needs to go away. A model database
 * that wraps a ClientInterface and emits sigc signals when things change would
 * be a much better way of doing this.
 *
 * \ingroup OmGtk
 */
class GtkClientInterface : public OSCListener, public ModelClientInterface
{
public:
	GtkClientInterface(ControlInterface* interface, int client_port);
	
	void set_ignore_port(const string& path) { _ignore_port_path = path; }
	void clear_ignore_port()                 { _ignore_port_path = ""; }
	
	// FIXME: ugly accessor
	size_t num_plugins() const { return _num_plugins; }

	// OSC thread functions (deferred calls)
	
	void bundle_begin() {}
	void bundle_end()   {}

	void num_plugins(size_t num) { _num_plugins = num; }

	void error(const string& msg)
		{ push_event(sigc::bind(_interface->error_slot, msg)); }
	
	void new_plugin_model(PluginModel* const pm)
		{ push_event(sigc::bind(_interface->new_plugin_slot, pm)); }
	
	void new_patch_model(PatchModel* const pm)
		{ push_event(sigc::bind(_interface->new_patch_slot, pm)); }

	void new_node_model(NodeModel* const nm)
		{ assert(nm); push_event(sigc::bind(_interface->new_node_slot, nm)); }

	void new_port_model(PortModel* const pm)
		{ push_event(sigc::bind(_interface->new_port_slot, pm)); }
	
	void connection_model(ConnectionModel* const cm)
		{ push_event(sigc::bind(_interface->connection_slot, cm)); }
	
	void object_destroyed(const string& path)
		{ push_event(sigc::bind(_interface->object_destroyed_slot, path)); }
	
	void patch_enabled(const string& path)
		{ push_event(sigc::bind(_interface->patch_enabled_slot, path)); }
	
	void patch_disabled(const string& path)
		{ push_event(sigc::bind(_interface->patch_disabled_slot, path)); }

	void patch_cleared(const string& path)
		{ push_event(sigc::bind(_interface->patch_cleared_slot, path)); }

	void object_renamed(const string& old_path, const string& new_path)
		{ push_event(sigc::bind(_interface->object_renamed_slot, old_path, new_path)); }
	
	void disconnection(const string& src_port_path, const string& dst_port_path)
		{ push_event(sigc::bind(_interface->disconnection_slot, src_port_path, dst_port_path)); }
	
	void metadata_update(const string& path, const string& key, const string& value)
		{ push_event(sigc::bind(_interface->metadata_update_slot, path, key, value)); }

	void control_change(const string& port_path, float value)
		{ push_event(sigc::bind(_interface->control_change_slot, port_path, value)); }

	void program_add(const string& path, uint32_t bank, uint32_t program, const string& name)
		{ push_event(sigc::bind(_interface->program_add_slot, path, bank, program, name)); }
	
	void program_remove(const string& path, uint32_t bank, uint32_t program)
		{ push_event(sigc::bind(_interface->program_remove_slot, path, bank, program)); }

	/** Process all queued events - MUST be called from Gtk thread.
	 * Registered as a GTK idle handler by App. */
	bool process_events();

	static void instantiate(ControlInterface* interface, int client_port)
	{ if (!_instance) _instance = new GtkClientInterface(interface, client_port); }
	
	inline static CountedPtr<GtkClientInterface> instance()
	{ assert(_instance); return _instance; }

private:
	
	static CountedPtr<GtkClientInterface> _instance;

	/** Provides the functions/slots the closures will actually call in the GTK thread */
	ControlInterface* _interface;

	size_t _num_plugins;

	void push_event(Closure ev);

	/** Set if a port slider is grabbed and is being dragged.
	 * If a control event comes in for a port with this path, we'll just
	 * ignore it outright.  (Just an optimization over doing all the searching
	 * for the port slider just to ignore the event) */
	string _ignore_port_path;

	Queue<Closure> _events;
};
#endif

} // namespace OmGtk

#endif // GTKCLIENTHOOKS_H
