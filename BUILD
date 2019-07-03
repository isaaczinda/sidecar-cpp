cc_library(
  name = "proto_test",
  srcs = ["proto_test.cpp"],
  deps = ["@com_lightstep_tracer_common//:collector"],
)

cc_binary(
    name = "http_example",
    srcs = ["http_example.cpp"],
    deps = [
      "@com_github_libevent_libevent//:libevent",
      "@com_lightstep_tracer_common//:collector",
      "@io_opentracing_cpp//:opentracing",
    ]
)

cc_binary(
    name = "boost_example",
    srcs = [
      "boost_example.cpp",
    ],
    deps = [
      "@boost//:boost_headers"
    ]
)
