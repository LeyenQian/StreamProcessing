#include "pre_define.h"

class LinkPool
{
private:
    PLINK_POOL_MANAGE p_link_pool_manage = NULL;
    CRITICAL_SECTION CriticalSection = {0};
public:
    OPSTATUS InitWinSock();
    OPSTATUS LinkPoolBuild();
    OPSTATUS LinkPoolDestroy();
    PPER_LINK_INFO LinkPoolAlloc();
};