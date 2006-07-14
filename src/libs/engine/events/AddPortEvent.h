/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ADDPORTEVENT_H
#define ADDPORTEVENT_H

#include "QueuedEvent.h"
#include "util/Path.h"
#include "DataType.h"
#include "Array.h"
#include <string>
using std::string;

template <typename T> class Array;

namespace Om {

class Patch;
class Port;
class Plugin;
class DriverPort;


/** An event to add a Port to a Patch.
 *
 * \ingroup engine
 */
class AddPortEvent : public QueuedEvent
{
public:
	AddPortEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& path, const string& type, bool is_output);

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	Path          _path;
	string        _type;
	bool          _is_output;
	DataType      _data_type;
	Patch*        _patch;
	Port*         _patch_port;
	Array<Port*>* _ports_array; ///< New (external) ports array for Patch
	DriverPort*   _driver_port; ///< Driver (eg Jack) port if this is a toplevel port
	bool          _succeeded;
};


} // namespace Om

#endif // ADDPORTEVENT_H
