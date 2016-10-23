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
#include <assert.h>
#include <string.h>
#include "hyperrpc/endpoint_list.h"

namespace hrpc {

constexpr size_t EndpointList::kListCacheSize;
constexpr size_t EndpointList::kListInHeapInitSize;

EndpointList::EndpointList()
  : size_(0), list_in_heap_(nullptr)
{
}

EndpointList::EndpointList(const EndpointList& other)
  : size_(0), list_in_heap_(nullptr)
{
  for (size_t i = 0; i < other.size_ && i < kListCacheSize; i++) {
    list_cache_[i] = other.list_cache_[i];
  }
  if (other.size_ > kListCacheSize) {
    // deep copy list_in_heap_
    size_t list_in_heap_size = other.size_ - kListCacheSize;
    size_t capacity = kListInHeapInitSize;
    while (capacity < list_in_heap_size) {
      capacity <<= 1UL;
    }
    list_in_heap_ = new Endpoint[capacity];
    memcpy(list_in_heap_, other.list_in_heap_,
           sizeof(Endpoint) * list_in_heap_size);
  }
  size_ = other.size_;
}

EndpointList::~EndpointList()
{
  Clear();
}

void EndpointList::PushBack(const Addr& endpoint)
{
  if (size_ < kListCacheSize) {
    Endpoint* slot = &list_cache_[size_];
    slot->ip = endpoint.ip();
    slot->port = endpoint.port();
    size_++;
    return;
  }
  // append list_in_heap_
  size_t list_in_heap_size = size_ - kListCacheSize;
  if (list_in_heap_size == 0) {
    // initialize list_in_heap_
    list_in_heap_ = new Endpoint[kListInHeapInitSize];
  } else if (list_in_heap_size >= kListInHeapInitSize &&
             (list_in_heap_size & (list_in_heap_size - 1)) == 0) {
    // reallocate to double size
    auto list_in_heap_old = list_in_heap_;
    list_in_heap_ = new Endpoint[list_in_heap_size << 1];
    memcpy(list_in_heap_, list_in_heap_old,
           sizeof(Endpoint) * list_in_heap_size);
    delete[] list_in_heap_old;
  }
  Endpoint* slot = &list_in_heap_[list_in_heap_size];
  slot->ip = endpoint.ip();
  slot->port = endpoint.port();
  size_++;
}

void EndpointList::Clear()
{
  if (list_in_heap_) {
    delete[] list_in_heap_;
    list_in_heap_ = nullptr;
  }
  size_ = 0;
}

Addr EndpointList::GetEndpoint(size_t index) const
{
  assert(index < size_);
  if (index < kListCacheSize) {
    return {list_cache_[index].ip, list_cache_[index].port};
  } else {
    size_t heap_index = index - kListCacheSize;
    return {list_in_heap_[heap_index].ip, list_in_heap_[heap_index].port};
  }
}

} // namespace hrpc
