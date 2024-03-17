#pragma once

#include "Havok.h"

#pragma pack(push, 4)

// C4
struct TraceInfo {
	TraceInfo() :
		rayCaster(nullptr),
		closestCollector(nullptr), allCollector1(nullptr), allCollector2(nullptr),
		invalid(false) { }

	hkpWorldRayCastInput		in;					// 00
	hkpWorldRayCastOutput		out;				// 30
	hkVector4					offset;				// 90
	hkpWorldRayCaster			*rayCaster;			// A0
	hkpClosestRayHitCollector	*closestCollector;	// A8
	// Seems like either is used if non-null, not sure why there are 2
	hkpAllRayHitCollector		*allCollector1;		// B0
	hkpAllRayHitCollector		*allCollector2;		// B8
	bool						invalid;			// C0
};

#pragma pack(pop)
