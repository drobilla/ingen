/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_RGBA_HPP
#define INGEN_GUI_RGBA_HPP

#include <cmath>

namespace Ingen {
namespace GUI {

static inline uint32_t
rgba_to_uint(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((((uint32_t)(r)) << 24) |
	        (((uint32_t)(g)) << 16) |
	        (((uint32_t)(b)) << 8) |
	        (((uint32_t)(a))));
}

static inline uint8_t
mono_interpolate(uint8_t v1, uint8_t v2, float f)
{
	return ((int)rint((v2) * (f) + (v1) * (1 - (f))));
}

#define RGBA_R(x) (((uint32_t)(x)) >> 24)
#define RGBA_G(x) ((((uint32_t)(x)) >> 16) & 0xFF)
#define RGBA_B(x) ((((uint32_t)(x)) >> 8) & 0xFF)
#define RGBA_A(x) (((uint32_t)(x)) & 0xFF)

static inline uint32_t
rgba_interpolate(uint32_t c1, uint32_t c2, float f)
{
	return rgba_to_uint(
		mono_interpolate(RGBA_R(c1), RGBA_R(c2), f),
		mono_interpolate(RGBA_G(c1), RGBA_G(c2), f),
		mono_interpolate(RGBA_B(c1), RGBA_B(c2), f),
		mono_interpolate(RGBA_A(c1), RGBA_A(c2), f));
}

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_RGBA_HPP
