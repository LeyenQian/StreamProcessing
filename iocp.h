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

class IOCP
{
private:
    HANDLE h_iocp = NULL;
    PPER_IO_INFO p_acce_io_info = NULL;
    PPER_LINK_INFO p_ser_link_info = NULL;

    // dynamically load functions
    LPFN_ACCEPTEX AcceptEx = NULL;
    LPFN_DISCONNECTEX DisconnectEx = NULL;
    LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrs = NULL;

public:
    OPSTATUS CompletePortStart( PCHAR Address, INT Port );
    UINT WINAPI DealThread( LPVOID ArgList );
    BOOL AcceptClient( PPER_LINK_INFO pSerLinkInfo, PPER_IO_INFO pAcceIoInfo, PLINK_POOL_MANAGE pLinkPoolManage );
    BOOL IsRecvFinish( PPER_LINK_INFO pPerLinkInfo, ULONG ActualTrans );
    BOOL PostRecv( PPER_LINK_INFO pPerLinkInfo, ULONG BuffOffest, ULONG BuffLen );
    UINT WINAPI AgingThread( LPVOID ArgList );
    BOOL PostAcceptEx( PPER_LINK_INFO pSerLinkInfo, PPER_IO_INFO pAcceIoInfo, PLINK_POOL_MANAGE pLinkPoolManage );
};