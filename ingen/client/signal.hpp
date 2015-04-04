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

#ifndef INGEN_CLIENT_SIGNAL_HPP
#define INGEN_CLIENT_SIGNAL_HPP

#include <sigc++/sigc++.h>

#define INGEN_SIGNAL(name, ...) \
protected: \
sigc::signal<__VA_ARGS__> _signal_##name; \
public: \
sigc::signal<__VA_ARGS__>  signal_##name() const { return _signal_##name; } \
sigc::signal<__VA_ARGS__>& signal_##name()       { return _signal_##name; }

#define INGEN_TRACKABLE sigc::trackable

#endif // INGEN_CLIENT_SIGNAL_HPP
