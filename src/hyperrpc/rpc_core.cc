#include "hyperrpc/rpc_core.h"
#include "hyperrpc/service.h"

namespace hrpc {

RpcCore::RpcCore(const Env& env)
  : env_(env)
{
}

RpcCore::~RpcCore()
{
}


bool RpcCore::Init(OnSendPacket on_send_pkt,
                   OnFindService on_find_svc,
                   OnServiceRouting on_svc_routing)
{
  on_send_packet_ = on_send_pkt;
  on_find_service_ = on_find_svc;
  on_service_routing_ = on_svc_routing;
  return true;
}

void RpcCore::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                         const ::google::protobuf::Message* request,
                         ::google::protobuf::Message* response,
                         ::ccb::ClosureFunc<void(Result)> done)
{
}

void RpcCore::OnRecvPacket(const Buf& buf, const Addr& addr)
{
}

} // namespace hrpc
