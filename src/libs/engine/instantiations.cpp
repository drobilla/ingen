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

/** @file
 * Explicit template instantiations.
 *
 * Need to do this to avoid undefined references, because GCC doesn't seem to
 * know how to recursively instantiate templates.  Cleaner to do it all here
 * than pollute everything with it. :/
 */

#include "Tree.h"
#include "TreeImplementation.h"
#include "GraphObject.h"
#include "Node.h"


/* Tree */
template class Tree<Om::Node*>;
template class TreeNode<Om::Node*>;

template                          Tree<Om::GraphObject*>::Tree();
template                          Tree<Om::GraphObject*>::~Tree();
template void                     Tree<Om::GraphObject*>::insert(TreeNode<Om::GraphObject*>* const n);
template TreeNode<Om::GraphObject*>* Tree<Om::GraphObject*>::remove(const string& key);
template Om::GraphObject*            Tree<Om::GraphObject*>::find(const string& key) const;
template TreeNode<Om::GraphObject*>* Tree<Om::GraphObject*>::find_treenode(const string& key) const;

template Tree<Om::GraphObject*>::iterator Tree<Om::GraphObject*>::begin() const;
template Tree<Om::GraphObject*>::iterator Tree<Om::GraphObject*>::end() const;

template Tree<Om::GraphObject*>::iterator::~iterator();
template Om::GraphObject* Tree<Om::GraphObject*>::iterator::operator*() const;
template Tree<Om::GraphObject*>::iterator&     Tree<Om::GraphObject*>::iterator::operator++();
template bool          Tree<Om::GraphObject*>::iterator::operator!=(const iterator& iter) const;

