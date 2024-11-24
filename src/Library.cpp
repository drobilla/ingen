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

#include <ingen/FilePath.hpp>
#include <ingen/Library.hpp>

#ifdef _WIN32
#    include <windows.h>
#    define dlopen(path, flags) LoadLibrary(path)
#    define dlclose(lib)        FreeLibrary((HMODULE)lib)
#    define dlerror()           "unknown error"
#else
#    include <dlfcn.h>
#endif

namespace ingen {

Library::Library(const FilePath& path) : _lib(dlopen(path.c_str(), RTLD_NOW))
{}

Library::~Library()
{
	dlclose(_lib);
}

Library::VoidFuncPtr
Library::get_function(const char* name)
{
#ifdef _WIN32
	return (VoidFuncPtr)GetProcAddress((HMODULE)_lib, name);
#else
	using VoidFuncGetter = VoidFuncPtr (*)(void*, const char*);
	auto dlfunc = reinterpret_cast<VoidFuncGetter>(dlsym);
	return dlfunc(_lib, name);
#endif
}

const char*
Library::get_last_error()
{
	return dlerror();
}

} // namespace ingen
