#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <winsock.h>
#include <math.h>

#define FormatWSAMessage(lpszError) FormatMessage( \
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
    NULL, WSAGetLastError(), LANG_NEUTRAL, \
    (LPTSTR)&lpszError, 0, \
    NULL \
)