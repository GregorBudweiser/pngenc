#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <cstdio>
#include <cstdarg>

class Log
{
public:
    enum Type
    {
        LT_DEFAULT = 0,
        LT_ERROR,
        LT_WARNING,
        LT_SUCCESS,
        LT_INFO
    };

public:
    Log(const std::string & fileName, const std::string & intentString);
    ~Log();

    void log(Type type, const char * format, ...);
    void flush();

    void push() { m_intent++; }
    void pop() { m_intent--; }

public:
    static Log s_log;
    //static Log s_perfLog;

private:
    void printType(Type type);

private:
    FILE * m_file;
    int m_intent;
    std::string m_intentText;
};

#define LOG_DEFAULT(text, ...) Log::s_log.log(Log::LT_DEFAULT, text, ##__VA_ARGS__);
#define LOG_ERROR(text, ...) Log::s_log.log(Log::LT_ERROR, text, ##__VA_ARGS__);
#define LOG_WARNING(text, ...) Log::s_log.log(Log::LT_WARNING, text, ##__VA_ARGS__);
#define LOG_SUCCESS(text, ...) Log::s_log.log(Log::LT_SUCCESS, text, ##__VA_ARGS__);
#define LOG_INFO(text, ...) Log::s_log.log(Log::LT_INFO, text, ##__VA_ARGS__);

#define LOG_PUSH() Log::s_log.push();
#define LOG_POP() Log::s_log.pop();
#define LOG_FLUSH() Log::s_log.flush();

#endif // LOGGER_H
