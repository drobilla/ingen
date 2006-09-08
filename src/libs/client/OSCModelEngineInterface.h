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

#ifndef OSCCONTROLLER_H
#define OSCCONTROLLER_H

#include <string>
#include <lo/lo.h>
#include "util/Semaphore.h"
#include "interface/EngineInterface.h"
#include "OSCEngineSender.h"
#include "ModelEngineInterface.h"
using std::string;

/** \defgroup IngenClient Client Library
 */

namespace Ingen {
namespace Client {

class NodeModel;
class PresetModel;
class PatchModel;
class ModelClientInterface;


/** Old model-based OSC engine command interface.
 *
 * This is an old class from before when the well-defined interfaces between
 * engine and client were defined.  I've wrapped it around OSCEngineSender
 * so all the common functions are implemented there, and implemented the
 * remaining functions using those, for compatibility.  Hopefully something
 * better gets figured out and this can go away completely, but for now this
 * gets the existing clients working through EngineInterface in the easiest
 * way possible.  This class needs to die.
 *
 * \ingroup IngenClient
 */
class OSCModelEngineInterface : public OSCEngineSender, public ModelEngineInterface
{
public:
	//OSCModelEngineInterface(ModelClientInterface* const client_hooks, const string& engine_url);
	OSCModelEngineInterface(const string& engine_url);
	~OSCModelEngineInterface();
	
	void attach(bool block = true);
	void detach();

	bool is_attached() { return m_is_attached; }
	
	// FIXME: reimplement
	void set_wait_response_id(int32_t id) {}
	bool wait_for_response() { return false; }
	int  get_next_request_id() { return m_request_id++; }
	
	/* *** Model alternatives to EngineInterface functions below *** */

	void create_patch_from_model(const PatchModel* pm);
	void create_node_from_model(const NodeModel* nm);

	void set_all_metadata(const NodeModel* nm);
	void set_preset(const string& patch_path, const PresetModel* const pm);

protected:
	void start_listen_thread();
	
	int m_request_id;

	bool m_is_attached;
	bool m_is_registered;
	/*
	bool m_blocking;
	bool m_waiting_for_response;
	int  m_wait_response_id;
	bool m_response_received;
	bool m_wait_response_was_affirmative;
	
	Semaphore m_response_semaphore;
	*/
private:
	// Prevent copies
	OSCModelEngineInterface(const OSCModelEngineInterface& copy);
	OSCModelEngineInterface& operator=(const OSCModelEngineInterface& copy);
};
/*
inline int
OSCModelEngineInterface::om_response_ok_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* comm) {
	return ((OSCModelEngineInterface*)comm)->m_om_response_ok_cb(path, types, argv, argc, data);
}

inline int
OSCModelEngineInterface::om_response_error_cb(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* comm) {
	return ((OSCModelEngineInterface*)comm)->m_om_response_error_cb(path, types, argv, argc, data);
}
*/
} // namespace Client
} // namespace Ingen

#endif // OSCCONTROLLER_H
