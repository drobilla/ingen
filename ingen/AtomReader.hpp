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

#ifndef INGEN_ATOMREADER_HPP
#define INGEN_ATOMREADER_HPP

#include <boost/optional.hpp>

#include "ingen/Interface.hpp"
#include "ingen/AtomSink.hpp"
#include "ingen/URIs.hpp"
#include "ingen/ingen.h"
#include "serd/serd.h"

namespace Ingen {

class AtomSink;
class Forge;
class Log;
class URIMap;

/** An AtomSink that calls methods on an Interface.
 * @ingroup IngenShared
 */
class INGEN_API AtomReader : public AtomSink
{
public:
	AtomReader(URIMap&    map,
	           URIs&      uris,
	           Log&       log,
	           Forge&     forge,
	           Interface& iface);
	~AtomReader() {}

	static bool is_message(URIs& uris, const LV2_Atom* msg);

	bool write(const LV2_Atom* msg);

private:
	void get_atom(const LV2_Atom* in, Atom& out);

	boost::optional<Raul::URI>  atom_to_uri(const LV2_Atom* atom);
	boost::optional<Raul::Path> atom_to_path(const LV2_Atom* atom);

	void get_props(const LV2_Atom_Object*       obj,
	               Ingen::Resource::Properties& props);

	URIMap&    _map;
	URIs&      _uris;
	Log&       _log;
	Forge&     _forge;
	Interface& _iface;
};

} // namespace Ingen

#endif // INGEN_ATOMREADER_HPP
