cmake_minimum_required (VERSION 2.8.12)
project (KRAKENAPI)

add_definitions("-std=c++11")

#-------------------------------------------------------------------------------
IF (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	add_definitions("-Wno-tautological-compare")

ENDIF()

#-------------------------------------------------------------------------------
IF (APPLE)
	set (CMAKE_MODULE_LINKER_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
ENDIF (APPLE)

#-------------------------------------------------------------------------------
IF (POLICY CMP0043)
	cmake_policy(SET CMP0043 OLD)
ENDIF()

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
aux_source_directory(kraken KAPI_FILES)
add_library (kapi STATIC ${KAPI_FILES})

# set some properties when CMAKE_BUILD_TYPE is "Debug"
set_target_properties (kapi PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT"
		      DEBUG_POSTFIX "d")

list (INSERT LIBS 0 kapi)

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
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT"
		      DEBUG_POSTFIX "d")

list (APPEND LIBS libjson)

#-------------------------------------------------------------------------------
# Add the executable 'kph'
#-------------------------------------------------------------------------------
add_executable (kph kph.cpp)
set_target_properties (kph PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT")
target_link_libraries (kph ${LIBS})

#-------------------------------------------------------------------------------
# Add the executable 'krt'
#-------------------------------------------------------------------------------
add_executable (krt krt.cpp)
set_target_properties (krt PROPERTIES 
		      COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT")
target_link_libraries (krt ${LIBS})

#-------------------------------------------------------------------------------
# Add the executable 'private_method'
#-------------------------------------------------------------------------------
add_executable (private_method examples/private_method.cpp kapi.cpp)
set_target_properties (private_method PROPERTIES
		COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT")
target_link_libraries (private_method ${LIBS})

#-------------------------------------------------------------------------------
# Add the executable 'public_method'
#-------------------------------------------------------------------------------
add_executable (public_method examples/public_method.cpp kapi.cpp)
set_target_properties (public_method PROPERTIES
		COMPILE_DEFINITIONS_DEBUG "JSON_DEBUG;JSON_SAFE;JSON_ISO_STRICT")
target_link_libraries (public_method ${LIBS})