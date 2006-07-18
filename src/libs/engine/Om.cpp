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

#include "config.h"
#include "Ingen.h"
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif

/** Om namespace.
 *
 * Everything internal to the Om engine lives in this namespace.  Generally
 * none of this code is used by clients - as special (possibly temporary)
 * exceptions, some classes in src/common are used by clients but are part
 * of the Om namespace.
 */
namespace Om
{
	Ingen* om = 0;
}
