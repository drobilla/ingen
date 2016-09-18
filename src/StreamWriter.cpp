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

#include "ingen/ColorContext.hpp"
#include "ingen/StreamWriter.hpp"

namespace Ingen {

StreamWriter::StreamWriter(URIMap&             map,
                           URIs&               uris,
                           const Raul::URI&    uri,
                           FILE*               stream,
                           ColorContext::Color color)
	: TurtleWriter(map, uris, uri)
	, _stream(stream)
	, _color(color)
{}

size_t
StreamWriter::text_sink(const void* buf, size_t len)
{
	ColorContext ctx(_stream, _color);
	return fwrite(buf, 1, len, _stream);
}

} // namespace Ingen
