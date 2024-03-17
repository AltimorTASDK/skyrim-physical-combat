#include "TraceInfo.h"
#include "MeleeExtraData.h"
#include "StringPool.h"
#include "HitFrameHandler.h"
#include "ActorValue.h"
#include <skse64/PluginAPI.h>
#include <skse64/GameData.h>
#include <skse64/GameEvents.h>
#include <skse64/GameForms.h>
#include <skse64/GameReferences.h>
#include <skse64/GameRTTI.h>
#include <skse64/PapyrusEvents.h>
#include <skse64/PapyrusVM.h>
#include <skse64/NiNodes.h>
#include <skse64_common/BranchTrampoline.h>
#include <skse64_common/SafeWrite.h>
#include <ShlObj.h>

constexpr auto kHavokCoordScale = .0142875f;

class TES;

RelocAddr<uintptr_t> kHook_Actor_FindValidMeleeTarget_Call(0x00627EA4);
RelocAddr<uintptr_t> kHook_Actor_MeleeTargetTraceFallback_Call(0x00627C0D);
RelocAddr<uintptr_t> kHook_Actor_Update_Call(0x005B407A);
constexpr auto kActor_Update_VtableIndex = 173;

// Used to deal melee damage on HitFrame animation event
RelocAddr<uintptr_t> HitFrameHandler_Vtable(0x01672080);

RelocPtr<TES*> g_TES(0x2F26B20);

using _FindValidMeleeTarget = Actor*(*)(Actor *attacker, float reach);
RelocAddr<_FindValidMeleeTarget> FindValidMeleeTarget(0x007BA290);

using _MeleeTargetTraceFallback = Actor*(*)(Actor *attacker);
RelocAddr<_MeleeTargetTraceFallback> MeleeTargetTraceFallback(0x00629090);

using _CollisionTrace = NiAVObject*(*)(TES *tes, TraceInfo *traceInfo);
RelocAddr<_CollisionTrace> CollisionTrace(0x001580C0);

using _GetActorCollisionMask = UInt32*(*)(Actor *actor, UInt32 *mask);
RelocAddr<_GetActorCollisionMask> GetActorCollisionMask(0x005EBD90);

using _GetNiAVObjectOwnerRecursive = TESObjectREFR*(*)(NiAVObject *object);
RelocAddr<_GetNiAVObjectOwnerRecursive> GetNiAVObjectOwnerRecursive(0x002945E0);

using _MeleeSwing = void(*)(Actor *attacker, bool leftHand, bool param_3);
RelocAddr<_MeleeSwing> MeleeSwing(0x00627930);

using _SendAnimationEvent_Native = void(*)(
	VMClassRegistry *registry, UInt32 stackId, void *debugScript,
	TESObjectREFR *arRef, StringCache::Ref *asEventName);
RelocAddr<_SendAnimationEvent_Native> SendAnimationEvent_Native(0x0096DE60);

IDebugLog gLog;

BranchTrampoline branchTrampoline;

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface *skse, PluginInfo *info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\PhysicalCombat.log");

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Physical Combat";
	info->version = 1;

	if (skse->isEditor) {
		_MESSAGE("Loaded in editor, marking as incompatible");
		return false;
	}

	return true;
}

NiNode *FindNodeByName(NiNode *node, const char *name)
{
	if (strcmp(node->m_name, name) == 0)
		return node;

	for (auto i = 0; i < node->m_children.m_size; i++) {
		auto *child = DYNAMIC_CAST(node->m_children.m_data[i], NiAVObject, NiNode);
		if (child == nullptr)
			continue;

		if (auto *result = FindNodeByName(child, name); result != nullptr)
			return result;
	}

	return nullptr;
}

TESObjectREFR *LineTrace(Actor *actor, const NiPoint3 &start, const NiPoint3 &end, float *outFraction)
{
	UInt32 collisionMask;
	GetActorCollisionMask(actor, &collisionMask);

	TraceInfo traceInfo;
	traceInfo.in.m_filterInfo = (collisionMask & 0xFFFF0000) | 0x29;
	traceInfo.in.m_from = start * kHavokCoordScale;
	traceInfo.in.m_to = end * kHavokCoordScale;

	auto *traceObject = CollisionTrace(*g_TES, &traceInfo);

	if (outFraction != nullptr)
		*outFraction = traceInfo.out.m_hitFraction;

	return GetNiAVObjectOwnerRecursive(traceObject);
}

