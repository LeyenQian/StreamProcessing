/*+===================================================================
  File:      redis_connector.cpp

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
#include "redis_connector.h"

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::RedisConnector

  Summary:  Initialize private class variables

  Args:     const string address
              IP address / domain of the redis server
            const INT port
              Port of the redis server
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
RedisConnector::RedisConnector(const string address, const INT port)
{
    this->p_redis_context = NULL;
    this->timeout = {1, 500000};
    this->address = address;
    this->port = port;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::~RedisConnector

  Summary:  Free allocated resource
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
RedisConnector::~RedisConnector()
{
    redisFree(p_redis_context);
    freeReplyObject(p_redis_reply);
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::Connect

  Summary:  Short summary of purpose and content of MyMethodOne.
            Short summary of purpose and content of MyMethodOne.

  Args:     MYTYPE MyArgOne
              Short description of argument MyArgOne.
            MYTYPE MyArgTwo
              Short description of argument MyArgTwo.

  Modifies: [list of member data variables modified by this method].

  Returns:  MYRETURNTYPE
              Short description of meaning of the return type values.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
INT RedisConnector::Connect()
{
    p_redis_context = redisConnectWithTimeout(address.c_str(), port, timeout);

    if ( p_redis_context == NULL || p_redis_context->err )
    {
        if ( p_redis_context )
        {
            printf("#RedisConnector: %s\n", p_redis_context->errstr);
        }
        else
        {
            printf("#RedisConnector: Cannot allocate redis context\n");
        }

        return OP_FAILED;
    }

    return OP_SUCCESS;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::ExecuteCommand

  Summary:  Short summary of purpose and content of MyMethodOne.
            Short summary of purpose and content of MyMethodOne.

  Args:     MYTYPE MyArgOne
              Short description of argument MyArgOne.
            MYTYPE MyArgTwo
              Short description of argument MyArgTwo.

  Modifies: [list of member data variables modified by this method].

  Returns:  MYRETURNTYPE
              Short description of meaning of the return type values.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
INT RedisConnector::ExecuteCommand(const string command)
{
    return OP_SUCCESS;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::TestRedis

  Summary:  Test basic redis command, adapted from hiredis/example/example.c
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::TestRedis()
{
    /* PING server */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "PING");
    printf("PING: %s\n", p_redis_reply->str);
    freeReplyObject(p_redis_reply);

    /* Set a key */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "SET %s %s", "foo", "hello world");
    printf("SET: %s\n", p_redis_reply->str);
    freeReplyObject(p_redis_reply);

    /* Set a key using binary safe API */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "SET %b %b", "bar", (size_t)3, "hello", (size_t)5);
    printf("SET (binary API): %s\n", p_redis_reply->str);
    freeReplyObject(p_redis_reply);

    /* Try a GET and two INCR */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "GET foo");
    printf("GET foo: %s\n", p_redis_reply->str);
    freeReplyObject(p_redis_reply);

    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "INCR counter");
    printf("INCR counter: %lld\n", p_redis_reply->integer);
    freeReplyObject(p_redis_reply);

    /* again ... */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "INCR counter");
    printf("INCR counter: %lld\n", p_redis_reply->integer);
    freeReplyObject(p_redis_reply);

    /* Create a list of numbers, from 0 to 9 */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "DEL mylist");
    freeReplyObject(p_redis_reply);
    for (int j = 0; j < 10; j++)
    {
        char buf[64] = {0};

        snprintf(buf, 64, "%u", j);
        p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "LPUSH mylist element-%s", buf);
        freeReplyObject(p_redis_reply);
    }

    /* Let's check what we have inside the list */
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, "LRANGE mylist 0 -1");
    if (p_redis_reply->type == REDIS_REPLY_ARRAY)
    {
        for (int j = 0; j < p_redis_reply->elements; j++)
        {
            printf("%u) %s\n", j, p_redis_reply->element[j]->str);
        }
    }
}


void RedisConnector_Test()
{
    RedisConnector redis_connector("192.168.1.140", 6379);
    if (redis_connector.Connect() == OP_FAILED) return;
    redis_connector.TestRedis();
}