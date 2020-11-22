#ifndef __PRE_DEFINE_H
#define __PRE_DEFINE_H

#include <stdio.h>
#include <process.h>
#include <winsock2.h>
#include <MSWSock.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <iostream>
#include <string>
using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

typedef INT OPSTATUS;
#define OP_SUCCESS 0x0
#define OP_FAILED  0x1

#define MAX_BUF_SEND        8192
#define MAX_BUF_RECV        8192
#define MAX_BUF_LEN         (MAX_BUF_SEND + MAX_BUF_RECV)

#define IO_SEND             0x1
#define IO_RECV             0x2
#define IO_ACCE             0x3

#define LINK_FREE           0x0
#define LINK_BUSY           0x1

#define SM_IDLE             0x0
#define SM_FULL             0x1
#define SM_OVER             0x2
#define SM_FAIL             0x3

#define MSG_HEART_BEAT		0x1
#define MSG_LOGON			0x2
#define MSG_LOGOUT			0x3
#define MSG_LOGON_SUCCESS	0x4
#define MSG_LOGON_FAILURE	0x5


typedef struct _PER_IO_INFO
{
	ULONG       op_type;
	ULONG       curr_data_len;
	ULONG       post_recv_times;
	CHAR        buffer[MAX_BUF_LEN];
	WSABUF      w_buf;
	OVERLAPPED	overlapped;
	// ...
} PER_IO_INFO, * PPER_IO_INFO;


typedef struct _PER_LINK_INFO
{
	LIST_ENTRY		list_entry;
	ULONG			free_flag;
	ULONG			state_machine;
	SOCKET			socket;
	PPER_IO_INFO	p_per_io_info;
	// ...
} PER_LINK_INFO, * PPER_LINK_INFO;


typedef struct _PACKET_HEADER
{
	ULONG comm_code;
	ULONG packet_len;
	// ...
} PACKET_HEADER, * PPACKET_HEADER;

#endif