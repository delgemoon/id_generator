# id_generator

A very simple ID GENERATOR in C++ only have one dependency( boost asio) to generate client id for any trading robots:

Most of trading exchanges allow at minimum 32 bytes length. some might support 64 bytes. But We will cover all of them.

exchange_code.txt: the exchanges which are supported

### Dependencies:
1. C++20
2. boost-asio

## Make sure you download boost asio to your local machine, then change CMakeLists.txt accordingly.

```shell
./ticker 127.0.0.1 12345 exchange_code.txt 1 1
```

* 127.0.0.1: host
* 12345: port
* exchange_code.txt : the supported exchange codes
* 1 : server id
* 1 : data center id.


**The client id contains 32 byte length** :

1. The first byte: The trading side: 
* 0: Buy
* 1: Sell
2. the next 13 bytes: The timestamp in miliseconds.
3. The next 5 bytes: The exchange id according to the exchange code file
4. The next 3 bytes: the server id
5. The next 2 bytes: The data center id
6. The last 6 bytes: The incremental in a day.


## API

**POST /api/ftx/sell**

* ftx: the supported exchange
* sell : side type( buy or sell)

*Response*

1. *404 : if the params not correct*
2. *200 : Ok if success. the body is the string with 32 byte length*

