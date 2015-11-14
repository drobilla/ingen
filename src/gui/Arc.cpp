/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Arc.hpp"
#include "ingen/client/ArcModel.hpp"
#include "ingen/client/BlockModel.hpp"

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

namespace Ingen {
namespace GUI {

Arc::Arc(Ganv::Canvas&                canvas,
         SPtr<const Client::ArcModel> model,
         Ganv::Node*                  src,
         Ganv::Node*                  dst)
	: Ganv::Edge(canvas, src, dst)
	, _arc_model(model)
{
	SPtr<const Client::ObjectModel> tparent = model->tail()->parent();
	SPtr<const Client::BlockModel>  tparent_block;
	if ((tparent_block = dynamic_ptr_cast<const Client::BlockModel>(tparent))) {
		if (tparent_block->plugin_uri() == NS_INTERNALS "BlockDelay") {
			g_object_set(_gobj, "dash-length", 4.0, NULL);
			set_constraining(false);
		}
	}
}

} // namespace GUI
} // namespace Ingen
