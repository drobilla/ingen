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

#include "config.h"
#include "OmApp.h"
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
	OmApp* om = 0;
#ifdef HAVE_LASH
	LashDriver* lash_driver = 0;
#endif
}
