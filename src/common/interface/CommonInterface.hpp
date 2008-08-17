/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef COMMONINTERFACE_H
#define COMMONINTERFACE_H

#include <inttypes.h>
#include <string>
#include <raul/SharedPtr.hpp>
#include <raul/Atom.hpp>
#include "interface/CommonInterface.hpp"

namespace Ingen {
namespace Shared {


/** Abstract interface common to both engine and clients.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class CommonInterface
{
public:
	virtual ~CommonInterface() {}
	
	// Bundles
	virtual void bundle_begin() = 0;
	virtual void bundle_end()   = 0;
	
	// Object commands
	
	virtual void new_patch(const std::string& path,
	                       uint32_t           poly) = 0;
	
	virtual void new_node(const std::string& path,
	                      const std::string& plugin_uri,
	                      bool               polyphonic) = 0;
	
	virtual void connect(const std::string& src_port_path,
	                     const std::string& dst_port_path) = 0;
	
	virtual void disconnect(const std::string& src_port_path,
	                        const std::string& dst_port_path) = 0;
	
	virtual void set_variable(const std::string& subject_path,
	                          const std::string& predicate,
	                          const Raul::Atom&  value) = 0;
	
	virtual void set_port_value(const std::string& port_path,
								const Raul::Atom&  value) = 0;
	
	virtual void set_voice_value(const std::string& port_path,
	                             uint32_t           voice,
	                             const Raul::Atom&  value) = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // COMMONINTERFACE_H

