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

#include "PatchModel.h"
#include "PatchController.h"
#include "ControlInterface.h"
#include "OSCModelEngineInterface.h"
#include "OSCListener.h"
//#include "GtkClientInterface.h"
#include "PatchLibrarian.h"
#include "Controller.h"
#include "Loader.h"
#include "interface/ClientKey.h"

namespace OmGtk {


Controller::Controller(const string& engine_url)
: OSCModelEngineInterface(engine_url),
  m_patch_librarian(new PatchLibrarian(this)),
  m_loader(new Loader(m_patch_librarian))
{
	m_loader->launch();
}


Controller::~Controller()
{
	if (is_attached()) {
		unregister_client(ClientKey()); // FIXME
		detach();
	}

	delete m_loader;
	delete m_patch_librarian;
}


/** "Attach" to the Om engine.
 * See documentation OSCModelEngineInterface::attach.
 */
void
Controller::attach()
{
	OSCModelEngineInterface::attach(false);
}

/*
void
Controller::register_client_and_wait()
{
//	int id = get_next_request_id();
//	set_wait_response_id(id);
	OSCModelEngineInterface::register_client();
//	wait_for_response();
	cout << "[Controller] Registered with engine.  maybe.  fixme." << endl;
}
*/
void   Controller::set_engine_url(const string& url) { _engine_url = url; }

void
Controller::create_node_from_model(const NodeModel* nm)
{
	//push_added_node(nm);
	OSCModelEngineInterface::create_node_from_model(nm);
	char temp_buf[16];
	snprintf(temp_buf, 16, "%f", nm->x());
	set_metadata(nm->path(), "module-x", temp_buf);
	snprintf(temp_buf, 16, "%f", nm->y());
	set_metadata(nm->path(), "module-y", temp_buf);
}

void
Controller::create_patch_from_model(const PatchModel* pm)
{ 
	//push_added_patch(pm);

	//int id = get_next_request_id();
	//set_wait_response_id(id);
	create_patch_from_model(pm);
	if (pm->parent()) {
	//	wait_for_response();
		char temp_buf[16];
		snprintf(temp_buf, 16, "%f", pm->x());
		set_metadata(pm->path(), "module-x", temp_buf);
		snprintf(temp_buf, 16, "%f", pm->y());
		set_metadata(pm->path(), "module-y", temp_buf);
	}
	enable_patch(pm->path());
}


void
Controller::set_patch_path(const string& path)
{
	m_patch_librarian->path(path);
}


/** Load patch in a seperate thread.
 * This will return immediately and the patch will be loaded in the background.
 * FIXME: merge parameter makes no sense, always true */
void
Controller::load_patch(PatchModel* model, bool wait, bool merge)
{
	//push_added_patch(model);
	m_loader->load_patch(model, wait, merge);
}


/** Load patch in a seperate thread.
 * This will return immediately and the patch will be saved in the background. */
void
Controller::save_patch(PatchModel* model, const string& filename, bool recursive)
{
	m_loader->save_patch(model, filename, recursive);
}


#if 0
/** Returns the added node with the given path and removes it from the cache.
 */
NodeModel*
Controller::yank_added_node(const string& path)
{
	NodeModel* nm = NULL;
	
	for (list<NodeModel*>::iterator i = m_added_nodes.begin(); i != m_added_nodes.end(); ++i) {
		if ((*i)->path() == path) {
			nm = *i;
			m_added_nodes.erase(i);
			break;
		}
	}

	return nm;
}


/** Returns the added patch with the given path and removes it from the cache.
 */
PatchModel*
Controller::yank_added_patch(const string& path)
{
	PatchModel* pm = NULL;
	
	for (list<PatchModel*>::iterator i = m_added_patches.begin(); i != m_added_patches.end(); ++i) {
		if ((*i)->path() == path) {
			pm = *i;
			m_added_patches.erase(i);
			break;
		}
	}

	return pm;
}
#endif

} // namespace OmGtk

