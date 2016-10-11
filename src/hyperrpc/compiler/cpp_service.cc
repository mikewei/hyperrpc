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
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include "hyperrpc/compiler/cpp_service.h"

namespace hrpc {
namespace compiler {

using namespace ::google::protobuf::compiler::cpp;

CppServiceGenerator::CppServiceGenerator(const ServiceDescriptor* descriptor)
  : descriptor_(descriptor)
{
  vars_["classname"] = descriptor_->name();
  vars_["fullname"] = descriptor_->full_name();
}

CppServiceGenerator::~CppServiceGenerator()
{
}

void CppServiceGenerator::GenerateDeclarations(io::Printer* printer)
{
  // forward declarations
  printer->Print(vars_,
    "\n"
    "class $classname$_Stub;\n"
    "\n");

  // service class
  printer->Print(vars_,
    "class $classname$ : public ::hrpc::Service\n"
    "{\n"
    " protected:\n"
    "  // This class should be treated as an abstract interface.\n"
    "  inline $classname$() {};\n"
    " public:\n"
    "  virtual ~$classname$();\n");
  printer->Indent();

  printer->Print(vars_,
    "\n"
    "typedef $classname$_Stub Stub;\n"
    "\n"
    "static const ::google::protobuf::ServiceDescriptor* descriptor();\n"
    "\n");

  GenerateMethodSignatures(true, printer);

  printer->Print(
    "\n"
    "// implements Service ----------------------------------------------\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* GetDescriptor();\n"
    "::hrpc::Result CallMethod(\n"
    "    const ::google::protobuf::MethodDescriptor* method,\n"
    "    const ::google::protobuf::Message* request,\n"
    "    ::google::protobuf::Message* response,\n"
    "    ::google::protobuf::Closure* done);\n"
    "const ::google::protobuf::Message& GetRequestPrototype(\n"
    "    const ::google::protobuf::MethodDescriptor* method) const;\n"
    "const ::google::protobuf::Message& GetResponsePrototype(\n"
    "    const ::google::protobuf::MethodDescriptor* method) const;\n");

  printer->Outdent();
  printer->Print(vars_,
    "\n"
    " private:\n"
    "  // not copyable and movable\n"
    "  $classname$(const $classname$&) = delete;\n"
    "  void operator=(const $classname$&) = delete;\n"
    "  $classname$($classname$&&) = delete;\n"
    "  void operator=($classname$&&) = delete;\n"
    "};\n"
    "\n");

  // stub class
  printer->Print(vars_,
    "class $classname$_Stub : public $classname$\n"
    "{\n"
    " public:\n");

  printer->Indent();

  printer->Print(vars_,
    "$classname$_Stub(::hrpc::HyperRpc* hrpc);\n"
    "~$classname$_Stub();\n"
    "\n"
    "inline ::hrpc::HyperRpc* hrpc() const { return hrpc_; }\n"
    "\n"
    "// implements $classname$ ------------------------------------------\n"
    "\n");

  GenerateMethodSignatures(false, printer);

  printer->Outdent();
  printer->Print(vars_,
    "\n"
    " private:\n"
    "  // not copyable and movable\n"
    "  $classname$_Stub(const $classname$_Stub&) = delete;\n"
    "  void operator=(const $classname$_Stub&) = delete;\n"
    "  $classname$_Stub($classname$_Stub&&) = delete;\n"
    "  void operator=($classname$_Stub&&) = delete;\n"
    "\n"
    "  ::hrpc::HyperRpc* hrpc_;\n"
    "};\n"
    "\n");
}

void CppServiceGenerator::GenerateMethodSignatures(bool is_virtual,
                                                   io::Printer* printer)
{
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    std::map<std::string, std::string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);
    sub_vars["virtual"] = (is_virtual ? "virtual " : "");

    printer->Print(sub_vars,
      "$virtual$::hrpc::Result $name$(\n"
      "    const $input_type$* request,\n"
      "    $output_type$* response,\n"
      "    ::ccb::ClosureFunc<void(::hrpc::Result)> done);\n");
  }
}

void CppServiceGenerator::GenerateDescriptorInitializer(io::Printer* printer,
                                                        int index)
{
  std::map<std::string, std::string> vars;
  vars["classname"] = descriptor_->name();
  vars["index"] = SimpleItoa(index);

  printer->Print(vars,
    "$classname$_descriptor_ = file->service($index$);\n");
}

