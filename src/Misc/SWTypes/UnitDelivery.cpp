#include "UnitDelivery.h"
#include "../../Ext/Techno/Body.h"
#include "../../Utilities/TemplateDef.h"

void SW_UnitDelivery::Initialize(SWTypeExt::ExtData *pData, SuperWeaponTypeClass *pSW)
{
	pData->SW_AITargetingType = SuperWeaponAITargetingMode::ParaDrop;
}

void SW_UnitDelivery::LoadFromINI(
	SWTypeExt::ExtData *pData, SuperWeaponTypeClass *pSW, CCINIClass *pINI)
{
	const char * section = pSW->ID;

	if(!pINI->GetSection(section)) {
		return;
	}

	INI_EX exINI(pINI);
	pData->SW_Deliverables.Read(exINI, section, "Deliver.Types");
	pData->SW_DeliverBuildups.Read(exINI, section, "Deliver.Buildups");
}

bool SW_UnitDelivery::Activate(SuperClass* pThis, const CellStruct &Coords, bool IsPlayer)
{
	SuperWeaponTypeClass *pSW = pThis->Type;
	SWTypeExt::ExtData *pData = SWTypeExt::ExtMap.Find(pSW);

	int deferment = pData->SW_Deferment.Get(-1);
	if(deferment < 0) {
		deferment = 20;
	}

	this->newStateMachine(deferment, Coords, pThis);

	return true;
}

void UnitDeliveryStateMachine::Update() {
	if(this->Finished()) {
		CoordStruct coords = CellClass::Cell2Coord(this->Coords);

		auto pData = this->FindExtData();

		pData->PrintMessage(pData->Message_Activate, this->Super->Owner);

		if(pData->SW_ActivationSound != -1) {
			VocClass::PlayAt(pData->SW_ActivationSound, coords, nullptr);
		}

		this->PlaceUnits();
	}
}

// Replaced my own implementation with AlexB's shown below

//This function doesn't skip any placeable items, no matter how
//they are ordered. Infantry is grouped. Units are moved to the
//center as close as they can. There is an additional fix for a
//glitch: previously, even if a unit could be placed, it would
//have been unloaded again, when it was at index 100.

void UnitDeliveryStateMachine::PlaceUnits() {
	SWTypeExt::ExtData *pData = this->FindExtData();

	if(!pData) {
		return;
	}

	for(size_t i=0; i<pData->SW_Deliverables.size(); ++i) {
		TechnoTypeClass * Type = pData->SW_Deliverables[i];
		TechnoClass * Item = generic_cast<TechnoClass *>(Type->CreateObject(this->Super->Owner));
		BuildingClass * ItemBuilding = specific_cast<BuildingClass *>(Item);

		if(ItemBuilding && pData->SW_DeliverBuildups) {
			ItemBuilding->QueueMission(mission_Construction, false);
		}

		int cellIdx = 0;
		bool Placed = false;
		do {
			CellStruct tmpCell = CellSpread::GetCell(cellIdx) + this->Coords;
			if(CellClass *cell = MapClass::Instance->TryGetCellAt(tmpCell)) {
				CoordStruct XYZ;
				cell->GetCoordsWithBridge(&XYZ);

				bool validCell = true;
				if(cell->OverlayTypeIndex != -1) {
					// disallow placing on rocks, rubble and walls
					OverlayTypeClass *Overlay = OverlayTypeClass::Array->GetItem(cell->OverlayTypeIndex);
					validCell = !Overlay->Wall && !Overlay->IsARock && !Overlay->IsRubble;
				}
				if(AircraftClass * ItemAircraft = specific_cast<AircraftClass *>(Item)) {
					// for aircraft: cell must be empty: non-water, non-cliff, non-shore, non-anything
					validCell &= !cell->GetContent() && !cell->Tile_Is_Cliff()
						&& !cell->Tile_Is_DestroyableCliff() && !cell->Tile_Is_Shore()
						&& !cell->Tile_Is_Water() && !cell->ContainsBridge();
				}
				if(Type->Naval && validCell) {
					// naval types look stupid on bridges
					validCell = (!cell->ContainsBridge() && cell->LandType != LandType::Road)
						|| Type->SpeedType == SpeedType::Hover;
				}

				if(validCell) {
					Item->OnBridge = cell->ContainsBridge();
					if((Placed = Item->Put(XYZ, (cellIdx & 7))) == true) {
						if(ItemBuilding) {
							if(pData->SW_DeliverBuildups) {
								ItemBuilding->DiscoveredBy(this->Super->Owner);
								ItemBuilding->unknown_bool_6DD = 1;
							}
						} else {
							if(Type->BalloonHover || Type->JumpJet) {
								Item->Scatter(CoordStruct::Empty, true, false);
							}
						}
						if(TechnoExt::ExtData* pItemData = TechnoExt::ExtMap.Find(Item)) {
							if(!pItemData->IsPowered() || !pItemData->IsOperated()) {
								Item->Deactivate();
								if(ItemBuilding) {
									Item->Owner->RecheckTechTree = true; 
								}
							}
						}
					}
				}
			}

			++cellIdx;
			if(cellIdx >= 100) { // 100 cells should be enough for any sane delivery
				cellIdx = 0;
				if(!Placed) {
					Item->UnInit();
				}
				break;
			}
		} while(!Placed);
	}
}
