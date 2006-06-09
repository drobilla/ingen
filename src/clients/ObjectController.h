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

#ifndef OBJECTCONTROLLER_H
#define OBJECTCONTROLLER_H

namespace LibOmClient {

	
/** A trivial base class for controllers of an ObjectModel.
 *
 * This is so ObjectModels can have pointers to app-specified controllers,
 * and the pointer relationships in models (ie PatchModel has pointers to
 * all it's NodeModel children, etc) can be used to find controllers of
 * Models, rather than having a parallel structure of pointers in the
 * app's controllers.
 *
 * \ingroup libomclient
 */
class ObjectController
{
public:
	virtual ~ObjectController() {}
};


} // namespace LibOmClient


#endif // OBJECTCONTROLLER_H
