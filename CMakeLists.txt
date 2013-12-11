cmake_minimum_required (VERSION 2.6)
project (KRAKENAPI)

set (LIBS "")

#-------------------------------------------------------------------------------
# Find OpenSSL
#-------------------------------------------------------------------------------
find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})
list(APPEND LIBS ${OPENSSL_LIBRARIES}) 

#-------------------------------------------------------------------------------
# Find CURL
#-------------------------------------------------------------------------------
find_package (CURL REQUIRED)
include_directories (${CURL_INCLUDE_DIR})
list(APPEND LIBS ${CURL_LIBRARIES})

#-------------------------------------------------------------------------------
# Add library libjson
#-------------------------------------------------------------------------------
set(LIBJSON_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/libjson/_internal/Source)
aux_source_directory(${LIBJSON_SOURCE_DIR} LIBJSON_SOURCE_FILES)
add_library(libjson STATIC ${LIBJSON_SOURCE_FILES})
# remove "lib" prefix from the name of the libjson archive
set_target_properties(libjson PROPERTIES PREFIX "")
list(APPEND LIBS libjson)

#-------------------------------------------------------------------------------
# Add the executable
#-------------------------------------------------------------------------------
add_executable (test main.cpp kapi.cpp)
target_link_libraries (test ${LIBS} libjson)

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")