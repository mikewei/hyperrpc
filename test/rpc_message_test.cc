#include <gtestx/gtestx.h>
#include <google/protobuf/arena.h>
#include "hyperrpc/rpc_message.pb.h"

class RpcMessageTest : public testing::Test
{
protected:
  virtual void SetUp() {
    rpc_header_.set_packet_type(hrpc::RpcHeader::REQUEST);
    rpc_header_.set_service_name("TestService");
    rpc_header_.set_method_name("TestMethod");
    rpc_header_.set_rpc_id(1);
    rpc_header_.set_rpc_result(0);
    rpc_header_.SerializeToArray(rpc_header_buf_, sizeof(rpc_header_buf_));
    rpc_header_buf_len_ = rpc_header_.ByteSizeLong();
  }

  hrpc::RpcHeader rpc_header_;
  char rpc_header_buf_[1024];
  size_t rpc_header_buf_len_;
};

PERF_TEST_F(RpcMessageTest, Encoding)
{
  rpc_header_.SerializeToArray(rpc_header_buf_, sizeof(rpc_header_buf_));
}

PERF_TEST_F(RpcMessageTest, Decoding)
{
  hrpc::RpcHeader rpc_header;
  rpc_header.ParseFromArray(rpc_header_buf_, rpc_header_buf_len_);
}

PERF_TEST_F(RpcMessageTest, ReuseDecoding)
{
  rpc_header_.ParseFromArray(rpc_header_buf_, rpc_header_buf_len_);
}

