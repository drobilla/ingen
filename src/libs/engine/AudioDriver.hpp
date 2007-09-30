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

#ifndef AUDIODRIVER_H
#define AUDIODRIVER_H

#include <raul/List.hpp>
#include <raul/Path.hpp>
#include "Driver.hpp"
#include "types.hpp"
#include "DataType.hpp"

namespace Ingen {

class Patch;
class AudioDriver;
class Port;
class ProcessContext;


/** Audio driver abstract base class.
 *
 * \ingroup engine
 */
class AudioDriver : public Driver
{
public:
	AudioDriver() : Driver(DataType::FLOAT) {}
	
	virtual void   set_root_patch(Patch* patch) = 0;
	virtual Patch* root_patch()                 = 0;
	
	virtual void        add_port(DriverPort* port)    = 0;
	virtual DriverPort* remove_port(const Raul::Path& path) = 0;
	
	virtual SampleCount buffer_size()  const = 0;
	virtual SampleCount sample_rate()  const = 0;
	virtual SampleCount frame_time()   const = 0;

	virtual bool is_realtime() const = 0;

	virtual ProcessContext& context() = 0;
};


} // namespace Ingen

#endif // AUDIODRIVER_H
