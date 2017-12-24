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

#ifndef INGEN_LOG_HPP
#define INGEN_LOG_HPP

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <string>

#include <boost/format.hpp>

#include "ingen/LV2Features.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

namespace Ingen {

typedef boost::basic_format<char> fmt;

class Node;
class URIs;
class World;

class INGEN_API Log {
public:
	typedef std::function<int(LV2_URID, const char*, va_list)> Sink;

	Log(LV2_Log_Log* log, URIs& uris);

	struct Feature : public LV2Features::Feature {
		const char* uri() const { return LV2_LOG__log; }

		SPtr<LV2_Feature> feature(World* world, Node* block);

		struct Handle {
			LV2_Log_Log lv2_log;
			Log*        log;
			Node*       node;
		};
	};

	void rt_error(const char* msg);

	void error(const std::string& msg);
	void info(const std::string& msg);
	void warn(const std::string& msg);
	void trace(const std::string& msg);

	inline void error(const fmt& fmt) { error(fmt.str()); }
	inline void info(const fmt& fmt)  { info(fmt.str()); }
	inline void warn(const fmt& fmt)  { warn(fmt.str()); }

	int vtprintf(LV2_URID type, const char* fmt, va_list args);

	void set_flush(bool f) { _flush = f; }
	void set_trace(bool f) { _trace = f; }
	void set_sink(Sink s)  { _sink = s; }

private:
	void print(FILE* stream, const std::string& msg);

	LV2_Log_Log* _log;
	URIs&        _uris;
	Sink         _sink;
	bool         _flush;
	bool         _trace;
};

}  // namespace Ingen

#endif  // INGEN_LOG_HPP
