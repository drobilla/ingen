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

#ifndef NODETREE_H
#define NODETREE_H

#include <string>
#include <cassert>
#include "MaidObject.h"
using std::string;

template<typename T> class Tree;


/** A node in a Tree.
 */
template <typename T>
class TreeNode : public MaidObject
{
public:
	TreeNode(const string& key)
	: m_parent(NULL), m_left_child(NULL), m_right_child(NULL),
	  m_key(key), m_node(NULL) {}

	TreeNode(const string& key, T n)
	: m_parent(NULL), m_left_child(NULL), m_right_child(NULL),
	  m_key(key), m_node(n) {}

	~TreeNode() {
		assert(m_parent == NULL || m_parent->left_child() != this);
		assert(m_parent == NULL || m_parent->right_child() != this);
		assert(m_left_child == NULL || m_left_child->parent() != this);
		assert(m_right_child == NULL || m_right_child->parent() != this);
		m_parent = m_left_child = m_right_child = NULL;
	}
	
	string       key() const                 { return m_key; }
	void         key(const string& key)      { m_key = key; }
	TreeNode<T>* parent() const              { return m_parent; }
	void         parent(TreeNode<T>* n)      { m_parent = n; }
	TreeNode<T>* left_child() const          { return m_left_child; }
	void         left_child(TreeNode<T>* n)  { m_left_child = n; }
	TreeNode<T>* right_child() const         { return m_right_child; }
	void         right_child(TreeNode<T>* n) { m_right_child = n; }
	
	bool is_leaf()        { return (m_left_child == NULL && m_right_child == NULL); }
	bool is_left_child()  { return (m_parent != NULL && m_parent->left_child() == this); }
	bool is_right_child() { return (m_parent != NULL && m_parent->right_child() == this); }

	T node() { return m_node; }
	
	friend class Tree<T>;
	
protected:
	// Prevent copies (undefined)
	TreeNode(const TreeNode&);
	TreeNode& operator=(const TreeNode&);
	
	TreeNode<T>* m_parent;
	TreeNode<T>* m_left_child;
	TreeNode<T>* m_right_child;
	string       m_key;
	T            m_node;
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
class Tree
{
public:
	Tree<T>() : m_root(0), m_size(0) {}
	~Tree<T>();

	void         insert(TreeNode<T>* const n);
	TreeNode<T>* remove(const string& key);
	T            find(const string& key) const;
	TreeNode<T>* find_treenode(const string& key) const;

	size_t size() const { return m_size; }
	
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
		int            m_depth;
		size_t         m_size;
		TreeNode<T>**  m_stack;
		const Tree<T>* m_tree;
	};

	iterator begin() const;
	iterator end() const;

private:
	// Prevent copies (undefined)
	Tree<T>(const Tree<T>&);
	Tree<T>& operator=(const Tree<T>&);
	
	void m_set_all_traversed_recursive(TreeNode<T>* root, bool b);
	
	TreeNode<T>* m_find_smallest(TreeNode<T>* root);
	TreeNode<T>* m_find_largest(TreeNode<T>* root);

	TreeNode<T>* m_root;
	size_t       m_size;
};


/* This needs to be done so the templates are defined and can get instantiated
 * automatically by the compilter.
 */
//#include "TreeImplementation.h"


#endif // NODETREE_H
