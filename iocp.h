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

#include "PreDefine.h"

#define  DB_OFFSET(s,m) = &((s)0)->m
constexpr INT DB_BUFF_LEN = 16 * 1024;
constexpr INT DB_ACPT_NUM = 16;
constexpr INT DB_HOLD_AGE = 180;

constexpr INT DB_IO_SEND = 0x001;
constexpr INT DB_IO_RECV = 0x002;
constexpr INT DB_IO_ACPT = 0x003;

constexpr INT DB_DIR_RQST = 0x001;
constexpr INT DB_DIR_RPLY = 0x002;
constexpr INT DB_FLE_RQST = 0x003;
constexpr INT DB_FLE_RPLY = 0x004;
constexpr INT DB_ACL_RQST = 0x005;
constexpr INT DB_ACL_RPLY = 0x006;

constexpr INT DB_POST_EROR = -0x001;
constexpr INT DB_POST_LINK = -0x002;
constexpr INT DB_POST_CONF = -0x003;
constexpr INT DB_POST_TIME = -0x004;

constexpr INT  DB_EROR_LINK = 0x001;


typedef struct _DB_LIST_MAN
{
	LIST_ENTRY HeadList;
	ULONG NodeNum;
	CRITICAL_SECTION SynLock;

	// ...

} DB_LIST_MAN, * PDB_LIST_MAN;


typedef struct _DB_PCKT_HEAD
{
	ULONG Len;
	ULONG Cmc;

	// ...

} DB_PCKT_HEAD, * PDB_PCKT_HEAD;


typedef struct _DB_SOCK_INFO
{
	SOCKET Socket;
	SOCKADDR_IN SockAddr;

	// ...

} DB_SOCK_INFO, * PDB_SOCK_INFO;


typedef struct _DB_IO_INFO
{
	LIST_ENTRY ListEntry;
	ULONG IoType;
	WSABUF wBuf;
	OVERLAPPED Overlapped;
	CHAR Buff[DB_BUFF_LEN];

	// ...

} DB_IO_INFO, * PDB_IO_INFO;


typedef struct _DB_LINK_INFO
{
	LIST_ENTRY ListEntry;
	ULONG HoldAge;
	DB_LIST_MAN IoList;
	DB_SOCK_INFO SockInfo;

	// ...

} DB_LINK_INFO, * PDB_LINK_INFO;


typedef struct _DB_EROR_INFO
{
	LIST_ENTRY ListEntry;
	ULONG Type;
	PVOID Data;

	// ...

} DB_EROR_INFO, * PDB_EROR_INFO;


BOOL DB_Quit = FALSE;
HANDLE DB_IOCP = NULL;
DB_LIST_MAN DB_ErorMan = { 0 };
DB_LIST_MAN DB_LinkMan = { 0 };
DB_LIST_MAN DB_ConfMan = { 0 };
DB_LIST_MAN DB_FileMan = { 0 };
LPFN_ACCEPTEX DB_AcceptEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS DB_GetAcceptExSockAddrs = NULL;


VOID DB_LinkError(PDB_LINK_INFO LinkInfo);

class IOCP
{
private:

public:
    BOOL DB_InitHelpFunc();
    BOOL DB_LoadSockLib();
    VOID DB_PostRecv(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    VOID DB_PostSend(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    BOOL DB_PostAcpt(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    VOID DB_Recv(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    VOID DB_Send(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    BOOL DB_Acpt(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    BOOL DB_Listen(PDB_LINK_INFO LinkInfo);
    BOOL DB_RecvCheck(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    VOID DB_DoRecv(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    VOID DB_DoSend(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    BOOL DB_DoAcpt(PCHAR AcptInfo, PDB_LINK_INFO AcptLink);
    VOID DB_DoPost(PDB_LINK_INFO LinkInfo, LONG PostCode);
    VOID DB_DoError(PDB_IO_INFO IoInfo, PDB_LINK_INFO LinkInfo);
    unsigned int WINAPI DB_IocpWorkThread(LPVOID Context);
};