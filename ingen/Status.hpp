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

#ifndef INGEN_STATUS_HPP
#define INGEN_STATUS_HPP

namespace Ingen {

enum class Status {
	SUCCESS,
	FAILURE,

	BAD_INDEX,
	BAD_OBJECT_TYPE,
	BAD_REQUEST,
	BAD_URI,
	BAD_VALUE_TYPE,
	BAD_VALUE,
	CLIENT_NOT_FOUND,
	CREATION_FAILED,
	DIRECTION_MISMATCH,
	EXISTS,
	INTERNAL_ERROR,
	INVALID_PARENT,
	INVALID_POLY,
	NOT_DELETABLE,
	NOT_FOUND,
	NOT_MOVABLE,
	NOT_PREPARED,
	NO_SPACE,
	PARENT_DIFFERS,
	PARENT_NOT_FOUND,
	PROTOTYPE_NOT_FOUND,
	PORT_NOT_FOUND,
	TYPE_MISMATCH,
	UNKNOWN_TYPE
};

static inline const char*
ingen_status_string(Status st)
{
	switch (st) {
	case Status::SUCCESS:             return "Success";
	case Status::FAILURE:             return "Failure";

	case Status::BAD_INDEX:           return "Invalid index";
	case Status::BAD_OBJECT_TYPE:     return "Invalid object type";
	case Status::BAD_REQUEST:         return "Invalid request";
	case Status::BAD_URI:             return "Invalid URI";
	case Status::BAD_VALUE_TYPE:      return "Invalid value type";
	case Status::BAD_VALUE:           return "Invalid value";
	case Status::CLIENT_NOT_FOUND:    return "Client not found";
	case Status::CREATION_FAILED:     return "Creation failed";
	case Status::DIRECTION_MISMATCH:  return "Direction mismatch";
	case Status::EXISTS:              return "Object exists";
	case Status::INTERNAL_ERROR:      return "Internal error";
	case Status::INVALID_PARENT:      return "Invalid parent";
	case Status::INVALID_POLY:        return "Invalid polyphony";
	case Status::NOT_DELETABLE:       return "Object not deletable";
	case Status::NOT_FOUND:           return "Object not found";
	case Status::NOT_MOVABLE:         return "Object not movable";
	case Status::NOT_PREPARED:        return "Not prepared";
	case Status::NO_SPACE:            return "Insufficient space";
	case Status::PARENT_DIFFERS:      return "Parent differs";
	case Status::PARENT_NOT_FOUND:    return "Parent not found";
	case Status::PROTOTYPE_NOT_FOUND: return "Prototype not found";
	case Status::PORT_NOT_FOUND:      return "Port not found";
	case Status::TYPE_MISMATCH:       return "Type mismatch";
	case Status::UNKNOWN_TYPE:        return "Unknown type";
	}

	return "Unknown error";
}

} // namespace Ingen

#endif // INGEN_STATUS_HPP
