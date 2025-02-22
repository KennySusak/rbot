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

#ifndef FAKEPING_H
#define FAKEPING_H

#include <engine.h>

#ifdef _WIN32
#include <stdint.h>
#endif

static const int SVC_PINGS = 17; // sems like atleast in rehlds never get called ?

void SendFakePingToClient(edict_t* to);
void SendFakePingToAll(void);
void ReSendFakePingAll(void);

extern bool g_needFakePingResend;
extern float g_fakePingResendTime;

class MessageWriter final {
    private:
        bool m_autoDestruct { false };
        
    public:
        MessageWriter () = default;
    
        MessageWriter(int dest, int type, const float* pos = nullptr, edict_t *to = nullptr) {
            start(dest, type, pos, to);
            m_autoDestruct = true;
        }
    
        ~MessageWriter () {
            if (m_autoDestruct) {
                end();
            }
       }
    
    public:
        MessageWriter &start(int dest, int type, const float* pos, edict_t *to) {
            g_engfuncs.pfnMessageBegin(dest, type, pos, to);
            return *this;
        }

        void end () {
            g_engfuncs.pfnMessageEnd();
        }
    
        MessageWriter &writeByte (int val) {
            g_engfuncs.pfnWriteByte(val);
            return *this;
        }    
    
        MessageWriter &writeLong (int val) {
            g_engfuncs.pfnWriteLong(val);
            return *this;
        }
        
        MessageWriter &writeChar (int val) {
            g_engfuncs.pfnWriteChar(val);
            return *this;
        }
        
        MessageWriter &writeShort (int val) {
            g_engfuncs.pfnWriteShort(val);
            return *this;
        }
        
        MessageWriter &writeCoord (float val) {
            g_engfuncs.pfnWriteCoord(val);
            return *this;
        }
        
        MessageWriter &writeString (const char *val) {
            g_engfuncs.pfnWriteString(val);
            return *this;
        }
};

class PingBitMsg final {
    private:
       int32_t bits_ {};
       int32_t used_ {};
    
       MessageWriter msg_ {};
       bool started_ {};
    
    public:
       enum : int32_t {
          Single = 1,
          PlayerID = 5,
          Loss = 7,
          Ping = 12
       };
    
    public:
       PingBitMsg () = default;
       ~PingBitMsg () = default;
    
       void write(int32_t bit, int32_t size) {
          if (size > 32 - used_ || size < 1)
             return;
    
          const auto maxSize = 1 << size;
          if (bit >= maxSize)
             bit = maxSize - 1;
    
          bits_ = bits_ + (bit << used_);
          used_ += size;
       }
    
       void send(bool remaining = false) {
          while (used_ >= 8) {
             msg_.writeByte(bits_ & 255);
             bits_ >>= 8;
             used_ -= 8;
          }
    
          if (remaining && used_ > 0) {
             msg_.writeByte(bits_);
             bits_ = used_ = 0;
          }
       }
    
       void start(edict_t *ent) {
          if (started_)
             return;
    
          msg_.start(MSG_ONE_UNRELIABLE, 17, nullptr, ent);
          started_ = true;
       }
    
       void flush() {
          if (!started_)
             return;
    
          write(0, Single);
          send(true);
    
          started_ = false;
          msg_.end();
       }
};

#endif // FAKEPING_H
