/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_SHARED_ATOMREADER_HPP
#define INGEN_SHARED_ATOMREADER_HPP

#include "ingen/Interface.hpp"
#include "ingen/shared/AtomSink.hpp"
#include "ingen/shared/URIs.hpp"
#include "serd/serd.h"

namespace Ingen {

class Forge;

namespace Shared {

class AtomSink;
class URIMap;

/** An AtomSink that calls methods on an Interface. */
class AtomReader : public AtomSink
{
public:
	AtomReader(URIMap& map, URIs& uris, Forge& forge, Interface& iface);
	~AtomReader() {}

	void write(const LV2_Atom* msg);

private:
	void get_atom(const LV2_Atom* in, Raul::Atom& out);

	void get_props(const LV2_Atom_Object*       obj,
	               Ingen::Resource::Properties& props);

	URIMap&    _map;
	URIs&      _uris;
	Forge&     _forge;
	Interface& _iface;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_ATOMREADER_HPP

