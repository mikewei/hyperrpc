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
#ifndef _HRPC_ENDPOINT_LIST_H
#define _HRPC_ENDPOINT_LIST_H

#include <hyperudp/addr.h>

namespace hrpc {

using ::hudp::Addr;

class EndpointList
{
public:
  EndpointList();
  EndpointList(const EndpointList& other);
  ~EndpointList();

  // modifiers
  bool PushBack(const Addr& endpoint);
  void Clear();

  // accessors
  size_t size() const {
    return size_;
  }
  bool empty() const {
    return size_ == 0;
  }
  Addr operator[](size_t pos) const {
    return {list_[pos].ip, list_[pos].port};
  }


private:
  struct Endpoint {
    uint32_t ip;
    uint16_t port;
  };
  // max of 4 backup endpoints are enough for one rpc session
  static constexpr size_t kCapacity = 4;

  size_t size_;
  Endpoint list_[kCapacity];
};

} // namespace hrpc

#endif // _HRPC_ENDPOINT_LIST_H
