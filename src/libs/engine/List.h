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
	ListNode(T elem) : m_elem(elem), m_next(NULL), m_prev(NULL) {}
	virtual ~ListNode() {}

	ListNode* next() const       { return m_next; }
	void      next(ListNode* ln) { m_next = ln; }
	ListNode* prev() const       { return m_prev; }
	void      prev(ListNode* ln) { m_prev = ln; }
	T&        elem()             { return m_elem;}
	const T&  elem() const       { return m_elem; }
	
private:
	// Prevent copies (undefined)
	ListNode(const ListNode& copy);
	ListNode& operator=(const ListNode& copy);

	T         m_elem;
	ListNode* m_next;
	ListNode* m_prev;
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
	List() : m_head(NULL), m_tail(NULL), m_size(0), m_end_iter(this), m_const_end_iter(this)
	{
		m_end_iter.m_listnode = NULL;
		m_end_iter.m_next = NULL;
		m_const_end_iter.m_listnode = NULL;
		m_const_end_iter.m_next = NULL;
	}
	~List();

	void          push_back(ListNode<T>* elem);
	ListNode<T>*  remove(const T elem);

	void clear();
	size_t size() const { return m_size; }

	class iterator;

	/** Realtime safe const iterator for a List. */
	class const_iterator
	{
	public:
		const_iterator(const List<T>* const list);
		const_iterator(const iterator& i)
		: m_list(i.m_list), m_listnode(i.m_listnode), m_next(i.m_next) {}
	
		inline const T&        operator*();
		inline const_iterator& operator++();
		inline bool            operator!=(const const_iterator& iter) const;
		inline bool            operator!=(const iterator& iter) const;
	
		friend class List<T>;
		
	private:
		const List<T>* const m_list;
		const ListNode<T>*   m_listnode;
		const ListNode<T>*   m_next;  // use this instead of m_listnode->next() to allow deleting
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
		const List<T>* m_list;
		ListNode<T>*   m_listnode;
		ListNode<T>*   m_next;  // use this instead of m_listnode->next() to allow deleting
	};

	
	ListNode<T>* remove(const iterator iter);

	iterator begin();
	const iterator end() const;
	
	const_iterator begin() const;
	//const_iterator end()   const;

private:
	// Prevent copies (undefined)
	List(const List& copy);
	List& operator=(const List& copy);

	ListNode<T>*   m_head;
	ListNode<T>*   m_tail;
	size_t         m_size;
	iterator       m_end_iter;
	const_iterator m_const_end_iter;
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
	if (m_head == NULL) return;
	
	ListNode<T>* node = m_head;
	ListNode<T>* next = NULL;
	
	while (node != NULL) {
		next = node->next();
		delete node;
		node = next;
	}
	m_tail = m_head = NULL;
	m_size = 0;
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
	if (m_head == NULL) {
		ln->prev(NULL);
		m_head = m_tail = ln;
	} else {
		ln->prev(m_tail);
		m_tail->next(ln);
		m_tail = ln;
	}
	++m_size;
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
	ListNode<T>* n = m_head;
	while (n != NULL) {
		if (n->elem() == elem)
			break;
		n = n->next();
	}
	if (n != NULL) {
		if (n == m_head) m_head = m_head->next();
		if (n == m_tail) m_tail = m_tail->prev();
		if (n->prev() != NULL)
			n->prev()->next(n->next());
		if (n->next() != NULL)
			n->next()->prev(n->prev());
		--m_size;
		if (m_size == 0) m_head = m_tail = NULL; // FIXME: Shouldn't be necessary
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
	ListNode<T>* n = iter.m_listnode;
	if (n != NULL) {
		if (n == m_head) m_head = m_head->next();
		if (n == m_tail) m_tail = m_tail->prev();
		if (n->prev() != NULL)
			n->prev()->next(n->next());
		if (n->next() != NULL)
			n->next()->prev(n->prev());
		--m_size;
		if (m_size == 0) m_head = m_tail = NULL; // FIXME: Shouldn't be necessary
		return n;
	}
	return NULL;
}


//// Iterator stuff ////

template <typename T>
List<T>::iterator::iterator(List<T>* list)
: m_list(list),
  m_listnode(NULL),
  m_next(NULL)
{
}


template <typename T>
T&
List<T>::iterator::operator*()
{
	assert(m_listnode != NULL);
	return m_listnode->elem();
}


template <typename T>
inline typename List<T>::iterator&
List<T>::iterator::operator++()
{
	assert(m_listnode != NULL);
	m_listnode = m_next;
	if (m_next != NULL)
		m_next = m_next->next();
	else
		m_next = NULL;

	return *this;
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const iterator& iter) const
{
	return (m_listnode != iter.m_listnode);
}


template <typename T>
inline bool
List<T>::iterator::operator!=(const const_iterator& iter) const
{
	return (m_listnode != iter.m_listnode);
}


template <typename T>
inline typename List<T>::iterator
List<T>::begin()
{
	typename List<T>::iterator iter(this);
	iter.m_listnode = m_head;
	if (m_head != NULL)
		iter.m_next = m_head->next();
	else
		iter.m_next = NULL;
	return iter;
}


template <typename T>
inline const typename List<T>::iterator
List<T>::end() const
{
	/*typename List<T>::iterator iter(this);
	iter.m_listnode = NULL;
	iter.m_next = NULL;
	return iter;*/
	return m_end_iter;
}



/// const_iterator stuff ///


template <typename T>
List<T>::const_iterator::const_iterator(const List<T>* const list)
: m_list(list),
  m_listnode(NULL),
  m_next(NULL)
{
}


template <typename T>
const T&
List<T>::const_iterator::operator*() 
{
	assert(m_listnode != NULL);
	return m_listnode->elem();
}


template <typename T>
inline typename List<T>::const_iterator&
List<T>::const_iterator::operator++()
{
	assert(m_listnode != NULL);
	m_listnode = m_next;
	if (m_next != NULL)
		m_next = m_next->next();
	else
		m_next = NULL;

	return *this;
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const const_iterator& iter) const
{
	return (m_listnode != iter.m_listnode);
}


template <typename T>
inline bool
List<T>::const_iterator::operator!=(const iterator& iter) const
{
	return (m_listnode != iter.m_listnode);
}


template <typename T>
inline typename List<T>::const_iterator
List<T>::begin() const
{
	typename List<T>::const_iterator iter(this);
	iter.m_listnode = m_head;
	if (m_head != NULL)
		iter.m_next = m_head->next();
	else
		iter.m_next = NULL;
	return iter;
}

#if 0
template <typename T>
inline typename List<T>::const_iterator
List<T>::end() const
{
	/*typename List<T>::const_iterator iter(this);
	iter.m_listnode = NULL;
	iter.m_next = NULL;
	return iter;*/
	return m_const_end_iter;
}
#endif

#endif // LIST_H
