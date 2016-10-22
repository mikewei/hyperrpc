package(default_visibility = ["//visibility:public"])

load("hrpc", "cc_hrpc_proto_library")

cc_library(
  name = "hyperrpc",
  includes = ["src"],
  copts = [
    "-g",
    "-O2",
    "-Wall",
    "-Wno-unused-result",
  ],
  linkopts = [
    "-lrt",
  ],
  nocopts = "-fPIC",
  linkstatic = 1,
  srcs = glob([
    "src/hyperrpc/*.cc",
    "src/hyperrpc/*.h",
  ]),
  deps = [
    ":hrpc_message",
    "//hyperudp",
    "//third_party/protobuf",
  ],
)

cc_hrpc_proto_library(
  name = "hrpc_message",
  include = "src",
  srcs = glob(["src/hyperrpc/*.proto"]),
  deps = [],
  use_hrpc_plugin = False,
  copts = ["-O2"],
  nocopts = "-fPIC",
  linkstatic = 1,
)

cc_binary(
  name = "protoc-gen-hrpc_cpp",
  copts = [
    "-g",
    "-O2",
    "-Wall",
  ],
  nocopts = "-fPIC",
  linkstatic = 1,
  srcs = glob([
    "src/hyperrpc/compiler/*.cc",
    "src/hyperrpc/compiler/*.h",
  ]),
  deps = [
    ":hyperrpc",
    "//third_party/protobuf:protoc_lib",
  ],
  malloc = "//third_party/jemalloc-360"
)

cc_hrpc_proto_library(
  name = "test_message",
  include = "test",
  srcs = glob(["test/*.proto"]),
  deps = [],
  use_hrpc_plugin = True,
  copts = ["-O2"],
  nocopts = "-fPIC",
  linkstatic = 1,
)

cc_test(
  name = "test",
  copts = [
    "-g",
    "-O2",
    "-Wall",
    "-fno-strict-aliasing",
  ],
  nocopts = "-fPIC",
  linkstatic = 1,
  srcs = glob(["test/*_test.cc"]),
  deps = [
    ":hyperrpc",
    ":test_message",
    "//gtestx",
  ],
  malloc = "//third_party/jemalloc-360"
)

cc_hrpc_proto_library(
  name = "rpc_example_proto",
  include = "example",
  srcs = glob(["example/*.proto"]),
  deps = [],
  use_hrpc_plugin = True,
  copts = ["-O2"],
  nocopts = "-fPIC",
  linkstatic = 1,
)

cc_binary(
  name = "rpc_server",
  copts = [
    "-g",
    "-O2",
    "-Wall",
  ],
  nocopts = "-fPIC",
  linkstatic = 1,
  srcs = ["example/rpc_server.cc"],
  deps = [
    ":hyperrpc",
  ],
  malloc = "//third_party/jemalloc-360"
)

cc_binary(
  name = "rpc_client",
  copts = [
    "-g",
    "-O2",
    "-Wall",
  ],
  nocopts = "-fPIC",
  linkstatic = 1,
  srcs = ["example/rpc_client.cc"],
  deps = [
    ":hyperrpc",
  ],
  malloc = "//third_party/jemalloc-360"
)

