/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ARRAY_H
#define ARRAY_H

#include "MaidObject.h"
#include <cassert>
#include <cstdlib>
#include "util/types.h"


/** An array.
 * 
 * Has a stack-like push_back() too, for find_process_order...
 */
template <class T>
class Array : public MaidObject
{
public:
	Array(size_t size = 0) : m_size(size), m_top(0), m_elems(NULL) {
		if (size > 0)
			m_elems = new T[size];
	}
	
	Array(size_t size, T initial_value) : m_size(size), m_top(0), m_elems(NULL) {
		if (size > 0) {
			m_elems = new T[size];
			for (size_t i=0; i < size; ++i)
				m_elems[i] = initial_value;
		}
	}
	
	Array(size_t size, const Array<T>* contents) : m_size(size), m_top(size+1) {
		m_elems = new T[size];
		if (contents) {
			if (size <= contents->size())
				memcpy(m_elems, contents->m_elems, size * sizeof(T));
			else
				memcpy(m_elems, contents->m_elems, contents->size() * sizeof(T));
		}
	}

	~Array() {
		free();
	}

	void alloc(size_t num_elems) {
		assert(num_elems > 0);
		
		delete[] m_elems;
		m_size = num_elems;
		m_top = 0;
		
		m_elems = new T[num_elems];
	}
	
	void alloc(size_t num_elems, T initial_value) {
		assert(num_elems > 0);
		
		delete[] m_elems;
		m_size = num_elems;
		m_top = 0;

		m_elems = new T[num_elems];
		for (size_t i=0; i < m_size; ++i)
			m_elems[i] = initial_value;
	}
	
	void free() {
		delete[] m_elems;
		m_size = 0;
		m_top = 0;
	}

	void push_back(T n) {
		assert(m_top < m_size);
		m_elems[m_top++] = n;
	}
	
	inline size_t size() const  { return m_size; }

	inline T& operator[](size_t i) const { assert(i < m_size); return m_elems[i]; }
	
	inline T& at(size_t i) const { assert(i < m_size); return m_elems[i]; }

private:
	// Disallow copies (undefined)
	Array(const Array& copy);
	Array& operator=(const Array& copy);

	size_t m_size;
	size_t m_top; // points to empty element above "top" element
	T*     m_elems;
};


#endif // ARRAY_H
