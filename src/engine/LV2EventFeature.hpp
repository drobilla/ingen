/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef LV2_EVENT_FEATURE_H
#define LV2_EVENT_FEATURE_H

#include "shared/LV2Features.hpp"

namespace Ingen {

struct EventFeature : public Shared::LV2Features::Feature {
	EventFeature() {
		LV2_Event_Feature* data = (LV2_Event_Feature*)malloc(sizeof(LV2_Event_Feature));
		data->lv2_event_ref   = &event_ref;
		data->lv2_event_unref = &event_unref;
		data->callback_data   = this;
		_feature.URI          = LV2_EVENT_URI;
		_feature.data         = data;
	}

	static uint32_t event_ref(LV2_Event_Callback_Data callback_data,
	                          LV2_Event*              event) { return 0; }

	static uint32_t event_unref(LV2_Event_Callback_Data callback_data,
	                            LV2_Event*              event) { return 0; }

	SharedPtr<LV2_Feature> feature(Shared::Node*) {
		return SharedPtr<LV2_Feature>(&_feature, NullDeleter<LV2_Feature>);
	}

private:
	LV2_Feature _feature;
};

} // namespace Ingen

#endif // LV2_EVENT_FEATURE_H
