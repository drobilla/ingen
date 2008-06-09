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

#ifndef UTIL_HPP
#define UTIL_HPP

#include CONFIG_H_PATH
#include <iostream>
#include <cstdlib>

#include <fenv.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

#ifdef USE_ASSEMBLY
# if SIZEOF_VOID_P==8
#  define cpuid(a,b,c,d,n) asm("xchgq %%rbx, %1; cpuid; xchgq %%rbx, %1": "=a" (a), "=r" (b), "=c" (c), "=d" (d) : "a" (n));
# else
#  define cpuid(a,b,c,d,n) asm("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1": "=a" (a), "=r" (b), "=c" (c), "=d" (d) : "a" (n));
# endif
#endif

namespace Ingen {

/** Set flags to disable denormal processing.
 */
inline void
set_denormal_flags()
{
#ifdef USE_ASSEMBLY
#ifdef __SSE__
	unsigned long a, b, c, d0, d1;
	int stepping, model, family, extfamily;

	cpuid(a,b,c,d1,1);
	if (d1 & 1<<25) { /* It has SSE support */
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		family = (a >> 8) & 0xf;
		extfamily = (a >> 20) & 0xff;
		model = (a >> 4) & 0xf;
		stepping = a & 0xf;
		cpuid(a,b,c,d0,0);
		if (b == 0x756e6547) { /* It's an Intel */
			if (family == 15 && extfamily == 0 && model == 0 && stepping < 7) {
				return;
			}
		}
		if (d1 & 1<<26) { /* bit 26, SSE2 support */
			_mm_setcsr(_mm_getcsr() | 0x8040); // set DAZ and FZ bits of MXCSR
			//cerr << "Set SSE denormal fix flag." << endl;
		}
	} else {
		std::cerr << "This code has been built with SSE support, but your processor does"
			<< " not support the SSE instruction set." << std::endl << "Exiting." << std::endl;
		exit(EXIT_FAILURE);
	}
#endif
#endif
}

} // namespace Ingen

#endif // UTIL_HPP
