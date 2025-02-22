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
#include <query.h>

// A static global for our hook
static ServerQueryHook g_serverQueryHook;
static ServerQueryHook* g_serverQueryHookPtr = &g_serverQueryHook;

// Detour
static int patchMemory(void* addr, const unsigned char *patch)
{
    // Linux
    long pageSize = sysconf(_SC_PAGE_SIZE);
    uintptr_t start = (uintptr_t)addr & ~(pageSize - 1);

    if (mprotect((void*)start, pageSize, PROT_READ|PROT_WRITE|PROT_EXEC) != 0)
        return 0;

    memcpy(addr, patch, 5);

    if (mprotect((void*)start, pageSize, PROT_READ|PROT_EXEC) != 0)
        return 0;

    return 1;
}

void Detour_Init(Detour *dt, void *orig, void *det)
{
    dt->installed = 0;
    dt->origFunc  = orig;
    dt->detourFunc= det;

    memset(dt->originalBytes, 0, 5);
    memset(dt->patchBytes,   0, 5);
}

int Detour_Install(Detour *dt)
{
    if (!dt->origFunc || !dt->detourFunc)
        return 0;
    if (dt->installed)
        return 1;

    // Save original 5 bytes
    memcpy(dt->originalBytes, dt->origFunc, 5);

    // Build patch: E9 <relative jump>
    // jump = (detour - original) - 5
    uintptr_t rel = (uintptr_t)dt->detourFunc - (uintptr_t)dt->origFunc - 5;
    dt->patchBytes[0] = 0xE9; // JMP
    memcpy(&dt->patchBytes[1], &rel, sizeof(rel));

    if (!patchMemory(dt->origFunc, dt->patchBytes))
        return 0;

    dt->installed = 1;
    return 1;
}

int Detour_Restore(Detour *dt)
{
    if (!dt->installed)
        return 1;

    if (!patchMemory(dt->origFunc, dt->originalBytes))
        return 0;

    dt->installed = 0;
    return 1;
}

void QueryBuffer_Init(QueryBuffer *qb, const unsigned char* msg, size_t length, size_t shift)
{
    qb->buffer = NULL;
    qb->size   = 0;
    qb->cursor = 0;

    if (!msg || length == 0)
        return;

    qb->size   = length;
    qb->cursor = shift;

    qb->buffer = (unsigned char*)malloc(length);
    if (qb->buffer)
        memcpy(qb->buffer, msg, length);
}

void QueryBuffer_Cleanup(QueryBuffer *qb)
{
    if (qb->buffer) {
        free(qb->buffer);
        qb->buffer = NULL;
    }
    qb->size   = 0;
    qb->cursor = 0;
}

void QueryBuffer_Skip(QueryBuffer *qb, size_t n)
{
    if (qb->cursor + n <= qb->size)
        qb->cursor += n;
}

void QueryBuffer_SkipString(QueryBuffer *qb)
{
    while (qb->cursor < qb->size && qb->buffer[qb->cursor] != 0)
        qb->cursor++;
    qb->cursor++;
}

const char* QueryBuffer_ReadString(QueryBuffer *qb)
{
    if (qb->cursor >= qb->size)
        return "";

    const char *ret = (const char*)(qb->buffer + qb->cursor);
    QueryBuffer_SkipString(qb);
    return ret;
}

void QueryBuffer_ShiftToEnd(QueryBuffer *qb)
{
    qb->cursor = qb->size;
}

const unsigned char* QueryBuffer_Data(const QueryBuffer *qb)
{
    return qb->buffer;
}

size_t QueryBuffer_Length(const QueryBuffer *qb)
{
    return qb->size;
}

void QueryBuffer_WriteFloat(QueryBuffer *qb, float val)
{
    if (qb->cursor + sizeof(float) <= qb->size) {
        memcpy(qb->buffer + qb->cursor, &val, sizeof(val));
        qb->cursor += sizeof(val);
    }
}

float QueryBuffer_ReadFloat(QueryBuffer *qb)
{
    float val = 0.0f;
    if (qb->cursor + sizeof(float) <= qb->size) {
        memcpy(&val, qb->buffer + qb->cursor, sizeof(val));
        qb->cursor += sizeof(val);
    }
    return val;
}

uint8_t QueryBuffer_ReadUInt8(QueryBuffer *qb)
{
    uint8_t val = 0;
    if (qb->cursor + sizeof(uint8_t) <= qb->size) {
        val = qb->buffer[qb->cursor];
        qb->cursor += sizeof(uint8_t);
    }
    return val;
}

int32_t QueryBuffer_ReadInt32(QueryBuffer *qb) {
    int32_t val = 0;
    if (qb->cursor + sizeof(int32_t) <= qb->size) {
        memcpy(&val, qb->buffer + qb->cursor, sizeof(int32_t));
        qb->cursor += sizeof(int32_t);
    }
    return val;
}


