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

#include "Tree.h"
#include <cstdlib>
#include <iostream>
#include <cassert>
using std::cerr; using std::endl;


/* FIXME: this is all in horrible need of a rewrite. */


/** Destroy the tree.
 *
 * Note that this does not delete any TreeNodes still inside the tree,
 * that is the user's responsibility.
 */
template <typename T>
Tree<T>::~Tree()
{
}


/** Insert a node into the tree.  Realtime safe.
 *
 * @a n will be inserted using the key() field for searches.
 * n->key() must not be the empty string.
 */
template<typename T>
void
Tree<T>::insert(TreeNode<T>* const n)
{
	assert(n != NULL);
	assert(n->left_child() == NULL);
	assert(n->right_child() == NULL);
	assert(n->parent() == NULL);
	assert(n->key().length() > 0);
	assert(find_treenode(n->key()) == NULL);

	if (_root == NULL) {
		_root = n;
	} else {
		bool left = false; // which child to insert at
		bool right = false;
		TreeNode<T>* i = _root;
		while (true) {
			assert(i != NULL);
			if (n->key() <= i->key()) {
				if (i->left_child() == NULL) {
					left = true;
					break;
				} else {
					i = i->left_child();
				}
			} else {
				if (i->right_child() == NULL) {
					right = true;
					break;
				} else {
					i = i->right_child();
				}
			}
		}
		
		assert(i != NULL);
		assert(left || right);
		assert( ! (left && right) );
		
		if (left) {
			assert(i->left_child() == NULL);
			i->left_child(n);
		} else if (right) {
			assert(i->right_child() == NULL);
			i->right_child(n);
		} 
		n->parent(i);
	}
	++_size;
}


/** Remove a node from the tree.
 *
 * Realtime safe, caller is responsible to delete returned value.
 *
 * @return NULL if object with @a key is not in tree.
 */
template<typename T>
TreeNode<T>*
Tree<T>::remove(const string& key)
{
	TreeNode<T>* node      = find_treenode(key);
	TreeNode<T>* n         = node;
	TreeNode<T>* swap      = NULL;
	T            temp_node;
	string       temp_key;

	if (node == NULL)
		return NULL;
	
	// Node is not even in tree
	if (node->parent() == NULL && _root != node)
		return NULL;
	// FIXME: What if the node is in a different tree?  Check for this?

#ifndef NDEBUG
	const T& remove_node = node->node(); // for error checking
#endif // NDEBUG
	
	// n has two children
	if (n->left_child() != NULL && n->right_child() != NULL) {
		if (rand()%2)
			swap = _find_largest(n->left_child());
		else
			swap = _find_smallest(n->right_child());
		
		// Swap node's elements
		temp_node = swap->_node;
		swap->_node = n->_node;
		n->_node = temp_node;
		
		// Swap node's keys
		temp_key = swap->_key;
		swap->_key = n->_key;
		n->_key = temp_key;

		n = swap;
		assert(n != NULL);
	}

	// be sure we swapped correctly (ie right node is getting removed)
	assert(n->node() == remove_node);
	
	// n now has at most one child
	assert(n->left_child() == NULL || n->right_child() == NULL);

	if (n->is_leaf()) {
		if (n->is_left_child())
			n->parent()->left_child(NULL);
		else if (n->is_right_child())
			n->parent()->right_child(NULL);
		
		if (_root == n) _root = NULL;
	} else {  // has a single child
		TreeNode<T>* child = NULL;
		if (n->left_child() != NULL)
			child = n->left_child();
		else if (n->right_child() != NULL)
			child = n->right_child();
		else
			exit(EXIT_FAILURE);

		assert(child != n);
		assert(child != NULL);
		assert(n->parent() != n);

		if (n->is_left_child()) {
			assert(n->parent() != child);
			n->parent()->left_child(child);
			child->parent(n->parent());
		} else if (n->is_right_child()) {
			assert(n->parent() != child);
			n->parent()->right_child(child);
			child->parent(n->parent());
		} else {
			child->parent(NULL);
		}
		if (_root == n) _root = child;	
	}
	
	// Be sure node is cut off completely
	assert(n != NULL);
	assert(n->parent() == NULL || n->parent()->left_child() != n);
	assert(n->parent() == NULL || n->parent()->right_child() != n);
	assert(n->left_child() == NULL || n->left_child()->parent() != n);
	assert(n->right_child() == NULL || n->right_child()->parent() != n);
	assert(_root != n);

	n->parent(NULL);
	n->left_child(NULL);
	n->right_child(NULL);

	--_size;

	if (_size == 0) _root = NULL;

	// Be sure right node is being removed
	assert(n->node() == remove_node);
			
	return n;
}


