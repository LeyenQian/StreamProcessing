#include "iocp_client.h"

OPSTATUS IOCP_Client::InitWinSock()
{
    WSAData wsa_data = { 0 };

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        printf("#Err: load WinSock failed\n");
        return OP_FAILED;
    }

    if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion != 2))
    {
        printf("#Err: WinSock version not correct\n");
        WSACleanup();
        return OP_FAILED;
    }

    return OP_SUCCESS;
}


OPSTATUS IOCP_Client::PostRecv(PPER_LINK_INFO p_per_link_info, ULONG buff_offset, ULONG buff_len)
{
    ULONG u_flag = 0;
    ULONG u_recv = 0;

    ZeroMemory(&p_per_link_info->p_per_io_info[0].overlapped, sizeof(OVERLAPPED));
    p_per_link_info->p_per_io_info[0].w_buf.buf = &p_per_link_info->p_per_io_info[0].buffer[buff_offset];
    p_per_link_info->p_per_io_info[0].w_buf.len = buff_len;

    if (WSARecv(p_per_link_info->socket, &p_per_link_info->p_per_io_info[0].w_buf, 1, &u_recv, &u_flag, &p_per_link_info->p_per_io_info[0].overlapped, NULL) == SOCKET_ERROR &&
        WSAGetLastError() != WSA_IO_PENDING)
    {
        printf("#Err: post receive failed, discard connection\n");
        return FALSE;
    }

    return TRUE;
}


OPSTATUS IOCP_Client::IsRecvFinish(PPER_LINK_INFO p_per_link_info, ULONG actual_trans)
{
    // package length cannot bigger than buffer 
    if (p_per_link_info->p_per_io_info[0].w_buf.len != actual_trans)
    {
        // not completely, post receive
        p_per_link_info->p_per_io_info[0].curr_data_len += actual_trans;
        PostRecv(p_per_link_info, actual_trans, p_per_link_info->p_per_io_info[0].w_buf.len - actual_trans);
        p_per_link_info->p_per_io_info[0].post_recv_times++;
        return FALSE;
    }
    else
    {
        // received completely, change reveived length
        p_per_link_info->p_per_io_info[0].curr_data_len += actual_trans;
    }

    // package length bigger than buffer, discard the link directly
    if (((PPACKET_HEADER)(p_per_link_info->p_per_io_info[0].buffer))->packet_len >= sizeof(PACKET_HEADER) && ((PPACKET_HEADER)(p_per_link_info->p_per_io_info[0].buffer))->packet_len > p_per_link_info->p_per_io_info[0].curr_data_len)
    {
        // not completely, post receive
        PostRecv(p_per_link_info, p_per_link_info->p_per_io_info[0].curr_data_len, ((PPACKET_HEADER)(p_per_link_info->p_per_io_info[0].buffer))->packet_len - p_per_link_info->p_per_io_info[0].curr_data_len);
        p_per_link_info->p_per_io_info[0].post_recv_times++;
        return FALSE;
    }

    return TRUE;
}


