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

#ifndef INGEN_ENGINE_AUDIOBUFFER_HPP
#define INGEN_ENGINE_AUDIOBUFFER_HPP

#include <cassert>
#include <cmath>
#include <cstddef>

#include <boost/utility.hpp>

#include "ingen/URIs.hpp"

#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "Context.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class AudioBuffer : public Buffer
{
public:
	AudioBuffer(BufferFactory& bufs, LV2_URID type, uint32_t size);

	void clear();

	void set_block(Sample val, size_t start_offset, size_t end_offset);
	void copy(Context& context, const Buffer* src);

	float peak(Context& context) const;

	inline bool is_control() const { return _type == _factory.uris().atom_Float; }

	inline Sample* data() const {
		return (is_control())
			? (Sample*)LV2_ATOM_BODY(atom())
			: (Sample*)LV2_ATOM_CONTENTS(LV2_Atom_Vector, atom());
	}

	inline SampleCount nframes() const {
		return (is_control())
			? 1
			: (_capacity - sizeof(LV2_Atom_Vector)) / sizeof(Sample);
	}

	inline Sample& value_at(size_t offset) const {
		assert(offset < nframes());
		return data()[offset];
	}

	void prepare_write(Context& context) {}

private:
	LV2_Atom_Vector* vector() { return (LV2_Atom_Vector*)atom(); }
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_AUDIOBUFFER_HPP
