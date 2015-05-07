#include <ngx_yar/yar.h>

void yar_method_test(yar_request *request, yar_response *response, void *cookie){

    yar_packager *misko_success_pack = NULL;

    misko_success_pack = yar_pack_start_map(3);
    yar_pack_push_string(misko_success_pack,"err_code",8);
    yar_pack_push_long(misko_success_pack,0);

    yar_pack_push_string(misko_success_pack,"err_msg",7);
    yar_pack_push_string(misko_success_pack,"success",7);

    yar_pack_push_string(misko_success_pack,"data",4);
    yar_pack_push_map(misko_success_pack,1);

    yar_pack_push_string(misko_success_pack,"count",5);
    yar_pack_push_long(misko_success_pack,1);
    yar_response_set_retval(response,misko_success_pack);
    yar_pack_free(misko_success_pack);

}



