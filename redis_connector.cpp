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


INT main(int argc, char **argv)
{
    RedisConnector redis_connector("192.168.1.140", 6379);
    if (redis_connector.Connect() == OP_FAILED)
    {
        printf("failed\n");
        return 0;
    }

    return 0;
}