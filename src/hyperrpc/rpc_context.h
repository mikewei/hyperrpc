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
#ifndef _HRPC_RPC_CONTEXT_H
#define _HRPC_RPC_CONTEXT_H

#include <google/protobuf/arena.h>
#include <google/protobuf/message.h>
#include "hyperrpc/hyperrpc.h"
#include "hyperrpc/constants.h"

namespace hrpc {

class IncomingRpcContext
{
public:
  IncomingRpcContext(const google::protobuf::MethodDescriptor* method,
                     uint64_t rpc_id,
                     const Addr& addr);
  virtual ~IncomingRpcContext();

  virtual void Init(const google::protobuf::Message& req_prot,
                    const google::protobuf::Message& resp_prot);

  const google::protobuf::MethodDescriptor* method() const {
    return method_;
  }
  google::protobuf::Message* request() const {
    return request_;
  }
  google::protobuf::Message* response() const {
    return response_;
  }
  uint64_t rpc_id() const {
    return rpc_id_;
  }
  const Addr& addr() const {
    return addr_;
  }

protected:
  const google::protobuf::MethodDescriptor* method_;
  google::protobuf::Message* request_;
  google::protobuf::Message* response_;
  uint64_t rpc_id_;
  Addr addr_;
};

class ArenaIncomingRpcContext : public IncomingRpcContext
{
public:
  ArenaIncomingRpcContext(const google::protobuf::MethodDescriptor* method,
                          uint64_t rpc_id,
                          const Addr& addr);
  virtual ~ArenaIncomingRpcContext();

  virtual void Init(const google::protobuf::Message& req_prot,
                    const google::protobuf::Message& resp_prot) override;

protected:
  char arena_buf_[kArenaInitBufSize];
  google::protobuf::Arena arena_;
};

class OutgoingRpcContext
{
public:
  OutgoingRpcContext(const google::protobuf::MethodDescriptor* method,
                     const google::protobuf::Message* request,
                     google::protobuf::Message* response,
                     const std::vector<Addr>& addrs);
  ~OutgoingRpcContext();

private:
  const google::protobuf::MethodDescriptor* method_;
  google::protobuf::Message* request_;
  google::protobuf::Message* response_;
};

} // namespace hrpc

#endif // _HRPC_RPC_CONTEXT_H