BOOL IOCP_Client::PacketSend(PPER_LINK_INFO p_per_link_info)
{
    ULONG u_send = 0;
    ULONG u_flag = 0;

    ZeroMemory(&p_per_link_info->p_per_io_info[1].overlapped, sizeof(OVERLAPPED));

    EnterCriticalSection(&SendCriticalSection);
    ULONG uRet = WSASend(p_per_link_info->socket, &p_per_link_info->p_per_io_info[1].w_buf, 1, &u_send, u_flag, &p_per_link_info->p_per_io_info[1].overlapped, NULL);
    if (uRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
    {
        printf("#Err: send data package asynchronously failed\n");
        LeaveCriticalSection(&SendCriticalSection);

        return FALSE;
    }
    LeaveCriticalSection(&SendCriticalSection);

    return TRUE;
}


BOOL IOCP_Client::HeartBeat(PPER_LINK_INFO p_per_link_info)
{
    ZeroMemory(p_per_link_info->p_per_io_info[1].buffer, MAX_BUF_LEN);

    ((PPACKET_HEADER)p_per_link_info->p_per_io_info[1].buffer)->comm_code = MSG_HEART_BEAT;
    ((PPACKET_HEADER)p_per_link_info->p_per_io_info[1].buffer)->packet_len = sizeof(PACKET_HEADER);

    p_per_link_info->p_per_io_info[1].w_buf.len = sizeof(PACKET_HEADER);

    return PacketSend(p_per_link_info);
}


BOOL IOCP_Client::Logon(PPER_LINK_INFO p_per_link_info, PCHAR account)
{
    p_per_link_info = p_ser_link_info;
    ZeroMemory(p_per_link_info->p_per_io_info[1].buffer, MAX_BUF_LEN);

    ((PPACKET_LOGON)p_per_link_info->p_per_io_info[1].buffer)->packet_header.comm_code = MSG_LOGON;
    ((PPACKET_LOGON)p_per_link_info->p_per_io_info[1].buffer)->packet_header.packet_len = sizeof(PACKET_LOGON);

    strcpy_s(((PPACKET_LOGON)p_per_link_info->p_per_io_info[1].buffer)->account, account);

    p_per_link_info->p_per_io_info[1].w_buf.len = sizeof(PACKET_LOGON);

    return PacketSend(p_per_link_info);
}


UINT WINAPI IOCP_Client::HeartBeatThread(LPVOID arg_list)
{
    IOCP_Client* p_this = static_cast<IOCP_Client*>(arg_list);

    ULONG times = 0;
    ULONG send_fail_times = 0;

    while (TRUE)
    {
        if (p_this->app_quit_flag) break;

        if (times % 10 == 0)
        {
            if (!p_this->HeartBeat(p_this->p_ser_link_info)) send_fail_times++;
        }

        Sleep(1000);
        times++;

        if (send_fail_times >= 5)
        {
            if (p_this->h_thread[0] != NULL)
            {
                PER_LINK_INFO ExitLinkInfo;
                ZeroMemory(&ExitLinkInfo, sizeof(PER_LINK_INFO));
                PostQueuedCompletionStatus(p_this->h_iocp, 0, (ULONG_PTR)&ExitLinkInfo, NULL);
            }

            break;
        }
    }

    printf("#Log: aging thread ended\n");
    p_this->app_quit_flag = FALSE;
    p_this->h_thread[1] = NULL;
    return 0;
}


UINT WINAPI IOCP_Client::DealThread(LPVOID arg_list)
{
    IOCP_Client* p_this = static_cast<IOCP_Client*>(arg_list);

    ULONG actual_trans = 0;
    OVERLAPPED* p_overlapped = NULL;
    PPER_IO_INFO p_per_io_Info = NULL;
    PPER_LINK_INFO p_per_link_info = NULL;

    while (TRUE)
    {
        GetQueuedCompletionStatus(p_this->h_iocp, &actual_trans, (PULONG_PTR)&p_per_link_info, &p_overlapped, INFINITE);
        
        if (p_per_link_info->socket == 0)
        {
            printf("#Log: Deal thread ended\n");
            p_this->h_thread[1] = NULL;
            return 0;
        }

        p_per_io_Info = (PPER_IO_INFO)CONTAINING_RECORD(p_overlapped, PER_IO_INFO, overlapped);

        if (p_per_io_Info->op_type == IO_RECV)
        {
            // break, if the package received is not complete
            if (!p_this->IsRecvFinish(p_per_link_info, actual_trans)) continue;

            // receive completed. prepare for next package receiving
            p_per_link_info->p_per_io_info[0].w_buf.len = sizeof(PACKET_HEADER);
            p_per_link_info->p_per_io_info[0].w_buf.buf = p_per_link_info->p_per_io_info[0].buffer;
            p_per_link_info->p_per_io_info[0].curr_data_len = 0;

            // do different thing for different communication code
            switch (((PPACKET_HEADER)p_per_io_Info->buffer)->comm_code)
            {
            case MSG_LOGON_SUCCESS:
            {
                printf("#Log: Logon succeed!\n");
                p_this->logon_status = TRUE;

                if ((p_this->h_thread[1] = (HANDLE)_beginthreadex(NULL, 0, IOCP_Client::HeartBeatThread, NULL, 0, NULL)) == NULL)
                {
                    printf("[IOCP] build aginethread failed\n");
                    return FALSE;
                }
                p_this->PostRecv(p_per_link_info, 0, sizeof(PACKET_HEADER));
                break;
            }

            case MSG_LOGON_FAILURE:
            {
                printf("[IOCP] Logon failed\n");
                closesocket(p_per_link_info->socket);
                CloseHandle(p_this->h_iocp);
                p_this->logon_status = FALSE;
                return 0;
            }

            default:
            {
                p_this->PostRecv(p_per_link_info, 0, sizeof(PACKET_HEADER));
                break;
            }
            }
        }
    }
    return 0;
}


OPSTATUS IOCP_Client::InitialEnvironment()
{
    // allocate link info for server
    p_ser_link_info = (PPER_LINK_INFO)VirtualAlloc(NULL, sizeof(PER_LINK_INFO), MEM_COMMIT, PAGE_READWRITE);
    p_acce_io_info = (PPER_IO_INFO)VirtualAlloc(NULL, 2 * sizeof(PER_IO_INFO), MEM_COMMIT, PAGE_READWRITE);

    if (p_ser_link_info == NULL || p_acce_io_info == NULL)
    {
        printf("#Err: allocating server io info failed\n");
        return OP_FAILED;
    }

    ZeroMemory(p_ser_link_info, sizeof(PER_LINK_INFO));
    ZeroMemory(p_acce_io_info, 2 * sizeof(PER_IO_INFO));

    p_ser_link_info->p_per_io_info = p_acce_io_info;

    // initial socket for server
    p_ser_link_info->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (p_ser_link_info->socket == NULL)
    {
        printf("#Err: creating server socket failed\n");
        return OP_FAILED;
    }

    p_acce_io_info[0].op_type = IO_RECV;
    p_acce_io_info[0].w_buf.len = MAX_BUF_LEN;
    p_acce_io_info[0].w_buf.buf = p_acce_io_info[0].buffer;

    p_acce_io_info[1].op_type = IO_SEND;
    p_acce_io_info[1].w_buf.len = MAX_BUF_LEN;
    p_acce_io_info[1].w_buf.buf = p_acce_io_info[1].buffer;

    InitializeCriticalSection(&SendCriticalSection);
    return OP_SUCCESS;
}


OPSTATUS IOCP_Client::CompletePortStart(string address, INT port)
{
    SOCKADDR_IN sock_addr = { 0 };
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.S_un.S_addr = inet_addr(address.c_str());

    // connect to server side
    while (TRUE)
    {
        Sleep(1000);
        printf("#Log: Try to connect the server from %s : %d\n", address.c_str(), port);
        if (connect(p_ser_link_info->socket, (PSOCKADDR)&sock_addr, sizeof(SOCKADDR_IN)) == 0)
        {
            break;
        }
    }
    // create compltion port
    h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (h_iocp == NULL)
    {
        printf("#Err: create IOCP failed\n");
        return OP_FAILED;
    }

    // bind socket to compltion port
    if (CreateIoCompletionPort((HANDLE)p_ser_link_info->socket, h_iocp, (ULONG_PTR)p_ser_link_info, 0) == NULL)
    {
        printf("#Err: binding IOCP failed\n");
        return OP_FAILED;
    }

    for (ULONG i = 0; i < 1; i++)
    {
        printf("#Log: start IOCP deal thread #%u\n", i + 1);
        if ((h_thread[i] = (HANDLE)_beginthreadex(NULL, 0, IOCP_Client::DealThread, this, 0, NULL)) == NULL)
        {
            printf("#Err: start IOCP thread failed\n");
            return OP_FAILED;
        }
    }

    return OP_SUCCESS;
}


IOCP_Client::IOCP_Client()
{
    InitWinSock();
    InitialEnvironment();
}


IOCP_Client::~IOCP_Client()
{
    VirtualFree(p_acce_io_info, 0, MEM_RELEASE);
    VirtualFree(p_ser_link_info, 0, MEM_RELEASE);
}


int main(int argc, char const* argv[])
{
    printf("\n\n------------------- Test  IOCP -------------------\n");
    IOCP_Client client;
    client.CompletePortStart("127.0.0.1", 9001);
    client.Logon(NULL, "test_client");
    while (TRUE)
    {
        Sleep(1000);
    }
    return 0;
}