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

/* RPC closure type
 */
using DoneFunc = ::ccb::ClosureFunc<void(Result)>;

class OptionsBuilder
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

  /* Set the log level and log handler
   * @lv      only logs with level <= @lv will be handled
   * @f       callback to output logs
   *
   * This method allows you to use any logger library you want to ouput the 
   * log from HyperUdp. By default no log will be ouput in any way.
   *
   * @return  self reference as Builder-Pattern
   */
  OptionsBuilder& LogHandler(LogLevel lv, 
                             ccb::ClosureFunc<void(LogLevel, const char*)> f);

  /* Set number of worker threads
   * @num     number of worker threads
   *
   * Proper number of worker threads can get best permformance on modern 
   * multi-core system. Normally the best number is between 2 and CORES.
   *
   * @return  self reference as Builder-Pattern
   */
  OptionsBuilder& WorkerNumber(size_t num);
  OptionsBuilder& WorkerQueueSize(size_t num);

  OptionsBuilder& MaxRpcSessions(size_t num);
  OptionsBuilder& DefaultRpcTimeout(size_t ms);

  ::hudp::OptionsBuilder& hudp_options() {
    return hudp_opt_builder_;
  }

private:
  // not copyable and movable
  OptionsBuilder(const OptionsBuilder&) = delete;
  void operator=(const OptionsBuilder&) = delete;
  OptionsBuilder(OptionsBuilder&&) = delete;
  void operator=(OptionsBuilder&&) = delete;

  ::hudp::OptionsBuilder hudp_opt_builder_;
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
  Result CallMethod(const ::google::protobuf::MethodDescriptor* method,
                    const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
                    DoneFunc done);

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
