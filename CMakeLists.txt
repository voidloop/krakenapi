cmake_minimum_required (VERSION 2.6)
project (KRAKENAPI)

set (LIBS "")

#
# Find OpenSSL
#
find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})
list(APPEND LIBS ${OPENSSL_LIBRARIES}) 

#
# Find CURL
#
find_package (CURL REQUIRED)
include_directories (${CURL_INCLUDE_DIR})
list(APPEND LIBS ${CURL_LIBRARIES})

#
# Add the executable
#
add_executable (test main.cpp kapi.cpp)
target_link_libraries (test ${LIBS})
