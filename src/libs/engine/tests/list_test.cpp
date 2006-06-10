#include "../List.h"
#include <iostream>
#include <cstddef>

using std::cout; using std::endl;


int main()
{
	List<int> l;

	l.push_back(new ListNode<int>(1));
	l.push_back(new ListNode<int>(2));
	l.push_back(new ListNode<int>(3));
	l.push_back(new ListNode<int>(4));
	l.push_back(new ListNode<int>(5));
	l.push_back(new ListNode<int>(6));
	l.push_back(new ListNode<int>(7));
	l.push_back(new ListNode<int>(8));

	cout << "List:" << endl;
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;


	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 4)
			l.remove(i);
	}

	std::cerr << "Removed 4 (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	l.remove(1);

	std::cerr << "Removed 1 (head) (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 2)
			l.remove(i);
	}

	std::cerr << "Removed 2 (head) (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	l.remove(5);

	std::cerr << "Removed 5 (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	l.remove(8);

	std::cerr << "Removed 8 (tail) (by value)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;
	
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		if ((*i) == 7)
			l.remove(i);
	}

	std::cerr << "Removed 7 (tail) (by iterator)...\n";
	for (List<int>::iterator i = l.begin(); i != l.end(); ++i) {
		cout << *i << endl;
	}
	cout << endl;

	List<int> r;
	r.push_back(new ListNode<int>(9));
	r.remove(9);
	std::cerr << "Should not see ANY numbers:\n";
	for (List<int>::iterator i = r.begin(); i != r.end(); ++i) {
		cout << *i << endl;
	}
	return 0;
}
