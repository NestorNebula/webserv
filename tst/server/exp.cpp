#include <iostream>
#include <string>

int main (void)
{
	char buf[] = { 'a', '\0', 'b', '\0'};

	std::cout << buf << std::endl;

	std::string s1(buf);
	std::string s2;
	s2.append(buf, 4);

	std::cout << s1.size() << std::endl;
	std::cout << s2.size() << std::endl;
	
	return (0);
}