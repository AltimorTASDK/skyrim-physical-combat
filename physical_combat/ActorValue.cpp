#include "ActorValue.h"

AVValues *GetAVValues(Actor *actor, UInt32 av)
{
	switch (av) {
	case kActorValue_Stamina:
		return (AVValues*)&actor->unk240;
	}

	_ERROR("Unhandled actor value type %i", av);
	return nullptr;
}
