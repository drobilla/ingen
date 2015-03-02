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

#ifndef INGEN_LOG_HPP
#define INGEN_LOG_HPP

#include <string>

#include <boost/format.hpp>

#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"

namespace Ingen {

typedef boost::basic_format<char> fmt;

class URIs;

class INGEN_API Log {
public:
	Log(LV2_Log_Log* log, URIs& uris);

	void error(const std::string& msg);
	void info(const std::string& msg);
	void warn(const std::string& msg);

	inline void error(const fmt& fmt) { error(fmt.str()); }
	inline void info(const fmt& fmt)  { info(fmt.str()); }
	inline void warn(const fmt& fmt)  { warn(fmt.str()); }

private:
	LV2_Log_Log* _log;
	URIs&        _uris;
};

}  // namespace Ingen

#endif  // INGEN_LOG_HPP
