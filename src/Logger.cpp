#include <list>
#include "Logger.h"

#define MAX_LOG_ENTRIES 3 

std::list<std::string> Logger::m_buf;

void Logger::add(const std::string& str) {
	if (m_buf.size() == MAX_LOG_ENTRIES) 
		m_buf.pop_front();
	m_buf.push_back(str);
}

const std::list< std::string>& Logger::getBuf(void) {
	return m_buf;
}