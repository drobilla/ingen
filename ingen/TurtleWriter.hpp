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

#ifndef INGEN_TURTLE_WRITER_HPP
#define INGEN_TURTLE_WRITER_HPP

#include <stdint.h>

#include "ingen/AtomSink.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Interface.hpp"
#include "ingen/types.hpp"
#include "ingen/ingen.h"
#include "raul/URI.hpp"
#include "sratom/sratom.h"

namespace Ingen {

/** An Interface that writes Turtle messages to a sink method.
 *
 * Derived classes must implement text_sink() to do something with the
 * serialized messages.
 */
class INGEN_API TurtleWriter : public AtomWriter, public AtomSink
{
public:
	TurtleWriter(URIMap&          map,
	             URIs&            uris,
	             const Raul::URI& uri);

	virtual ~TurtleWriter();

	/** AtomSink method which receives calls serialized to LV2 atoms. */
	bool write(const LV2_Atom* msg, int32_t default_id=0);

	/** Pure virtual text sink which receives calls serialized to Turtle. */
	virtual size_t text_sink(const void* buf, size_t len) = 0;

	Raul::URI uri() const { return _uri; }

protected:
	URIMap&     _map;
	Sratom*     _sratom;
	SerdNode    _base;
	SerdURI     _base_uri;
	SerdEnv*    _env;
	SerdWriter* _writer;
	Raul::URI   _uri;
	bool        _wrote_prefixes;
};

}  // namespace Ingen

#endif  // INGEN_TURTLE_WRITER_HPP
