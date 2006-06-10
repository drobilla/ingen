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

/** @file
 * Explicit template instantiations.
 *
 * Need to do this to avoid undefined references, because GCC doesn't seem to
 * know how to recursively instantiate templates.  Cleaner to do it all here
 * than pollute everything with it. :/
 */

#include "Tree.h"
#include "TreeImplementation.h"
#include "OmObject.h"
#include "Node.h"


/* Tree */
template class Tree<Om::Node*>;
template class TreeNode<Om::Node*>;

template                          Tree<Om::OmObject*>::Tree();
template                          Tree<Om::OmObject*>::~Tree();
template void                     Tree<Om::OmObject*>::insert(TreeNode<Om::OmObject*>* const n);
template TreeNode<Om::OmObject*>* Tree<Om::OmObject*>::remove(const string& key);
template Om::OmObject*            Tree<Om::OmObject*>::find(const string& key) const;
template TreeNode<Om::OmObject*>* Tree<Om::OmObject*>::find_treenode(const string& key) const;

template Tree<Om::OmObject*>::iterator Tree<Om::OmObject*>::begin() const;
template Tree<Om::OmObject*>::iterator Tree<Om::OmObject*>::end() const;

template Tree<Om::OmObject*>::iterator::~iterator();
template Om::OmObject* Tree<Om::OmObject*>::iterator::operator*() const;
template Tree<Om::OmObject*>::iterator&     Tree<Om::OmObject*>::iterator::operator++();
template bool          Tree<Om::OmObject*>::iterator::operator!=(const iterator& iter) const;

