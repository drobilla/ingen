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

#ifndef INGEN_TYPES_HPP
#define INGEN_TYPES_HPP

#include <cstdlib>
#include <memory>

#include "raul/Maid.hpp"

namespace Ingen {

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

template<class T, class U>
SPtr<T> static_ptr_cast(const SPtr<U>& r) {
	return std::static_pointer_cast<T>(r);
}

template<class T, class U>
SPtr<T> dynamic_ptr_cast(const SPtr<U>& r) {
	return std::dynamic_pointer_cast<T>(r);
}

template<class T, class U>
SPtr<T> const_ptr_cast(const SPtr<U>& r) {
	return std::const_pointer_cast<T>(r);
}

} // namespace Ingen

#endif // INGEN_TYPES_HPP
