#include "link_pool.h"
#include "list_manage.h"


LinkPool::LinkPool()
{
    p_link_pool_manage = (PLINK_POOL_MANAGE)VirtualAlloc(NULL, sizeof(LINK_POOL_MANAGE), MEM_COMMIT, PAGE_READWRITE);
}


LinkPool::~LinkPool()
{
    VirtualFree(p_link_pool_manage, sizeof(LINK_POOL_MANAGE), MEM_RELEASE);
}


OPSTATUS LinkPool::InitWinSock()
{
    WSAData wsa_data = {0};

    if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa_data ) != 0 )
    {
        printf( "#Err: load WinSock failed\n" );
        return OP_FAILED;
    }

    if ( LOBYTE( wsa_data.wVersion ) != 2 || HIBYTE( wsa_data.wVersion != 2 ) )
    {
        printf( "#Err: WinSock version not correct\n" );
        WSACleanup();
        return OP_FAILED;
    }

    return OP_SUCCESS;
}


OPSTATUS LinkPool::LinkPoolBuild()
{
    if ( InitWinSock() != OP_SUCCESS )
    {
        return OP_FAILED;
    }

    PPER_LINK_INFO p_per_link_info = (PPER_LINK_INFO)VirtualAlloc(NULL, MAX_LINK_NUM * sizeof(PER_LINK_INFO), MEM_COMMIT, PAGE_READWRITE);
    PPER_IO_INFO p_per_io_info = (PPER_IO_INFO)VirtualAlloc(NULL, 2 * MAX_LINK_NUM * sizeof(PER_IO_INFO), MEM_COMMIT, PAGE_READWRITE);

    if ( p_per_link_info == NULL || p_per_io_info == NULL )
    {
        printf( "#Err: allocate links pool failed\n" );
        return FALSE;
    }

    ZeroMemory( p_per_link_info, MAX_LINK_NUM * sizeof(PER_LINK_INFO) );
    ZeroMemory( p_per_io_info, 2 * MAX_LINK_NUM * sizeof(PER_IO_INFO) );

    InitListHead( &p_link_pool_manage->free_per_link_list_head );
    p_link_pool_manage->p_per_link_info = p_per_link_info;

    for ( ULONG i = 0; i < MAX_LINK_NUM; ++ i )
    {
        p_per_io_info[2*i].w_buf.buf = p_per_io_info[2*i].buffer;
        p_per_io_info[2*i].w_buf.len = sizeof(PACKET_HEADER);
        p_per_io_info[2*i].op_type = IO_RECV;

        p_per_io_info[2*i+1].w_buf.buf = p_per_io_info[2*i+1].buffer;
        p_per_io_info[2*i+1].w_buf.len = MAX_BUF_LEN;
        p_per_io_info[2*i+1].op_type = IO_SEND;

        p_per_link_info[i].free_flag = LINK_FREE;
        p_per_link_info[i].state_machine = SM_IDLE;
        p_per_link_info[i].p_per_io_info = &p_per_io_info[2*i];
        p_per_link_info[i].heartbeat_info.hold_time = 240;
        p_per_link_info[i].socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
        if ( p_per_link_info[i].socket == INVALID_SOCKET )
        {
            printf( "#Err: initial link pool failed\n" );
            return FALSE;
        }

        ListTailInsert( &p_link_pool_manage->free_per_link_list_head, &p_per_link_info[i].list_entry );
    }

    InitializeCriticalSection(&CriticalSection);
    return TRUE;
}


OPSTATUS LinkPool::LinkPoolDestroy()
{
    EnterCriticalSection(&CriticalSection);

    for ( ULONG i = 0; i < MAX_LINK_NUM; ++ i )
    {
        closesocket( p_link_pool_manage->p_per_link_info[i].socket );
    }

    VirtualFree(p_link_pool_manage->p_per_link_info->p_per_io_info, 0, MEM_RELEASE);
    VirtualFree(p_link_pool_manage->p_per_link_info, 0, MEM_RELEASE);

    DeleteCriticalSection(&CriticalSection);

    return TRUE;
}


PPER_LINK_INFO LinkPool::LinkPoolAlloc()
{
    PPER_LINK_INFO p_per_link_info = NULL;

    EnterCriticalSection(&CriticalSection);

    if ( IsListEmpty( &p_link_pool_manage->free_per_link_list_head ) )
    {
        for ( ULONG i = 0; i < MAX_LINK_NUM; ++ i )
        {
            if ( p_link_pool_manage->p_per_link_info[i].free_flag == LINK_FREE )
            {
                ListTailInsert( &p_link_pool_manage->free_per_link_list_head, &p_link_pool_manage->p_per_link_info[i].list_entry );
            }
        }
    }

    if ( !IsListEmpty( &p_link_pool_manage->free_per_link_list_head ) )
    {
        if ( ((PPER_LINK_INFO)( p_link_pool_manage->free_per_link_list_head.Flink ))->free_flag == LINK_FREE)
        {
            p_per_link_info = (PPER_LINK_INFO)( p_link_pool_manage->free_per_link_list_head.Flink );
            ListEntryDelete( &p_per_link_info->list_entry );
            p_per_link_info->free_flag = LINK_BUSY;
        }
    }
    else
    {
        printf( "#Err: no available link in pool\n" );
    }

    LeaveCriticalSection(&CriticalSection);

    return p_per_link_info;
}


VOID LinkPool::LinkPoolCheck(LPFN_DISCONNECTEX p_DisconnectEx)
{
    for ( ULONG j = 0; j < MAX_LINK_NUM; ++ j )
    {
        PPER_LINK_INFO p_per_link_info = &p_link_pool_manage->p_per_link_info[j];

        switch( p_per_link_info->state_machine )
        {
        case SM_IDLE:
            {
                p_per_link_info->heartbeat_info.hold_time = 240;
            }
            break;

        case SM_FULL:
            {
                p_per_link_info->heartbeat_info.hold_time == 60 ? p_per_link_info->state_machine = SM_OVER : p_per_link_info->heartbeat_info.hold_time -= 1;

                if ( p_per_link_info->heartbeat_info.hold_time % 60 == 0 && p_per_link_info->heartbeat_info.hold_time != 240 )
                {
                    p_per_link_info->heartbeat_info.lost_time ++;
                }
            }
            break;

        case SM_OVER:
            {
                p_per_link_info->heartbeat_info.hold_time == 0 ? p_per_link_info->state_machine = SM_FAIL : p_per_link_info->heartbeat_info.hold_time -= 1;
            }
            break;

        case SM_FAIL:
            {
                p_DisconnectEx( p_per_link_info->socket, NULL, TF_REUSE_SOCKET, 0 );
                p_per_link_info->free_flag = LINK_FREE;
                p_per_link_info->state_machine = SM_IDLE;
                p_per_link_info->heartbeat_info.hold_time = 240;
                p_per_link_info->heartbeat_info.lost_time = 0;
            }
            break;
        }
    }
}