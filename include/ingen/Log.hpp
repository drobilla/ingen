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

#include "ingen/LV2Features.hpp"
#include "ingen/fmt.hpp" // IWYU pragma: export
#include "ingen/ingen.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/urid/urid.h"

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace ingen {

class Node;
class URIs;
class World;

class INGEN_API Log {
public:
	using Sink = std::function<int(LV2_URID, const char*, va_list)>;

	Log(LV2_Log_Log* log, URIs& uris);

	struct Feature : public LV2Features::Feature {
		const char* uri() const override { return LV2_LOG__log; }

		std::shared_ptr<LV2_Feature>
		feature(World& world, Node* block) override;

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

	template <typename... Args>
	void error(const char* format, Args&&... args)
	{
		error(fmt(format, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void info(const char* format, Args&&... args)
	{
		info(fmt(format, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void warn(const char* format, Args&&... args)
	{
		warn(fmt(format, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void trace(const char* format, Args&&... args)
	{
		trace(fmt(format, std::forward<Args>(args)...));
	}

	int vtprintf(LV2_URID type, const char* fmt, va_list args);

	void set_flush(bool f) { _flush = f; }
	void set_trace(bool f) { _trace = f; }
	void set_sink(Sink s)  { _sink = std::move(s); }

private:
	void print(FILE* stream, const std::string& msg) const;

	LV2_Log_Log* _log;
	URIs&        _uris;
	Sink         _sink;
	bool         _flush;
	bool         _trace;
};

} // namespace ingen

#endif  // INGEN_LOG_HPP
