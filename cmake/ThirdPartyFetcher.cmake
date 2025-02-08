if((ENABLE_UNDEFINED_BEHAVIOR_SANITIZER OR ENABLE_ADDRESS_SANITIZER)
   AND ENABLE_THREAD_SANITIZER)
  message(
    FATAL_ERROR
      "ENABLE_UNDEFINED_BEHAVIOR_SANITIZER/ENABLE_ADDRESS_SANITIZER and ENABLE_THREAD_SANITIZER cannot both be ON."
  )
endif()

include(FetchContent)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER OR ENABLE_ADDRESS_SANITIZER)
    FetchContent_Declare(
      RocketFSThirdParty
      URL https://rocketfs.oss-rg-china-mainland.aliyuncs.com/third-party-artifact-b73737c-clang-asan-ON-tsan-OFF.tar.gz
    )
  elseif(ENABLE_THREAD_SANITIZER)
    FetchContent_Declare(
      RocketFSThirdParty
      URL https://rocketfs.oss-rg-china-mainland.aliyuncs.com/third-party-artifact-b73737c-clang-asan-OFF-tsan-ON.tar.gz
    )
  else()
    FetchContent_Declare(
      RocketFSThirdParty
      URL https://rocketfs.oss-rg-china-mainland.aliyuncs.com/third-party-artifact-b73737c-clang-asan-OFF-tsan-OFF.tar.gz
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  FetchContent_Declare(
    RocketFSThirdParty
    URL https://rocketfs.oss-rg-china-mainland.aliyuncs.com/third-party-artifact-b73737c-gcc-asan-OFF-tsan-OFF.tar.gz
  )
else()
  message(FATAL_ERROR "Unsupported compiler. Only Clang and GCC are supported.")
endif()

FetchContent_MakeAvailable(RocketFSThirdParty)
list(APPEND CMAKE_MODULE_PATH ${rocketfsthirdparty_SOURCE_DIR})
include(Targets)