template<typename T>
T
Tree<T>::find(const string& name) const
{
	TreeNode<T>* tn = find_treenode(name);

	return (tn == NULL) ? NULL : tn->node();
}


template<typename T>
TreeNode<T>*
Tree<T>::find_treenode(const string& name) const
{
	TreeNode<T>* i = _root;
	int cmp = 0;
	
	while (i != NULL) {
		cmp = name.compare(i->key());
		if (cmp < 0)
			i = i->left_child();
		else if (cmp > 0)
			i = i->right_child();
		else
			break;
	}

	return i;
}


/// Private ///
template<typename T>
void
Tree<T>::_set_all_traversed_recursive(TreeNode<T>* root, bool b)
{
	assert(root != NULL);
	
	// Preorder traversal
	root->node()->traversed(b);
	if (root->left_child() != NULL)
		_set_all_traversed_recursive(root->left_child(), b);
	if (root->right_child() != NULL)
		_set_all_traversed_recursive(root->right_child(), b);
}


/** Finds the smallest (key) node in the subtree rooted at "root"
 */
template<typename T>
TreeNode<T>*
Tree<T>::_find_smallest(TreeNode<T>* root)
{
	TreeNode<T>* r = root;

	while (r->left_child() != NULL)
		r = r->left_child();

	return r;
}


/** Finds the largest (key) node in the subtree rooted at "root".
 */
template<typename T>
TreeNode<T>*
Tree<T>::_find_largest(TreeNode<T>* root)
{
	TreeNode<T>* r = root;

	while (r->right_child() != NULL)
		r = r->right_child();

	return r;

}



//// Iterator Stuff ////



template<typename T>
Tree<T>::iterator::iterator(const Tree *tree, size_t size)
: _depth(-1),
  _size(size),
  _stack(NULL),
  _tree(tree)
{
	if (size > 0)
		_stack = new TreeNode<T>*[size];
}


template<typename T>
Tree<T>::iterator::~iterator()
{
	delete[] _stack;
}


/* FIXME: Make these next two not memcpy (possibly have to force a single
 * iterator existing at any given time) for speed.
 */

// Copy constructor (for the typical for loop usage)
template<typename T>
Tree<T>::iterator::iterator(const Tree<T>::iterator& copy)
: _depth(copy._depth),
  _size(copy._size),
  _tree(copy._tree)
{
	if (_size > 0) {
		_stack = new TreeNode<T>*[_size];
		memcpy(_stack, copy._stack, _size * sizeof(TreeNode<T>*));
	}
}


// Assignment operator
template<typename T>
typename Tree<T>::iterator&
Tree<T>::iterator::operator=(const Tree<T>::iterator& copy) {
	_depth = copy._depth;
	_size = copy._size;
	_tree = copy._tree;
	
	if (_size > 0) {
		_stack = new TreeNode<T>*[_size];
		memcpy(_stack, copy._stack, _size * sizeof(TreeNode<T>*));
	}
	return *this;
}


template<typename T>
T
Tree<T>::iterator::operator*() const
{
	assert(_depth >= 0);
	return _stack[_depth]->node();
}


template<typename T>
typename Tree<T>::iterator&
Tree<T>::iterator::operator++()
{
	assert(_depth >= 0);

	TreeNode<T>* tn = _stack[_depth];
	--_depth;

	tn = tn->right_child();
	while (tn != NULL) {
		++_depth;
		_stack[_depth] = tn;
		tn = tn->left_child();
	}

	return *this;
}


template<typename T>
bool
Tree<T>::iterator::operator!=(const Tree<T>::iterator& iter) const
{
	// (DeMorgan's Law)
	return (_tree != iter._tree || _depth != iter._depth);
}


template<typename T>
typename Tree<T>::iterator
Tree<T>::begin() const
{
	typename Tree<T>::iterator iter(this, _size);
	iter._depth = -1;
	
	TreeNode<T> *ptr = _root;
	while (ptr != NULL) {
		iter._depth++;
		iter._stack[iter._depth] = ptr;
		ptr = ptr->left_child();
	}

	return iter;
}


template<typename T>
typename Tree<T>::iterator
Tree<T>::end() const
{
	typename Tree<T>::iterator iter(this, 0);
	iter._depth = -1;

	return iter;
}


