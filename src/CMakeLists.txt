PROJECT(quartz)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(LIBNAME "quartzd")
else()
    set(LIBNAME "quartz")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")

# headers
file(GLOB_RECURSE HDRS_G "../include/*.h")

# sources
file(GLOB SRCS_G "*.cpp")

include_directories(..)
include_directories($ENV{CPP_WORKSPACE}/common/foundation-2.0.2/include)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "./")

add_library(${LIBNAME} SHARED "${SRCS_G}")
add_definitions(-fPIC)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_link_libraries(${LIBNAME} $ENV{CPP_WORKSPACE}/common/foundation-2.0.2/lib/libfoundationd.so)
else()
    target_link_libraries(${LIBNAME} $ENV{CPP_WORKSPACE}/common/foundation-2.0.2/lib/libfoundation.so)
endif()

add_definitions(-fPIC)

#install(TARGETS ${LIBNAME} LIBRARY DESTINATION lib)
#install(FILES  ${HDRS_G} DESTINATION include/crypto)
