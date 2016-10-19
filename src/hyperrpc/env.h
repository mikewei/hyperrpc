/* Copyright (c) 2016, Bin Wei <bin@vip.qq.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * The name of of its contributors may not be used to endorse or 
 * promote products derived from this software without specific prior 
 * written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _HRPC_ENV_H
#define _HRPC_ENV_H

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <ccbase/worker_group.h>
#include <hyperudp/addr.h>
#include <hyperudp/buf.h>
#include <hyperudp/chunk_alloc.h>
#include <hyperudp/env.h>
#include "hyperrpc/hyperrpc.h"
#include "hyperrpc/options.h"

namespace hrpc {

#define LOG(lv, ...) do { \
  env_.Log(lv, __VA_ARGS__); \
} while(0)

#define ELOG(...) LOG(kError, __VA_ARGS__)
#define WLOG(...) LOG(kWarning, __VA_ARGS__)
#define ILOG(...) LOG(kInfo, __VA_ARGS__)
#define DLOG(...) LOG(kDebug, __VA_ARGS__)

#define RET(lv, ...) do { \
  env_.Log(lv, __VA_ARGS__); \
  return; \
} while(0)

#define ERET(...) RET(kError, __VA_ARGS__)
#define WRET(...) RET(kWarning, __VA_ARGS__)
#define IRET(...) RET(kInfo, __VA_ARGS__)
#define DRET(...) RET(kDebug, __VA_ARGS__)

#define RET_F(lv, ...) do { \
  env_.Log(lv, __VA_ARGS__); \
  return false; \
} while(0)

#define ERET_F(...) RET_F(kError, __VA_ARGS__)
#define WRET_F(...) RET_F(kWarning, __VA_ARGS__)
#define IRET_F(...) RET_F(kInfo, __VA_ARGS__)
#define DRET_F(...) RET_F(kDebug, __VA_ARGS__)

#define HRPC_ASSERT(EXP) do { \
  if (!(EXP)) { \
    ELOG("ASSERT failed: " #EXP ); \
    abort(); \
  } \
} while(0)

using hudp::Buf;
using hudp::Addr;
using hudp::ChunkAlloc;

class Env : public hudp::Env
{
public:
  Env(const Options& opt, ccb::TimerWheel* tw = nullptr);
  ~Env();

  const Options& opt() const {
    return hrpc_opt_;
  }

private:
  Options hrpc_opt_;
};

} // namespace hrpc

#endif // _HRPC_ENV_H
