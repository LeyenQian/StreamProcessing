#include "pre_define.h"
#include "link_pool.h"
#include "redis_connector.h"

class IOCP
{
private:
    HANDLE h_thread[11]  = {0};
    PPER_IO_INFO p_acce_io_info = NULL;
    PPER_LINK_INFO p_ser_link_info = NULL;
    LinkPool link_pool;
    Document config;
    
    LPFN_ACCEPTEX p_AcceptEx = NULL;
    LPFN_DISCONNECTEX p_DisconnectEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS p_GetAcceptExSockAddrs = NULL;

public:
    HANDLE h_iocp = NULL;
    CRITICAL_SECTION SendCriticalSection = { 0 };
    RedisConnector* p_redis = NULL;

    IOCP();
    ~IOCP();

    static UINT WINAPI DealThread(LPVOID arg_list);
    static UINT WINAPI AgingThread(LPVOID arg_list);

    BOOL PacketSend(PPER_LINK_INFO p_per_link_info);
    BOOL LogonStatus(PPER_LINK_INFO p_per_link_info, ULONG status);

    OPSTATUS InitialEnvironment();
    OPSTATUS CompletePortStart( string address, INT port );
    OPSTATUS AcceptClient(PPER_IO_INFO p_per_io_Info);
    OPSTATUS IsRecvFinish(PPER_LINK_INFO p_per_link_info, ULONG actual_trans);
    OPSTATUS PostRecv(PPER_LINK_INFO p_per_link_info, ULONG buff_offset, ULONG buff_len);
    OPSTATUS PostAcceptEx( PPER_IO_INFO p_acce_io_info );
};