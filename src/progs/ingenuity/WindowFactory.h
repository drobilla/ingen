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

#ifndef WINDOW_FACTORY_H
#define WINDOW_FACTORY_H

#include <map>
#include "util/CountedPtr.h"
#include "PatchController.h"

namespace Ingenuity {

class PatchWindow;


class WindowFactory {
public:
	void present(CountedPtr<PatchController> patch, PatchWindow* preferred = NULL);

private:
	PatchWindow* create_new(CountedPtr<PatchController> patch);
	bool remove(PatchWindow* win, GdkEventAny* ignored);

	std::map<Path, PatchWindow*> _windows;
};

}

#endif // WINDOW_FACTORY_H
