/*
  This file is part of Ingen.
  Copyright 2007-2020 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_MEMORY_HPP
#define INGEN_MEMORY_HPP

#include <cstdlib>
#include <memory>
#include <utility>

namespace ingen {

template <class T>
void NullDeleter(T* ptr) {}

template <class T>
struct FreeDeleter { void operator()(T* const ptr) { free(ptr); } };

template <typename T, typename... Args>
std::unique_ptr<T>
make_unique(Args&&... args)
{
	return std::unique_ptr<T>{new T{std::forward<Args>(args)...}};
}

} // namespace ingen

#endif // INGEN_MEMORY_HPP
