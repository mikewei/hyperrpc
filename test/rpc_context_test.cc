#include <google/protobuf/descriptor.h>
#include <gtestx/gtestx.h>
#include <ccbase/timer_wheel.h>
#include "hyperrpc/rpc_context.h"
#include "test_message.hrpc.pb.h"

class RpcContextTest : public testing::Test
{
protected:
  virtual void SetUp() {
    TestRequest request;
    request.set_id(1);
    request.set_param("hello");
    request.SerializeToArray(request_buf_, sizeof(request_buf_));
    request_buf_len_ = request.ByteSizeLong();
  }

  virtual void TearDown() {
  }

  char request_buf_[1024];
  size_t request_buf_len_;
};

TEST_F(RpcContextTest, IncomingRpcContext)
{
  constexpr uint64_t kRpcId = 1000UL;
  const hrpc::Addr addr{"127.0.0.1", 1234};
  auto ctx = new hrpc::IncomingRpcContext(
                            TestService::descriptor()->method(0),
                            kRpcId, addr);
  ctx->Init(TestRequest::default_instance(),
            TestResponse::default_instance());
  ASSERT_EQ(TestService::descriptor()->method(0), ctx->method());
  ASSERT_EQ(TestRequest::descriptor(), ctx->request()->GetDescriptor());
  ASSERT_EQ(TestResponse::descriptor(), ctx->response()->GetDescriptor());
  ASSERT_EQ(kRpcId, ctx->rpc_id());
  ASSERT_EQ(addr, ctx->addr());
}

TEST_F(RpcContextTest, ArenaIncomingRpcContext)
{
  constexpr uint64_t kRpcId = 1000UL;
  const hrpc::Addr addr{"127.0.0.1", 1234};
  auto ctx = new hrpc::ArenaIncomingRpcContext(
                            TestService::descriptor()->method(0),
                            kRpcId, addr);
  ctx->Init(TestRequest::default_instance(),
            TestResponse::default_instance());
  ASSERT_EQ(TestService::descriptor()->method(0), ctx->method());
  ASSERT_EQ(TestRequest::descriptor(), ctx->request()->GetDescriptor());
  ASSERT_EQ(TestResponse::descriptor(), ctx->response()->GetDescriptor());
  ASSERT_EQ(kRpcId, ctx->rpc_id());
  ASSERT_EQ(addr, ctx->addr());
}

PERF_TEST_F(RpcContextTest, IncomingRpcContextPerf)
{
  static constexpr uint64_t kRpcId = 1000UL;
  static const hrpc::Addr addr{"127.0.0.1", 1234};
  hrpc::IncomingRpcContext ctx {
    TestService::descriptor()->method(0), kRpcId, addr
  };
  ctx.Init(TestRequest::default_instance(),
           TestResponse::default_instance());
  ctx.request()->ParseFromArray(request_buf_, request_buf_len_);
  static_cast<TestRequest*>(ctx.response())->set_id(
                      static_cast<TestRequest*>(ctx.request())->id());
  static_cast<TestResponse*>(ctx.response())->set_value(
                      static_cast<TestRequest*>(ctx.request())->param());
}

PERF_TEST_F(RpcContextTest, ArenaIncomingRpcContextPerf)
{
  static constexpr uint64_t kRpcId = 1000UL;
  static const hrpc::Addr addr{"127.0.0.1", 1234};
  hrpc::ArenaIncomingRpcContext ctx {
    TestService::descriptor()->method(0), kRpcId, addr
  };
  ctx.Init(TestRequest::default_instance(),
           TestResponse::default_instance());
  ctx.request()->ParseFromArray(request_buf_, request_buf_len_);
  static_cast<TestRequest*>(ctx.response())->set_id(
                      static_cast<TestRequest*>(ctx.request())->id());
  static_cast<TestResponse*>(ctx.response())->set_value(
                      static_cast<TestRequest*>(ctx.request())->param());
}
