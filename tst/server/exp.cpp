#include <iostream>
#include <string>
#include <set>

int main (void)
{
// 	char buf[] = { 'a', '\0', 'b', '\0'};

// 	std::cout << buf << std::endl;

// 	std::string s1(buf);
// 	std::string s2;
// 	s2.append(buf, 4);

// 	std::cout << s1.size() << std::endl;
// 	std::cout << s2.size() << std::endl;
	
	std::set<int> num;
	for (int i=0; i < 12; i++)
		num.insert(i);
	std::set<int>::iterator it;
	
	it = num.begin();
	while (it != num.end())
	{
		std::cerr << *it << std::endl;
		it++;
	}
	it = num.begin();
	while (it != num.end())
	{
		it = num.erase(it);
		it++;
	}
	std::cerr << std::endl;
	it = num.begin();
	while (it != num.end())
	{
		std::cerr << *it << std::endl;
		it++;
	}
	return (0);
}