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
#ifndef _HRPC_RPC_SESSION_MANAGER_H
#define _HRPC_RPC_SESSION_MANAGER_H

#include <ccbase/timer_wheel.h>
#include "hyperrpc/hyperrpc.h"
#include "hyperrpc/env.h"
#include "hyperrpc/constants.h"
#include "hyperrpc/endpoint_list.h"

namespace hrpc {

class RpcSessionManager
{
public:
  using OnSendRequest = ccb::ClosureFunc<
                             void(const google::protobuf::MethodDescriptor*,
                                  const google::protobuf::Message&,
                                  uint64_t rpc_id,
                                  const Addr&)>;

  RpcSessionManager(const Env& env);
  ~RpcSessionManager();

  bool Init(size_t sess_pool_size,
            size_t rpc_core_id,
            OnSendRequest on_send_req);
  bool AddSession(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  const EndpointList& endpoint_list,
                  ::ccb::ClosureFunc<void(Result)> done);
  void OnRecvResponse(const std::string& service,
                      const std::string& method,
                      uint64_t rpc_id,
                      Result rpc_result,
                      const Buf& resp_body);

private:
  struct SessionNode {
    SessionNode() : rpc_id(0) {}
    uint64_t rpc_id; // value 0 stands for empty node
    const google::protobuf::MethodDescriptor* method;
    const google::protobuf::Message* request;
    google::protobuf::Message* response;
    ccb::ClosureFunc<void(Result)> done;
    ccb::TimerOwner timer_owner;
    EndpointList endpoint_list;
  };

  SessionNode* AllocSessionNode();
  SessionNode* FindSessionNode(uint64_t rpc_id);
  void FreeSessionNode(SessionNode* node);

  uint64_t NextRpcId() {
    constexpr size_t mask = (1UL << kRpcIdSeqPartBits) - 1;
    next_rpc_id_ = (next_rpc_id_ & ~mask) + ((next_rpc_id_ + 1) & mask);
    return next_rpc_id_;
  }

  const Env& env_;
  size_t pool_size_order_;
  size_t pool_size_mask_;
  std::unique_ptr<SessionNode[]> sess_pool_;
  uint64_t next_rpc_id_;
  OnSendRequest on_send_request_;
};

} // namespace hrpc

#endif // _HRPC_RPC_SESSION_MANAGER_H
