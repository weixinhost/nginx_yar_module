project(ngx_yar)
set(HEADER_LIST yar.h  yar_common.h  yar_response.h yar_request.h yar_pack.h yar_protocol.h yar_log.h yar_method.h)
set(SRC_LIST ${HEADER_LIST}  yar_response.c yar_request.c yar_pack.c yar_protocol.c yar_log.c yar_method.c)

ADD_LIBRARY(ngx_yar  SHARED ${SRC_LIST})
target_link_libraries(ngx_yar msgpack)
install(TARGETS ngx_yar DESTINATION lib)
install(FILES ${HEADER_LIST} DESTINATION include/ngx_yar)