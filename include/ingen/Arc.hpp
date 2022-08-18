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

#ifndef INGEN_ARC_HPP
#define INGEN_ARC_HPP

#include "ingen/ingen.h"
#include "raul/Deletable.hpp"

namespace raul {
class Path;
} // namespace raul

namespace ingen {

/** A connection between two ports.
 *
 * @ingroup Ingen
 */
class INGEN_API Arc : public raul::Deletable
{
public:
	virtual const raul::Path& tail_path() const = 0;
	virtual const raul::Path& head_path() const = 0;
};

} // namespace ingen

#endif // INGEN_ARC_HPP
