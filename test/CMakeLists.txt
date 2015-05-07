project(ngx_test_yar)
ADD_LIBRARY(ngx_test_yar  SHARED ./yar_test_method.c)
target_link_libraries(ngx_test_yar ngx_yar msgpack )
install(TARGETS ngx_test_yar DESTINATION lib)
