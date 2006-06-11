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

#include "GtkObjectController.h"

namespace OmGtk {

	
GtkObjectController::GtkObjectController(CountedPtr<ObjectModel> model)
: m_model(model)
{
	assert(m_model);
}


/** Derived classes should override this to handle special metadata
 * keys, then call this to set the model's metadata key.
 */
void
GtkObjectController::metadata_update(const string& key, const string& value)
{
	m_model->set_metadata(key, value);
}


} // namespace OmGtk

