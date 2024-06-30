#include "FileUtils.h"
#include <sys/stat.h>

bool fileExists(ccharp path){
#if defined (_WIN32)
    struct _stat64i32 fstat;
    return _stat(path, &fstat)==0;
#else
    struct stat fstat;
    return _stat(path, &fstat)==0;
#endif
}

time_t fileTime(ccharp path){
#if defined (_WIN32)
    struct _stat64i32 fstat;
    _stat(path, &fstat);
    return fstat.st_ctime;
#else
    struct stat fstat;
    _stat(path, &fstat);
    return fstat.st_ctime;
#endif        
}