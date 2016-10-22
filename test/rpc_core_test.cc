#include <google/protobuf/descriptor.h>
#include <gtestx/gtestx.h>
#include <ccbase/timer_wheel.h>
#include "hyperrpc/rpc_core.h"
#include "test_message.hrpc.pb.h"

class TestServiceImpl : public TestService
{
protected:
  void Query(const TestRequest* request, TestResponse* response,
             ccb::ClosureFunc<void(hrpc::Result)> done) {
    response->set_id(request->id());
    response->set_value(request->param());
    done(hrpc::kSuccess);
  }
};

class RpcCoreTest : public testing::Test
{
protected:
  static constexpr size_t kRpcCoreId = 0;

  RpcCoreTest()
    : tw_(1000, false)
    , env_(hrpc::OptionsBuilder().DefaultRpcTimeout(10)
                                 .LogHandler(hrpc::kError,
                                    [](hrpc::LogLevel, const char* s) {
                                      printf("%s\n", s);
                                    }).Build(), &tw_)
    , rpc_core_(env_)
    , enable_send_packet_(true)
    , send_packet_timeout_(1) {}

  virtual void SetUp() {
    ASSERT_TRUE(rpc_core_.Init(kRpcCoreId,
        ccb::BindClosure(this, &RpcCoreTest::OnSendPacket),
        ccb::BindClosure(this, &RpcCoreTest::OnFindService),
        ccb::BindClosure(this, &RpcCoreTest::OnServiceRouting)));

    request_.set_id(10000);
    request_.set_param("hello");
    tw_.MoveOn();
  }

  virtual void TearDown() {
  }

  void OnSendPacket(const hrpc::Buf& buf, const hrpc::Addr& addr, void* ctx) {
    if (!enable_send_packet_) {
      tw_.AddTimer(send_packet_timeout_, [this, ctx] {
          rpc_core_.OnSendPacketFailed(ctx);
      });
      return;
    }
    rpc_core_.OnRecvPacket(buf, addr);
  }

  hrpc::Service* OnFindService(const std::string& service_name) {
    return &service_;
  }

  bool OnServiceRouting(const std::string& service, const std::string& method,
                        hrpc::EndpointListBuilder* out) {
    static hrpc::Addr addr{"127.0.0.1", 1234};
    out->PushBack(addr);
    out->PushBack(addr);
    return true;
  }

  void EnableSendPacket(bool enable) {
    enable_send_packet_ = enable;
  }

  ccb::TimerWheel tw_;
  hrpc::Env env_;
  hrpc::RpcCore rpc_core_;
  bool enable_send_packet_;
  size_t send_packet_timeout_;

  TestRequest request_;
  TestResponse response_;
  TestServiceImpl service_;
};

TEST_F(RpcCoreTest, LoopCall)
{
  rpc_core_.CallMethod(TestService::descriptor()->method(0),
                       &request_, &response_, [this](hrpc::Result result) {
                         ASSERT_EQ(hrpc::kSuccess, result);
                         ASSERT_EQ(request_.id(), response_.id());
                         ASSERT_EQ(request_.param(), response_.value());
                       });
}

PERF_TEST_F(RpcCoreTest, LoopCallPerf)
{
  rpc_core_.CallMethod(TestService::descriptor()->method(0),
                       &request_, &response_, [this](hrpc::Result result) {
                         ASSERT_EQ(hrpc::kSuccess, result);
                       });
}

TEST_F(RpcCoreTest, TryAllFailed)
{
  bool done = false;
  EnableSendPacket(false);
  rpc_core_.CallMethod(TestService::descriptor()->method(0),
                       &request_, &response_, [&done](hrpc::Result result) {
                         ASSERT_EQ(hrpc::kTimeout, result);
                         done = true;
                       });
  for (int i = 0; i < 5; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_TRUE(done);
}

TEST_F(RpcCoreTest, TimeoutFailed)
{
  bool done = false;
  EnableSendPacket(false);
  send_packet_timeout_ = 50; // larger than rpc-timeout
  rpc_core_.CallMethod(TestService::descriptor()->method(0),
                       &request_, &response_, [&done](hrpc::Result result) {
                         ASSERT_EQ(hrpc::kTimeout, result);
                         done = true;
                       });
  for (int i = 0; i < 20; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_TRUE(done);
}

TEST_F(RpcCoreTest, RetrySuccess)
{
  bool done = false;
  EnableSendPacket(false);
  rpc_core_.CallMethod(TestService::descriptor()->method(0),
                &request_, &response_, [this, &done](hrpc::Result result) {
                  ASSERT_EQ(hrpc::kSuccess, result);
                  ASSERT_EQ(request_.id(), response_.id());
                  ASSERT_EQ(request_.param(), response_.value());
                  done = true;
                });
  ASSERT_FALSE(done);
  EnableSendPacket(true);
  for (int i = 0; i < 5; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_TRUE(done);
}

