/*
MIT License

Copyright (c) 2025 - "Kenny Susak"

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifdef _WIN32
// RBOT 1.0 - Dont Support Windows, maybe future update it will (22, February 2025 09:23)
#else
#ifndef QUERY_INCLUDED
#define QUERY_INCLUDED

#include <core.h>

#if defined(__linux__) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE 1
#endif
#if defined(__linux__) && !defined(_DEFAULT_SOURCE)
# define _DEFAULT_SOURCE 1
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

# include <sys/types.h>
# include <unistd.h>
# include <sys/mman.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <dlfcn.h>

typedef int (*SendTo_t)(int, const void*, size_t, int, const struct sockaddr*, int);

typedef struct Detour_s {
   int       installed;
   void     *origFunc;
   void     *detourFunc;
   unsigned char originalBytes[5];
   unsigned char patchBytes[5];
} Detour;

#ifdef __cplusplus
extern "C" {
#endif

// Detour manipulation
void Detour_Init(Detour *dt, void *orig, void *det);
int  Detour_Install(Detour *dt);
int  Detour_Restore(Detour *dt);

typedef struct ServerQueryHook_s {
   int hooked;
   SendTo_t originalSendTo;
   Detour   det;
} ServerQueryHook;

typedef struct QueryBuffer_s {
   unsigned char *buffer;
   size_t size;
   size_t cursor;
} QueryBuffer;

// Global accessor
ServerQueryHook* GetServerQueryHook(void);

void ServerQueryHook_Init(ServerQueryHook *self);
int  ServerQueryHook_Disable(ServerQueryHook *self);
int  ServerQueryHook_HookSendTo(int socket, const void* message, size_t length,
                                int flags, const struct sockaddr* dest, int destLength);

// QueryBuffer
void  QueryBuffer_Init(QueryBuffer *qb, const unsigned char* msg, size_t length, size_t shift);
void  QueryBuffer_Cleanup(QueryBuffer *qb);
void  QueryBuffer_Skip(QueryBuffer *qb, size_t n);
void  QueryBuffer_SkipString(QueryBuffer *qb);
const char* QueryBuffer_ReadString(QueryBuffer *qb);
void  QueryBuffer_ShiftToEnd(QueryBuffer *qb);
const unsigned char* QueryBuffer_Data(const QueryBuffer *qb);
size_t QueryBuffer_Length(const QueryBuffer *qb);

void  QueryBuffer_WriteFloat(QueryBuffer *qb, float val);
float QueryBuffer_ReadFloat(QueryBuffer *qb);
uint8_t QueryBuffer_ReadUInt8(QueryBuffer *qb);
int32_t QueryBuffer_ReadInt32(QueryBuffer *qb);

#ifdef __cplusplus
}
#endif

#endif // QUERY_INCLUDED
#endif