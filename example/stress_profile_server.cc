#include <unistd.h>
#include <stdio.h>
#include <string>
#include <atomic>
#include <gflags/gflags.h>
#include <hyperrpc/hyperrpc.h>
#include "profile_service.hrpc.pb.h"

DEFINE_uint64(port, 16930, "local port to bind");
DEFINE_int32(v, 1, "log level (1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG");

static std::atomic<size_t> call_count{0};
static std::atomic<size_t> fail_count{0};

class ProfileServiceImpl : public hrpc::example::ProfileService
{
public:
  virtual ~ProfileServiceImpl() {}
protected:
  virtual void GetProfile(const hrpc::example::ProfileRequest* request,
                          hrpc::example::ProfileResponse* response,
                          hrpc::DoneFunc done) override {
    response->set_user_id(request->user_id());
    response->set_name("hrpcuser");
    response->set_age(25);
    call_count++;
    done(hrpc::kSuccess);
  }
};

void StartProfileServiceServer()
{
  // set options
  hrpc::OptionsBuilder options_builder;
  options_builder
    .LogHandler(static_cast<hrpc::LogLevel>(FLAGS_v),
                [](hrpc::LogLevel lv, const char* s) {
                  printf("[%d] %s\n", (int)lv, s);
                })
    .WorkerNumber(2);

  // initialize HyperRpc server
  hrpc::HyperRpc hyper_rpc{options_builder.Build()};
  ProfileServiceImpl profile_service;
  if (!hyper_rpc.InitAsServer({&profile_service})) {
    printf("InitAsServer failed!\n");
    return;
  }
  if (!hyper_rpc.Start({"0.0.0.0", static_cast<uint16_t>(FLAGS_port)})) {
    printf("HyperRpc Start failed!\n");
    return;
  }

  while (true) {
    usleep(1000*1000);
    printf("RPC server %lu qps, %lu failed\n", call_count.load(),
                                               fail_count.load());
    call_count.store(0);
    fail_count.store(0);
  }
}

int main(int argc, char* argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  StartProfileServiceServer();
  return 0;
}
