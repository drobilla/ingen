/* A Reference Counting Smart Pointer.
 * Copyright (C) 2006 Dave Robillard.
 * 
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This file is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef COUNTED_PTR_H
#define COUNTED_PTR_H

#include <cassert>
#include <cstddef>

#ifdef BOOST_SP_ENABLE_DEBUG_HOOKS
#include <iostream>
#include <list>
#include <algorithm>

static std::list<void*> counted_ptr_counters;

// Use debug hooks to ensure 2 shared_ptrs never point to the same thing
namespace boost {
	
	inline void sp_scalar_constructor_hook(void* object, unsigned long cnt, void* ptr) {
		assert(std::find(counted_ptr_counters.begin(), counted_ptr_counters.end(),
				(void*)object) == counted_ptr_counters.end());
		counted_ptr_counters.push_back(object);
		//std::cerr << "Creating CountedPtr to "
		//	<< object << ", count = " << cnt << std::endl;
	}
	
	inline void sp_scalar_destructor_hook(void* object, unsigned long cnt, void* ptr) {
		counted_ptr_counters.remove(object);
		//std::cerr << "Destroying CountedPtr to "
		//	<< object << ", count = " << cnt << std::endl;
	}

}
#endif // BOOST_SP_ENABLE_DEBUG_HOOKS


#include <boost/shared_ptr.hpp>

#define CountedPtr boost::shared_ptr
#define PtrCast    boost::dynamic_pointer_cast

#endif // COUNTED_PTR_H

