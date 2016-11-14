[![Build Status](https://travis-ci.org/bitprim/bitprim-network.svg?branch=master)](https://travis-ci.org/bitprim/bitprim-network)

# Bitprim Network

*Bitcoin P2P Network Library*

Make sure you have installed [bitprim-core](https://github.com/bitprim/bitprim-core) beforehand according to its build instructions. 

```sh
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

bitprim-network is now installed in `/usr/local/`.

**About Bitprim Network**

Bitprim Network is a partial implementation of the Bitcoin P2P network protocol. Excluded are all protocols that require access to a blockchain. The [bitprim-node](https://github.com/bitprim/bitprim-node) library extends the P2P networking capability and incorporates [bitprim-blockchain](https://github.com/bitprim/bitprim-blockchain) in order to implement a full node. The [bitprim-explorer](https://github.com/bitprim/bitprim-explorer) library uses the P2P networking capability to post transactions to the P2P network.
