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


#include "ConsoleClientHooks.h"

#include <iostream>


void
ConsoleClientHooks::new_node(NodeModel* nm)
{
	std::cerr << "[ConsoleClient] New node notification" << std::endl;
}

void
ConsoleClientHooks::error(std::string msg)
{
	std::cerr << "Server returned an error: " << msg << std::endl;
}
