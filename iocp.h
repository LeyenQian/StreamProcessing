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
    HANDLE h_iocp = NULL;
    HANDLE h_thread[11]  = {0};
    PPER_IO_INFO p_acce_io_info = NULL;
    PPER_LINK_INFO p_ser_link_info = NULL;
    LinkPool link_pool;

    // dynamically load functions
    LPFN_ACCEPTEX p_AcceptEx = NULL;
    LPFN_DISCONNECTEX p_DisconnectEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS p_GetAcceptExSockAddrs = NULL;

public:
    IOCP();
    ~IOCP();
    OPSTATUS InitialEnvironment();
    OPSTATUS CompletePortStart( string Address, INT Port );

    static UINT WINAPI DealThread( LPVOID ArgList );
    static UINT WINAPI AgingThread( LPVOID ArgList );

    OPSTATUS AcceptClient( PPER_LINK_INFO pSerLinkInfo, PPER_IO_INFO pAcceIoInfo );
    OPSTATUS IsRecvFinish( PPER_LINK_INFO pPerLinkInfo, ULONG ActualTrans );
    OPSTATUS PostRecv( PPER_LINK_INFO pPerLinkInfo, ULONG BuffOffest, ULONG BuffLen );
    
    OPSTATUS PostAcceptEx( PPER_IO_INFO p_acce_io_info );
};