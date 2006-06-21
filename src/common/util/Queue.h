/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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

#ifndef QUEUE_H
#define QUEUE_H

#include <cassert>
#include <cstdlib>


/** Realtime-safe single-reader single-writer queue (aka lock-free ringbuffer)
 *
 * Implemented as a dequeue in a fixed array.  This is read/write thread-safe, 
 * pushing and popping may occur simultaneously by seperate threads, but
 * the push and pop operations themselves are not thread-safe.
 *
 * FIXME: Verify atomicity of everything here.
 */
template <typename T>
class Queue
{
public:
	Queue(size_t size);
	~Queue();
	
	inline bool is_empty() const;
	inline bool is_full()  const;
	
	inline size_t capacity() const { return m_size-1; }
	inline size_t fill() const;

	inline T&   front() const;

	inline bool push(T obj);
	inline T&   pop();
	
private:
	// Prevent copies (these are undefined)
	Queue(const Queue& copy);
	Queue& operator=(const Queue& copy);
	
	volatile size_t m_front;   ///< Index to front of queue (circular)
	volatile size_t m_back;    ///< Index to back of queue (one past last element) (circular)
	const    size_t m_size;    ///< Size of @ref m_objects (you can store m_size-1 objects)
	T* const        m_objects; ///< Fixed array containing queued elements
};


template<typename T>
Queue<T>::Queue(size_t size)
: m_front(0),
  m_back(0),
  m_size(size+1),
  m_objects((T*)calloc(m_size, sizeof(T)))
{
}


template <typename T>
Queue<T>::~Queue()
{
	free(m_objects);
}


/** Return whether or not the queue is empty.
 */
template <typename T>
inline bool
Queue<T>::is_empty() const
{
	return (m_back == m_front);
}


/** Return whether or not the queue is full.
 */
template <typename T>
inline bool
Queue<T>::is_full() const
{
	// FIXME: This can probably be faster
	return (fill() == capacity());
}


/** Returns how many elements are currently in the queue.
 */
template <typename T>
inline size_t
Queue<T>::fill() const
{
	return (m_back + m_size - m_front) % m_size;
}


/** Return the element at the front of the queue without removing it
 */
template <typename T>
inline T&
Queue<T>::front() const
{
	return m_objects[m_front];
}


/** Push an item onto the back of the Queue - realtime-safe, not thread-safe.
 *
 * @returns true if @a elem was successfully pushed onto the queue,
 * false otherwise (queue is full).
 */
template <typename T>
inline bool
Queue<T>::push(T elem)
{
	if (is_full()) {
		return false;
	} else {
		m_objects[m_back] = elem;
		m_back = (m_back + 1) % (m_size);
		return true;
	}
}


/** Pop an item off the front of the queue - realtime-safe, not thread-safe.
 *
 * It is a fatal error to call pop() when the queue is empty.
 *
 * @returns the element popped.
 */
template <typename T>
inline T&
Queue<T>::pop()
{
	assert(!is_empty());

	T& r = m_objects[m_front];
	m_front = (m_front + 1) % (m_size);
	
	return r;
}


#endif // QUEUE_H
