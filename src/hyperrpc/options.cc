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
  DEFINE_##type(hudp_##name, deft, desc)
#define GFLAGS_DEFINE_U64(name, desc) GFLAGS_DEFINE(name, uint64, 0, desc)
#define GFLAGS_DEFINE_STR(name, desc) GFLAGS_DEFINE(name, string, "", desc)
#define GFLAGS_DEFINE_BOOL(name, desc) GFLAGS_DEFINE(name, bool, false, desc)

#define GFLAGS_MAY_OVERRIDE(name, setter) do { \
  google::CommandLineFlagInfo info; \
  if (google::GetCommandLineFlagInfo("hrpc_" #name, &info) \
      && !info.is_default) { \
    setter(FLAGS_hudp_##name); \
  } \
} while(0)

// WorkerGroup options
//GFLAGS_DEFINE_U64(worker_num, "number of worker threads");

OptionsBuilder::OptionsBuilder()
  : hrpc_opt_(new Options)
{
}

OptionsBuilder::~OptionsBuilder()
{
}

Options OptionsBuilder::Build()
{
  static_cast<hudp::Options&>(*hrpc_opt_) = hudp::OptionsBuilder::Build();
  return *hrpc_opt_;
}

} // namespace hrpc
