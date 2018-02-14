#pragma once
#include <string>
#include <list>


enum LogType {
	SUCCESS,
	FAILURE, 
	INFO 	
};

typedef std::pair<std::string, LogType> LogEntry;

class Logger {
public:
	static void add(const std::string&, LogType);
	static const std::list <LogEntry>& getBuf(void);
private:
	Logger(void);
	static std::list<LogEntry> m_buf;

};