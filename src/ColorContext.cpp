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

#include <ingen/ColorContext.hpp>

#include "ingen_config.h"

#if USE_ISATTY
#    include <unistd.h>
#else
inline int isatty(int fd) { return 0; }
#endif

namespace ingen {

ColorContext::ColorContext(FILE* stream, Color color)
	: _stream(stream)
{
	if (isatty(fileno(_stream))) {
		fprintf(_stream, "\033[0;%dm", static_cast<int>(color));
	}
}

ColorContext::~ColorContext()
{
	if (isatty(fileno(_stream))) {
		fprintf(_stream, "\033[0m");
		fflush(_stream);
	}
}

} // namespace ingen
