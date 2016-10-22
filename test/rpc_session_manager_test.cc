#include <google/protobuf/descriptor.h>
#include <gtestx/gtestx.h>
#include <ccbase/timer_wheel.h>
#include "hyperrpc/rpc_session_manager.h"
#include "test_message.hrpc.pb.h"

class RpcSessionManagerTest : public testing::Test
{
protected:
  static constexpr size_t kMaxRpcSessions = 128;
  static constexpr size_t kRpcCoreId = 0;

  RpcSessionManagerTest()
    : enable_send_request_(true)
    , hudp_send_timeout_(1)
    , tw_(1000, false)
    , env_(hrpc::OptionsBuilder().DefaultRpcTimeout(5)
                                 .LogHandler(hrpc::kError,
                                    [](hrpc::LogLevel, const char* s) {
                                      printf("%s\n", s);
                                    }).Build(), &tw_)
    , sess_mgr_(env_) {}

  virtual void SetUp() {
    ASSERT_TRUE(sess_mgr_.Init(kMaxRpcSessions, kRpcCoreId,
        ccb::BindClosure(this, &RpcSessionManagerTest::OnSendRequest)));
    request_.set_id(10000);
    request_.set_param("hello");
    endpoints_.PushBack({"127.0.0.1", 1234});
    endpoints_.PushBack({"127.0.0.2", 1234});
    endpoints_.PushBack({"127.0.0.3", 1234});
    send_request_count_ = 0;
    tw_.MoveOn();
  }

  virtual void TearDown() {
  }

  void EnableSendRequest(bool enable) {
    enable_send_request_ = enable;
  }

  void OnSendRequest(const google::protobuf::MethodDescriptor* method,
                     const google::protobuf::Message& request,
                     uint64_t rpc_id,
                     const hrpc::Addr&) {
    send_request_count_++;
    if (!enable_send_request_) {
      tw_.AddTimer(hudp_send_timeout_, [this, rpc_id] {
          sess_mgr_.OnSendRequestFailed(rpc_id);
      });
      return;
    }
    // TestRequest and TestResponse are binary compatible
    char resp_buf[1024];
    size_t resp_len = request.ByteSizeLong();
    ASSERT_TRUE(request.SerializeToArray(resp_buf, sizeof(resp_buf)));
    sess_mgr_.OnRecvResponse(method->service()->name(), method->name(),
                             rpc_id, hrpc::kSuccess, {resp_buf, resp_len});
  }

  bool enable_send_request_;
  size_t hudp_send_timeout_;
  ccb::TimerWheel tw_;
  hrpc::Env env_;
  hrpc::RpcSessionManager sess_mgr_;
  TestRequest request_;
  TestResponse response_;
  hrpc::EndpointList endpoints_;
  size_t send_request_count_;
};

TEST_F(RpcSessionManagerTest, SimpleCall)
{
  ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                           &request_, &response_, endpoints_,
                           [this](hrpc::Result result) {
                             ASSERT_EQ(hrpc::kSuccess, result);
                             ASSERT_EQ(request_.id(), response_.id());
                             ASSERT_EQ(request_.param(), response_.value());
                           }));
}

PERF_TEST_F(RpcSessionManagerTest, SimpleCallPerf)
{
  ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                           &request_, &response_, endpoints_,
                           [](hrpc::Result result) {}));
}

TEST_F(RpcSessionManagerTest, SessionPoolOverflow)
{
  EnableSendRequest(false);
  for (size_t i = 0; i < kMaxRpcSessions; i++) {
    ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                             &request_, &response_, endpoints_,
                             [](hrpc::Result result) {
                               ASSERT_TRUE(false);
                             }));
  }
  ASSERT_FALSE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                             &request_, &response_, endpoints_,
                             [](hrpc::Result result) {
                               ASSERT_EQ(hrpc::kInError, result);
                             }));
}

TEST_F(RpcSessionManagerTest, TryAllFailed)
{
  bool done = false;
  EnableSendRequest(false);
  ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                           &request_, &response_, endpoints_,
                           [&done](hrpc::Result result) {
                             ASSERT_EQ(hrpc::kTimeout, result);
                             done = true;
                           }));
  for (int i = 0; i < 10; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_EQ(3, send_request_count_);
  ASSERT_TRUE(done);
}

TEST_F(RpcSessionManagerTest, TimeoutFailed)
{
  bool done = false;
  EnableSendRequest(false);
  hudp_send_timeout_ = 3;
  ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                           &request_, &response_, endpoints_,
                           [&done](hrpc::Result result) {
                             ASSERT_EQ(hrpc::kTimeout, result);
                             done = true;
                           }));
  for (int i = 0; i < 10; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_EQ(2, send_request_count_);
  ASSERT_TRUE(done);
}

TEST_F(RpcSessionManagerTest, RetrySuccess)
{
  bool done = false;
  EnableSendRequest(false);
  hudp_send_timeout_ = 3;
  ASSERT_TRUE(sess_mgr_.AddSession(TestService::descriptor()->method(0),
                           &request_, &response_, endpoints_,
                           [&done](hrpc::Result result) {
                             ASSERT_EQ(hrpc::kSuccess, result);
                             done = true;
                           }));
  ASSERT_FALSE(done);
  EnableSendRequest(true);
  for (int i = 0; i < 10; i++) {
    usleep(1000);
    tw_.MoveOn();
  }
  ASSERT_EQ(2, send_request_count_);
  ASSERT_TRUE(done);
}

