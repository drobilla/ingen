/*
  This file is part of Ingen.
  Copyright 2012-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_STREAMWRITER_HPP
#define INGEN_STREAMWRITER_HPP

#include <cstdio>

#include "ingen/ingen.h"
#include "ingen/ColorContext.hpp"
#include "ingen/TurtleWriter.hpp"

namespace Raul { class URI; }

namespace Ingen {

class URIMap;
class URIs;

/** An Interface that writes Turtle messages to a stream.
 */
class INGEN_API StreamWriter : public TurtleWriter
{
public:
	StreamWriter(URIMap&             map,
	             URIs&               uris,
	             const Raul::URI&    uri,
	             FILE*               stream,
	             ColorContext::Color color);

	size_t text_sink(const void* buf, size_t len) override;

protected:
	FILE*               _stream;
	ColorContext::Color _color;
};

}  // namespace Ingen

#endif  // INGEN_STREAMWRITER_HPP
