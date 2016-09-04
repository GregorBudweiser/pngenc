#include "Log.h"
#ifdef WIN32
#else
#include <unistd.h>
#endif

Log Log::s_log = Log("", "|  ");

Log::Log(const std::string &fileName, const std::string &intentString)
    : m_file(0), m_intent(0), m_intentText(intentString)
{
    if(!fileName.empty())
    {
        m_file = fopen(fileName.c_str(), "w");
    }
    else
    {
        m_file = stdout;
    }
}

Log::~Log()
{
    if(m_file != stdout && m_file != NULL)
    {
        fclose(m_file);
    }
}

void Log::log(Type type, const char * format, ...)
{
    for(int i = 0; i < m_intent; i++)
    {
        fprintf(m_file, "%s", m_intentText.c_str());
    }

    printType(type);

    va_list argptr;
    va_start(argptr, format);
    vfprintf(m_file, format, argptr);
    va_end(argptr);
    fprintf(m_file, "\n");
}

void Log::printType(Type type)
{
    std::string colors[] = { "", "\033[1;31m", "\033[1;33m", "\033[1;32m", "\033[1;34m" };
    std::string text[] = { "", "ERROR: ", "WARNING: ", "SUCCESS: ", "INFO: " };
    std::string defaultColor = "\033[0m";

#ifndef WIN32
    bool hasColor = isatty(fileno(m_file));
#else
    bool hasColor = false;
#endif

    fprintf(m_file, "%s%s%s",
            hasColor ? colors[int(type)].c_str() : "",
            text[int(type)].c_str(),
            hasColor ? defaultColor.c_str() : "");
}

void Log::flush()
{
    fflush(m_file);
}
