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

#ifndef TREE_H
#define TREE_H

#include <string>
#include <cassert>
#include <boost/utility.hpp>
#include <raul/Deletable.hpp>
using std::string;

template<typename T> class Tree;


/** A node in a Tree.
 */
template <typename T>
class TreeNode : public Raul::Deletable
{
public:
	TreeNode(const string& key)
	: _parent(NULL), _left_child(NULL), _right_child(NULL),
	  _key(key), _node(NULL) {}

	TreeNode(const string& key, T n)
	: _parent(NULL), _left_child(NULL), _right_child(NULL),
	  _key(key), _node(n) {}

	~TreeNode() {
		assert(_parent == NULL || _parent->left_child() != this);
		assert(_parent == NULL || _parent->right_child() != this);
		assert(_left_child == NULL || _left_child->parent() != this);
		assert(_right_child == NULL || _right_child->parent() != this);
		_parent = _left_child = _right_child = NULL;
	}
	
	string       key() const                 { return _key; }
	void         key(const string& key)      { _key = key; }
	TreeNode<T>* parent() const              { return _parent; }
	void         parent(TreeNode<T>* n)      { _parent = n; }
	TreeNode<T>* left_child() const          { return _left_child; }
	void         left_child(TreeNode<T>* n)  { _left_child = n; }
	TreeNode<T>* right_child() const         { return _right_child; }
	void         right_child(TreeNode<T>* n) { _right_child = n; }
	
	bool is_leaf()        { return (_left_child == NULL && _right_child == NULL); }
	bool is_left_child()  { return (_parent != NULL && _parent->left_child() == this); }
	bool is_right_child() { return (_parent != NULL && _parent->right_child() == this); }

	T node() { return _node; }
	
	friend class Tree<T>;
	
protected:
	TreeNode<T>* _parent;
	TreeNode<T>* _left_child;
	TreeNode<T>* _right_child;
	string       _key;
	T            _node;
};


/** The tree all objects are stored in.
 *
 * Textbook naive (unbalanced) Binary Search Tree.  Slightly different
 * from a usual BST implementation in that the "Node" classes (TreeNode) are
 * exposed to the user.  This is so QueuedEvent's can create the TreeNode in
 * another thread, and the realtime jack thread can insert them (without having
 * to allocating a TreeNode which is a no-no).
 *
 * It's also a more annoying implementation because there's no leaf type (since
 * a leaf object would have to be deleted on insert).
 *
 * Tree<T>::iterator is not realtime safe, but the insert/remove/find methods
 * of Tree<T> do not use them.
 */
template <typename T>
class Tree : boost::noncopyable
{
public:
	Tree() : _root(0), _size(0) {}
	~Tree();

	void         insert(TreeNode<T>* const n);
	TreeNode<T>* remove(const string& key);
	T            find(const string& key) const;
	TreeNode<T>* find_treenode(const string& key) const;

	size_t size() const { return _size; }
	
	/** NON realtime safe iterator for a Tree<T>. */
	class iterator
	{
	public:
		iterator(const Tree<T>* tree, size_t size);
		~iterator();
	
		T         operator*() const;
		iterator& operator++();
		bool      operator!=(const iterator& iter) const;
	
		friend class Tree<T>;

		iterator(const iterator& copy);
		iterator& operator=(const iterator& copy);

	private:
		int            _depth;
		size_t         _size;
		TreeNode<T>**  _stack;
		const Tree<T>* _tree;
	};

	iterator begin() const;
	iterator end() const;

private:
	TreeNode<T>* _find_smallest(TreeNode<T>* root);
	TreeNode<T>* _find_largest(TreeNode<T>* root);

	TreeNode<T>* _root;
	size_t       _size;
};


#endif // TREE_H
