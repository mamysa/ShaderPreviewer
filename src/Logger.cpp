#include <list>
#include "Logger.h"

#define MAX_LOG_ENTRIES 32 

std::list<LogEntry> Logger::m_buf;

void Logger::add(const std::string& str, LogType type) {
	if (m_buf.size() == MAX_LOG_ENTRIES) 
		m_buf.pop_front();
	m_buf.push_back(LogEntry(str, type));
}

const std::list<LogEntry>& Logger::getBuf(void) {
	return m_buf;
}