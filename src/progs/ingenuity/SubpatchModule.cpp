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

#include "SubpatchModule.h"
#include <cassert>
#include <iostream>
#include "App.h"
#include "ModelEngineInterface.h"
#include "NodeModule.h"
#include "NodeControlWindow.h"
#include "PatchModel.h"
#include "PatchWindow.h"
#include "PatchCanvas.h"
#include "Port.h"
#include "WindowFactory.h"
using std::cerr; using std::cout; using std::endl;

namespace Ingenuity {


SubpatchModule::SubpatchModule(PatchCanvas* canvas, CountedPtr<PatchModel> patch)
: NodeModule(canvas, patch),
  m_patch(patch)
{
	assert(canvas);
	assert(patch);
}


void
SubpatchModule::on_double_click(GdkEventButton* event)
{
	assert(m_patch);

	CountedPtr<PatchModel> parent = PtrCast<PatchModel>(m_patch->parent());

	PatchWindow* const preferred = ( (parent && (event->state & GDK_SHIFT_MASK))
		? NULL
		: App::instance().window_factory()->patch_window(parent) );

	App::instance().window_factory()->present_patch(m_patch, preferred);
}



/** Browse to this patch in current (parent's) window
 * (unless an existing window is displaying it)
 */
void
SubpatchModule::browse_to_patch()
{
	assert(m_patch->parent());
	
	CountedPtr<PatchModel> parent = PtrCast<PatchModel>(m_patch->parent());

	PatchWindow* const preferred = ( (parent)
		? App::instance().window_factory()->patch_window(parent)
		: NULL );
	
	App::instance().window_factory()->present_patch(m_patch, preferred);
}



void
SubpatchModule::show_dialog()
{
	cerr << "FIXME: dialog\n";
	//m_patch->show_control_window();
}


void
SubpatchModule::menu_remove()
{
	App::instance().engine()->destroy(m_patch->path());
}

} // namespace Ingenuity
