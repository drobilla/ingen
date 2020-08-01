/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_LIBRARY_HPP
#define INGEN_LIBRARY_HPP

#include "ingen/FilePath.hpp"
#include "ingen/ingen.h"

namespace ingen {

/** A dynamically loaded library (module, plugin). */
class INGEN_API Library {
public:
	Library(const FilePath& path);
	~Library();

	Library(const Library&) = delete;
	Library& operator=(const Library&) = delete;

	using VoidFuncPtr = void (*)();

	VoidFuncPtr get_function(const char* name);

	static const char* get_last_error();

	operator bool() const { return _lib; }

private:
	void* _lib;
};

} // namespace ingen

#endif // INGEN_LIBRARY_HPP
