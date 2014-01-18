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

#include <iostream>

#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"

namespace Ingen {

static const char* const ANSI_RESET   = "\033[0m";
static const char* const ANSI_RED     = "\033[0;31m";
//static const char* const ANSI_GREEN   = "\033[0;32m";
static const char* const ANSI_YELLOW  = "\033[0;33m";
//static const char* const ANSI_BLUE    = "\033[0;34m";
//static const char* const ANSI_MAGENTA = "\033[0;35m";
//static const char* const ANSI_CYAN    = "\033[0;36m";
//static const char* const ANSI_WHITE   = "\033[0;37m";

Log::Log(LV2_Log_Log* log, URIs& uris)
	: _log(log)
	, _uris(uris)
{}

void
Log::error(const std::string& msg)
{
	if (_log) {
		_log->printf(_log->handle, _uris.log_Error, msg.c_str());
	} else {
		std::cerr << ANSI_RED << msg << ANSI_RESET;
	}
}

void
Log::warn(const std::string& msg)
{
	if (_log) {
		_log->printf(_log->handle, _uris.log_Warning, msg.c_str());
	} else {
		std::cerr << ANSI_YELLOW << msg << ANSI_RESET;
	}
}

void
Log::info(const std::string& msg)
{
	if (_log) {
		_log->printf(_log->handle, _uris.log_Note, msg.c_str());
	} else {
		std::cout << msg;
	}
}

}  // namespace Ingen
