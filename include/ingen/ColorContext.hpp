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

#ifndef INGEN_COLORCONTEXT_HPP
#define INGEN_COLORCONTEXT_HPP

#include "ingen/ingen.h"

#include <cstdio>

namespace ingen {

class INGEN_API ColorContext
{
public:
	enum class Color { RED = 31, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

	ColorContext(FILE* stream, Color color);
	~ColorContext();

private:
	FILE* _stream;
};

} // namespace ingen

#endif // INGEN_COLORCONTEXT_HPP
