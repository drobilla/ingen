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

#include "raul/log.hpp"
#include "raul/Atom.hpp"
#include "module/World.hpp"
#include "uri-map.lv2/uri-map.h"
#include "object.lv2/object.h"
#include "LV2Features.hpp"
#include "LV2Object.hpp"
#include "LV2URIMap.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {
namespace LV2Object {


bool
to_atom(World* world, const LV2_Object* object, Raul::Atom& atom)
{
	SharedPtr<LV2URIMap> uris = world->uris;

	if (object->type == uris->object_class_string.id) {
		atom = Raul::Atom((char*)(object + 1));
		return true;
	} else if (object->type == uris->object_class_bool.id) {
		atom = Raul::Atom((bool)(int32_t*)(object + 1));
		return true;
	} else if (object->type == uris->object_class_int32.id) {
		atom = Raul::Atom((int32_t*)(object + 1));
		return true;
	} else if (object->type == uris->object_class_float32.id) {
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
	SharedPtr<LV2URIMap> uris = world->uris;

	char* str;
	switch (atom.type()) {
	case Raul::Atom::FLOAT:
		object->type = uris->object_class_float32.id;
		object->size = sizeof(float);
		*(float*)(object + 1) = atom.get_float();
		break;
	case Raul::Atom::INT:
		object->type = uris->object_class_int32.id;
		object->size = sizeof(int32_t);
		*(int32_t*)(object + 1) = atom.get_int32();
		break;
	case Raul::Atom::STRING:
		object->type = uris->object_class_string.id;
		object->size = std::min((uint16_t)object->size, (uint16_t)(strlen(atom.get_string()) + 1));
		str = ((char*)(object + 1));
		str[object->size - 1] = '\0';
		strncpy(str, atom.get_string(), object->size);
		break;
	case Raul::Atom::BLOB:
		error << "TODO: Blob support" << endl;
		/*object->type = uris->object_class_string;
		*(uint16_t*)(object + 1) = uris->uri_to_id(NULL, atom.get_blob_type());
		memcpy(((char*)(object + 1) + sizeof(uint32_t)), atom.get_blob(),
				std::min(atom.data_size(), (size_t)object->size));*/
	default:
		error << "Unsupported value type for toggle control" << endl;
		return false;
	}
	return true;
}


} // namespace LV2Object

} // namespace Shared
} // namespace Ingen
