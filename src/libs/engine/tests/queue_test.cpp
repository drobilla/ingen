#include <iostream>
#include <string>
#include "../../common/Queue.h"

using std::string; using std::cerr; using std::cout; using std::endl;


int main()
{
	Queue<int> q(10);

	cout << "New queue.  Should be empty: " << q.is_empty() << endl;
	cout << "Capacity: " << q.capacity() << endl;
	cout << "Fill: " << q.fill() << endl;
	
	for (uint i=0; i < 5; ++i) {
		q.push(i);
		assert(!q.is_full());
		q.pop();
	}
	cout << "Pushed and popped 5 elements.  Queue should be empty: " << q.is_empty() << endl;
	cout << "Fill: " << q.fill() << endl;

	for (uint i=10; i < 20; ++i) {
		q.push(i);
	}
	cout << "Pushed 10 elements.  Queue should be full: " << q.is_full() << endl;
	cout << "Fill: " << q.fill() << endl;

	cout << "The digits 10->19 should print: " << endl;
	while (!q.is_empty()) {
		int foo = q.pop();
		cout << "Popped: " << foo << endl;
	}
	cout << "Queue should be empty: " << q.is_empty() << endl;
	cout << "Fill: " << q.fill() << endl;
	
	cout << "Attempting to add eleven elements to queue of size 10. Only first 10 should succeed:" << endl;
	for (uint i=20; i <= 39; ++i) {
		cout << i;
		cout << " - Fill: " << q.fill();
		cout << ", is full: " << q.is_full();
		cout << ", succeeded: " << q.push(i) << endl;
	}
	
	return 0;
}
