#include <unistd.h>
#include <string>
#include <atomic>
#include <gflags/gflags.h>
#include <ccbase/token_bucket.h>
#include <ccbase/timer_wheel.h>
#include <hyperrpc/hyperrpc.h>
#include "profile_service.hrpc.pb.h"

DEFINE_string(server, "127.0.0.1:16930", "server address (ip:port)");
DEFINE_uint64(qps, 10000, "qps");
DEFINE_uint64(port, 0, "local port to bind");
DEFINE_int32(v, 1, "log level (1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG");

static hrpc::Addr server_addr;
static std::atomic<size_t> call_count{0};
static std::atomic<size_t> fail_count{0};

bool OnServiceRouting(const std::string& service,
                      const std::string& method,
                      const google::protobuf::Message& request,
                      hrpc::RouteInfoBuilder* out)
{
  out->AddEndpoint(server_addr);
  return true;
}

void CallProfileService(hrpc::HyperRpc* hyper_rpc)
{
  // launch RPC call
  hrpc::example::ProfileService::Stub service{hyper_rpc};
  auto request = new hrpc::example::ProfileRequest;
  request->set_user_id(123);
  auto response = new hrpc::example::ProfileResponse;
  service.GetProfile(request, response, [request, response]
                                        (hrpc::Result result) {
    if (result != hrpc::kSuccess) fail_count++;
    delete request;
    delete response;
  });
}

void StartProfileServiceClient()
{
  // set options
  hrpc::OptionsBuilder options_builder;
  options_builder
    .LogHandler(static_cast<hrpc::LogLevel>(FLAGS_v),
                [](hrpc::LogLevel lv, const char* s) {
                  printf("[%d] %s\n", (int)lv, s);
                })
    .WorkerNumber(1);

  // initialize HyperRpc client
  hrpc::HyperRpc hyper_rpc{options_builder.Build()};
  if (!hyper_rpc.InitAsClient(ccb::BindClosure(&OnServiceRouting))) {
    printf("InitAsClient failed!\n");
    return;
  }
  if (!hyper_rpc.Start({"0.0.0.0", static_cast<uint16_t>(FLAGS_port)})) {
    printf("HyperRpc Start failed!\n");
    return;
  }

  // parse server address
  auto ip_port = hrpc::Addr::ParseFromString(FLAGS_server);
  if (!ip_port.first) {
    printf("bad server address param!\n");
    return;
  }
  server_addr = ip_port.second;

  // initialize counters
  ccb::TimerWheel timerw;
  timerw.AddPeriodTimer(1000, [&] {
    printf("RPC client %lu qps, %lu failed\n", call_count.load(),
                                              fail_count.load());
    call_count.store(0);
    fail_count.store(0);
  });

  // start calling with configured speed
  uint32_t qps = static_cast<uint32_t>(FLAGS_qps);
  ccb::TokenBucket token_bucket{qps, qps/10, 0};
  while (true) {
    if (!token_bucket.Get(1)) {
      usleep(1000);
      token_bucket.Gen();
      timerw.MoveOn();
      continue;
    }
    call_count++;
    // do RPC call
    CallProfileService(&hyper_rpc);
  }
}

int main(int argc, char* argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  StartProfileServiceClient();
  return 0;
}

