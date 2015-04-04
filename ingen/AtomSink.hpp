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

#ifndef INGEN_ATOMSINK_HPP
#define INGEN_ATOMSINK_HPP

#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

namespace Ingen {

/** A sink for LV2 Atoms.
 * @ingroup IngenShared
 */
class INGEN_API AtomSink {
public:
	virtual ~AtomSink() {}

	/** Write an Atom to the sink.
	 * @return True on success.
	 */
	virtual bool write(const LV2_Atom* msg) = 0;
};

} // namespace Ingen

#endif // INGEN_ATOMSINK_HPP
