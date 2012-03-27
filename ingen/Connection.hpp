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


#ifndef INGEN_INTERFACE_CONNECTION_HPP
#define INGEN_INTERFACE_CONNECTION_HPP

namespace Raul { class Path; }

namespace Ingen {

/** A connection between two ports.
 *
 * \ingroup interface
 */
class Connection
{
public:
	virtual ~Connection() {}

	virtual const Raul::Path& src_port_path() const = 0;
	virtual const Raul::Path& dst_port_path() const = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_CONNECTION_HPP
