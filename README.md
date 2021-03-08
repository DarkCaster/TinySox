# TinySox SOCKS5 server

[![deepcode](https://www.deepcode.ai/api/gh/badge?key=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybTEiOiJnaCIsIm93bmVyMSI6IkRhcmtDYXN0ZXIiLCJyZXBvMSI6IlRpbnlTb3giLCJpbmNsdWRlTGludCI6ZmFsc2UsImF1dGhvcklkIjoxNTI4NCwiaWF0IjoxNjE1MTY1NjcyfQ.4xrt5zOikQ3PKoWSzxwgCNXOfOv7gWpVv_dYQMk12UQ)](https://www.deepcode.ai/app/gh/DarkCaster/TinySox/_/dashboard?utm_content=gh%2FDarkCaster%2FTinySox)

SOCKS5 server with minimal feature-set (RFC1928 and RFC1929) intended for use with small memory constrained devices running Linux/OpenWRT.

## Build requirements and usage

- Linux based OS with kernel support for epoll
- C++ compiler supporting C++17 standard (GCC v7 and up)
- Cmake (v3.1 and up)
- libudns library development files (v0.4 and up)

After building project by running `cmake && make`, tinysox binary will be saved to [project root]/Build directory (by default). Run tinysox binary without any parameters for usage info, currently it can only be configured via command line parameters (there is no config file support).

## Currently implemented features

- Listening and accepting client connections on single IP address/port
- Plain login/password and anonymous authentication modes
- CONNECT request support
- IPv4/IPv6 support
- Custom DNS server (and search domain) support

## Planned features (sometime in the distant future)

- Provide configuration via config file(s)
- Support for multiple listen addresses/ports, automatic handling of network changes
- Support for multiple users (currently only 1 user account supported)
- Support for allow/block lists of domain-names/ip-addresses for user accounts
- UDP ASSOCIATE request support
- TCP BIND request support
- Support for tunneling TCP out-of-band data

Originally this was my pet/home/educational project to learn modern C++ concepts. So, in order to keep relatively low complexity and ability to run on low-end linux devices - some complex features will be not available (GSSAPI auth support for example). Also, poor performance and scalability issues are expected at the moment, and project architecture may be somwhat overcomplicated.
