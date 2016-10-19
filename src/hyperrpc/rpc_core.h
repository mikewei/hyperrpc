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
#ifndef _HRPC_RPC_CORE_H
#define _HRPC_RPC_CORE_H

#include "hyperrpc/env.h"
#include "hyperrpc/rpc_session_manager.h"

namespace hrpc {

class Service;
class RpcHeader;
class IncomingRpcContext;

class RpcCore
{
public:
  using OnSendPacket = ccb::ClosureFunc<void(const Buf&, const Addr&)>;
  using OnFindService = ccb::ClosureFunc<Service*(const std::string&)>;
  using OnServiceRouting = HyperRpc::OnServiceRouting;

  RpcCore(const Env& env);
  ~RpcCore();

  bool Init(size_t rpc_core_id, OnSendPacket on_send_pkt,
                                OnFindService on_find_svc,
                                OnServiceRouting on_svc_routing);
  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  ccb::ClosureFunc<void(Result)> done);
  size_t OnRecvPacket(const Buf& buf, const Addr& addr);

private:
  void OnRecvRequestMessage(const RpcHeader& header,
                            const Buf& body, const Addr& addr);
  void OnRecvResponseMessage(const RpcHeader& header,
                             const Buf& body, const Addr& addr);
  void OnIncomingRpcDone(const IncomingRpcContext* rpc, Result result);
  void OnOutgoingRpcSend(const google::protobuf::MethodDescriptor* method,
                         const google::protobuf::Message& request,
                         uint64_t rpc_id, const Addr& addr);
  void SendMessage(const RpcHeader& header,
                   const google::protobuf::Message& body,
                   const Addr& addr);

  // not copyable and movable
  RpcCore(const RpcCore&) = delete;
  void operator=(const RpcCore&) = delete;
  RpcCore(RpcCore&&) = delete;
  void operator=(RpcCore&&) = delete;

  const Env& env_;
  size_t rpc_core_id_;
  RpcSessionManager rpc_sess_mgr_;
  OnSendPacket on_send_packet_;
  OnFindService on_find_service_;
  OnServiceRouting on_service_routing_;
};

} // namespace hrpc

#endif // _HRPC_RPC_CORE_H
