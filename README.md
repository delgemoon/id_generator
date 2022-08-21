# id_generator

Very simple ID GENERATOR in C++ only depend on boost asio to generate client id for trading robots:

exchange_code.txt: the list of exchanges which are supported

## Make sure you download boost asio to your local machine, then change CMakeLists.txt accordingly.

```shell
./ticker 127.0.0.1 12345 exchange_code.txt 1 1
```

* 127.0.0.1: host
* 12345: port
* exchange_code.txt : the supported exchange codes
* 1 : server id
* 1 : data center id.


## API

**/api/ftx/sell**

* ftx: the supported exchange
* sell : side type( buy or sell)

** Return **

** 404 : if the params not correct
** 200 : Ok if success. the body is the string with 32 byte length.

