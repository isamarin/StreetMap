#include "StreetMapActorFactory.h"
#include "AssetRegistry/AssetData.h"
#include "StreetMapActor.h"
#include "StreetMapComponent.h"
#include "StreetMap.h"

UStreetMapActorFactory::UStreetMapActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("StreetMap", "StreetMapFactoryDisplayName", "Add StreetMap Actor");
	NewActorClass = AStreetMapActor::StaticClass();
}

void UStreetMapActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	if (UStreetMap* StreetMapAsset = Cast<UStreetMap>(Asset))
	{
		AStreetMapActor* StreetMapActor = CastChecked<AStreetMapActor>(NewActor);
		UStreetMapComponent* StreetMapComponent = StreetMapActor->GetStreetMapComponent();
		StreetMapComponent->SetStreetMap(StreetMapAsset, false, true);
	}
}

#if ENGINE_MAJOR_VERSION < 5 || ( ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 8 )
void UStreetMapActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (Asset != nullptr && CDO != nullptr)
	{
		UStreetMap* StreetMapAsset = CastChecked<UStreetMap>(Asset);
		AStreetMapActor* StreetMapActor = CastChecked<AStreetMapActor>(CDO);
		UStreetMapComponent* StreetMapComponent = StreetMapActor->GetStreetMapComponent();
		StreetMapComponent->SetStreetMap(StreetMapAsset, true, false);
	}
}
#endif

bool UStreetMapActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UStreetMap::StaticClass()));
}
