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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <string>
#include <libxml/tree.h>
#include "util/Path.h"
#include "EngineInterface.h"
using std::string;
using Om::Path;
using Om::Shared::EngineInterface;

namespace LibOmClient {

	
/** Loads patch files, sending the necessary messages to an EngineInterface
 * to recreate the patch.
 *
 * Because this only uses EngineInterface, this same code can be used to load
 * patches client side or server side.
 *
 * \ingroup libomclient
 */
class PatchLibrarian
{
public:

	//static void save_patch(PatchModel* patch_model, const string& filename, bool recursive);
	
	static bool
	PatchLibrarian::load_patch(EngineInterface* engine,
	                           const string&    filename,
	                           const Path&      parent_path = "/",
	                           string           name = "",
	                           uint32_t         poly = 0);

private:
	PatchLibrarian() {}

	/*
	static NodeModel*       parse_node(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	static ConnectionModel* parse_connection(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	static PresetModel*     parse_preset(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	static void             load_subpatch(PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	*/
};


} // namespace LibOmClient

#endif // PATCHLIBRARIAN_H
