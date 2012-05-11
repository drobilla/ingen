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

#ifndef INGEN_SHARED_ATOMSINK_HPP
#define INGEN_SHARED_ATOMSINK_HPP

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

namespace Ingen {
namespace Shared {

/** A sink for LV2 Atoms.
 * @ingroup IngenShared
 */
class AtomSink {
public:
	virtual ~AtomSink() {}
	virtual void write(const LV2_Atom* msg) = 0;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_ATOMSINK_HPP

