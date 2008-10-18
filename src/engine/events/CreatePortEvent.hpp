/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CREATEPORTEVENT_H
#define CREATEPORTEVENT_H

#include "QueuedEvent.hpp"
#include "raul/Path.hpp"
#include "raul/Array.hpp"
#include "interface/DataType.hpp"
#include <string>
using std::string;

template <typename T> class Array;

namespace Ingen {

class PatchImpl;
class PortImpl;
class DriverPort;


/** An event to add a Port to a Patch.
 *
 * \ingroup engine
 */
class CreatePortEvent : public QueuedEvent
{
public:
	CreatePortEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& type, bool is_output, QueuedEventSource* source);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	
	enum ErrorType { 
		NO_ERROR,
		UNKNOWN_TYPE
	};

	ErrorType               _error;
	Raul::Path              _path;
	string                  _type;
	bool                    _is_output;
	DataType                _data_type;
	PatchImpl*              _patch;
	PortImpl*               _patch_port;
	Raul::Array<PortImpl*>* _ports_array; ///< New (external) ports array for Patch
	DriverPort*             _driver_port; ///< Driver (eg Jack) port if this is a toplevel port
	bool                    _succeeded;
};


} // namespace Ingen

#endif // CREATEPORTEVENT_H
