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

#include "raul/Maid.hpp"

#include <cstdlib>
#include <memory>

namespace ingen {

template <class T>
void NullDeleter(T* ptr) {}

template <class T>
struct FreeDeleter { void operator()(T* const ptr) { free(ptr); } };

template <class T, class Deleter = std::default_delete<T>>
using UPtr = std::unique_ptr<T, Deleter>;

template <class T>
using SPtr = std::shared_ptr<T>;

template <class T>
using WPtr = std::weak_ptr<T>;

template <class T>
using MPtr = Raul::managed_ptr<T>;

template <typename T, typename... Args>
std::unique_ptr<T>
make_unique(Args&&... args)
{
	return std::unique_ptr<T>{new T{std::forward<Args>(args)...}};
}

} // namespace ingen

#endif // INGEN_MEMORY_HPP
