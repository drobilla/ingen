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

#include <ganv/Edge.hpp>
#include <ingen/URI.hpp>
#include <ingen/client/ArcModel.hpp>
#include <ingen/client/BlockModel.hpp>
#include <ingen/client/ObjectModel.hpp>
#include <ingen/client/PortModel.hpp>

#include <glib-object.h>

#include <memory>

#define NS_INTERNALS "http://drobilla.net/ns/ingen-internals#"

namespace ingen::gui {

Arc::Arc(Ganv::Canvas&                                  canvas,
         const std::shared_ptr<const client::ArcModel>& model,
         Ganv::Node*                                    src,
         Ganv::Node*                                    dst)
    : Ganv::Edge(canvas, src, dst), _arc_model(model)
{
	const std::shared_ptr<const client::ObjectModel> tparent = model->tail()->parent();
	std::shared_ptr<const client::BlockModel>        tparent_block;
	if ((tparent_block = std::dynamic_pointer_cast<const client::BlockModel>(tparent))) {
		if (tparent_block->plugin_uri() == NS_INTERNALS "BlockDelay") {
			g_object_set(_gobj, "dash-length", 4.0, nullptr);
			set_constraining(false);
		}
	}
}

} // namespace ingen::gui
