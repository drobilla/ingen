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

#ifndef INGEN_ENGINE_BUFFER_REF_HPP
#define INGEN_ENGINE_BUFFER_REF_HPP

#include "server.h"

#include <boost/smart_ptr/intrusive_ptr.hpp> // IWYU pragma: export

namespace ingen {
namespace server {

class Buffer;

using BufferRef = boost::intrusive_ptr<Buffer>;

// Defined in Buffer.cpp
INGEN_SERVER_API void intrusive_ptr_add_ref(Buffer* b);
INGEN_SERVER_API void intrusive_ptr_release(Buffer* b);

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_BUFFER_REF_HPP
