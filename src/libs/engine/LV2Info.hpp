/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef LV2INFO_H
#define LV2INFO_H

#include CONFIG_H_PATH
#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include "module/global.hpp"
#include <slv2/slv2.h>
	
namespace Ingen {
	

class LV2Info {
public:
	LV2Info(SLV2World world)
		: input_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_INPUT))
		, output_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_OUTPUT))
		, control_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL))
		, audio_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_AUDIO))
		, event_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_EVENT))
	{}

	~LV2Info() {
		slv2_value_free(input_class);
		slv2_value_free(output_class);
		slv2_value_free(control_class);
		slv2_value_free(audio_class);
		slv2_value_free(event_class);
	}

	SLV2Value input_class;
	SLV2Value output_class;
	SLV2Value control_class;
	SLV2Value audio_class;
	SLV2Value event_class;
};


} // namespace Ingen

#endif // LV2INFO_H
