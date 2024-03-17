#include "StringPool.h"
#include <skse64_common/Relocation.h>

using _GetStringPool = StringPool*(*)();
RelocAddr<_GetStringPool> GetStringPool(0x00104AD0);

StringPool *StringPool::GetInstance()
{
	return GetStringPool();
}