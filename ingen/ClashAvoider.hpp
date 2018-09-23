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

#ifndef INGEN_CLASHAVOIDER_HPP
#define INGEN_CLASHAVOIDER_HPP

#include <map>

#include "ingen/ingen.h"
#include "raul/Path.hpp"

namespace ingen {

class Store;
class URI;

/** Maps paths so they do not clash with an existing object in a store.
 *
 * @ingroup ingen
 */
class INGEN_API ClashAvoider
{
public:
	explicit ClashAvoider(const Store& store);

	const URI        map_uri(const URI& in);
	const Raul::Path map_path(const Raul::Path& in);

	bool exists(const Raul::Path& path) const;

private:
	typedef std::map<Raul::Path, unsigned>   Offsets;
	typedef std::map<Raul::Path, Raul::Path> SymbolMap;

	const Store& _store;
	Offsets      _offsets;
	SymbolMap    _symbol_map;
};

} // namespace ingen

#endif // INGEN_CLASHAVOIDER_HPP
