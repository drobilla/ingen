#include <iostream>
#include "../NodeTree.h"
#include "../NodeBase.h"

using std::cout; using std::cerr; using std::endl;

int main()
{
	cout << "\n\n\n\n";
	
	NodeBase* n = NULL;
	TreeNode* tn = NULL;
	TreeNode* baz = NULL;
	TreeNode* quuux = NULL;
	TreeNode* bar = NULL;
	
	NodeTree tree;
	n = new NodeBase("foo", 0, 0, 0);
	tn = new TreeNode(n);
	tree.insert(tn);
	n = new NodeBase("bar", 0, 0, 0);
	bar = new TreeNode(n);
	tree.insert(bar);
	n = new NodeBase("baz", 0, 0, 0);
	baz = new TreeNode(n);
	tree.insert(baz);
	n = new NodeBase("quux", 0, 0, 0);
	tn = new TreeNode(n);
	tree.insert(tn);
	n = new NodeBase("quuux", 0, 0, 0);
	quuux = new TreeNode(n);
	tree.insert(quuux);
	n = new NodeBase("quuuux", 0, 0, 0);
	tn = new TreeNode(n);
	tree.insert(tn);


	cout << "Added 6 nodes." << endl;
	cout << "Tree size: " << tree.size() << endl << endl;

	cout << "Iterating: " << endl;
	for (NodeTree::iterator i = tree.begin(); i != tree.end(); ++i) {
		cout << (*i)->name() << endl;
	}
	
	cout << endl << "Search 'foo' - " << tree.find("foo")->name() << endl;
	cout << endl << "Search 'bar' - " << tree.find("bar")->name() << endl;
	cout << endl << "Search 'baz' - " << tree.find("baz")->name() << endl;
	cout << endl << "Search 'quux' - " << tree.find("quux")->name() << endl;
	cout << endl << "Search 'quuux' - " << tree.find("quuux")->name() << endl;
	cout << endl << "Search 'quuuux' - " << tree.find("quuuux")->name() << endl;
	cout << endl << "Search 'dave' - " << tree.find("dave") << endl;
	cout << endl << "Search 'fo' - " << tree.find("fo") << endl << endl;
	
	cout << "Removing 'baz'." << endl;
	tree.remove(baz);
	
	cout << "Iterating: " << endl;
	for (NodeTree::iterator i = tree.begin(); i != tree.end(); ++i) {
		cout << (*i)->name() << endl;
	}
	
	cout << "Removing 'bar' (the root): " << endl;
	tree.remove(bar);

	cout << "Iterating: " << endl;
	for (NodeTree::iterator i = tree.begin(); i != tree.end(); ++i) {
		cout << (*i)->name() << endl;
	}
	
	return 0;
}
