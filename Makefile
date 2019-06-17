# opentracing and lightstep tracer use c++11, so we use this too
GLOBAL_FLAGS=-std=c++11 -Wall -pedantic

# When both a static and dynamic version of a library exist, we must explicitly
# pass the path to the static library if we wish to link statically with Clang.
# -l/usr/local/lib/lightstep_tracer.a
LINKER_FLAGS=-levent -lprotobuf # -llightstep_tracer -lopentracing
COMPILER_FLAGS=-I generated

PROTO_OBJECTS=collector.o annotations.o http.o

run_http_example: http_example
	./http_example

http_example: http_example.o $(PROTO_OBJECTS)
	gcc $(LINKER_FLAGS) $(GLOBAL_FLAGS) -o http_example http_example.o $(PROTO_OBJECTS)

proto_example: proto_example.o $(PROTO_OBJECTS)
	gcc $(LINKER_FLAGS) $(GLOBAL_FLAGS) -o proto_example proto_example.o $(PROTO_OBJECTS)

#

generate_proto: proto/collector.proto proto/google/api/http.proto proto/google/api/annotations.proto
	protoc --proto_path=proto --cpp_out=generated proto/google/api/annotations.proto proto/collector.proto proto/google/api/http.proto

# compile objects

proto_example.o: proto_example.cpp generate_proto
	gcc $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c proto_example.cpp

http_example.o: http_example.cpp generate_proto
	gcc $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c http_example.cpp

collector.o: generate_proto
	gcc $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/collector.pb.cc -o collector.o

annotations.o: generate_proto
	gcc $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/google/api/annotations.pb.cc -o annotations.o

http.o: generate_proto
	gcc $(COMPILER_FLAGS) $(GLOBAL_FLAGS) -c generated/google/api/http.pb.cc -o http.o
