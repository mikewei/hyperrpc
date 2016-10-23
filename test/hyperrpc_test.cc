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
    service_ = new TestServiceImpl;
    ASSERT_TRUE(hyper_rpc_.InitAsClient(
                ccb::BindClosure(this, &HyperRpcTest::OnServiceRouting)));
    ASSERT_TRUE(hyper_rpc_.InitAsServer({service_}));
    ASSERT_TRUE(hyper_rpc_.Start(addr_));
  }

  virtual void TearDown() {
    delete service_;
  }

  bool OnServiceRouting(const std::string& service,
                        const std::string& method,
                        const google::protobuf::Message& request,
                        hrpc::RouteInfoBuilder* out) {
    out->AddEndpoint(addr_);
    return true;
  }

  hrpc::HyperRpc hyper_rpc_;
  hrpc::Service* service_;
  const hrpc::Addr addr_ = {"127.0.0.1", 17777};
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

