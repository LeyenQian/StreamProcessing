# BISON - Redis Stand-Alone Solution
## Description
This repo holds the Redis stand-alone solution to the provided use cases. 
### Redis Server
- For this project to function, a linux server with Redis instance listen on the port 6379 is necessary. 
- The Redis server (version 6.2.1) can be retrieved from https://redis.io/download .
### Redis Modules
- Geospatial: it is a build-in module.
- Time Series: it is not a build-in module, please install it manually. Visit https://github.com/RedisTimeSeries/RedisTimeSeries for more information.
### Third Party Modules (being integrated into this project)
- Hiredis ( C client library for Redis Server ): https://github.com/redis/hiredis
- Rapid JSON ( JSON parser and generator for C++ ): https://github.com/Tencent/rapidjson/
## Building
### Required Environment
- Windows 10 operating system
- Visual Studio 2019 (recommended) or Visual Studio Code
- CMake
### Instruction
- Select "Clone a repository" through Visual Studio 2019 (with link https://github.com/LeyenQian/StreamProcessing.git)
- Two startup items have been configured, you may selected either server.exe or client.exe for compiling.
- The first time compiling on server.exe will fail due to lost of library files. Copy hiredis/hiredisd.dll, hiredis/hiredisd.lib, config.json to out\build\x64-Debug for compiling and running.
- client.exe also need the client\config.json for running, please copy it to out\build\x64-Debug\client.
- server.exe receives data from client.exe, and send it the Redis Server. It also generate event_report.txt and geospatial_report.txt per second.
- client.exe generates random event and geospatial data and send it to server.exe.