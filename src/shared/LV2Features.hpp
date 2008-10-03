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

#ifndef LV2FEATURES_HPP
#define LV2FEATURES_HPP

#include "config.h"
#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <map>
#include <string>
#include <slv2/slv2.h>
	
namespace Ingen {
namespace Shared {
	

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Features {
public:
	LV2Features();

	struct Feature {
		Feature(LV2_Feature* f, void* c=NULL) : feature(f), controller(c) {}
		LV2_Feature* feature;    ///< LV2 feature struct (plugin exposed)
		void*        controller; ///< Ingen internals, not exposed to plugin
	};

	typedef std::map<std::string, Feature> Features;
	
	const Feature* feature(const std::string& uri);
	
	void add_feature(const std::string& uri, LV2_Feature* feature, void* controller);

	LV2_Feature** lv2_features() const { return _lv2_features; }

private:
	Features      _features;
	LV2_Feature** _lv2_features;
};


} // namespace Shared
} // namespace Ingen

#endif // LV2FEATURES_HPP
