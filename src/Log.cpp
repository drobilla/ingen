/*
  This file is part of Ingen.
  Copyright 2007-2024 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ingen/Log.hpp>

#include <ingen/ColorContext.hpp>
#include <ingen/Node.hpp>
#include <ingen/URIs.hpp>
#include <ingen/World.hpp>
#include <lv2/core/lv2.h>
#include <lv2/log/log.h>
#include <lv2/urid/urid.h>
#include <raul/Path.hpp>

#include <cstdio>
#include <cstdlib>
#include <memory>

namespace ingen {

Log::Log(LV2_Log_Log* log, URIs& uris)
	: _log(log)
	, _uris(uris)
{}

void
Log::rt_error(const char* msg)
{
#ifndef NDEBUG
	tprintf(_uris.log_Error, msg);
#endif
}

void
Log::error(const std::string& msg)
{
	tprintf(_uris.log_Error, msg.c_str());
}

void
Log::warn(const std::string& msg)
{
	tprintf(_uris.log_Warning, msg.c_str());
}

void
Log::info(const std::string& msg)
{
	tprintf(_uris.log_Note, msg.c_str());
}

void
Log::trace(const std::string& msg)
{
	tprintf(_uris.log_Trace, msg.c_str());
}

int
Log::tprintf(LV2_URID type, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	const int ret = vtprintf(type, fmt, args);

	va_end(args);
	return ret;
}

int
Log::vtprintf(LV2_URID type, const char* fmt, va_list args)
{
	int ret = 0;
	if (type == _uris.log_Trace && !_trace) {
		return 0;
	}

	if (_sink) {
		_sink(type, fmt, args);
	}

	if (_log) {
		ret = _log->vprintf(_log->handle, type, fmt, args);
	} else if (type == _uris.log_Error) {
		const ColorContext ctx{stderr, ColorContext::Color::RED};
		ret = vfprintf(stderr, fmt, args);
	} else if (type == _uris.log_Warning) {
		const ColorContext ctx{stderr, ColorContext::Color::YELLOW};
		ret = vfprintf(stderr, fmt, args);
	} else if (type == _uris.log_Note) {
		const ColorContext ctx{stderr, ColorContext::Color::GREEN};
		ret = vfprintf(stdout, fmt, args);
	} else if (_trace && type == _uris.log_Trace) {
		const ColorContext ctx{stderr, ColorContext::Color::GREEN};
		ret = vfprintf(stderr, fmt, args);
	} else {
		fprintf(stderr, "Unknown log type %u\n", type);
		return 0;
	}
	if (_flush) {
		fflush(stdout);
	}
	return ret;
}

static int
log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list args)
{
	auto* const f = static_cast<Log::Feature::Handle*>(handle);

	int ret  = f->log->tprintf(type, f->node->path().c_str());
	ret     += f->log->tprintf(type, ": ");
	ret     += f->log->vtprintf(type, fmt, args);

	return ret;
}

static int
log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const int ret = log_vprintf(handle, type, fmt, args);
	va_end(args);

	return ret;
}

static void
free_log_feature(LV2_Feature* feature) {
	auto* lv2_log = static_cast<LV2_Log_Log*>(feature->data);
	free(lv2_log->handle);
	free(feature);
}

std::shared_ptr<LV2_Feature>
Log::Feature::feature(World& world, Node* block)
{
	auto* handle = static_cast<Handle*>(calloc(1, sizeof(Handle)));
	handle->lv2_log.handle  = handle;
	handle->lv2_log.printf  = log_printf;
	handle->lv2_log.vprintf = log_vprintf;
	handle->log             = &world.log();
	handle->node            = block;

	auto* f = static_cast<LV2_Feature*>(malloc(sizeof(LV2_Feature)));
	f->URI  = LV2_LOG__log;
	f->data = &handle->lv2_log;

	return {f, &free_log_feature};
}

} // namespace ingen
