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


/** Simple reference counted pointer.
 *
 * Allocates one counter on the heap on initial construction.  You can safely
 * cast a CountedPtr<X> to a CountedPtr<Y> iff Y is a base of X (eg the cast
 * will only compile if it is a valid up-cast).
 *
 * It is possible for this to be a NULL pointer, and a boolean conversion
 * operator is provided so you can test for this with "if (someCountedPtr)".
 * Dereferencing a NULL CountedPtr will result in a failed assertion if
 * debugging is enabled.
 *
 * FIXME: test this more thoroughly
 */
template <class T>
class CountedPtr
{
public:
	
	// Declare some other type of CountedPtr as a friend (for casting)
	template <class Y> friend class CountedPtr;


	/** Allocate a new Counter (if p is non-NULL) */
	CountedPtr(T* p)
	: _counter(NULL)
	{
		if (p)
			_counter = new Counter(p);
	}

	/** Make a NULL CountedPtr.
	 * It would be best if this didn't exist, but it makes these storable
	 * in STL containers :/
	 */
	CountedPtr()
	: _counter(NULL)
	{}

	~CountedPtr()
	{
		release();
	}

	/** Copy a CountedPtr with the same type. */
	CountedPtr(const CountedPtr& copy) 
	: _counter(NULL)
	{
		assert(this != &copy);
		
		if (copy)
			retain(copy._counter);

		assert(_counter == copy._counter);
	}
	
	/** Copy a CountedPtr to a valid base class.
	 */
	template <class Y>
	CountedPtr(const CountedPtr<Y>& y)
	: _counter(NULL)
	{
		assert(this != (CountedPtr*)&y);

		// Fail if this is not a valid cast
		if (y) {
#ifdef WITH_RTTI
			T* const casted_y = dynamic_cast<T* const>(y._counter->ptr);
#else 
			T* const casted_y = static_cast<T* const>(y._counter->ptr);
#endif
			if (casted_y) {
				assert(casted_y == y._counter->ptr);
				//release(); // FIXME: leak?
				retain((Counter*)y._counter);
				assert(_counter == (Counter*)y._counter);
			}
		}

		assert(_counter == NULL || _counter == (Counter*)y._counter);
	}

	/** Assign to the value of a CountedPtr of the same type. */
	CountedPtr& operator=(const CountedPtr& copy)
	{
		if (this != &copy) {
			assert(_counter == NULL || _counter != copy._counter);
			release();
			retain(copy._counter);
		}
		assert(_counter == copy._counter);
		return *this;
	}

	/** Assign to the value of a CountedPtr of a different type. */
	template <class Y>
	CountedPtr& operator=(const CountedPtr<Y>& y)
	{
		if (this != (CountedPtr*)&y) {
			assert(_counter != y._counter);
			release();
			retain(y._counter);
		}
		assert(_counter == y._counter);
		return *this;
	}

	inline bool operator==(const CountedPtr& p) const
	{
		return (_counter == p._counter);
	}

	inline bool operator!=(const CountedPtr& p) const
	{
		return (_counter != p._counter);
	}

	/** Allow testing for NULL nicely like a real pointer */
	operator bool() const
	{
		return (_counter && _counter->ptr);
	}

	inline T& operator*() const
	{
		assert(_counter);
		assert(_counter->count > 0);
		assert(_counter->ptr);
		return *_counter->ptr;
	}

	inline T* operator->() const
	{
		assert(_counter);
		assert(_counter->count > 0);
		assert(_counter->ptr);
		return _counter->ptr;
	}

	inline T* get() const { return _counter ? _counter->ptr : 0; }

	bool unique() const { return (_counter ? _counter->count == 1 : true); }

private:
	/** Stored on the heap and referred to by all existing CountedPtr's to
	 * the object */
	struct Counter
	{
		Counter(T* p)
		: ptr(p)
		, count(1)
		{
			assert(p);
		}
		
		T* const        ptr;
		volatile size_t count;
	};

	/** Increment the count */
	void retain(Counter* c) 
	{	
		assert(_counter == NULL || _counter == c);
		_counter = c;
		if (_counter)
			++(c->count);
	}
	
	/** Decrement the count, delete if we're the last reference */
	void release()
	{	
		if (_counter) {
			if (--(_counter->count) == 0) {
				delete _counter->ptr;
				delete _counter;
			}
			_counter = NULL;
		}
	}
	

	Counter* _counter;
};

#endif // COUNTED_PTR_H
