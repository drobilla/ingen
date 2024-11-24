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

#include <ingen/ingen.h>
#include <lv2/atom/atom.h>

#include <cstdint>

namespace ingen {

/** A sink for LV2 Atoms.
 * @ingroup IngenShared
 */
class INGEN_API AtomSink
{
public:
	virtual ~AtomSink() = default;

	/** Write an Atom to the sink.
	 *
	 * @param msg The atom to write.
	 * @param default_id The default response ID to use if no
	 * patch:sequenceNumber property is present on the message.
	 *
	 * @return True on success.
	 */
	virtual bool write(const LV2_Atom* msg, int32_t default_id=0) = 0;
};

} // namespace ingen

#endif // INGEN_ATOMSINK_HPP
