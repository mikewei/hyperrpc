#include <unordered_map>
#include <google/protobuf/descriptor.h>
#include "hyperrpc/hyperrpc.h"
#include "hyperrpc/env.h"
#include "hyperrpc/service.h"
#include "hyperrpc/rpc_core.h"

namespace hrpc {

class HyperRpc::Impl
{
public:
  Impl(const Options& opt);
  ~Impl();

  bool InitAsClient(OnServiceRouting& on_svc_routing);
  bool InitAsServer(std::vector<Service*>& services);
  bool Start(const Addr& bind_local_addr);

  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::ccb::ClosureFunc<void(Result)>& done);
private:
  Service* OnFindService(const std::string& service_name);
  void OnSendPacket(const Buf& buf, const Addr& addr);
  void OnRecvPacket(const Buf& buf, const Addr& addr);

  Env env_;
  hudp::HyperUdp hyper_udp_;
  OnServiceRouting on_service_routing_;
  std::unordered_map<std::string, Service*> service_map_;
  std::vector<std::unique_ptr<RpcCore>> rpc_core_vec_;
  bool is_initialized_;
};

HyperRpc::Impl::Impl(const Options& opt)
  : env_(opt)
  , service_map_(1024)
  , is_initialized_(false)
{
}

HyperRpc::Impl::~Impl()
{
}

bool HyperRpc::Impl::InitAsClient(OnServiceRouting& on_svc_routing)
{
  on_service_routing_ = on_svc_routing;
  return true;
}

bool HyperRpc::Impl::InitAsServer(std::vector<Service*>& services)
{
  for (Service* svc : services) {
    service_map_[svc->GetDescriptor()->name()] = svc;
  }
  return true;
}

bool HyperRpc::Impl::Start(const Addr& bind_local_addr)
{
  HRPC_ASSERT(!is_initialized_);
  // initialize RpcCore vector
  RpcCore::OnSendPacket on_send_packet {
    ccb::BindClosure(this, &HyperRpc::Impl::OnSendPacket)
  };
  RpcCore::OnFindService on_find_service {
    ccb::BindClosure(this, &HyperRpc::Impl::OnFindService)
  };
  size_t worker_num = env_.opt().worker_num;
  for (size_t i = 0; i < worker_num; i++) {
    rpc_core_vec_.emplace_back(new RpcCore(env_));
    if (!rpc_core_vec_[i]->Init(i, on_send_packet,
                                   on_find_service,
                                   on_service_routing_)) {
      rpc_core_vec_.clear();
      return false;
    }
  }
  // initialize HyperUdp
  if (!hyper_udp_.Init(bind_local_addr,
                       ccb::BindClosure(this, &HyperRpc::Impl::OnRecvPacket)))
  {
    rpc_core_vec_.clear();
    return false;
  }
  // done
  is_initialized_ = true;
  return true;
}

Service* HyperRpc::Impl::OnFindService(const std::string& service_name)
{
  auto it = service_map_.find(service_name);
  if (it != service_map_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

void HyperRpc::Impl::OnSendPacket(const Buf& buf, const Addr& addr)
{
  hyper_udp_.Send(buf, addr);
}

void HyperRpc::Impl::OnRecvPacket(const Buf& buf, const Addr& addr)
{
  size_t cur_core_id = ccb::Worker::self()->id();
  size_t dst_core_id = rpc_core_vec_[cur_core_id]->OnRecvPacket(buf, addr);
  if (dst_core_id != cur_core_id) {
    // cross thread redirect
    size_t redirect_buf_len = buf.len();
    void*  redirect_buf_ptr = env_.alloc().Alloc(redirect_buf_len);
    memcpy(static_cast<char*>(redirect_buf_ptr), buf.ptr(), buf.len());
    if (!ccb::Worker::self()->worker_group()->PostTask(dst_core_id, [=] {
      rpc_core_vec_[dst_core_id]->OnRecvPacket(
                             {redirect_buf_ptr, redirect_buf_len}, addr);
      env_.alloc().Free(redirect_buf_ptr, redirect_buf_len);
    })) {
      // worker-queue overflow
      WLOG("OnRecvPacket PostTask failed because of worker-queue overflow!");
      env_.alloc().Free(redirect_buf_ptr, redirect_buf_len);
    }
  }
}

void HyperRpc::Impl::CallMethod(
                  const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::ccb::ClosureFunc<void(Result)>& done)
{
  HRPC_ASSERT(is_initialized_);
  ccb::WorkerGroup* worker_group = hyper_udp_.GetWorkerGroup();
  if (worker_group->is_current_thread()) {
    // dispatch in current worker-thread
    size_t rpc_core_id = ccb::Worker::self()->id();
    rpc_core_vec_[rpc_core_id]->CallMethod(method, request, response,
                                           std::move(done));
  } else {
    // dispatch to a worker-thread of worker-group
    if (!worker_group->PostTask([=] {
      size_t rpc_core_id = ccb::Worker::self()->id();
      rpc_core_vec_[rpc_core_id]->CallMethod(method, request, response,
                                             std::move(done));
    })) {
      // worker-queue overflow
      WLOG("CallMethod PostTask failed because of worker-queue overflow!");
      done(kInError);
    }
  }
}

//------------------------- class HyperRpc -----------------------------

HyperRpc::HyperRpc()
  : HyperRpc(OptionsBuilder().Build())
{
}

HyperRpc::HyperRpc(const Options& opt)
  : pimpl_(new HyperRpc::Impl(opt))
{
}

HyperRpc::~HyperRpc()
{
}

bool HyperRpc::InitAsClient(OnServiceRouting on_svc_routing)
{
  return pimpl_->InitAsClient(on_svc_routing);
}

bool HyperRpc::InitAsServer(std::vector<Service*> services)
{
  return pimpl_->InitAsServer(services);
}

bool HyperRpc::Start(const Addr& bind_local_addr)
{
  return pimpl_->Start(bind_local_addr);
}

void HyperRpc::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* response,
                          ::ccb::ClosureFunc<void(Result)> done)
{
  pimpl_->CallMethod(method, request, response, done);
}

} // namespace hrpc
