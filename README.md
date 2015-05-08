####Intro

    *Version 0.0.1-dev
    *Author Misko_Lee,Laruence

    yar-c是一个优秀的RPC框架，目前支持PHP与C语言版本。
    现在将其移植到Nginx中，作为一个Nginx模块运行。
    可以与Nginx的强大Web处理能力结合。


####Deps

        1.libcurl
        2. msgpack-c(0.5.9)
        3. nginx(1.6+)

####Install

        cd ngx_yar
        cmake .
        make install

        cd $NGX_DIR
        ./configure xxx --add-module=$prefix/nginx-yar-module
        make
        make install

####Todo
    1. yar_client的http支持
    2. JSON打包协议的支持
    3. 直接使用nginx的subrequest做并发调用





