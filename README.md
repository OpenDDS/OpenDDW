# OpenDDW: An OpenDDS Data Distribution Wrapper

A wrapper library for management of DDS entity creation and notification callbacks

## Requirements

* OpenDDS (https://www.opendds.org) and its dependencies (ACE/TAO, and possibly openssl or xerces3)
* CMake
* A compiler and tool chain capable of C++17

## Tested Platforms

* Ubuntu 22.04 (g++ 11.2.0)
* Ubuntu 20.04 (g++ 9.4.0)
* macOS 12 (Apple clang 14.0.0)
* macOS 11 (Apple clang 13.0.0)
* Windows Server 2022 (VS2022)
* Windows Server 2019 (VS2019)

## Building

Assuming a valid development environment and OpenDDS environment variables are set:
```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```
See `.github/workflows/build.yml` for explicit list of steps for building on several supported platforms listed above.

## Configuration

The environment variable `DDS_CONFIG_FILE` should be set to the location of the OpenDDS configuration file, otherwise OpenDDW
will search for a file named `opendds.ini` in either the same directory as (or parent directory of) an application using
OpenDDW. See the OpenDDS Developers Guide for (at opendds.org) for more details on configuration options.
