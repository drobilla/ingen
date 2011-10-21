/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <cassert>
#include <utility>

#include "ingen/ServerInterface.hpp"
#include "ingen/client/PatchModel.hpp"

#include "shared/LV2URIMap.hpp"

#include "App.hpp"
#include "NodeControlWindow.hpp"
#include "NodeModule.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "SubpatchModule.hpp"
#include "WindowFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {

SubpatchModule::SubpatchModule(PatchCanvas&                canvas,
                               SharedPtr<const PatchModel> patch)
	: NodeModule(canvas, patch)
	, _patch(patch)
{
	assert(patch);
}

void
SubpatchModule::on_double_click(GdkEventButton* event)
{
	assert(_patch);

	SharedPtr<PatchModel> parent = PtrCast<PatchModel>(_patch->parent());

	PatchWindow* const preferred = ( (parent && (event->state & GDK_SHIFT_MASK))
		? NULL
		: App::instance().window_factory()->patch_window(parent) );

	App::instance().window_factory()->present_patch(_patch, preferred);
}

void
SubpatchModule::store_location()
{
	const Atom x(static_cast<float>(property_x()));
	const Atom y(static_cast<float>(property_y()));

	const LV2URIMap& uris = App::instance().uris();

	const Atom& existing_x = _node->get_property(uris.ingenui_canvas_x);
	const Atom& existing_y = _node->get_property(uris.ingenui_canvas_y);

	if (x != existing_x && y != existing_y) {
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingenui_canvas_x, uris.wildcard));
		remove.insert(make_pair(uris.ingenui_canvas_y, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingenui_canvas_x,
		                     Resource::Property(x, Resource::EXTERNAL)));
		add.insert(make_pair(uris.ingenui_canvas_y,
		                     Resource::Property(y, Resource::EXTERNAL)));
		App::instance().engine()->delta(_node->path(), remove, add);
	}
}

/** Browse to this patch in current (parent's) window
 * (unless an existing window is displaying it)
 */
void
SubpatchModule::browse_to_patch()
{
	assert(_patch->parent());

	SharedPtr<PatchModel> parent = PtrCast<PatchModel>(_patch->parent());

	PatchWindow* const preferred = ( (parent)
		? App::instance().window_factory()->patch_window(parent)
		: NULL );

	App::instance().window_factory()->present_patch(_patch, preferred);
}

void
SubpatchModule::menu_remove()
{
	App::instance().engine()->del(_patch->path());
}

} // namespace GUI
} // namespace Ingen
