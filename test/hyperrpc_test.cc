#include <gtestx/gtestx.h>
#include <ccbase/timer_wheel.h>
#include "hyperrpc/hyperrpc.h"
#include "test_message.hrpc.pb.h"

namespace {

class TestServiceImpl : public TestService
{
protected:
  virtual ~TestServiceImpl() {}
  virtual void Query(const TestRequest* request, TestResponse* response,
                     hrpc::DoneFunc done) override {
    response->set_id(request->id());
    response->set_value(request->param());
    done(hrpc::kSuccess);
  }
};

} // namespace

class HyperRpcTest : public testing::Test
{
protected:

  virtual void SetUp() {
    addr_list_.emplace_back("127.0.0.1", 17777);
    service_ = new TestServiceImpl;
    ASSERT_TRUE(hyper_rpc_.InitAsClient(
                ccb::BindClosure(this, &HyperRpcTest::OnServiceRouting)));
    ASSERT_TRUE(hyper_rpc_.InitAsServer({service_}));
    ASSERT_TRUE(hyper_rpc_.Start(addr_list_[0]));
  }

  virtual void TearDown() {
    delete service_;
  }

  bool OnServiceRouting(const std::string& service,
                        const std::string& method,
                        const google::protobuf::Message& request,
                        hrpc::RouteInfoBuilder* out) {
    for (auto& addr : addr_list_) {
      out->AddEndpoint(addr);
    }
    return true;
  }

  hrpc::HyperRpc hyper_rpc_;
  hrpc::Service* service_;
  std::vector<hrpc::Addr> addr_list_;
};

TEST_F(HyperRpcTest, AsyncCall)
{
  TestService::Stub test_service(&hyper_rpc_);
  TestRequest* request = new TestRequest;
  request->set_id(10000);
  request->set_param("hello");
  TestResponse* response = new TestResponse;
  bool done = false;
  test_service.Query(request, response,
    [request, response, &done](hrpc::Result result) {
      ASSERT_EQ(hrpc::kSuccess, result);
      ASSERT_EQ(request->id(), response->id());
      ASSERT_EQ(request->param(), response->value());
      delete request;
      delete response;
      done = true;
    });
  usleep(1000*10);
  ASSERT_TRUE(done);
}

TEST_F(HyperRpcTest, SyncCall)
{
  TestService::Stub test_service(&hyper_rpc_);
  TestRequest request;
  request.set_id(10000);
  request.set_param("hello");
  TestResponse response;
  ASSERT_EQ(hrpc::kSuccess, test_service.Query(request, &response));
  ASSERT_EQ(request.id(), response.id());
  ASSERT_EQ(request.param(), response.value());
}

TEST_F(HyperRpcTest, TimeoutFailed)
{
  addr_list_[0] = {"127.0.0.1", 28888};
  TestService::Stub test_service(&hyper_rpc_);
  TestRequest request;
  request.set_id(10000);
  request.set_param("hello");
  TestResponse response;
  ASSERT_EQ(hrpc::kTimeout, test_service.Query(request, &response));
}

TEST_F(HyperRpcTest, RetrySuccess)
{
  addr_list_.emplace(addr_list_.begin(), "127.0.0.1", 28888);
  TestService::Stub test_service(&hyper_rpc_);
  TestRequest request;
  request.set_id(10000);
  request.set_param("hello");
  TestResponse response;
  ASSERT_EQ(hrpc::kSuccess, test_service.Query(request, &response));
  ASSERT_EQ(request.id(), response.id());
  ASSERT_EQ(request.param(), response.value());
}

