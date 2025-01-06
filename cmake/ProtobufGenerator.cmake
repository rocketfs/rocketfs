find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

file(GLOB PROTO_FILES ${CMAKE_SOURCE_DIR}/src/proto/*.proto)
add_library(protos ${PROTO_FILES})
target_link_libraries(protos PUBLIC gRPC::grpc++)
protobuf_generate(TARGET protos)
protobuf_generate(
  TARGET
  protos
  LANGUAGE
  grpc
  PLUGIN
  protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
  PLUGIN_OPTIONS
  generate_mock_code=true
  GENERATE_EXTENSIONS
  .grpc.pb.h
  .grpc.pb.cc)
target_include_directories(protos PUBLIC ${CMAKE_BINARY_DIR})
