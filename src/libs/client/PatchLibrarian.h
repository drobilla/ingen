/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <string>
#include <libxml/tree.h>
#include <cassert>
//#include "DummyModelClientInterface.h"

using std::string;

namespace LibOmClient {

class PatchModel;
class NodeModel;
class ConnectionModel;
class PresetModel;
class OSCModelEngineInterface;
class ModelClientInterface;

	
/** Handles all patch saving and loading.
 *
 * \ingroup libomclient
 */
class PatchLibrarian
{
public:
	// FIXME: return booleans and set an errstr that can be checked?
	
	PatchLibrarian(OSCModelEngineInterface* const osc_model_engine_interface/*,ModelClientInterface* const client_hooks*/)
	: m_patch_path("."), m_osc_model_engine_interface(osc_model_engine_interface)//, m_client_hooks(client_hooks)
	{
		assert(m_osc_model_engine_interface);
		//assert(m_client_hooks != NULL);
	}

//	PatchLibrarian(OSCModelEngineInterface* osc_model_engine_interface)
//	: m_osc_model_engine_interface(osc_model_engine_interface), m_client_hooks(new DummyModelClientInterface())
//	{}

	void          path(const string& path) { m_patch_path = path; }
	const string& path()                   { return m_patch_path; }
	
	string find_file(const string& filename, const string& additional_path = "");
	
	void save_patch(PatchModel* patch_model, const string& filename, bool recursive);
	string load_patch(PatchModel* pm, bool wait = true, bool existing = false);

private:
	string                      m_patch_path;
	OSCModelEngineInterface* const        m_osc_model_engine_interface;

	NodeModel*       parse_node(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	ConnectionModel* parse_connection(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	PresetModel*     parse_preset(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	void             load_subpatch(PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
};


} // namespace LibOmClient

#endif // PATCHLIBRARIAN_H
