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

#include <iostream>
#include "raul/Atom.hpp"
#include "module/World.hpp"
#include "uri-map.lv2/uri-map.h"
#include "object.lv2/object.h"
#include "LV2Features.hpp"
#include "LV2Object.hpp"
#include "LV2URIMap.hpp"

using namespace std;

namespace Ingen {
namespace Shared {
namespace LV2Object {


bool
to_atom(World* world, LV2_Object* object, Raul::Atom& atom)
{
	SharedPtr<LV2URIMap> map = PtrCast<LV2URIMap>(world->lv2_features->feature(LV2_URI_MAP_URI));

	if (object->type == map->object_class_string) {
		atom = Raul::Atom((char*)(object + 1));
		return true;
	} else if (object->type == map->object_class_bool) {
		atom = Raul::Atom((bool)(int32_t*)(object + 1));
		return true;
	} else if (object->type == map->object_class_int32) {
		atom = Raul::Atom((int32_t*)(object + 1));
		return true;
	} else if (object->type == map->object_class_float32) {
		atom = Raul::Atom((float*)(object + 1));
		return true;
	}
	return false;
}


/** Convert an atom to an LV2 object, if possible.
 * object->size should be the capacity of the object (not including header)
 */
bool
from_atom(World* world, const Raul::Atom& atom, LV2_Object* object)
{
	SharedPtr<LV2URIMap> map = PtrCast<LV2URIMap>(world->lv2_features->feature(LV2_URI_MAP_URI));

	char* str;
	switch (atom.type()) {
	case Raul::Atom::FLOAT:
		object->type = map->object_class_float32;
		object->size = sizeof(float);
		*(float*)(object + 1) = atom.get_float();
		break;
	case Raul::Atom::INT:
		object->type = map->object_class_int32;
		object->size = sizeof(int32_t);
		*(int32_t*)(object + 1) = atom.get_int32();
		break;
	case Raul::Atom::STRING:
		object->type = map->object_class_string;
		object->size = std::min(object->size, (uint32_t)strlen(atom.get_string()) + 1);
		str = ((char*)(object + 1));
		str[object->size - 1] = '\0';
		strncpy(str, atom.get_string(), object->size);
		break;
	case Raul::Atom::BLOB:
		object->type = map->object_class_string;
		*(uint32_t*)(object + 1) = map->uri_to_id(NULL, atom.get_blob_type());
		memcpy(((char*)(object + 1) + sizeof(uint32_t)), atom.get_blob(),
				std::min(atom.data_size(), (size_t)object->size));
	default:
		cerr << "Unsupported value type for toggle control" << endl;
		return false;
	}
	return true;
}


} // namespace LV2Object

} // namespace Shared
} // namespace Ingen
