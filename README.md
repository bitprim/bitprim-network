# Bitprim Network <a target="_blank" href="https://gitter.im/bitprim/Lobby">![Gitter Chat][badge.Gitter]</a>

*Bitcoin P2P Network Library*

| **master(linux/osx)** | **conan-build-win(linux/osx)**   | **master(windows)**   | **conan-build-win(windows)** |
|:------:|:-:|:-:|:-:|
| [![Build Status](https://travis-ci.org/bitprim/bitprim-network.svg)](https://travis-ci.org/bitprim/bitprim-network)       | [![Build StatusB](https://travis-ci.org/bitprim/bitprim-network.svg?branch=conan-build-win)](https://travis-ci.org/bitprim/bitprim-network?branch=conan-build-win)  | [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/bitprim/bitprim-network?svg=true)](https://ci.appveyor.com/project/bitprim/bitprim-network)  | [![Appveyor StatusB](https://ci.appveyor.com/api/projects/status/github/bitprim/bitprim-network?branch=conan-build-win&svg=true)](https://ci.appveyor.com/project/bitprim/bitprim-network?branch=conan-build-win)  |

Table of Contents
=================

   * [Bitprim Network](#bitprim-network)
      * [About Bitprim Network](#about-bitprim-network)
      * [Installation](#installation)
        * [Using Conan](#using-conan-recommended)
        * [Build from source](#build-from-source)
            * [Debian/Ubuntu](#debianubuntu)
            * [Windows with Visual Studio](#windows-with-visual-studio)

## About Bitprim Network

Bitprim Network is a partial implementation of the Bitcoin P2P network protocol. Excluded are all protocols that require access to a blockchain. The [bitprim-node](https://github.com/bitprim/bitprim-node) library extends the P2P networking capability and incorporates [bitprim-blockchain](https://github.com/bitprim/bitprim-blockchain) in order to implement a full node. The [bitprim-explorer](https://github.com/bitprim/bitprim-explorer) library uses the P2P networking capability to post transactions to the P2P network.

## Installation

### Using Conan (recommended)

Conan is a Python package for dependency management; it only requires Python and Pip.
With Conan, install can be performed on any OS. If there are no prebuilt binaries for a given
OS-compiler-arch combination, Conan will build from source.

```
pip install conan
conan remote add bitprim https://api.bintray.com/conan/bitprim/bitprim
conan install bitprim-network/0.1@bitprim/stable
```

The last step will install binaries and headers in Conan's cache, a directory outside the usual
system paths. This will avoid conflict with system packages such as boost.
Also, notice it references the stable version 0.1. To see which versions are available,
please check [Bintray](https://bintray.com/bitprim/bitprim/bitprim-network%3Abitprim).

### Build from source

#### Debian/Ubuntu

Make sure you have installed [bitprim-core](https://github.com/bitprim/bitprim-core) beforehand according to its build instructions. 

```
$ git clone https://github.com/bitprim/bitprim-network.git
$ cd bitprim-network
$ mkdir build
$ cd build
$ cmake .. -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-std=c++11" 
$ make -j2
$ sudo make install
```

bitprim-network is now installed in `/usr/local/`.

#### Windows with Visual Studio

This project, unlike secp256k1, has external dependencies such as boost.
The easiest way to build them is to use Conan from the CMake script,
which will install boost and other libraries in non-system directories.

From a [Visual Studio Developer Command Prompt](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs):

```
$ pip install conan
$ git clone https://github.com/bitprim/bitprim-network.git
$ cd bitprim-network
$ mkdir build
$ cd build
$ conan install ..
$ cmake .. -DUSE_CONAN=ON -DNO_CONAN_AT_ALL=OFF
$ msbuild ALL_BUILD.vcxproj
```

[badge.Gitter]: https://img.shields.io/badge/gitter-join%20chat-blue.svg
