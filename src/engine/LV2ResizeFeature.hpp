/* This file is part of Ingen.
 * Copyright (C) 2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_LV2RESIZEFEATURE_HPP
#define INGEN_ENGINE_LV2RESIZEFEATURE_HPP

#include "raul/log.hpp"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "shared/LV2Features.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

struct ResizeFeature : public Shared::LV2Features::Feature {
	static bool resize_port(LV2_Resize_Port_Feature_Data data,
	                        uint32_t                     index,
	                        size_t                       size) {
		NodeImpl* node = (NodeImpl*)data;
		PortImpl* port = node->port_impl(index);
		switch (port->context()) {
		case Context::MESSAGE:
			port->buffer(0)->resize(size);
			port->connect_buffers();
			return true;
		default:
			// TODO: Implement realtime allocator and support this in audio thread
			return false;
		}
	}

	static void delete_feature(LV2_Feature* feature) {
		free(feature->data);
		free(feature);
	}

	SharedPtr<LV2_Feature> feature(Shared::World* w, Shared::Node* n) {
		NodeImpl* node = dynamic_cast<NodeImpl*>(n);
		if (!node)
			return SharedPtr<LV2_Feature>();
		LV2_Resize_Port_Feature* data
				= (LV2_Resize_Port_Feature*)malloc(sizeof(LV2_Resize_Port_Feature));
		data->data        = node;
		data->resize_port = &resize_port;
		LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
		f->URI  = LV2_RESIZE_PORT_URI;
		f->data = data;
		return SharedPtr<LV2_Feature>(f, &delete_feature);
	}
};

} // namespace Ingen

#endif // INGEN_ENGINE_LV2RESIZEFEATURE_HPP
