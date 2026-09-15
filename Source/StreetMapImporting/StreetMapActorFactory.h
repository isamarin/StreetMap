#pragma once
#include "ActorFactories/ActorFactory.h"
#include "StreetMapActorFactory.generated.h"

UCLASS()
class UStreetMapActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UActorFactory Interface
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
#if ENGINE_MAJOR_VERSION < 5 || ( ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 8 )
	// UActorFactory::PostCreateBlueprint was removed from the engine in (at latest) UE 5.8.
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
#endif
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	//~ End UActorFactory Interface
};
