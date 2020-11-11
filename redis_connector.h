
/*+===================================================================
  File:      redis_connector.h

  Summary:   Brief summary of the file contents and purpose.

  Classes:   Classes declared or used (in source files).

  Functions: Functions exported (in source files).

  Origin:    Indications of where content may have come from. This
             is not a change history but rather a reference to the
             editor-inheritance behind the content or other
             indications about the origin of the source.

  Copyright and Legal notices.
  Copyright and Legal notices.
===================================================================+*/
#ifndef __REDIS_CONNECTOR_H
#define __REDIS_CONNECTOR_H

#include <string>
#include <iostream>
#include <Windows.h>
#include "hiredis/hiredis.h"
#include <winsock2.h>
using namespace std;

// redfine types from "hiredis.h"
typedef redisContext    REDIS_CONTEXT;
typedef redisContext*   P_REDIS_CONTEXT;
typedef redisReply      REDIS_REPLY;
typedef redisReply*     P_REDIS_REPLY;

constexpr INT OP_SUCCESS = 0x0;
constexpr INT OP_FAILED  = 0x1;

/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Class:    RedisConnector

  Summary:  basic methods for redis interaction

  Methods:  Connect
              connect the redis server
            Disconnect
              disconnect the redis server
            ExecuteCommand
              send command to redis server
            RedisConnector
              Constructor.
            ~RedisConnector
              Destructor.
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
class RedisConnector
{
    private:
        HINSTANCE redis_lib;
        P_REDIS_CONTEXT p_redis_context;
        P_REDIS_REPLY   p_redis_reply;
        string address;
        INT port;
        timeval timeout;

    public:
        RedisConnector(const string address, const INT port);
        ~RedisConnector();
        INT Connect();
        INT ExecuteCommand(const string command);
        VOID TestRedis();
};

#endif