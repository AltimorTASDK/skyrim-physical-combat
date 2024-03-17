#include "MeleeExtraData.h"
#include <skse64/ObScript.h>
#include <algorithm>

using _Disable_Native = void(*)(VMClassRegistry *registry, UInt32 stackId, TESObjectREFR *object, bool abFadeOut);
RelocAddr<_Disable_Native> Disable_Native(0x00993A00);

using _MarkForDelete = void(*)(void*, void*, TESObjectREFR *object);
RelocAddr<_MarkForDelete> MarkForDelete(0x00305960);

constexpr auto kDebugSphereFormId = 0xAD80F;

// These need to be defined so BSExtraData can be inherited from
BSExtraData::BSExtraData()
{
}

BSExtraData::~BSExtraData()
{
}

UInt32 BSExtraData::GetType()
{
	return 0;
}

ExtraMelee::~ExtraMelee()
{
	ClearHitActors();
	ClearDebugSpheres();
}

void ExtraMelee::Update(float deltaTime)
{
	if (deltaTime >= this->activeTime)
		activeTime = 0.f;
	else
		activeTime -= deltaTime;

	for (auto sphere = debugSpheres.begin(); sphere != debugSpheres.end(); ) {
		sphere->timeLeft -= deltaTime;

		if (sphere->timeLeft > 0.f) {
			sphere++;
			continue;
		}

		Disable_Native(nullptr, -1, sphere->object, false);
		MarkForDelete(nullptr, nullptr, sphere->object);
		sphere = debugSpheres.erase(sphere);
	}
}

void ExtraMelee::ClearHitActors()
{
	for (auto *actor : hitActors)
		actor->DecRef();


	hitActors.clear();
}

void ExtraMelee::ClearDebugSpheres()
{
	for (const auto &sphere : debugSpheres) {
		Disable_Native(nullptr, -1, sphere.object, false);
		MarkForDelete(nullptr, nullptr, sphere.object);
	}

	debugSpheres.clear();
}

void ExtraMelee::AddHitActor(Actor *actor)
{
	actor->IncRef();
	hitActors.push_back(actor);
}

bool ExtraMelee::WasActorHit(Actor *actor)
{
	return std::find(hitActors.begin(), hitActors.end(), actor) != hitActors.end();
}

void ExtraMelee::AddDebugSphere(TESObjectREFR *target, const NiPoint3 &spawnPos)
{
	TESForm *form = LookupFormByID(kDebugSphereFormId);
	auto *spawned = PlaceAtMe_Native(nullptr, -1, target, form, 1, true, false);

	auto nullHandle = *g_invalidRefHandle;
	auto *cell = spawned->parentCell;
	auto *worldSpace = CALL_MEMBER_FN(spawned, GetWorldspace)();
	auto position = spawnPos;
	MoveRefrToPosition(spawned, &nullHandle, cell, worldSpace, &position, &spawned->rot);

	spawned->IncRef();
	debugSpheres.push_back(spawned);
}