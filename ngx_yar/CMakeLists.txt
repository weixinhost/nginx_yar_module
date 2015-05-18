 cmake_minimum_required(VERSION 2.8)
project(ngx_yar)
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(msgpack/msgpack.h HAVE_MSGPACK)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


IF(NOT HAVA_MSGPACK)
    MESSAGE("library `msgpack` not found.")
ENDIF(NOT HAVA_MSGPACK)

CHECK_INCLUDE_FILES(curl/curl.h HAVE_CURL)

IF(NOT HAVE_CURL)
    MESSAGE("library `curl` not found.")
ENDIF(NOT HAVE_CURL)

set(HEADER_LIST yar.h  yar_common.h  yar_response.h yar_request.h yar_pack.h yar_protocol.h yar_log.h yar_method.h yar_client.h)
set(SRC_LIST ${HEADER_LIST}  yar_response.c yar_request.c yar_pack.c yar_protocol.c yar_log.c yar_method.c yar_client.c)

ADD_LIBRARY(ngx_yar  SHARED ${SRC_LIST})
target_link_libraries(ngx_yar msgpack)
install(TARGETS ngx_yar DESTINATION lib)
install(FILES ${HEADER_LIST} DESTINATION include/ngx_yar)