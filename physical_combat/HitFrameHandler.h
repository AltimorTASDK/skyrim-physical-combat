#pragma once

#include <skse64/GameHandlers.h>

// Can't use SKSE's IHandlerFunctor because it expects a 32 bit 2nd arg to Process
class HitFrameHandler {
public:
	virtual ~HitFrameHandler();
	virtual bool Process(Actor *form, const StringCache::Ref *hand);

	MEMBER_FN_PREFIX(HitFrameHandler);
	DEFINE_MEMBER_FN(Process_Origin, bool, 0x007211B0, Actor*, const StringCache::Ref*);
};
