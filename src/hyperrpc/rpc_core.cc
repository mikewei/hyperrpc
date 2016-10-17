#include <assert.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "hyperrpc/rpc_core.h"
#include "hyperrpc/service.h"
#include "hyperrpc/protocol.h"
#include "hyperrpc/constants.h"
#include "hyperrpc/rpc_context.h"
#include "hyperrpc/rpc_message.pb.h"

namespace hrpc {

RpcCore::RpcCore(const Env& env)
  : env_(env)
  , rpc_sess_mgr_(env)
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
  if (!rpc_sess_mgr_.Init(10000, 0, // todo: add options
                     ccb::BindClosure(this, &RpcCore::OnOutgoingRpcSend))) {
    ELOG("RpcSessionManager init failed!");
    return false;
  }
  return true;
}

void RpcCore::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                         const ::google::protobuf::Message* request,
                         ::google::protobuf::Message* response,
                         ::ccb::ClosureFunc<void(Result)> done)
{
  const std::string& service_name = method->service()->name();
  const std::string& method_name = method->name();
  // resolve address
  static thread_local std::vector<Addr> addrs;
  if (!on_service_routing_ ||
      !on_service_routing_(service_name, method_name, &addrs) ||
      addrs.empty()) {
    WLOG("cannot resolve address for %s.%s", service_name.c_str(),
                                             method_name.c_str());
    done(kNoRoute);
    return;
  }
  // create rpc session
  // todo: addrs
  rpc_sess_mgr_.AddSession(method, request, response, std::move(done));
}

void RpcCore::OnRecvPacket(const Buf& buf, const Addr& addr)
{
  // parse RpcPakcetHeader
  if (buf.len() <= sizeof(RpcPacketHeader))
    IRET("packet len too short!");
  auto pkt_header = static_cast<const RpcPacketHeader*>(buf.ptr());
  if (pkt_header->hrpc_pkt_tag != kHyperRpcPacketTag)
    IRET("bad packet tag!");
  if (pkt_header->hrpc_pkt_ver != kHyperRpcPacketVer)
    IRET("packet ver dismatch!");
  size_t rpc_header_len = ntohs(pkt_header->rpc_header_len);
  size_t rpc_body_len = ntohl(pkt_header->rpc_body_len);
  if (buf.len() != sizeof(RpcPacketHeader) + rpc_header_len + rpc_body_len)
    IRET("bad packet len!");
  const void* rpc_header_ptr = buf.char_ptr() + sizeof(RpcPacketHeader);
  const void* rpc_body_ptr = static_cast<const char*>(rpc_header_ptr)
                           + rpc_header_len;
  // parse RpcHeader
  static thread_local RpcHeader rpc_header;
  if (!rpc_header.ParseFromArray(rpc_header_ptr, rpc_header_len))
    IRET("parse RpcHeader failed!");
  if (rpc_header.packet_type() == RpcHeader::REQUEST) {
    // process REQUEST message
    Service* service = on_find_service_(rpc_header.service_name());
    if (!service) IRET("service requested not found locally!");
    auto service_d = service->GetDescriptor();
    if (!service_d) IRET("get service descriptor failed!");
    auto method_d = service_d->FindMethodByName(rpc_header.method_name());
    if (!method_d) IRET("get method descriptor failed!");
    IncomingRpcContext* rpc = new IncomingRpcContext(method_d,
                                    service->GetRequestPrototype(method_d),
                                    service->GetResponsePrototype(method_d),
                                    rpc_header.rpc_id(), addr);

    if (!rpc->request()->ParseFromArray(rpc_body_ptr, rpc_body_len))
      IRET("parse Request message failed!");
    // always run local method within receiving worker-thread
    service->CallMethod(method_d, rpc->request(), rpc->response(),
               ccb::BindClosure(this, &RpcCore::OnIncomingRpcDone, rpc));
  } else {
    // process RESPONSE message
    if (rpc_header.rpc_result() ||
        rpc_header.rpc_result() > kMaxResultValue) {
      IRET("invalid rpc_result value!");
    }
    Result rpc_result = static_cast<Result>(rpc_header.rpc_result());
    rpc_sess_mgr_.OnRecvResponse(rpc_header.service_name(),
                                 rpc_header.method_name(),
                                 rpc_header.rpc_id(),
                                 rpc_result,
                                 {rpc_body_ptr, rpc_body_len});
  }
}

void RpcCore::OnIncomingRpcDone(const IncomingRpcContext* rpc, Result result)
{
  static thread_local RpcHeader rpc_header;
  rpc_header.set_packet_type(RpcHeader::RESPONSE);
  rpc_header.set_service_name(rpc->method()->service()->name());
  rpc_header.set_method_name(rpc->method()->name());
  rpc_header.set_rpc_id(rpc->rpc_id());
  rpc_header.set_rpc_result(result);
  SendMessage(rpc_header, *rpc->response(), rpc->addr());
  delete rpc;
}

void RpcCore::OnOutgoingRpcSend(
                          const google::protobuf::MethodDescriptor* method,
                          const google::protobuf::Message& request,
                          uint64_t rpc_id, const Addr& addr)
{
  static thread_local RpcHeader rpc_header;
  rpc_header.set_packet_type(RpcHeader::REQUEST);
  rpc_header.set_service_name(method->service()->name());
  rpc_header.set_method_name(method->name());
  rpc_header.set_rpc_id(rpc_id);
  SendMessage(rpc_header, request, addr);
}

void RpcCore::SendMessage(const RpcHeader& header,
                          const google::protobuf::Message& body,
                          const Addr& addr)
{
  size_t rpc_header_len = header.ByteSizeLong();
  size_t rpc_body_len = body.ByteSizeLong();
  size_t pkt_size = sizeof(RpcPacketHeader) + rpc_header_len + rpc_body_len;
  char pkt_buffer[pkt_size];
  // set RpcPacketHeader
  auto pkt_header = reinterpret_cast<RpcPacketHeader*>(pkt_buffer);
  pkt_header->hrpc_pkt_tag = kHyperRpcPacketTag;
  pkt_header->hrpc_pkt_ver = kHyperRpcPacketVer;
  pkt_header->rpc_header_len = htons(static_cast<uint16_t>(rpc_header_len));
  pkt_header->rpc_body_len = htonl(static_cast<uint32_t>(rpc_body_len));
  // set RpcHeader & body
  google::protobuf::io::ArrayOutputStream out(
      pkt_buffer + sizeof(RpcPacketHeader), rpc_header_len + rpc_body_len);
  HRPC_ASSERT(header.SerializeToZeroCopyStream(&out));
  HRPC_ASSERT(body.SerializeToZeroCopyStream(&out));
  // send to network
  on_send_packet_({pkt_buffer, pkt_size}, addr);
}

} // namespace hrpc
