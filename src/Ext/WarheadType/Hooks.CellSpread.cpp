#include "Body.h"

#include "../Rules/Body.h"

#include <Helpers\Enumerators.h>

#include <algorithm>
#include <set>

// create enumerator
DEFINE_HOOK(4895B8, DamageArea_CellSpread1, 6) {
	REF_STACK(CellSpreadEnumerator*, pIter, STACK_OFFS(0xE0, 0xB4));
	GET(int, spread, EAX);

	pIter = nullptr;

	if(spread >= 0) {
		pIter = new CellSpreadEnumerator(spread);
	}

	return (pIter && *pIter) ? 0x4895C3 : 0x4899DA;
}

// apply the current value
DEFINE_HOOK(4895C7, DamageArea_CellSpread2, 8)
{
	GET_STACK(CellSpreadEnumerator*, pIter, STACK_OFFS(0xE0, 0xB4));

	auto &offset = **pIter;
	R->DX(offset.X);
	R->AX(offset.Y);

	return 0x4895D7;
}

// advance and delete if done
DEFINE_HOOK(4899BE, DamageArea_CellSpread3, 8)
{
	REF_STACK(CellSpreadEnumerator*, pIter, STACK_OFFS(0xE0, 0xB4));
	REF_STACK(int, index, STACK_OFFS(0xE0, 0xD0));

	// reproduce skipped instruction
	index++;

	// advance iterator
	if(++*pIter) {
		return 0x4895C0;
	}

	// all done. delete and go on
	delete pIter;

	return 0x4899DA;
}


// #895990: limit the number of times a warhead with
// CellSpread will hit the same object for each hit
DEFINE_HOOK(4899DA, DamageArea_Damage_MaxAffect, 7)
{
	struct DamageGroup {
		ObjectClass* Target;
		int Distance;
	};

	REF_STACK(DynamicVectorClass<DamageGroup*>, groups, STACK_OFFS(0xE0, 0xA8));
	GET_BASE(WarheadTypeClass*, pWarhead, 0xC);

	auto pExt = WarheadTypeExt::ExtMap.Find(pWarhead);
	const int MaxAffect = pExt->CellSpread_MaxAffect;

	if(MaxAffect < 0) {
		return 0;
	}

	std::set<ObjectClass*> handled;

	std::vector<DamageGroup**> target;
	for(auto& group : groups) {
		// could have been cleared by previous iteration
		if(group && !handled.count(group->Target)) {
			handled.insert(group->Target);
			target.clear();

			// collect all slots containing damage groups for this target
			std::for_each(&group, groups.end(), [group, &target](DamageGroup* &item) {
				if(item && item->Target == group->Target) {
					target.push_back(&item);
				}
			});

			// if more than allowed, sort them and remove the ones further away
			if(target.size() > static_cast<size_t>(MaxAffect)) {
				std::sort(target.begin(), target.end(), [](DamageGroup** a, DamageGroup** b) {
					return (*a)->Distance < (*b)->Distance;
				});

				std::for_each(target.begin() + MaxAffect, target.end(), [](DamageGroup** ppItem) {
					GameDelete(*ppItem);
					*ppItem = nullptr;
				});
			}
		}
	}

	// move all the empty ones to the back, then remove them
	auto end = std::stable_partition(groups.begin(), groups.end(), [](DamageGroup* pGroup) {
		return pGroup != nullptr;
	});

	auto validCount = std::distance(groups.begin(), end);
	for(int i = groups.Count - 1; i >= validCount; --i) {
		groups.RemoveItem(i);
	}

	return 0;
}

DEFINE_HOOK(4893BA, DamageArea_DamageAir, 9)
{
	GET(const CoordStruct* const, pCoords, EDI);
	GET(const WarheadTypeClass* const, pWarhead, ESI);
	GET(int const, heightFloor, EAX);
	GET_STACK(const CellClass*, pCell, STACK_OFFS(0xE0, 0xC0));

	int heightAboveGround = pCoords->Z - heightFloor;

	// consider explosions on and over bridges
	if(heightAboveGround > CellClass::BridgeHeight
		&& pCell->ContainsBridge()
		&& RulesExt::Global()->DamageAirConsiderBridges)
	{
		heightAboveGround -= CellClass::BridgeHeight;
	}

	// damage units in air if detonation is above a threshold
	auto const pExt = WarheadTypeExt::ExtMap.Find(pWarhead);
	auto const damageAir = heightAboveGround > pExt->DamageAirThreshold;

	return damageAir ? 0x4893C3u : 0x48955Eu;
}
