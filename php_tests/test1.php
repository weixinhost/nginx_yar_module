<?php
error_reporting(E_ALL);

$config = include "config.php";

var_dump($config);

try{
$client = new Yar_Client($config['host']);
$client->setOpt(YAR_OPT_PACKAGER,"JSON");
var_dump($client->test());
}catch(Exception $e){

    echo $e->getMessage();
}

try{
$client = new Yar_Client($config['host']);
$client->setOpt(YAR_OPT_PACKAGER,"MSGPACK");

var_dump($client->test2());
}catch(Exception $e){

     echo $e->getMessage();
 }


try{
$client = new Yar_Client($config['host']);
$client->setOpt(YAR_OPT_PACKAGER,"MSGPACK");

var_dump($client->test3());
}catch(Exception $e){

     echo $e->getMessage();
 }
