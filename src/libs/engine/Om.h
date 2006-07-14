/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef OM_H
#define OM_H

// FIXME: this dependency has to go
#include "config.h"


/* Things all over the place access various bits of Om through this.
 * This was a bad design decision, it just evolved this way :/
 * I may refactor it eventually, but it works.
 */

/** \defgroup engine Engine
 */
namespace Om
{
	class OmApp;
	extern OmApp* om;
}

#endif // OM_H

