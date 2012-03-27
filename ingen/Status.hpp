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

#ifndef INGEN_STATUS_HPP
#define INGEN_STATUS_HPP

namespace Ingen {

enum Status {
	SUCCESS,
	FAILURE,

	BAD_INDEX,
	BAD_OBJECT_TYPE,
	BAD_VALUE_TYPE,
	CLIENT_NOT_FOUND,
	CREATION_FAILED,
	DIRECTION_MISMATCH,
	EXISTS,
	INTERNAL_ERROR,
	INVALID_PARENT_PATH,
	INVALID_POLY,
	NOT_FOUND,
	NOT_MOVABLE,
	NO_SPACE,
	PARENT_DIFFERS,
	PARENT_NOT_FOUND,
	PLUGIN_NOT_FOUND,
	PORT_NOT_FOUND,
	TYPE_MISMATCH,
	UNKNOWN_TYPE
};

static inline const char*
ingen_status_string(Status st)
{
	switch (st) {
	case SUCCESS:             return "Success";
	case FAILURE:             return "Failure";

	case BAD_INDEX:           return "Invalid index";
	case BAD_OBJECT_TYPE:     return "Invalid object type";
	case BAD_VALUE_TYPE:      return "Invalid value type";
	case CLIENT_NOT_FOUND:    return "Client not found";
	case CREATION_FAILED:     return "Creation failed";
	case DIRECTION_MISMATCH:  return "Direction mismatch";
	case EXISTS:              return "Object exists";
	case INTERNAL_ERROR:      return "Internal error" ;
	case INVALID_PARENT_PATH: return "Invalid parent path";
	case INVALID_POLY:        return "Invalid polyphony";
	case NOT_FOUND:           return "Object not found";
	case NOT_MOVABLE:         return "Object not movable";
	case NO_SPACE:            return "Insufficient space";
	case PARENT_DIFFERS:      return "Parent differs";
	case PARENT_NOT_FOUND:    return "Parent not found";
	case PORT_NOT_FOUND:      return "Port not found";
	case PLUGIN_NOT_FOUND:    return "Plugin not found";
	case TYPE_MISMATCH:       return "Type mismatch";
	case UNKNOWN_TYPE:        return "Unknown type";
	}

	return "Unknown error";
}

} // namespace Ingen

#endif // INGEN_STATUS_HPP
