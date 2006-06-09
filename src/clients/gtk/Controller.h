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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cassert>
#include <string>
#include <list>
#include <glibmm.h>
#include "PluginModel.h"
#include "OSCModelEngineInterface.h"

namespace LibOmClient {
class PatchModel;
class NodeModel;
class PatchLibrarian;
}

using std::string; using std::list;
using namespace LibOmClient;

namespace OmGtk {

class PatchController;
//class GtkClientInterface;
class Loader;

	
/** Singleton engine controller for the entire app.
 *
 * This is hardly more than a trivial wrapper for OSCModelEngineInterface, suggesting something
 * needs a rethink around here..
 *
 * This needs to be either eliminated or the name changed to OmController.  Probably
 * best to keep around in case multiple engine control support comes around?
 *
 * \ingroup OmGtk
 */
class Controller : public OSCModelEngineInterface
{
public:
	~Controller();

	void attach();
	
	//void register_client_and_wait();

	void set_engine_url(const string& url);

	void create_node_from_model(const NodeModel* nm);

	void load_patch(PatchModel* model, bool wait = true, bool merge = false);
	void save_patch(PatchModel* model, const string& filename, bool recursive);

	void create_patch_from_model(const PatchModel* pm);

	//void lash_restore_finished();

	void set_patch_path(const string& path);

	/*
	void push_added_node(NodeModel* nm) { m_added_nodes.push_back(nm); }
	NodeModel* yank_added_node(const string& path);

	void push_added_patch(PatchModel* pm) { m_added_patches.push_back(pm); }
	PatchModel* yank_added_patch(const string& path);
	*/
	//GtkClientInterface*   client_hooks()      { return (GtkClientInterface*)m_client_hooks; }

	static void instantiate(const string& engine_url)
	{ if (!_instance) _instance = new Controller(engine_url); }

	inline static Controller& instance() { assert(_instance); return *_instance; }

private:
	Controller(const string& engine_url);
	static Controller* _instance;
	
	PatchLibrarian*   m_patch_librarian;
	Loader*           m_loader;

	/** Used to cache added nodes so client can remember some values when the
	 * new node notification comes (location, etc).  Used to prevent the node
	 * jumping locations during the delay between new node and the module-x/y
	 * metadata notifications */
	//list<NodeModel*> m_added_nodes;

	/** Used to cache added patches in the same was as m_added_nodes.  Used to
	 * rember filename so File->Save can work without prompting (filename can't
	 * be sent to the server as metadata, because a client on another machine
	 * doesn't want that information) */
	//list<PatchModel*> m_added_patches;
};


} // namespace OmGtk

#endif // CONTROLLER_H