// Uses type ID instead of RTTI like SKSE's
BSExtraData *GetExtraDataByTypeID(BaseExtraList *list, UInt32 type)
{
	BSReadLocker locker(&list->m_lock);

	if (!list->HasType(type))
		return nullptr;

	for (auto *data = list->m_data; data != nullptr; data = data->next) {
		if (data->GetType() == type)
			return data;
	}

	return nullptr;
}

ExtraMelee *GetMeleeData(Actor *actor)
{
	auto *extraList = &actor->extraData;
	auto *extraMelee = GetExtraDataByTypeID(extraList, kExtraData_Melee);

	if (extraMelee == nullptr) {
		extraMelee = new ExtraMelee();
		extraList->Add(kExtraData_Melee, extraMelee);
	}

	return (ExtraMelee*)extraMelee;
}

// Check if object is an actor and hasn't already been hit
bool CanHitObject(TESObjectREFR *object, ExtraMelee *meleeData)
{
	if (auto *actor = DYNAMIC_CAST(object, TESObjectREFR, Actor); actor != nullptr) {
		if (!meleeData->WasActorHit(actor))
			return true;
	}

	return false;
}

Actor *DetectMeleeHit(Actor *attacker, NiNode *weaponNode, float reach)
{
	auto *meleeData = GetMeleeData(attacker);
	const auto &transform = weaponNode->m_worldTransform;
	const auto &oldTransform = weaponNode->m_oldWorldTransform;

	// Trace along blade to check for obstructions
	const auto maxReach = NiPoint3(0, reach, 0);
	float reachFraction;
	auto *hitObject = LineTrace(attacker, transform.pos, transform * maxReach, &reachFraction);

	if (CanHitObject(hitObject, meleeData)) {
		return DYNAMIC_CAST(hitObject, TESObjectREFR, Actor);
	}

	// Don't interpolate on first frame
	if (meleeData->firstFrame)
		return nullptr;

	const auto obstructedReach = reach * reachFraction;
	const auto traceCount = max((int)(obstructedReach / 18), 1);

	for (auto i = 0; i <= traceCount; i++) {
		const auto offset = NiPoint3(0, obstructedReach * i / traceCount, 0);
		const auto start = oldTransform * offset;
		const auto end = transform * offset;
		hitObject = LineTrace(attacker, start, end, nullptr);

		meleeData->AddDebugSphere(attacker, start);
		meleeData->AddDebugSphere(attacker, end);

		if (CanHitObject(hitObject, meleeData)) {
			return DYNAMIC_CAST(hitObject, TESObjectREFR, Actor);
		}
	}

	return nullptr;
}

Actor *Hook_Actor_FindValidMeleeTarget(Actor *attacker, float reach)
{
	auto *meleeData = GetMeleeData(attacker);

	// Call original if not being called from MeleeSwing
	if (!meleeData->swinging)
		return FindValidMeleeTarget(attacker, reach);

	bool firstPerson;

	if (auto *pc = DYNAMIC_CAST(attacker, Actor, PlayerCharacter); pc != nullptr) {
		auto *camera = PlayerCamera::GetSingleton();
		firstPerson = camera->cameraState->stateId == PlayerCamera::kCameraState_FirstPerson;
	} else {
		firstPerson = false;
	}

	auto *rootNode = attacker->GetNiRootNode(firstPerson);

	NiNode *weaponNode;
	if (meleeData->leftHand)
		weaponNode = FindNodeByName(rootNode, "AnimObjectL");
	else
		weaponNode = FindNodeByName(rootNode, "AnimObjectR");

	if (weaponNode == nullptr)
		return nullptr;

	Actor *hitActor = DetectMeleeHit(attacker, weaponNode, reach);
	if (hitActor != nullptr)
		meleeData->AddHitActor(hitActor);

	return hitActor;
}

