#pragma once

#include <skse64/GameReferences.h>

enum : UInt32 {
	kActorValue_Stamina	= 0x1A
};const 

// 0C
struct AVValues {
	float	modifier;	// 00
	float	unk04;		// 04
	float	amountLost;	// 08
};

AVValues *GetAVValues(Actor *actor, UInt32 av);
