#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <set>

int main (void)
{
	std::string tstr("ff58\nThe quick, brown fox jumped over the lazy dogs.\nAnd then decided to eat them.\n0");

	unsigned int x;   
	std::stringstream ss(tstr);
	ss >> std::hex >> x;
	
	size_t pos = ss.tellg();

	std::cerr << "val " << x << std::endl;
	std::cerr << "pos " << pos << std::endl;

	// std::string nxt;
	// ss >> nxt;
	// std::cerr << "nxt " << nxt << std::endl;
	// std::cerr << "rst " << ss.str() << std::endl; // all 

	std::string ext;
	ext.resize(x);
	ss.read(&ext[0], x);
	std::cerr << "ext " << ext << std::endl;

	tstr.erase(0, pos);
	std::cerr << "str " << tstr << std::endl;



	#if 0
	char buf[] = { 'a', '\0', 'b', '\0'};

	std::cout << buf << std::endl;

	std::string s1(buf);
	std::string s2;
	s2.append(buf, 4);

	std::cout << s1.size() << std::endl;
	std::cout << s2.size() << std::endl;
#endif

#if 0
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
#endif
	return (0);
}