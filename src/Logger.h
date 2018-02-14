#pragma once
#include <string>
#include <list>


class Logger {
public:
	static void add(const std::string&);
	static const std::list < std::string>& getBuf(void);
private:
	Logger(void);
	static std::list<std::string> m_buf;

};