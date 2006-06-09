#include <cstdlib>
#include <iostream>
#include <vector>
#include "../Tree.h"
#include "../TreeImplementation.h"

using std::vector;
using std::cout; using std::cerr; using std::endl;

static const uint NUM_NODES = 20;

int
main()
{
	cout << "\n\n\n\n";
	
	Tree<string>   tree;
	vector<string> names;
	vector<bool>   in_tree; // arrays
	uint           num_in_tree = 0;
	
	
	string name;
	
	for (uint i=0; i < NUM_NODES; ++i) {
		name = (char)(i+'A');
		TreeNode<string>* n = new TreeNode<string>(name, name);
		tree.insert(n);
		names.push_back(name);
		in_tree.push_back(true);
		++num_in_tree;
	}

	cout << "Added " << NUM_NODES << " nodes." << endl;
	cout << "Tree size: " << tree.size() << endl << endl;

	cout << "Tree contents: " << endl;
	for (Tree<string>::iterator i = tree.begin(); i != tree.end(); ++i) {
		cout << (*i) << ", ";
	}
	cout << endl;

	
	while (true) {
		bool insert;
		int num = rand() % NUM_NODES;
		
		if (num_in_tree == 0)
			insert = true;
		else if (num_in_tree == NUM_NODES)
			insert = false;
		else {
			while (true) {
				insert = rand() % 2;
				num = rand() % NUM_NODES;
				if ((insert && !in_tree[num]) || (!insert && in_tree[num]))
					break;
			}
		}

		string name = names[num];
		
		if (insert) {
			assert(in_tree[num] == false);
			cout << "\nInserting '" << name << "'" << endl;
			tree.insert(new TreeNode<string>(name, name));
			in_tree[num] = true;
			++num_in_tree;
			cout << "Tree size: " << tree.size() << endl;
			assert(num_in_tree == tree.size());
		} else {	
			assert(in_tree[num] == true);
			cout << "\nRemoving '" << name << "'" << endl;
			TreeNode<string>* removed = tree.remove(name);
			assert(removed != NULL);
			assert(removed->node() == name);
			assert(removed->key() == name);
			delete removed;
			in_tree[num] = false;
			assert(names[num] == name);
			--num_in_tree;
			cout << "Tree size: " << tree.size() << endl;
			assert(num_in_tree == tree.size());
		}
		assert(num_in_tree == tree.size());
		cout << "Tree contents: " << endl;
		for (Tree<string>::iterator i = tree.begin(); i != tree.end(); ++i) {
			cout << (*i) << ", ";
		}
		cout << endl;
	}
	
	return 0;
}
