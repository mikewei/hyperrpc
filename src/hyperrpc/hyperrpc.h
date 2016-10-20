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
#ifndef _HRPC_HYPERRPC_H
#define _HRPC_HYPERRPC_H

#include <memory>
#include <vector>
#include <ccbase/closure.h>
#include <hyperudp/hyperudp.h>
#include "hyperrpc/options.h"

namespace google {
namespace protobuf {
  class MethodDescriptor;
  class Message;
} // namespace protobuf
} // namespace google


namespace hrpc {

class Options;
class Service;

/* Log levels defined:
 * kError, kWarning, kInfo, kDebug
 */
using ::hudp::LogLevel;
using ::hudp::kError;
using ::hudp::kWarning;
using ::hudp::kInfo;
using ::hudp::kDebug;

/* IPv4 address
 */
using ::hudp::Addr;

/* RPC result definition
 */
enum Result
{
  // user-code generated results
  kSuccess = 0, // rpc done successfully
  kFailure = 1, // user defined failure
  // framework generated results
  kNoRoute = 2, // cannot resolve ip:port of target service
  kTimeout = 3, // rpc timeout in the end
  kNotImpl = 4, // method called is not implemented by server
  kInError = 5, // other internal errors
};

class OptionsBuilder : public ::hudp::OptionsBuilder
{
public:
  OptionsBuilder();
  ~OptionsBuilder();

  /* Build the Options object
   * @num     number of worker threads
   *
   * Build the Options object as Builder-Pattern
   *
   * @return  created Options object
   */
  Options Build();

  OptionsBuilder& MaxRpcSessions(size_t num);

private:
  // not copyable and movable
  OptionsBuilder(const OptionsBuilder&) = delete;
  void operator=(const OptionsBuilder&) = delete;
  OptionsBuilder(OptionsBuilder&&) = delete;
  void operator=(OptionsBuilder&&) = delete;

  std::unique_ptr<Options> hrpc_opt_;
};

class EndpointListBuilder
{
public:
  virtual void PushBack(const Addr& endpoint) = 0;
  virtual void Clear() = 0;
  virtual size_t Size() const = 0;
protected:
  virtual ~EndpointListBuilder() {}
};

class HyperRpc
{
public:
  using OnServiceRouting = ::ccb::ClosureFunc<bool(const std::string& service,
                                                   const std::string& method,
                                                   EndpointListBuilder* out)>;
  HyperRpc();
  HyperRpc(const Options& opt);
  virtual ~HyperRpc();

  bool InitAsClient(OnServiceRouting on_svc_routing);
  bool InitAsServer(std::vector<Service*> services);
  bool Start(const Addr& bind_local_addr);

  // this method is called by generated service stub
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::ccb::ClosureFunc<void(Result)> done);

private:
  // not copyable and movable
  HyperRpc(const HyperRpc&) = delete;
  void operator=(const HyperRpc&) = delete;
  HyperRpc(HyperRpc&&) = delete;
  void operator=(HyperRpc&&) = delete;

  class Impl;
  std::unique_ptr<HyperRpc::Impl> pimpl_;
};

} // namespace hrpc

#endif // _HRPC_HYPERRPC_H
