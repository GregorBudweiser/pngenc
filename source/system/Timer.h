#pragma once
#include <time.h>

class Timer
{
public:
    Timer();
    ~Timer();

    void reset();

    double getTimePassedNS() const;
    double getResolutionNS() const;

private:
#ifdef WIN32
    __int64 m_last;
    double m_frequency;
#else
    //! This is a pointer to get timer include out of header
    struct timespec m_last;
#endif
};
