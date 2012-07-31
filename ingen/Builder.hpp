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

#ifndef INGEN_BUILDER_HPP
#define INGEN_BUILDER_HPP

#include "raul/SharedPtr.hpp"

namespace Ingen {

class GraphObject;
class Interface;
class URIs;

/** Wrapper for Interface to create existing objects/models.
 *
 * @ingroup IngenShared
 */
class Builder
{
public:
	Builder(URIs& uris, Interface& interface);
	virtual ~Builder() {}

	void build(SharedPtr<const GraphObject> object);
	void connect(SharedPtr<const GraphObject> object);

private:
	URIs&      _uris;
	Interface& _interface;
};

} // namespace Ingen

#endif // INGEN_BUILDER_HPP