// Called if GetMeleeTarget -> FindValidMeleeTarget doesn't find anything
Actor *Hook_Actor_MeleeTargetTraceFallback(Actor *attacker)
{
	auto *meleeData = GetMeleeData(attacker);

	// Call original if not being called from MeleeSwing
	if (!meleeData->swinging)
		return MeleeTargetTraceFallback(attacker);

	return nullptr;
}

// Continue the swing after HitFrame
void ContinueSwing(Actor *actor, ExtraMelee *meleeData)
{
	// Back up stamina so power attacks don't repeatedly drain stamina
	auto *staminaAv = GetAVValues(actor, kActorValue_Stamina);
	const auto staminaLost = staminaAv->amountLost;

	meleeData->swinging = true;
	MeleeSwing(actor, meleeData->leftHand, true);
	meleeData->swinging = false;

	staminaAv->amountLost = staminaLost;
}

void Hook_Actor_Update(Actor *actor, float deltaTime)
{
	auto *meleeData = GetMeleeData(actor);
	if (meleeData->activeTime > 0.f)
		ContinueSwing(actor, meleeData);

	// Call the original vfunc
	using _Update = void(*)(TESObjectREFR*, float);
	((_Update)((*(void***)actor)[kActor_Update_VtableIndex]))(actor, deltaTime);

	meleeData->Update(deltaTime);
}

bool Hook_HitFrameHandler_Process(HitFrameHandler *handler, Actor *actor, const StringCache::Ref *hand)
{
	if (actor->IsDead(0))
		return CALL_MEMBER_FN(handler, Process_Origin)(actor, hand);

	auto *meleeData = GetMeleeData(actor);
	meleeData->leftHand = hand->data == StringPool::GetInstance()->strings[StringPool::kLeft];
	meleeData->activeTime = .2f;
	meleeData->ClearHitActors();

	meleeData->swinging = true;
	meleeData->firstFrame = true;
	const auto result = CALL_MEMBER_FN(handler, Process_Origin)(actor, hand);
	meleeData->firstFrame = false;
	meleeData->swinging = false;

	return result;
}

/*class ActionEventSink : public BSTEventSink<SKSEActionEvent> {
	EventResult ReceiveEvent(SKSEActionEvent *evn, EventDispatcher<SKSEActionEvent> *dispatcher) override;
} g_actionEventSink;

EventResult ActionEventSink::ReceiveEvent(SKSEActionEvent *evn, EventDispatcher<SKSEActionEvent> *dispatcher)
{
	if (evn->actor->IsDead(0))
		return kEvent_Continue;

	if (evn->type != SKSEActionEvent::kType_WeaponSwing)
		return kEvent_Continue;

	auto *meleeData = GetMeleeData(evn->actor);
	meleeData->leftHand = evn->slot == SKSEActionEvent::kSlot_Left;
	meleeData->activeTime = .2f;
	meleeData->ClearHitActors();

	meleeData->swinging = true;
	MeleeSwing(evn->actor, meleeData->leftHand, true);
	meleeData->swinging = false;

	return kEvent_Continue;
}*/

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface *skse)
{
	_MESSAGE("Plugin loaded");

	if (!branchTrampoline.Create(64)) {
		_ERROR("Failed to create branch trampoline.");
		return false;
	}

	branchTrampoline.Write5Call(
		kHook_Actor_FindValidMeleeTarget_Call.GetUIntPtr(),
		(uintptr_t)Hook_Actor_FindValidMeleeTarget);

	branchTrampoline.Write5Call(
		kHook_Actor_MeleeTargetTraceFallback_Call.GetUIntPtr(),
		(uintptr_t)Hook_Actor_MeleeTargetTraceFallback);

	branchTrampoline.Write6Call(
		kHook_Actor_Update_Call.GetUIntPtr(),
		(uintptr_t)Hook_Actor_Update);

	SafeWrite64(HitFrameHandler_Vtable.GetUIntPtr() + 8, GetFnAddr(Hook_HitFrameHandler_Process));

	/*auto *messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
	auto *dispatcher = (EventDispatcher<SKSEActionEvent>*)
		messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ActionEvent);

	dispatcher->AddEventSink(&g_actionEventSink);*/

	return true;
}