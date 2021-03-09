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
    this->timeout = {3, 0};
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

  Summary:  build non-secure connection to the redis server

  Returns:  INT
              operation status
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

  Summary:  synchronously send command to the redis server for execution
            then receive operation results

  Args:     string command
              command for execution

  Modifies: [p_redis_reply]

  Returns:  INT
              operation status
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
INT RedisConnector::ExecuteCommand(const string command)
{
    p_redis_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, command.c_str());
    //printf("Reply: %s\n", p_redis_reply->str);
    freeReplyObject(p_redis_reply);

    return OP_SUCCESS;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::InsertGeospatial

  Summary:  insert geosptial info

  Args:     PPACKET_GEO_LOCATION p_location
              package include geo location infomation
            const string device_id

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::InsertGeospatial(PPACKET_GEO_LOCATION p_location, const string device_id)
{
    // GEOADD dev_loc CH -105.11 40.11 dev1
    stringstream commands;
    commands.precision(17);
    commands << "GEOADD dev_loc CH " << p_location->longitude << " " << p_location->latitude << " " << device_id;
    P_REDIS_REPLY p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands.str().c_str());
    if (p_reply != NULL) freeReplyObject(p_reply);
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::DetectObjectInRange

  Summary:  find object within range, query all devices

  Args:     const string distance
            string result

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::DetectObjectInRange(Value& device_ids, const string distance, string& result)
{
    stringstream result_stream;
    
    for (SizeType i = 0; i < device_ids.Size(); i++)
    {
        // GEOSEARCH dev_loc FROMMEMBER dev1 BYRADIUS 1200 ft ASC WITHDIST
        stringstream commands;
        commands << "GEOSEARCH dev_loc FROMMEMBER " << device_ids[i].GetString() << " BYRADIUS " << distance << " ASC WITHDIST";
        P_REDIS_REPLY p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands.str().c_str());

        result_stream << "*********************** Result for " << device_ids[i].GetString() << " ***********************" << endl;
        if (p_reply != NULL && p_reply->type == REDIS_REPLY_ARRAY)
        {
            for (int j = 0; j < p_reply->elements; j++)
            {
                if (p_reply->element[j]->elements == 0) continue;
                result_stream << "Device_ID: " << p_reply->element[j]->element[0]->str << endl;
                result_stream << "Distance to " << device_ids[i].GetString() << ": " << p_reply->element[j]->element[1]->str << endl << endl;
            }
        }

        if (p_reply != NULL) freeReplyObject(p_reply);
    }

    result.assign(result_stream.str());
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::CountObjectInRange

  Summary:  count objects passing a radius from a given position

  Args:     const string location
            const string distance
            string result

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::CountObjectInRange(const string location, const string distance, string& result)
{
    //GEOSEARCH dev_loc FROMLONLAT -105.00 40.00 BYRADIUS 120000 ft ASC WITHDIST
    stringstream commands;
    commands << "GEOSEARCH dev_loc FROMLONLAT " << location << " BYRADIUS " << distance << " ASC WITHDIST";
    P_REDIS_REPLY p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands.str().c_str());
    
    stringstream result_stream;
    result_stream << "*********************** Result for " << location << " ***********************" << endl;
    result_stream << "Total devices passing the radius " << distance << " from " << location << " is " << p_reply->elements << endl;
    if (p_reply != NULL && p_reply->type == REDIS_REPLY_ARRAY)
    {
        for (int j = 0; j < p_reply->elements; j++)
        {
            result_stream << "Device_ID: " << p_reply->element[j]->element[0]->str << endl;
            result_stream << "Distance to " << location << ": " << p_reply->element[j]->element[1]->str << endl << endl;
        }
    }

    if (p_reply != NULL) freeReplyObject(p_reply);
    result.assign(result_stream.str());
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::DetectGeofence

  Summary:  detect object is entering or leaving a geofence

  Args:     const string location
            const string distance
            const string device_id

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
BOOL RedisConnector::DetectGeofence(const string location, const string distance, const string device_id)
{
    //GEOSEARCH dev_loc FROMLONLAT -105.00 40.00 BYRADIUS 120000 ft ASC WITHDIST
    stringstream commands;
    commands << "GEOSEARCH dev_loc FROMLONLAT " << location << " BYRADIUS " << distance << " ASC";
    P_REDIS_REPLY p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands.str().c_str());

    if (p_reply != NULL && p_reply->type == REDIS_REPLY_ARRAY)
    {
        for (int j = 0; j < p_reply->elements; j++)
        {
            if (p_reply->element[j]->str == NULL) continue;
            if (strcmp(p_reply->element[j]->str, device_id.c_str()) == 0)
            {
                if (p_reply != NULL) freeReplyObject(p_reply);
                return TRUE;
            }
        }
    }

    if (p_reply != NULL) freeReplyObject(p_reply);
    return FALSE;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::TestRedis

  Summary:  Test basic redis command, adapted from hiredis/example/example.c

  Modifies: [p_redis_reply]

  Returns:  VOID
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
    freeReplyObject(p_redis_reply);
}


void RedisConnector_Test()
{
    RedisConnector redis_connector("192.168.1.140", 6379);
    if (redis_connector.Connect() == OP_FAILED) return;
    redis_connector.TestRedis();
}