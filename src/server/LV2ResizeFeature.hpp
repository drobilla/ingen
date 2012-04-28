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

#ifndef INGEN_ENGINE_LV2RESIZEFEATURE_HPP
#define INGEN_ENGINE_LV2RESIZEFEATURE_HPP

#include "raul/log.hpp"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "ingen/shared/LV2Features.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

struct ResizeFeature : public Ingen::Shared::LV2Features::Feature {
	static LV2_Resize_Port_Status resize_port(
		LV2_Resize_Port_Feature_Data data,
	    uint32_t                     index,
	    size_t                       size) {
		NodeImpl* node = (NodeImpl*)data;
		PortImpl* port = node->port_impl(index);
		if (node->context() == Context::MESSAGE) {
			port->buffer(0)->resize(size);
			port->connect_buffers();
			return LV2_RESIZE_PORT_SUCCESS;
		}
		return LV2_RESIZE_PORT_ERR_UNKNOWN;
	}

	static void delete_feature(LV2_Feature* feature) {
		free(feature->data);
		free(feature);
	}

	SharedPtr<LV2_Feature> feature(Shared::World* w, Node* n) {
		NodeImpl* node = dynamic_cast<NodeImpl*>(n);
		if (!node)
			return SharedPtr<LV2_Feature>();
		LV2_Resize_Port_Resize* data
				= (LV2_Resize_Port_Resize*)malloc(sizeof(LV2_Resize_Port_Resize));
		data->data   = node;
		data->resize = &resize_port;
		LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
		f->URI  = LV2_RESIZE_PORT_URI;
		f->data = data;
		return SharedPtr<LV2_Feature>(f, &delete_feature);
	}
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2RESIZEFEATURE_HPP
