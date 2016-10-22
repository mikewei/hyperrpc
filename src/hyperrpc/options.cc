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
#include <limits>
#include <gflags/gflags.h>
#include <hyperudp/options.h>
#include <hyperudp/module_registry.h>
#include "hyperrpc/hyperrpc.h"

namespace hrpc {

#define GFLAGS_DEFINE(name, type, deft, desc) \
  DEFINE_##type(hrpc_##name, deft, desc)
#define GFLAGS_DEFINE_U64(name, desc) GFLAGS_DEFINE(name, uint64, 0, desc)
#define GFLAGS_DEFINE_STR(name, desc) GFLAGS_DEFINE(name, string, "", desc)
#define GFLAGS_DEFINE_BOOL(name, desc) GFLAGS_DEFINE(name, bool, false, desc)

#define GFLAGS_MAY_OVERRIDE(name, setter) do { \
  google::CommandLineFlagInfo info; \
  if (google::GetCommandLineFlagInfo("hrpc_" #name, &info) \
      && !info.is_default) { \
    setter(FLAGS_hrpc_##name); \
  } \
} while(0)

// WorkerGroup options
GFLAGS_DEFINE_U64(worker_num, "number of worker threads");
GFLAGS_DEFINE_U64(worker_queue_size, "size of queue consumed by worker");
// RpcSessionManager options
GFLAGS_DEFINE_U64(max_rpc_sessions, "max number of pending RPC sessions");
GFLAGS_DEFINE_U64(default_rpc_timeout, "default RPC session timeout (ms)");

OptionsBuilder::OptionsBuilder()
  : hrpc_opt_(new Options)
{
  // RpcSessionManager options
  MaxRpcSessions(1000000);
  DefaultRpcTimeout(2500);
}

OptionsBuilder::~OptionsBuilder()
{
}

Options OptionsBuilder::Build()
{
  GFLAGS_MAY_OVERRIDE(worker_num, WorkerNumber);
  GFLAGS_MAY_OVERRIDE(worker_queue_size, WorkerQueueSize);
  GFLAGS_MAY_OVERRIDE(max_rpc_sessions, MaxRpcSessions);
  GFLAGS_MAY_OVERRIDE(default_rpc_timeout, DefaultRpcTimeout);
  hrpc_opt_->hudp_options = hudp_opt_builder_.Build();
  return *hrpc_opt_;
}

// Utility options

OptionsBuilder& OptionsBuilder::LogHandler(LogLevel lv, 
                     ccb::ClosureFunc<void(LogLevel, const char*)> f)
{
  hudp_opt_builder_.LogHandler(lv, std::move(f));
  return *this;
}

// WorkerGroup options

OptionsBuilder& OptionsBuilder::WorkerNumber(size_t num)
{
  hudp_opt_builder_.WorkerNumber(num);
  return *this;
}

OptionsBuilder& OptionsBuilder::WorkerQueueSize(size_t num)
{
  hudp_opt_builder_.WorkerQueueSize(num);
  return *this;
}

// RpcSessionManager options

OptionsBuilder& OptionsBuilder::MaxRpcSessions(size_t num)
{
  if (num < hrpc_opt_->hudp_options.worker_num) {
    throw std::invalid_argument("Invalid value!");
  }
  hrpc_opt_->max_rpc_sessions = num;
  return *this;
}

OptionsBuilder& OptionsBuilder::DefaultRpcTimeout(size_t ms)
{
  if (ms > std::numeric_limits<uint32_t>::max() || ms <= 0) {
    throw std::invalid_argument("Invalid timeout value!");
  }
  hrpc_opt_->default_rpc_timeout = ms;
  return *this;
}

} // namespace hrpc
