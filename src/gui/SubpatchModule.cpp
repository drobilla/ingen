/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <utility>

#include "ingen/Interface.hpp"
#include "ingen/client/PatchModel.hpp"

#include "App.hpp"
#include "NodeModule.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "SubpatchModule.hpp"
#include "WindowFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Client;

namespace GUI {

SubpatchModule::SubpatchModule(PatchCanvas&                canvas,
                               SharedPtr<const PatchModel> patch)
	: NodeModule(canvas, patch)
	, _patch(patch)
{
	assert(patch);
}

bool
SubpatchModule::on_double_click(GdkEventButton* event)
{
	assert(_patch);

	SharedPtr<PatchModel> parent = PtrCast<PatchModel>(_patch->parent());

	PatchWindow* const preferred = ( (parent && (event->state & GDK_SHIFT_MASK))
		? NULL
		: app().window_factory()->patch_window(parent) );

	app().window_factory()->present_patch(_patch, preferred);
	return true;
}

void
SubpatchModule::store_location(double ax, double ay)
{
	const Atom x(app().forge().make(static_cast<float>(ax)));
	const Atom y(app().forge().make(static_cast<float>(ay)));

	const Shared::URIs& uris = app().uris();

	const Atom& existing_x = _node->get_property(uris.ingen_canvasX);
	const Atom& existing_y = _node->get_property(uris.ingen_canvasY);

	if (x != existing_x && y != existing_y) {
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvasX, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvasY, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvasX,
		                     Resource::Property(x, Resource::EXTERNAL)));
		add.insert(make_pair(uris.ingen_canvasY,
		                     Resource::Property(y, Resource::EXTERNAL)));
		app().engine()->delta(_node->path(), remove, add);
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

	PatchWindow* const preferred = (parent)
		? app().window_factory()->patch_window(parent)
		: NULL;

	app().window_factory()->present_patch(_patch, preferred);
}

void
SubpatchModule::menu_remove()
{
	app().engine()->del(_patch->path());
}

} // namespace GUI
} // namespace Ingen
