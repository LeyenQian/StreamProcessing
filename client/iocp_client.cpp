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


OPSTATUS IOCP_Client::PostRecv(PPER_LINK_INFO pPerLinkInfo, ULONG BuffOffest, ULONG BuffLen)
{
    ULONG uFlag = 0;
    ULONG uRecv = 0;

    ZeroMemory(&pPerLinkInfo->p_per_io_info[0].overlapped, sizeof(OVERLAPPED));
    pPerLinkInfo->p_per_io_info[0].w_buf.buf = &pPerLinkInfo->p_per_io_info[0].buffer[BuffOffest];
    pPerLinkInfo->p_per_io_info[0].w_buf.len = BuffLen;

    if (WSARecv(pPerLinkInfo->socket, &pPerLinkInfo->p_per_io_info[0].w_buf, 1, &uRecv, &uFlag, &pPerLinkInfo->p_per_io_info[0].overlapped, NULL) == SOCKET_ERROR &&
        WSAGetLastError() != WSA_IO_PENDING)
    {
        printf("#Err: post receive failed, discard connection\n");
        return FALSE;
    }

    return TRUE;
}


OPSTATUS IOCP_Client::IsRecvFinish(PPER_LINK_INFO pPerLinkInfo, ULONG ActualTrans)
{
    // package length cannot bigger than buffer 
    if (pPerLinkInfo->p_per_io_info[0].w_buf.len != ActualTrans)
    {
        // not completely, post receive
        pPerLinkInfo->p_per_io_info[0].curr_data_len += ActualTrans;
        PostRecv(pPerLinkInfo, ActualTrans, pPerLinkInfo->p_per_io_info[0].w_buf.len - ActualTrans);
        pPerLinkInfo->p_per_io_info[0].post_recv_times++;
        return FALSE;
    }
    else
    {
        // received completely, change reveived length
        pPerLinkInfo->p_per_io_info[0].curr_data_len += ActualTrans;
    }

    // package length bigger than buffer, discard the link directly
    if (((PPACKET_HEADER)(pPerLinkInfo->p_per_io_info[0].buffer))->packet_len >= sizeof(PACKET_HEADER) && ((PPACKET_HEADER)(pPerLinkInfo->p_per_io_info[0].buffer))->packet_len > pPerLinkInfo->p_per_io_info[0].curr_data_len)
    {
        // not completely, post receive
        PostRecv(pPerLinkInfo, pPerLinkInfo->p_per_io_info[0].curr_data_len, ((PPACKET_HEADER)(pPerLinkInfo->p_per_io_info[0].buffer))->packet_len - pPerLinkInfo->p_per_io_info[0].curr_data_len);
        pPerLinkInfo->p_per_io_info[0].post_recv_times++;
        return FALSE;
    }

    return TRUE;
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

    return OP_SUCCESS;
}


OPSTATUS IOCP_Client::CompletePortStart(string Address, INT Port)
{
    SOCKADDR_IN sock_addr = { 0 };
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(Port);
    sock_addr.sin_addr.S_un.S_addr = inet_addr(Address.c_str());

    // connect to server side
    while (TRUE)
    {
        Sleep(1000);
        printf("#Log: Try to connect the server from %s : %d\n", Address.c_str(), Port);
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
    if (CreateIoCompletionPort((HANDLE)p_ser_link_info->socket, h_iocp, (ULONG)p_ser_link_info, 0) == NULL)
    {
        printf("#Err: binding IOCP failed\n");
        return OP_FAILED;
    }

    //for (ULONG i = 0; i < 1; i++)
    //{
    //    printf("#Log: start IOCP deal thread #%u\n", i + 1);
    //    if ((h_thread[i] = (HANDLE)_beginthreadex(NULL, 0, IOCP::DealThread, this, 0, NULL)) == NULL)
    //    {
    //        printf("#Err: start IOCP thread failed\n");
    //        return OP_FAILED;
    //    }
    //}

    return OP_SUCCESS;
}


IOCP_Client::IOCP_Client()
{
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
    return 0;
}