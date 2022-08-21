# id_generator

Very simple ID GENERATOR to generate client id for trading robots:

exchange_code.txt: contains the whole exchange which support to generate client id

## Make sure you download boost asio to your local machine, then change CMakeLists.txt accordingly.

```shell
./ticker 127.0.0.1 12345 exchange_code.txt 1 1
```

* 127.0.0.1: host
* 12345: port
* exchange_code.txt : the supported exchange codes
* 1 : server id
* 1 : data center id.

