/*+===================================================================
  File:      iocp.cpp

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
#include "iocp.h"

OPSTATUS IOCP::PostAcceptEx( PPER_IO_INFO p_acce_io_info )
{
    ULONG uBytesRet = 0;
    if ( (*(PPER_LINK_INFO *)p_acce_io_info->buffer = link_pool.LinkPoolAlloc()) == NULL )
    {
        return FALSE;
    }
    ZeroMemory( &p_acce_io_info->overlapped, sizeof(OVERLAPPED) );

    BOOL bRet = p_AcceptEx( p_ser_link_info->socket, ( *(PPER_LINK_INFO *)p_acce_io_info->buffer )->socket, &p_acce_io_info->buffer[sizeof(PPER_LINK_INFO)], 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &uBytesRet, &p_acce_io_info->overlapped );
    if ( !bRet && WSAGetLastError() != WSA_IO_PENDING )
    {
        printf( "#Err: post AcceptEx failed\n" );
        return OP_FAILED;
    }

    return OP_SUCCESS;
}


UINT WINAPI IOCP::AgingThread( LPVOID ArgList )
{
    printf("aging thread\n");
    IOCP* p_this = static_cast<IOCP *>(ArgList);

    while( TRUE )
    {
        Sleep( 1000 );
        p_this->link_pool.LinkPoolCheck(p_this->p_DisconnectEx);
    }
    return 0;
}


OPSTATUS IOCP::PostRecv( PPER_LINK_INFO pPerLinkInfo, ULONG BuffOffest, ULONG BuffLen )
{
    ULONG uFlag = 0;
    ULONG uRecv = 0;

    ZeroMemory( &pPerLinkInfo->p_per_io_info[0].overlapped, sizeof(OVERLAPPED) );
    pPerLinkInfo->p_per_io_info[0].w_buf.buf = &pPerLinkInfo->p_per_io_info[0].buffer[BuffOffest];
    pPerLinkInfo->p_per_io_info[0].w_buf.len = BuffLen;

    if( WSARecv( pPerLinkInfo->socket, &pPerLinkInfo->p_per_io_info[0].w_buf, 1, &uRecv, &uFlag, &pPerLinkInfo->p_per_io_info[0].overlapped, NULL ) == SOCKET_ERROR && 
        WSAGetLastError() != WSA_IO_PENDING )
    {
        printf( "#Err: post receive failed, discard connection\n" );
        return FALSE;
    }

    return TRUE;
}


OPSTATUS IOCP::IsRecvFinish( PPER_LINK_INFO pPerLinkInfo, ULONG ActualTrans )
{
    // package length cannot bigger than buffer 
    if ( pPerLinkInfo->p_per_io_info[0].w_buf.len != ActualTrans )
    {
        // not completely, post receive
        pPerLinkInfo->p_per_io_info[0].curr_data_len += ActualTrans;
        PostRecv( pPerLinkInfo, ActualTrans, pPerLinkInfo->p_per_io_info[0].w_buf.len - ActualTrans );
        pPerLinkInfo->p_per_io_info[0].post_recv_times ++;
        return FALSE;
    }
    else
    {
        // received completely, change reveived length
        pPerLinkInfo->p_per_io_info[0].curr_data_len += ActualTrans;
    }

    // package length bigger than buffer, discard the link directly
    if ( ((PPACKET_HEADER)( pPerLinkInfo->p_per_io_info[0].buffer ))->packet_len >= sizeof(PACKET_HEADER) && ((PPACKET_HEADER)( pPerLinkInfo->p_per_io_info[0].buffer ))->packet_len > pPerLinkInfo->p_per_io_info[0].curr_data_len)
    {
        // not completely, post receive
        PostRecv( pPerLinkInfo, pPerLinkInfo->p_per_io_info[0].curr_data_len, ((PPACKET_HEADER)( pPerLinkInfo->p_per_io_info[0].buffer ))->packet_len - pPerLinkInfo->p_per_io_info[0].curr_data_len );
        pPerLinkInfo->p_per_io_info[0].post_recv_times ++;
        return FALSE;
    }

    return TRUE;
}


OPSTATUS IOCP::AcceptClient( PPER_LINK_INFO pSerLinkInfo, PPER_IO_INFO pAcceIoInfo )
{
    PPER_LINK_INFO pPerLinkInfo = *(PPER_LINK_INFO *)pAcceIoInfo->buffer;

    if ( CreateIoCompletionPort( (HANDLE)pPerLinkInfo->socket, h_iocp, (ULONG)pPerLinkInfo, 0 ) == NULL )
    {
        printf( "#Err: accept client failed\n" );
        p_DisconnectEx( pPerLinkInfo->socket, NULL, TF_REUSE_SOCKET, 0 );
    }

    pPerLinkInfo->state_machine = SM_FULL;
    PostAcceptEx( pAcceIoInfo );
    PostRecv( pPerLinkInfo, 0, sizeof(PACKET_HEADER) );

    return TRUE;
}


UINT WINAPI IOCP::DealThread( LPVOID ArgList )
{
    cout << "deal thread" << endl;
    IOCP* p_this = static_cast<IOCP *>(ArgList);

    ULONG ActualTrans = 0;
    OVERLAPPED *pOverlapped = NULL;   
    PPER_IO_INFO pPerIOInfo = NULL;
    PPER_LINK_INFO pPerLinkInfo = NULL;

    while( TRUE )
    {
        GetQueuedCompletionStatus( p_this->h_iocp, &ActualTrans, (PULONG_PTR)&pPerLinkInfo, &pOverlapped, INFINITE );
        pPerIOInfo = (PPER_IO_INFO)CONTAINING_RECORD( pOverlapped, PER_IO_INFO, overlapped );

        if( pPerIOInfo->op_type == IO_ACCE )
        {
            printf("Received Client Connection Request\n");
            p_this->AcceptClient( pPerLinkInfo, pPerIOInfo );
        }
        else if( pPerIOInfo->op_type == IO_RECV )
        {
            // break, if the package received is not complete
            if ( !p_this->IsRecvFinish( pPerLinkInfo, ActualTrans ) ) continue;

            // receive completed. prepare for next package receiving
            pPerLinkInfo->p_per_io_info[0].w_buf.len = sizeof(PACKET_HEADER);
            pPerLinkInfo->p_per_io_info[0].w_buf.buf = pPerLinkInfo->p_per_io_info[0].buffer;
            pPerLinkInfo->p_per_io_info[0].curr_data_len = 0;

            // do different thing for different communication code
            switch( ((PPACKET_HEADER)pPerIOInfo->buffer)->comm_code )
            {
                case MSG_HEART_BEAT:
                {
                    if ( pPerLinkInfo->state_machine == SM_FULL || pPerLinkInfo->state_machine == SM_OVER )
                    {
                        pPerLinkInfo->state_machine = SM_FULL;
                        pPerLinkInfo->heartbeat_info.hold_time = 240;
                    }
                    p_this->PostRecv( pPerLinkInfo, 0, sizeof(PACKET_HEADER) );
                    break;
                }

                default:
                {
                    pPerLinkInfo->state_machine = SM_FULL;
                    pPerLinkInfo->heartbeat_info.hold_time = 240;
                    p_this->PostRecv( pPerLinkInfo, 0, sizeof(PACKET_HEADER));
                    break;
                }
            }
        }
    }
    return 0;
}


OPSTATUS IOCP::InitialEnvironment()
{
    // initial link pool
    link_pool.LinkPoolBuild();

    // allocate link info for server
    p_ser_link_info = (PPER_LINK_INFO)VirtualAlloc(NULL, sizeof(PER_LINK_INFO), MEM_COMMIT, PAGE_READWRITE);
    p_acce_io_info = (PPER_IO_INFO)VirtualAlloc(NULL, 10 * sizeof(PER_IO_INFO), MEM_COMMIT, PAGE_READWRITE);

    if ( p_ser_link_info == NULL || p_acce_io_info == NULL )
    {
        printf( "#Err: allocating server io info failed\n" );
        return OP_FAILED;
    }

    ZeroMemory( p_ser_link_info, sizeof(PER_LINK_INFO) );
    ZeroMemory( p_acce_io_info, 10 * sizeof(PER_IO_INFO) );

    p_ser_link_info->p_per_io_info = p_acce_io_info;

    // initial socket for server
    p_ser_link_info->socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
    if ( p_ser_link_info->socket == NULL )
    {
        printf( "#Err: creating server socket failed\n" );
        return OP_FAILED;
    }

    // load lib functions
    ULONG uBytesRet = 0;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    GUID GuidDisconnectEx = WSAID_DISCONNECTEX;

    WSAIoctl(p_ser_link_info->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &p_AcceptEx, sizeof(p_AcceptEx), &uBytesRet, NULL, NULL);
    WSAIoctl(p_ser_link_info->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnectEx, sizeof(GuidDisconnectEx), &p_DisconnectEx, sizeof(p_DisconnectEx), &uBytesRet, NULL, NULL);
    WSAIoctl(p_ser_link_info->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidGetAcceptExSockAddrs, sizeof(GuidGetAcceptExSockAddrs), &p_GetAcceptExSockAddrs, sizeof(p_GetAcceptExSockAddrs), &uBytesRet, NULL, NULL);

    if (p_AcceptEx == NULL || p_DisconnectEx == NULL || p_GetAcceptExSockAddrs == NULL)
    {
        printf("#Err: unable to load lib functions <AcceptEx, DisconnectEx, GetAcceptExSockAddrs>\n");
        return OP_FAILED;
    }

    return OP_SUCCESS;
}


OPSTATUS IOCP::CompletePortStart( string Address, INT Port )
{
    SOCKADDR_IN sock_addr = {0};
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons( Port );
    sock_addr.sin_addr.S_un.S_addr = inet_addr( Address.c_str() );

    // bind socket to address & port
    if ( bind( p_ser_link_info->socket, (PSOCKADDR)&sock_addr, sizeof(SOCKADDR_IN) ) != 0 )
    {
        printf( "#Err: binding server socket failed\n" );
        return OP_FAILED;
    }

    // create compltion port
    h_iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
    if ( h_iocp == NULL )
    {
        printf( "#Err: create IOCP failed\n" );
        return OP_FAILED;
    }

    // bind socket to compltion port
    if ( CreateIoCompletionPort( (HANDLE)p_ser_link_info->socket, h_iocp, (ULONG)p_ser_link_info, 0 ) == NULL )
    {
        printf( "#Err: binding IOCP failed\n" );
        return OP_FAILED;
    }

    // start listen
    if ( listen( p_ser_link_info->socket, SOMAXCONN ) == SOCKET_ERROR )
    {
        printf( "#Err: start listening socket failed\n" );
        return OP_FAILED;
    }

    // create 10 deal threads
    for (ULONG i = 0; i < 10; i ++)
    {
        if ( ( h_thread[i] = (HANDLE)_beginthreadex( NULL, 0, IOCP::DealThread, this, 0, NULL ) ) == NULL )
        {
            printf( "#Err: start IOCP thread failed\n" );
            return OP_FAILED;
        }
    }

    // create aging thread
    if ( ( h_thread[10] = (HANDLE)_beginthreadex( NULL, 0, IOCP::AgingThread, this, 0, NULL ) ) == NULL )
    {
        printf( "#Err: start IOCP aging thread failed\n" );
        return OP_FAILED;
    }

    for ( ULONG i = 0; i < 10; ++ i )
    {
        p_acce_io_info[i].op_type = IO_ACCE;
        p_acce_io_info[i].w_buf.len = MAX_BUF_LEN;
        p_acce_io_info[i].w_buf.buf = p_acce_io_info[i].buffer;

        if( PostAcceptEx( &p_acce_io_info[i] ) != OP_SUCCESS )
        {
            return OP_FAILED;
        }
    }

    while (TRUE)
    {
        Sleep(1000);
    }

    return OP_SUCCESS;
}


IOCP::IOCP()
{
    InitialEnvironment();
}


IOCP::~IOCP()
{
    link_pool.LinkPoolDestroy();
    VirtualFree(p_acce_io_info, sizeof(PER_LINK_INFO), MEM_RELEASE);
    VirtualFree(p_ser_link_info, sizeof(PER_IO_INFO), MEM_RELEASE);
}


int main(int argc, char const *argv[])
{
    IOCP server;
    server.CompletePortStart("127.0.0.1", 5001);
    return 0;
}
