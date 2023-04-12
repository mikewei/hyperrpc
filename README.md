# HyperRpc: A high performance protobuf based RPC framework

## Introduction

HyperRpc is a rpc framework based on protobuf and [HyperUdp](https://github.com/mikewei/hyperudp).

## HowToUse

### Compile


#### Prerequisites

```
# for gtestx
git clone https://github.com/mikewei/gtestx.git

# for ccbase,
# which contains a collection of useful foundation classes which are good complement to STL.
git clone https://github.com/mikewei/ccbase.git

# for shm_container which is used in HyperUdp
git clone https://github.com/mikewei/shm_container.git

# for HyperUdp which is used as network io framework
git clone https://github.com/mikewei/hyperudp.git

# for protobuf
git clone --recursive https://github.com/mikewei/third_party.git
```

We provide a [bazel](https://bazel.build/?hl=en) BUILD file, so just setting deps on it if you are using bazel, or you need to import .h and .cc files into your build system.

#### Compiling

Suppose you are using bazel:

```
cd {your_work}
# step1: for code generator
bazel build hyperrpc:protoc-gen-hrpc_cpp

# step2: for all test code and example code
bazel build hyperrpc:all --define=path=$(pwd)
```
P.S. 
- ${path} is used to find protoc-gen-hrpc_cpp at the right place
- we recommend to use [Bazelisk](https://github.com/bazelbuild/bazelisk/blob/master/README.md) to control bazel version, 'USE_BAZEL_VERSION=0.14.0' is a version that can compile successfully, latest bazel version such as 6.0.0 may complain when compiling with third_party/protobuf mentioned above.
if you are using a lastest bazel version, maybe you need to solve the compilation problems by your self.

#### Run

Referring to ./test directory for all unittest;
Referring to ./example directory for a simple hrpc client and hrpc server

