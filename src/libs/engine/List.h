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

#ifndef LIST_H
#define LIST_H

#include <cassert>
#include "types.h"
#include "MaidObject.h"


/** A node in a List.
 *
 * This class is (unusually) exposed to the user to allow list operations
 * to be realtime safe (ie so events can allocate list nodes in other threads
 * and then insert them in the realtime thread.
 */
template <typename T>
class ListNode : public MaidObject
{
public:
	ListNode(T elem) : _elem(elem), _next(NULL), _prev(NULL) {}
	virtual ~ListNode() {}

	ListNode* next() const       { return _next; }
	void      next(ListNode* ln) { _next = ln; }
	ListNode* prev() const       { return _prev; }
	void      prev(ListNode* ln) { _prev = ln; }
	T&        elem()             { return _elem;}
	const T&  elem() const       { return _elem; }
	
private:
	T         _elem;
	ListNode* _next;
	ListNode* _prev;
};



/** A realtime safe, (partially) thread safe doubly linked list.
 * 
 * Elements can be added safely while another thread is reading the list.  See
 * documentation for specific functions for realtime/thread safeness.
 */
template <typename T>
class List : public MaidObject
{
public:
	List() : _head(NULL), _tail(NULL), _size(0), _end_iter(this), _const_end_iter(this)
	{
		_end_iter._listnode = NULL;
		_end_iter._next = NULL;
		_const_end_iter._listnode = NULL;
		_const_end_iter._next = NULL;
	}
	~List();

	void          push_back(ListNode<T>* elem);
	ListNode<T>*  remove(const T elem);

	void clear();
	size_t size() const { return _size; }

	class iterator;

	/** Realtime safe const iterator for a List. */
	class const_iterator
	{
	public:
		const_iterator(const List<T>* const list);
		const_iterator(const iterator& i)
		: _list(i._list), _listnode(i._listnode), _next(i._next) {}
	
		inline const T&        operator*();
		inline const_iterator& operator++();
		inline bool            operator!=(const const_iterator& iter) const;
		inline bool            operator!=(const iterator& iter) const;
	
		friend class List<T>;
		
	private:
		const List<T>* const _list;
		const ListNode<T>*   _listnode;
		const ListNode<T>*   _next;  // use this instead of _listnode->next() to allow deleting
	};


	/** Realtime safe iterator for a List. */
	class iterator
	{
	public:
		iterator(List<T>* const list);

		inline T&        operator*();
		inline iterator& operator++();
		inline bool      operator!=(const iterator& iter) const;
		inline bool      operator!=(const const_iterator& iter) const;
	
		friend class List<T>;
		friend class List<T>::const_iterator;

	private:
		const List<T>* _list;
		ListNode<T>*   _listnode;
		ListNode<T>*   _next;  // use this instead of _listnode->next() to allow deleting
	};

	
	ListNode<T>* remove(const iterator iter);

	iterator begin();
	const iterator end() const;
	
	const_iterator begin() const;
	//const_iterator end()   const;

private:
	ListNode<T>*   _head;
	ListNode<T>*   _tail;
	size_t         _size;
	iterator       _end_iter;
	const_iterator _const_end_iter;
};




template <typename T>
List<T>::~List<T>()
{
	clear();
}


/** Clear the list, deleting all ListNodes contained (but NOT their contents!)
 *
 * Not realtime safe.
 */
template <typename T>
void
List<T>::clear()
{
	if (_head == NULL) return;
	
	ListNode<T>* node = _head;
	ListNode<T>* next = NULL;
	
	while (node != NULL) {
		next = node->next();
		delete node;
		node = next;
	}
	_tail = _head = NULL;
	_size = 0;
}


/** Add an element to the list.
 *
 * This method can be called while another thread is reading the list.
 * Realtime safe.
 */
template <typename T>
void
List<T>::push_back(ListNode<T>* const ln)
{
	assert(ln != NULL);

	ln->next(NULL);
	// FIXME: atomicity?  relevant?
	if (_head == NULL) {
		ln->prev(NULL);
		_head = _tail = ln;
	} else {
		ln->prev(_tail);
		_tail->next(ln);
		_tail = ln;
	}
	++_size;
}


