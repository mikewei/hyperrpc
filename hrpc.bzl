# -*- mode: python; -*- PYTHON-PREPROCESSING-REQUIRED

# This file is based on protobuf's protobuf.bzl

def _GetPath(ctx, path):
  if ctx.label.workspace_root:
    return ctx.label.workspace_root + '/' + path
  else:
    return path

def _GenDir(ctx):
  if not ctx.attr.includes:
    return ctx.label.workspace_root
  if not ctx.attr.includes[0]:
    return _GetPath(ctx, ctx.label.package)
  if not ctx.label.package:
    return _GetPath(ctx, ctx.attr.includes[0])
  return _GetPath(ctx, ctx.label.package + '/' + ctx.attr.includes[0])

def _CcOuts(srcs, use_hrpc_plugin=False):
  ret = [s[:-len(".proto")] + ".pb.h" for s in srcs] + \
        [s[:-len(".proto")] + ".pb.cc" for s in srcs]
  if use_hrpc_plugin:
    ret += [s[:-len(".proto")] + ".hrpc.pb.h" for s in srcs] + \
           [s[:-len(".proto")] + ".hrpc.pb.cc" for s in srcs]
  return ret

def _PyOuts(srcs):
  return [s[:-len(".proto")] + "_pb2.py" for s in srcs]

def _RelativeOutputPath(path, include, dest=""):
  if include == None:
    return path

  if not path.startswith(include):
    fail("Include path %s isn't part of the path %s." % (include, path))

  if include and include[-1] != '/':
    include = include + '/'
  if dest and dest[-1] != '/':
    dest = dest + '/'

  path = path[len(include):]
  return dest + path

def _proto_gen_impl(ctx):
  """General implementation for generating protos"""
  srcs = ctx.files.srcs
  deps = []
  deps += ctx.files.srcs
  gen_dir = _GenDir(ctx)
  if gen_dir:
    import_flags = ["-I" + gen_dir, "-I" + ctx.var["GENDIR"] + "/" + gen_dir]
  else:
    import_flags = ["-I."]

  for dep in ctx.attr.deps:
    import_flags += dep.proto.import_flags
    deps += dep.proto.deps

  args = []
  if ctx.attr.gen_cc:
    args += ["--cpp_out=" + ctx.var["GENDIR"] + "/" + gen_dir]
  if ctx.attr.gen_py:
    args += ["--python_out=" + ctx.var["GENDIR"] + "/" + gen_dir]

  if ctx.executable.plugin:
    plugin = ctx.executable.plugin
    lang = ctx.attr.plugin_language
    if not lang and plugin.basename.startswith('protoc-gen-'):
      lang = plugin.basename[len('protoc-gen-'):]
    if not lang:
      fail("cannot infer the target language of plugin", "plugin_language")

    outdir = ctx.var["GENDIR"] + "/" + gen_dir
    if ctx.attr.plugin_options:
      outdir = ",".join(ctx.attr.plugin_options) + ":" + outdir
    args += ["--plugin=protoc-gen-%s=%s" % (lang, plugin.path)]
    args += ["--%s_out=%s" % (lang, outdir)]

  if args:
    ctx.action(
        inputs=srcs + deps,
        outputs=ctx.outputs.outs,
        arguments=args + import_flags + [s.path for s in srcs],
        executable=ctx.executable.protoc,
        mnemonic="ProtoCompile",
    )

  return struct(
      proto=struct(
          srcs=srcs,
          import_flags=import_flags,
          deps=deps,
      ),
  )

proto_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = ["proto"]),
        "includes": attr.string_list(),
        "protoc": attr.label(
            cfg = "host",
            executable = True,
            single_file = True,
            mandatory = True,
        ),
        "plugin": attr.label(
            cfg = "host",
            allow_files = True,
            executable = True,
        ),
        "plugin_language": attr.string(),
        "plugin_options": attr.string_list(),
        "gen_cc": attr.bool(),
        "gen_py": attr.bool(),
        "outs": attr.output_list(),
    },
    output_to_genfiles = True,
    implementation = _proto_gen_impl,
)
"""Generates codes from Protocol Buffers definitions.

This rule helps you to implement Skylark macros specific to the target
language. You should prefer more specific `cc_proto_library `,
`py_proto_library` and others unless you are adding such wrapper macros.

Args:
  srcs: Protocol Buffers definition files (.proto) to run the protocol compiler
    against.
  deps: a list of dependency labels; must be other proto libraries.
  includes: a list of include paths to .proto files.
  protoc: the label of the protocol compiler to generate the sources.
  plugin: the label of the protocol compiler plugin to be passed to the protocol
    compiler.
  plugin_language: the language of the generated sources
  plugin_options: a list of options to be passed to the plugin
  gen_cc: generates C++ sources in addition to the ones from the plugin. 
  gen_py: generates Python sources in addition to the ones from the plugin.
  outs: a list of labels of the expected outputs from the protocol compiler.
"""

def cc_hrpc_library(
        name,
        srcs=[],
        deps=[],
        cc_libs=[],
        include=None,
        protoc="//third_party/protobuf:protoc",
        use_hrpc_plugin=True,
        default_runtime="//third_party/protobuf:protobuf",
        **kargs):
  """Bazel rule to create a C++ protobuf/hrpc library from proto source files

  Args:
    name: the name of the cc_proto_library.
    srcs: the .proto files of the cc_proto_library.
    deps: a list of dependency labels; must be cc_proto_library.
    cc_libs: a list of other cc_library targets depended by the generated
        cc_library.
    include: a string indicating the include path of the .proto files.
    protoc: the label of the protocol compiler to generate the sources.
    use_hrpc_plugin: a flag to indicate whether to call the grpc C++ plugin
        when processing the proto files.
    default_runtime: the implicitly default runtime which will be depended on by
        the generated cc_library target.
    **kargs: other keyword arguments that are passed to cc_library.

  """

  includes = []
  if include != None:
    includes = [include]

  if use_hrpc_plugin:
    hrpc_cpp_plugin = "//hyperrpc:protoc-gen-hrpc_cpp"

  outs = _CcOuts(srcs, use_hrpc_plugin)

  proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      includes=includes,
      protoc=protoc,
      plugin=hrpc_cpp_plugin,
      plugin_language="hrpc_cpp",
      gen_cc=1,
      outs=outs,
      visibility=["//visibility:public"],
  )

  if default_runtime and not default_runtime in cc_libs:
    cc_libs += [default_runtime]
  if use_hrpc_plugin:
    cc_libs += ["//hyperrpc:hyperrpc"]

  native.cc_library(
      name=name,
      srcs=outs,
      deps=cc_libs + deps,
      includes=includes,
      **kargs)


