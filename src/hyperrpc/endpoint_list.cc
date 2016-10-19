#include "hyperrpc/endpoint_list.h"

namespace hrpc {

constexpr size_t EndpointList::kCapacity;

EndpointList::EndpointList()
  : size_(0)
{
}

EndpointList::EndpointList(const EndpointList& other)
{
  for (size_t i = 0; i < other.size_; i++) {
    list_[i] = other.list_[i];
  }
  size_ = other.size_;
}

EndpointList::~EndpointList()
{
}

bool EndpointList::PushBack(const Addr& endpoint)
{
  if (size_ >= kCapacity)
    return false;
  Endpoint* slot = &list_[size_++];
  slot->ip = endpoint.ip();
  slot->port = endpoint.port();
  return true;
}

void EndpointList::Clear()
{
  size_ = 0;
}

} // namespace hrpc