void CppServiceGenerator::GenerateImplementation(io::Printer* printer)
{
  // service implementation
  printer->Print(vars_,
    "$classname$::~$classname$() {}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::descriptor() {\n"
    "  hrpc_protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n"
    "const ::google::protobuf::ServiceDescriptor* $classname$::GetDescriptor() {\n"
    "  hrpc_protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n");
  GenerateNotImplementedMethods(printer);
  GenerateCallMethod(printer);
  GenerateGetPrototype(true, printer);
  GenerateGetPrototype(false, printer);

  // stub implementation
  printer->Print(vars_,
    "$classname$_Stub::$classname$_Stub(::hrpc::HyperRpc* hrpc)\n"
    "  : hrpc_(hrpc) {}\n"
    "$classname$_Stub::~$classname$_Stub() {}\n"
    "\n");
  GenerateStubMethods(printer);
}

void CppServiceGenerator::GenerateNotImplementedMethods(io::Printer* printer)
{
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    std::map<std::string, std::string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["methodname"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "void $classname$::$methodname$(\n"
      "    const $input_type$*,\n"
      "    $output_type$*,\n"
      "    ::ccb::ClosureFunc<void(::hrpc::Result)> done) {\n"
      "  done(::hrpc::kNotImpl);\n"
      "}\n"
      "\n");
  }
}

void CppServiceGenerator::GenerateCallMethod(io::Printer* printer)
{
  printer->Print(vars_,
    "void $classname$::CallMethod(\n"
    "    const ::google::protobuf::MethodDescriptor* method,\n"
    "    const ::google::protobuf::Message* request,\n"
    "    ::google::protobuf::Message* response,\n"
    "    ::ccb::ClosureFunc<void(::hrpc::Result)> done) {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), $classname$_descriptor_);\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    std::map<std::string, std::string> sub_vars;
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "    case $index$:\n"
      "      $name$(\n"
      "        ::google::protobuf::down_cast<const $input_type$*>(request),\n"
      "        ::google::protobuf::down_cast< $output_type$*>(response),\n"
      "        done);\n"
      "      break;\n");
  }

  printer->Print(vars_,
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
    "      break;\n"
    "  }\n"
    "}\n"
    "\n");
}

void CppServiceGenerator::GenerateGetPrototype(bool is_request,
                                               io::Printer* printer)
{
  if (is_request) {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetRequestPrototype(\n");
  } else {
    printer->Print(vars_,
      "const ::google::protobuf::Message& $classname$::GetResponsePrototype(\n");
  }

  printer->Print(vars_,
    "    const ::google::protobuf::MethodDescriptor* method) const {\n"
    "  GOOGLE_DCHECK_EQ(method->service(), descriptor());\n"
    "  switch(method->index()) {\n");

  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    const Descriptor* type =
      (is_request ? method->input_type() : method->output_type());

    std::map<std::string, std::string> sub_vars;
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["type"] = ClassName(type, true);

    printer->Print(sub_vars,
      "    case $index$:\n"
      "      return $type$::default_instance();\n");
  }

  printer->Print(
    "    default:\n"
    "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
    "      return *::google::protobuf::MessageFactory::generated_factory()\n"
    "          ->GetPrototype(method->$input_or_output$_type());\n"
    "  }\n"
    "}\n"
    "\n",
    "input_or_output", is_request ? "input" : "output");
}

void CppServiceGenerator::GenerateStubMethods(io::Printer* printer)
{
  for (int i = 0; i < descriptor_->method_count(); i++) {
    const MethodDescriptor* method = descriptor_->method(i);
    std::map<std::string, std::string> sub_vars;
    sub_vars["classname"] = descriptor_->name();
    sub_vars["name"] = method->name();
    sub_vars["index"] = SimpleItoa(i);
    sub_vars["input_type"] = ClassName(method->input_type(), true);
    sub_vars["output_type"] = ClassName(method->output_type(), true);

    printer->Print(sub_vars,
      "::hrpc::Result $classname$_Stub::$name$(\n"
      "    const $input_type$* request,\n"
      "    $output_type$* response,\n"
      "    ::ccb::ClosureFunc<void(::hrpc::Result)> done) {\n"
      "  return hrpc_->CallMethod(descriptor()->method($index$),\n"
      "                           request, response, std::move(done));\n"
      "}\n");
  }
}

} // namespace compiler
} // namespace hrpc
