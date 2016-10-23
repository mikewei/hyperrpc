#include <assert.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/descriptor.pb.h>
#include "hyperrpc/rpc_core.h"
#include "hyperrpc/service.h"
#include "hyperrpc/protocol.h"
#include "hyperrpc/constants.h"
#include "hyperrpc/rpc_context.h"
#include "hyperrpc/route_info_builder.h"
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

static size_t CalcSessionPoolSize(const Options& opt)
{
  size_t size_per_core = opt.max_rpc_sessions / opt.hudp_options.worker_num;
  size_per_core = size_per_core * 10 / 9;
  return size_per_core;
}

bool RpcCore::Init(size_t rpc_core_id, OnSendPacket on_send_pkt,
                                   OnFindService on_find_svc,
                                   OnServiceRouting on_svc_routing)
{
  rpc_core_id_ = rpc_core_id;
  on_send_packet_ = on_send_pkt;
  on_find_service_ = on_find_svc;
  on_service_routing_ = on_svc_routing;
  if (!rpc_sess_mgr_.Init(CalcSessionPoolSize(env_.opt()), rpc_core_id,
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
  // resolve endpoints of service.method
  EndpointList endpoints;
  RouteInfoBuilderImpl builder(&endpoints);
  if (!on_service_routing_ ||
      !on_service_routing_(service_name, method_name, *request, &builder) ||
      endpoints.empty()) {
    WLOG("cannot resolve endpoints for %s.%s", service_name.c_str(),
                                               method_name.c_str());
    done(kNoRoute);
    return;
  }
  rpc_sess_mgr_.AddSession(method, request, response,
                           endpoints, std::move(done));
}

inline bool RpcCore::GetCoreIdFromRpcId(uint64_t rpc_id, size_t* rpc_core_id)
{
  size_t rpc_core_id_plus_1 = rpc_id >> kRpcIdSeqPartBits;
  if (rpc_core_id_plus_1 == 0 ||
      rpc_core_id_plus_1 > env_.opt().hudp_options.worker_num) {
    return false;
  }
  *rpc_core_id = rpc_core_id_plus_1 - 1;
  return true;
}

size_t RpcCore::OnRecvPacket(const Buf& buf, const Addr& addr)
{
  size_t cur_rpc_core_id = rpc_core_id_;
  // parse RpcPakcetHeader
  if (buf.len() <= sizeof(RpcPacketHeader)) {
    ILOG("packet len too short!");
    return cur_rpc_core_id;
  }
  auto pkt_header = static_cast<const RpcPacketHeader*>(buf.ptr());
  if (pkt_header->hrpc_pkt_tag != kHyperRpcPacketTag) {
    ILOG("bad packet tag!");
    return cur_rpc_core_id;
  }
  if (pkt_header->hrpc_pkt_ver != kHyperRpcPacketVer) {
    ILOG("packet ver dismatch!");
    return cur_rpc_core_id;
  }
  size_t rpc_header_len = ntohs(pkt_header->rpc_header_len);
  size_t rpc_body_len = ntohl(pkt_header->rpc_body_len);
  if (buf.len() != sizeof(RpcPacketHeader) + rpc_header_len + rpc_body_len) {
    ILOG("bad packet len!");
    return cur_rpc_core_id;
  }
  const void* rpc_header_ptr = buf.char_ptr() + sizeof(RpcPacketHeader);
  const void* rpc_body_ptr = static_cast<const char*>(rpc_header_ptr)
                           + rpc_header_len;
  // parse RpcHeader
  static thread_local RpcHeader rpc_header;
  if (!rpc_header.ParseFromArray(rpc_header_ptr, rpc_header_len)) {
    ILOG("parse RpcHeader failed!");
    return cur_rpc_core_id;
  }
  if (rpc_header.packet_type() == RpcHeader::REQUEST) {
    // process REQUEST message in current thread
    OnRecvRequestMessage(rpc_header, {rpc_body_ptr, rpc_body_len}, addr);
  } else {
    // process RESPONSE message in thread the rpc_id belongs
    size_t rpc_core_id_plus_1 = rpc_header.rpc_id() >> kRpcIdSeqPartBits;
    if (rpc_core_id_plus_1 == 0 ||
        rpc_core_id_plus_1 > env_.opt().hudp_options.worker_num) {
      ILOG("invalid rpc_core_id_plus_1:%lu!", rpc_core_id_plus_1);
      return cur_rpc_core_id;
    }
    size_t dst_rpc_core_id = rpc_core_id_plus_1 - 1;
    if (dst_rpc_core_id != cur_rpc_core_id) { // need redirect
      DLOG("OnRecvPacket redirect to rpc_core_id:%lu", dst_rpc_core_id);
      return dst_rpc_core_id;
    }
    OnRecvResponseMessage(rpc_header, {rpc_body_ptr, rpc_body_len}, addr);
  }
  return 0;
}

void RpcCore::OnRecvRequestMessage(const RpcHeader& header,
                                   const Buf& body, const Addr& addr)
{
  Service* service = on_find_service_(header.service_name());
  if (!service) IRET("service requested not found locally!");
  auto service_desc = service->GetDescriptor();
  if (!service_desc) IRET("get service descriptor failed!");
  auto method_desc = service_desc->FindMethodByName(header.method_name());
  if (!method_desc) IRET("get method descriptor failed!");
  auto& request_prot = service->GetRequestPrototype(method_desc);
  auto& response_prot = service->GetResponsePrototype(method_desc);

  IncomingRpcContext* ctx;
  if (!request_prot.GetDescriptor()->file()->options().cc_enable_arenas()) {
    // if message is not arena enabled using Arena will be even slower
    ctx = new IncomingRpcContext(method_desc, header.rpc_id(), addr);
  } else {
    ctx = new ArenaIncomingRpcContext(method_desc, header.rpc_id(), addr);
  }
  ctx->Init(request_prot, response_prot);

  if (!ctx->request()->ParseFromArray(body.ptr(), body.len()))
    IRET("parse Request message failed!");

  // dispatch incoming rpc within receiving worker-thread
  service->CallMethod(method_desc, ctx->request(), ctx->response(),
               ccb::BindClosure(this, &RpcCore::OnIncomingRpcDone, ctx));
}

void RpcCore::OnRecvResponseMessage(const RpcHeader& header,
                                    const Buf& body, const Addr& addr)
{
  if (header.rpc_result() < 0 || header.rpc_result() > kMaxResultValue) {
    ILOG("invalid rpc_result value!");
    return;
  }
  Result rpc_result = static_cast<Result>(header.rpc_result());
  rpc_sess_mgr_.OnRecvResponse(header.service_name(), header.method_name(),
                               header.rpc_id(), rpc_result, body);
}

void RpcCore::OnIncomingRpcDone(const IncomingRpcContext* ctx, Result result)
{
  static thread_local RpcHeader rpc_header;
  rpc_header.set_packet_type(RpcHeader::RESPONSE);
  rpc_header.set_service_name(ctx->method()->service()->name());
  rpc_header.set_method_name(ctx->method()->name());
  rpc_header.set_rpc_id(ctx->rpc_id());
  rpc_header.set_rpc_result(result);
  SendMessage(rpc_header, *ctx->response(), ctx->addr(), nullptr);
  delete ctx;
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
  SendMessage(rpc_header, request, addr, reinterpret_cast<void*>(rpc_id));
}

void RpcCore::SendMessage(const RpcHeader& header,
                          const google::protobuf::Message& body,
                          const Addr& addr,
                          void* ctx)
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
  on_send_packet_({pkt_buffer, pkt_size}, addr, ctx);
}

size_t RpcCore::OnSendPacketFailed(void* ctx)
{
  size_t cur_rpc_core_id = rpc_core_id_;
  if (ctx) { // non-zero means outgoing rpc-id
    uint64_t rpc_id = reinterpret_cast<uint64_t>(ctx);
    size_t dst_rpc_core_id;
    if (!GetCoreIdFromRpcId(rpc_id, &dst_rpc_core_id)) {
      ILOG("OnSendPacketFailed found bad rpc_id:%lu", rpc_id);
      return cur_rpc_core_id;
    }
    if (dst_rpc_core_id != cur_rpc_core_id) {
      DLOG("OnSendPacketFailed redirect to rpc_core_id:%lu", dst_rpc_core_id);
      return dst_rpc_core_id;
    }
    rpc_sess_mgr_.OnSendRequestFailed(rpc_id);
  }
  return cur_rpc_core_id;
}

} // namespace hrpc
