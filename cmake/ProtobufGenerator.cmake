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

file(GLOB FBS_FILES ${CMAKE_SOURCE_DIR}/src/proto/*.fbs)
foreach(FBS_FILE ${FBS_FILES})
  get_filename_component(FBS_NAME ${FBS_FILE} NAME_WE)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/${FBS_NAME}_generated.h
    COMMAND flatbuffers::flatc --cpp --gen-object-api -o
            ${CMAKE_BINARY_DIR}/generated ${FBS_FILE}
    DEPENDS ${FBS_FILE}
    COMMENT "Generating flatbuffer headers for ${FBS_NAME}")
  list(APPEND GENERATED_FBS_HEADERS
       ${CMAKE_BINARY_DIR}/generated/${FBS_NAME}_generated.h)
endforeach()
target_sources(protos PRIVATE ${GENERATED_FBS_HEADERS})
target_link_libraries(protos PUBLIC flatbuffers::flatbuffers)
