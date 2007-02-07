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
template class Tree<Ingen::Node*>;
template class TreeNode<Ingen::Node*>;

template                          Tree<Ingen::GraphObject*>::Tree();
template                          Tree<Ingen::GraphObject*>::~Tree();
template void                     Tree<Ingen::GraphObject*>::insert(TreeNode<Ingen::GraphObject*>* const n);
template TreeNode<Ingen::GraphObject*>* Tree<Ingen::GraphObject*>::remove(const string& key);
template Ingen::GraphObject*            Tree<Ingen::GraphObject*>::find(const string& key) const;
template TreeNode<Ingen::GraphObject*>* Tree<Ingen::GraphObject*>::find_treenode(const string& key) const;

template Tree<Ingen::GraphObject*>::iterator Tree<Ingen::GraphObject*>::begin() const;
template Tree<Ingen::GraphObject*>::iterator Tree<Ingen::GraphObject*>::end() const;

template Tree<Ingen::GraphObject*>::iterator::~iterator();
template Ingen::GraphObject* Tree<Ingen::GraphObject*>::iterator::operator*() const;
template Tree<Ingen::GraphObject*>::iterator&     Tree<Ingen::GraphObject*>::iterator::operator++();
template bool          Tree<Ingen::GraphObject*>::iterator::operator!=(const iterator& iter) const;