/** Remove an element from the list.
 *
 * This function is realtime safe - it is the caller's responsibility to
 * delete the returned ListNode, or there will be a leak.
 */
template <typename T>
ListNode<T>*
List<T>::remove(const T elem)
{
	// FIXME: atomicity?
	ListNode<T>* n = _head;
	while (n != NULL) {
		if (n->elem() == elem)
			break;
		n = n->next();
	}
	if (n != NULL) {
		if (n == _head) _head = _head->next();
		if (n == _tail) _tail = _tail->prev();
		if (n->prev() != NULL)
			n->prev()->next(n->next());
		if (n->next() != NULL)
			n->next()->prev(n->prev());
		--_size;
		if (_size == 0) _head = _tail = NULL; // FIXME: Shouldn't be necessary
		return n;
	}
	return NULL;
}


/** Remove an element from the list using an iterator.
 * 
 * This function is realtime safe - it is the caller's responsibility to
 * delete the returned ListNode, or there will be a leak.
 */
template <typename T>
ListNode<T>*
List<T>::remove(const iterator iter)
{
	ListNode<T>* n = iter._listnode;
	if (n != NULL) {
		if (n == _head) _head = _head->next();
		if (n == _tail) _tail = _tail->prev();
		if (n->prev() != NULL)
			n->prev()->next(n->next());
		if (n->next() != NULL)
			n->next()->prev(n->prev());
		--_size;
		if (_size == 0) _head = _tail = NULL; // FIXME: Shouldn't be necessary
		return n;
	}
	return NULL;
}


//// Iterator stuff ////

template <typename T>
List<T>::iterator::iterator(List<T>* list)
: _list(list),
  _listnode(NULL),
  _next(NULL)
{
}


template <typename T>
T&
List<T>::iterator::operator*()
{
	assert(_listnode != NULL);
	return _listnode->elem();
}


template <typename T>
inline typename List<T>::iterator&
List<T>::iterator::operator++()
{
	assert(_listnode != NULL);
	_listnode = _next;
	if (_next != NULL)
		_next = _next->next();
	else
		_next = NULL;

	return *this;
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const const_iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline typename List<T>::iterator
List<T>::begin()
{
	typename List<T>::iterator iter(this);
	iter._listnode = _head;
	if (_head != NULL)
		iter._next = _head->next();
	else
		iter._next = NULL;
	return iter;
}


template <typename T>
inline const typename List<T>::iterator
List<T>::end() const
{
	/*typename List<T>::iterator iter(this);
	iter._listnode = NULL;
	iter._next = NULL;
	return iter;*/
	return _end_iter;
}



/// const_iterator stuff ///


template <typename T>
List<T>::const_iterator::const_iterator(const List<T>* const list)
: _list(list),
  _listnode(NULL),
  _next(NULL)
{
}


template <typename T>
const T&
List<T>::const_iterator::operator*() 
{
	assert(_listnode != NULL);
	return _listnode->elem();
}


template <typename T>
inline typename List<T>::const_iterator&
List<T>::const_iterator::operator++()
{
	assert(_listnode != NULL);
	_listnode = _next;
	if (_next != NULL)
		_next = _next->next();
	else
		_next = NULL;

	return *this;
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const const_iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const iterator& iter) const
{
	return (_listnode != iter._listnode);
}


template <typename T>
inline typename List<T>::const_iterator
List<T>::begin() const
{
	typename List<T>::const_iterator iter(this);
	iter._listnode = _head;
	if (_head != NULL)
		iter._next = _head->next();
	else
		iter._next = NULL;
	return iter;
}

#if 0
template <typename T>
inline typename List<T>::const_iterator
List<T>::end() const
{
	/*typename List<T>::const_iterator iter(this);
	iter._listnode = NULL;
	iter._next = NULL;
	return iter;*/
	return _const_end_iter;
}
#endif

#endif // LIST_H