static int HookSendTo_Internal(int socket, const void* message, size_t length,
                               int flags, const struct sockaddr* dest, int destLength)
{
    Detour *dt = &g_serverQueryHookPtr->det;
    if (dt->installed)
        Detour_Restore(dt);
    {

        int (*orig)(int, const void*, size_t, int, const struct sockaddr*, int);
        orig = (int (*)(int, const void*, size_t, int, const struct sockaddr*, int)) dt->origFunc;
        int result = orig(socket, message, length, flags, dest, destLength);

        Detour_Install(dt);
        return result;
    }
}

ServerQueryHook* GetServerQueryHook(void)
{
    return g_serverQueryHookPtr;
}

void ServerQueryHook_Init(ServerQueryHook *self)
{
    if (!self)
        return;
    if (self->hooked)
        return;

    void *func = dlsym(RTLD_DEFAULT, "sendto");
    self->originalSendTo = (SendTo_t)func;

    if (!self->originalSendTo) {
        printf("[ServerQueryHook] Could not find sendto!\n");
        return;
    }
    Detour_Init(&self->det, (void*)self->originalSendTo, (void*)ServerQueryHook_HookSendTo);
    if (!Detour_Install(&self->det)) {
        printf("[ServerQueryHook] Failed to install detour!\n");
        return;
    }
    self->hooked = 1;
    printf("[ServerQueryHook] Hook installed successfully.\n");
}

int ServerQueryHook_Disable(ServerQueryHook *self)
{
    if (!self || !self->hooked)
        return 0;
    if (!Detour_Restore(&self->det)) {
        printf("[ServerQueryHook] Failed to restore patch!\n");
        return 0;
    }
    self->hooked = 0;
    return 1;
}

static int GetValidPlayerCount(void)
{
    int count = 0;

    for (int i = 1; i < 33; i++) {
        edict_t* ent = INDEXENT(i);

        if (!FNullEnt(ent) && (ent->v.flags & FL_CLIENT) && !ent->free) {
            count++;
        }
    }
    return count;
}

int ServerQueryHook_HookSendTo(int socket, const void* message, size_t length,
                               int flags, const struct sockaddr* dest, int destLength)
{
    ServerQueryHook* hook = GetServerQueryHook();
    if (!hook || !hook->originalSendTo)
    {
        return sendto(socket, message, length, flags, dest, destLength);
    }
    
    if (!message || length < 5)
        return HookSendTo_Internal(socket, message, length, flags, dest, destLength);
    
    const unsigned char* packet = (const unsigned char*)message;
    
    // Only process query packets (header 0xFF 0xFF 0xFF 0xFF)
    if (memcmp(packet, "\xff\xff\xff\xff", 4) == 0)
    {
        if (packet[4] == 'D')
        {
            if (GetValidPlayerCount() == 0)
            {
                unsigned char emptyPacket[7];
                memcpy(emptyPacket, packet, 5);
                emptyPacket[5] = 0;
                emptyPacket[6] = '\0';

                return HookSendTo_Internal(socket, emptyPacket, sizeof(emptyPacket), flags, dest, destLength);
            }

            QueryBuffer qb;
            QueryBuffer_Init(&qb, packet, length, 5);
            
            if (qb.cursor + sizeof(uint8_t) <= qb.size)
            {
                uint8_t origCount = QueryBuffer_ReadUInt8(&qb);
                
                for (uint8_t i = 0; i < origCount; i++) {
                    if (qb.cursor + 1 > qb.size) break;
                    // Read the slot ID
                    uint8_t slotId = qb.buffer[qb.cursor];
                    qb.cursor += sizeof(uint8_t);
                
                    // Read the name
                    if (qb.cursor >= qb.size) break;
                    const char *playerName = QueryBuffer_ReadString(&qb);
                    
                    // Read  players score
                    if (qb.cursor + sizeof(int32_t) > qb.size) break;
                    int32_t score = QueryBuffer_ReadInt32(&qb);
                    
                    // Read the connection time
                    if (qb.cursor + sizeof(float) > qb.size) break;
                    size_t floatPos = qb.cursor;
                    float connTime = QueryBuffer_ReadFloat(&qb);

                    if (playerName && playerName[0] != '\0' && g_botManager->ValidateBotByName(playerName))
                    {
                        float newTime = g_botManager->getConnectTime(playerName, connTime);
                        //printf("BOT: %s, Score: %d, ConnTime: %f => %f\n", 
                        //       playerName, score, connTime, newTime);
                        qb.cursor = floatPos;
                        QueryBuffer_WriteFloat(&qb, newTime);
                    }
                }
            }
            int ret = HookSendTo_Internal(socket, qb.buffer, qb.size, flags, dest, destLength);
            QueryBuffer_Cleanup(&qb);
            return ret;
        }
    }
    
    return HookSendTo_Internal(socket, message, length, flags, dest, destLength);
}
#endif