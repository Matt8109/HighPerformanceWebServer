#include <map>
#include <iostream>

#include "callback.hpp"
using namespace std;
using namespace base;

class test {
	public:
		void test_meth() {
			return;
		}
};


void TestRef(Callback<void>* test_var, map<Callback<void>*, int>* map);

int main() { 
	map<Callback<void>*, int> my_map;
  test my_obj;

	Callback<void>* test_cb = makeCallableMany(&test::test_meth, &my_obj);

	my_map[test_cb] = my_map[test_cb]++;
	my_map[test_cb] = my_map[test_cb]++;

	TestRef(test_cb, &my_map);

	cout << my_map[test_cb];

	return 0;
}

void TestRef(Callback<void>* test_var, map<Callback<void>*, int>* map) {
	(*map)[test_var] = (*map)[test_var]++;
}
