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
  Method:   RedisConnector::InsertEvent

  Summary:  insert event info

  Args:     PPACKET_EVENT p_event
            const string device_id

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::InsertEvent(PPACKET_EVENT p_event, const string device_id)
{
    // TS.ADD event:A:AB1345ED79 * 0
    stringstream commands;
    commands << "TS.ADD event:" << p_event->type << ":" << device_id << " * " << p_event->data;
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
  Method:   RedisConnector::Temporal

  Summary:  Detect when event B occurs within x minutes of event A.
            Detect when event A occurs at least n times within x minutes.

  Args:     Value& tem
            Value& devs

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::Temporal(Value& tem, Value& devs, string& result)
{
    stringstream result_stream;

    for (SizeType i = 0; i < devs.Size(); i++)
    {
        stringstream dev_result;
        string dev = devs[i].GetString();
        int time = tem["time"].GetInt() * 10000;
        int frequency = tem["frequency"].GetInt();

        stringstream commands_a;
        stringstream commands_b;
        stringstream commands_c;
        long long beg = 0;
        long long end = 0;
        
        // Detect when event B occurs within x minutes of event A.
        commands_a << "TS.GET event:A:" << dev;
        P_REDIS_REPLY p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands_a.str().c_str());
        if (p_reply != NULL && p_reply->element != NULL && p_reply->type == REDIS_REPLY_ARRAY)
        {
            beg = p_reply->element[0]->integer - time;
            end = p_reply->element[0]->integer;
            commands_b << "TS.RANGE event:B:" << dev << " " << beg << " " << end;
            if (p_reply != NULL) freeReplyObject(p_reply);
        }

        if (commands_b.str().size() > 8)
        {
            p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands_b.str().c_str());
            if (p_reply != NULL && p_reply->type == REDIS_REPLY_ARRAY)
            {
                dev_result << "From device " << dev << " [ event B occurs within " << tem["time"].GetInt() << " minutes of event A ]" << endl;
                for (int j = 0; j < p_reply->elements; j++)
                {
                    dev_result << p_reply->element[j]->element[0]->integer << endl;
                }
                if (p_reply != NULL) freeReplyObject(p_reply);
            }
        }

        // Detect when event A occurs at least n times within x minutes
        commands_c << "TS.RANGE event:A:" << dev << " " << beg << " " << end;
        p_reply = (P_REDIS_REPLY)redisCommand(p_redis_context, commands_c.str().c_str());
        if (p_reply != NULL && p_reply->type == REDIS_REPLY_ARRAY && p_reply->elements >= frequency)
        {
            dev_result << endl << "From device " << dev << " [ event A occurs at least " << frequency << " times within " << tem["time"].GetInt() << " minutes ]" << endl;
            dev_result << p_reply->elements << " times";
            if (p_reply != NULL) freeReplyObject(p_reply);
        }
        result_stream << dev_result.str() << endl;
    }
    result = result_stream.str();
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::Sequence

  Summary:  Detect when event A occurs, followed by event B, followed by event C
            Detect when event A occurs, followed by event B or event C, followed by event D

  Args:     Value& seq
            Value& devs

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::Sequence(Value& seq, Value& devs, string& result)
{
    for (SizeType i = 0; i < devs.Size(); i++)
    {
    }
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   RedisConnector::Evaluation

  Summary:  Detect when the value of Event E is greater than y
            Detect when the average of the last n values for Event F is less than y

  Args:     Value& eva
            Value& devs

  Modifies: [p_redis_reply]

  Returns:  VOID
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
VOID RedisConnector::Evaluation(Value& eva, Value& devs, string& result)
{
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