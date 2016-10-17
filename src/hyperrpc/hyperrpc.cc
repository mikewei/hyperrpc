#include <assert.h>
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
  assert(!is_initialized_);
  size_t worker_num = env_.opt().worker_num;
  for (size_t i = 0; i < worker_num; i++) {
    rpc_core_vec_.emplace_back(new RpcCore(env_));
    if (!rpc_core_vec_[i]->Init(
          // OnSendPacket
          [this](const Buf& buf, const Addr& addr) {
            hyper_udp_.Send(buf, addr);
          },
          // OnFindService
          [this](const std::string& name) -> Service* {
            auto it = service_map_.find(name);
            if (it != service_map_.end()) return it->second;
            else return nullptr;
          },
          on_service_routing_)) {
      rpc_core_vec_.clear();
      return false;
    }
  }
  if (!hyper_udp_.Init(bind_local_addr, nullptr)) {
    rpc_core_vec_.clear();
    return false;
  }
  is_initialized_ = true;
  return true;
}

void HyperRpc::Impl::CallMethod(
                  const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::ccb::ClosureFunc<void(Result)>& done)
{
  assert(is_initialized_);
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
