/*+===================================================================
  File:      iocp.h

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

    // dynamically load functions
    LPFN_ACCEPTEX p_AcceptEx = NULL;
    LPFN_DISCONNECTEX p_DisconnectEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS p_GetAcceptExSockAddrs = NULL;

public:
    HANDLE h_iocp = NULL;
    CRITICAL_SECTION SendCriticalSection = { 0 };

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