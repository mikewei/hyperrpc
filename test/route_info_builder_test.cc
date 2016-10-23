#include <gtestx/gtestx.h>
#include "hyperrpc/endpoint_list.h"
#include "hyperrpc/route_info_builder.h"

class RouteInfoBuilderTest : public testing::Test
{
protected:
  virtual void SetUp() {
    builder_ = new hrpc::RouteInfoBuilderImpl(&endpoint_list_);
  }

  virtual void TearDown() {
    delete static_cast<hrpc::RouteInfoBuilderImpl*>(builder_);
  }

  hrpc::EndpointList endpoint_list_;
  hrpc::RouteInfoBuilder* builder_;
};

TEST_F(RouteInfoBuilderTest, Builder)
{
  ASSERT_EQ(0, endpoint_list_.size());
  builder_->AddEndpoint({"127.0.0.1", 1234});
  ASSERT_EQ(1, endpoint_list_.size());
  builder_->AddEndpoint({"127.0.0.1", 5678});
  ASSERT_EQ(2, endpoint_list_.size());
  endpoint_list_.Clear();
  ASSERT_EQ(0, endpoint_list_.size());
  builder_->AddEndpoint({"127.0.0.1", 1234});
  ASSERT_EQ(1, endpoint_list_.size());
}

PERF_TEST_F(RouteInfoBuilderTest, BuilderPerf)
{
  static hrpc::Addr addr{"127.0.0.1", 1234};
  builder_->AddEndpoint(addr);
  endpoint_list_.Clear();
}

