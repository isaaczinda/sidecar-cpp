#include "collector.pb.h"
#include <iostream>
#include <sstream>
// #include <fstream>

int main ()
{
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // setup and serialize a test span
  lightstep::collector::Span span_out;
  lightstep::collector::Span span_in;
  std::stringstream buffer;

  // serialize and copy to buffer
  span_out.set_operation_name("isaac_test_span");

  if (!span_out.SerializeToOstream(&buffer)) {
    std::cerr << "Failed to serialize." << std::endl;
    return -1;
  } else {
    std::cout << "serialized." << std::endl;
  }


  if (!span_in.ParseFromIstream(&buffer)) {
    std::cerr << "failed to parse," << std::endl;
  } else {
    std::cout << "parsed." << std::endl;
  }

  std::cout << "operation name: " << span_in.operation_name() << std::endl;
}
