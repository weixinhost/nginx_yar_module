###Intro

Yar是一个高效且简单的RPC协议。目前已经有[PHP语言版本](https://github.com/laruence/yar) 以及 [C语言版本](https://github.com/laruence/yar)。	C语言使用直接tcp对外提供服务。灵活性上略逊于http服务。

###Install

	1. Install libcurl-devel

	2. Install Msgpack-0.5.9(https://github.com/msgpack/msgpack-c/releases/download/cpp-0.5.9/msgpack-0.5.9.tar.gz)


	3. Install libngx_yar

	   $ git clone https://github.com/weixinhost/nginx_yar_module.git
	   $ cd nginx_yar_module/ngx_yar
	   $ cmake .
	   $ make && make install

	4. ReInstall Nginx

	   $ cd nginx_source_dir/
	   $ ./configure --add-module=/path/to/ngx_yar_module ....
	   $ make && make install



###nginx server configure

	server {

			listen 80;
			server_name xxx;

			location / {

				yar_method_path		"xxxx.so";	    yar handler 库

				yar_timeout			100;			    yar handler超时
				yar_slow_timeout    500;                慢日志记录

				yar_bootstrap		"bootstrap";    每个request都首先调用该方法。

				yar_finalize		"finalize"	    每个request最后都调用该方法。

				yar_custom_config   "";
				yar_on;							    服务开启标识符

			}

			access_log		"xxx.log" access;
			error_log		"xxx.error.log" notice;
	}



#####yar_method_path

	yar的方法程序库。

	void yar_method_test(yar_request *request, yar_response *response, void *cookie)；

	上面的说明是一个典型的yar方法声明。 yar_method_为前缀，使用该前缀的方法才允许被访问。除去前缀的就是真实的方法名。


#####yar_timeout

		每个方法调用允许最大的执行时间(bootstrap + handler + finalize)

####yar_custom_config

		自定义文本。该配置最终将传入yar_bootstrap指定的符号中。

#####yar_bootstrap

		请求入口。适合做全局性的工作（全局资源初始化，全局参数检测）。
		不指定或符号不存在则跳过

		typedef void (*yar_bootstrap_method)(void *config,uint config_len);


####yar_finalize

	 handler调用后将触发该函数。
	 不指定或符号不存在则跳过
	 typedef void (*yar_finalize_method)(yar_request *request, yar_response *response);

####yar_on

	配置该项说明这个location正在对外提供yar服务。


####Devloper Guide

在源码包中的test目录为一个简单的种子程序。



#####注意事项

	1. 不主动对任何参数做内存写与释放的工作（比如 yar_request，yar_response）。	2. handler程序不应该执行过长时间（否则影响nginx吞吐量）。




