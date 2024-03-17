#pragma once

#include <skse64/GameBSExtraData.h>
#include <skse64/GameReferences.h>
#include <vector>

// Has to be between B4 (lowest unused) and BF (bitfield limit)
constexpr auto kExtraData_Melee = 0xBB;

class ExtraMelee : public BSExtraData {
	struct DebugSphere {
		static constexpr auto kLifetime = 1.f;

		DebugSphere(TESObjectREFR *object) : object(object), timeLeft(kLifetime) { }

		TESObjectREFR *object;
		float timeLeft;
	};

public:
	ExtraMelee() : activeTime(0.f) { }
	~ExtraMelee() override;

	UInt32 GetType() override
	{
		return kExtraData_Melee;
	}

	void Update(float deltaTime);
	void ClearHitActors();
	void ClearDebugSpheres();
	void AddHitActor(Actor *actor);
	bool WasActorHit(Actor *actor);
	void AddDebugSphere(TESObjectREFR *target, const NiPoint3 &spawnPos);

	float activeTime;
	bool leftHand;
	bool swinging;
	bool firstFrame;

private:
	std::vector<Actor*> hitActors;
	std::vector<DebugSphere> debugSpheres;

public:
	// Use the game's allocator so the game can free it later
	DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free);
};
