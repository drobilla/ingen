/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "Driver.h"
#include "types.h"
#include "List.h"

namespace Om {

class Patch;
class AudioDriver;
template <typename T> class TypedPort;


/** Audio driver abstract base class.
 *
 * \ingroup engine
 */
class AudioDriver : public Driver<Sample>
{
public:
	
	virtual void   set_root_patch(Patch* patch) = 0;
	virtual Patch* root_patch()                 = 0;
	
	virtual SampleCount buffer_size()  const = 0;
	virtual SampleCount sample_rate()  const = 0;
	virtual SampleCount time_stamp()   const = 0;
};


} // namespace Om

#endif // AUDIODRIVER_H
