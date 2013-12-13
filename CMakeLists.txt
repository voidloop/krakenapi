cmake_minimum_required (VERSION 2.6)
project (KRAKENAPI)

#-------------------------------------------------------------------------------
# Control CMAKE_BUILD_TYPE, default: Debug
#-------------------------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "Debug" CACHE string "Build type" FORCE)
endif (NOT CMAKE_BUILD_TYPE)
message (STATUS "Build type: " ${CMAKE_BUILD_TYPE})

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
# Add library kapi (krakenapi)
#-------------------------------------------------------------------------------
add_library (kapi STATIC kapi.cpp)

# set some properties when CMAKE_BUILD_TYPE is "Debug"
set_target_properties (kapi PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE"
		      DEBUG_POSTFIX "d")

list (APPEND LIBS kapi)

#-------------------------------------------------------------------------------
# Add library libjson
#-------------------------------------------------------------------------------
set (LIBJSON_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/libjson/_internal/Source)
aux_source_directory (${LIBJSON_SOURCE_DIR} LIBJSON_SOURCE_FILES)
add_library (libjson STATIC ${LIBJSON_SOURCE_FILES})

# remove "lib" prefix from the name of the libjson archive
set_target_properties (libjson PROPERTIES PREFIX "")

# set some properties when CMAKE_BUILD_TYPE is "Debug"
set_target_properties (libjson PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE"
		      DEBUG_POSTFIX "d")

list (APPEND LIBS libjson)

#-------------------------------------------------------------------------------
# Add the executable 'test'
#-------------------------------------------------------------------------------
add_executable (test main.cpp)
target_link_libraries (test ${LIBS})

#-------------------------------------------------------------------------------
# Add the executable 'kmarketdata'
#-------------------------------------------------------------------------------
add_executable (kmarketdata kmarketdata.cpp)
set_target_properties (kmarketdata PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE")
target_link_libraries (kmarketdata ${LIBS})

