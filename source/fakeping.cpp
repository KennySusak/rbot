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

#include <core.h>
#include "fakeping.h"

void SendFakePingToClient(edict_t* to)
{
    if (FNullEnt(to) || !(to->v.flags & FL_CLIENT))
        return;

    const int maxClients = engine->GetMaxClients();

    for (int i = 1; i <= maxClients; i++)
    {
        edict_t* botEnt = INDEXENT(i);
        if (FNullEnt(botEnt))
            continue;

        int botIndex = g_botManager->GetIndex(botEnt);
        if (botIndex < 0)
            continue;

        Bot* bot = g_botManager->GetBot(botIndex);
        if (!bot)
            continue;
        int chunkIndex = i - 1; 
        if (chunkIndex < 0)
            continue;

        // clamp ping if needed
        int ping0 = bot->m_ping[0];
        int ping1 = bot->m_ping[1];
        int ping2 = bot->m_ping[2];
        if (ping2 < 1 || ping2 > 999) {
            ping2 = 50;
        }

        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_PINGS, nullptr, to);

        // 1st chunk
        int offset0 = bot->m_pingOffset[0];
        WRITE_BYTE((offset0 << 6) + (1 + 2 * chunkIndex));
        WRITE_SHORT(ping0);

        // 2nd chunk
        int offset1 = bot->m_pingOffset[1];
        WRITE_BYTE((offset1 << 7) + (2 + 4 * chunkIndex));
        WRITE_SHORT(ping1);

        // 3nd chunk
        WRITE_BYTE(4 + 8 * chunkIndex);
        WRITE_SHORT(ping2);
        WRITE_BYTE(0);

        MESSAGE_END();
    }
}

void ReSendFakePingAll()
{
    for (int i = 1; i <= engine->GetMaxClients(); i++)
    {
        edict_t* realEnt = INDEXENT(i);
        if (FNullEnt(realEnt) || !(realEnt->v.flags & FL_CLIENT))
            continue;

        // scoreboard open check
        if ((realEnt->v.button | realEnt->v.oldbuttons) & IN_SCORE)
        {
            SendFakePingToClient(realEnt); 
        }
    }
}