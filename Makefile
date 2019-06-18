# TODO: this is potentially the most jank thing ever
PROTO_HEADER_DIR=/Users/isaaczinda/lightstep-tracer-cpp/.build/generated/lightstep-tracer-common

GLOBAL_FLAGS=-std=c++11 -Wall -pedantic
LINKER_FLAGS=-levent -lopentracing -l protobuf -l lightstep_tracer
COMPILER_FLAGS= -I $(PROTO_HEADER_DIR)

# PROTO_OBJECTS=collector.o annotations.o http.o

run_http_example: http_example
	./http_example

http_example: http_example.o span_converter.o
	clang++ $(LINKER_FLAGS) $(GLOBAL_FLAGS) -o http_example http_example.o span_converter.o

# proto_example: proto_example.o $(PROTO_OBJECTS)
# 	clang++ $(LINKER_FLAGS) $(GLOBAL_FLAGS) -o proto_example proto_example.o $(PROTO_OBJECTS)



# generate_proto: proto/collector.proto proto/google/api/http.proto proto/google/api/annotations.proto
# 	protoc --proto_path=proto --cpp_out=generated proto/google/api/annotations.proto proto/collector.proto proto/google/api/http.proto
#
# # compile objects
#
#

http_example.o: http_example.cpp span_converter.h
	clang++ $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c http_example.cpp

span_converter.o: span_converter.cpp span_converter.h
	clang++ $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c span_converter.cpp

#
# collector.o: generate_proto
# 	clang++ $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/collector.pb.cc -o collector.o
#
# annotations.o: generate_proto
# 	clang++ $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/google/api/annotations.pb.cc -o annotations.o
#
# http.o: generate_proto
# 	clang++ $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/google/api/http.pb.cc -o http.o
