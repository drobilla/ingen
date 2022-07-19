/*
  This file is part of Ingen.
  Copyright 2014-2022 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CLIENT_CLIENT_H
#define INGEN_CLIENT_CLIENT_H

#if defined(_WIN32) && !defined(INGEN_CLIENT_STATIC) && \
    defined(INGEN_CLIENT_INTERNAL)
#	define INGEN_CLIENT_API __declspec(dllexport)
#elif defined(_WIN32) && !defined(INGEN_CLIENT_STATIC)
#	define INGEN_CLIENT_API __declspec(dllimport)
#elif defined(__GNUC__)
#	define INGEN_CLIENT_API __attribute__((visibility("default")))
#else
#	define INGEN_CLIENT_API
#endif

#endif // INGEN_CLIENT_CLIENT_H
