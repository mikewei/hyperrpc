/* Copyright (c) 2016, Bin Wei <bin@vip.qq.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * The name of of its contributors may not be used to endorse or 
 * promote products derived from this software without specific prior 
 * written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _HRPC_CPP_SERVICE_H
#define _HRPC_CPP_SERVICE_H

#include <map>
#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;
  }
} // namespace protobuf
} // namespace google

namespace hrpc {
namespace compiler {

using namespace ::google::protobuf;

class CppServiceGenerator
{
 public:
  explicit CppServiceGenerator(const ServiceDescriptor* descriptor);
  ~CppServiceGenerator();

  void GenerateDeclarations(io::Printer* printer);
  void GenerateDescriptorInitializer(io::Printer* printer, int index);
  void GenerateImplementation(io::Printer* printer);

 private:
  void GenerateMethodSignatures(bool is_virtual, io::Printer* printer);
  void GenerateNotImplementedMethods(io::Printer* printer);
  void GenerateCallMethod(io::Printer* printer);
  void GenerateGetPrototype(bool is_request, io::Printer* printer);
  void GenerateStubMethods(io::Printer* printer);

  const ServiceDescriptor* descriptor_;
  std::map<std::string, std::string> vars_;
};

}  // namespace compiler
}  // namespace hrpc

#endif  // _HRPC_CPP_SERVICE_H
