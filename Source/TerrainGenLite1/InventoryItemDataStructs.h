#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Enums.h"
#include "BlockData.h"
#include "InventoryItemDataStructs.generated.h"


UENUM()
enum class EItemQuality : uint8
{
	Null UMETA(DisplayName = "Null"),
	Crap UMETA(DisplayName = "Crap"),
	Okay UMETA(DisplayName = "Okay"),
	Alright UMETA(DisplayName = "Alright"),
	PrettyGood UMETA(DisplayName = "PrettyGood"),
	FcknAwesome UMETA(DisplayName = "FcknAwesome")
};

UENUM()
enum class EItemType : uint8
{
	Armour UMETA(DisplayName =  "Armour"),
	Weapon UMETA(DisplayName =  "Weapon"),
	Consumable UMETA(DisplayName = "Consumable"),
	Block UMETA(DisplayName = "Block"),
	Plant UMETA(DisplayName = "Plant")
};


USTRUCT()
struct FItemStatistics
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	float ArmourRating;

	UPROPERTY(EditAnywhere)
	float DamageValue;

	UPROPERTY(EditAnywhere)
	float RestorationAmount;

	UPROPERTY(EditAnywhere)
	float SellValue;
};


USTRUCT()
struct FItemTextData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FText Name;

	UPROPERTY(EditAnywhere)
	FText Description;

	UPROPERTY(EditAnywhere)
	FText InteractionText;

	UPROPERTY(EditAnywhere)
	FText UsageText;
};

USTRUCT()
struct FItemNumericData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	float Weight;

	UPROPERTY(EditAnywhere)
	int32 MaxStackSize;

	UPROPERTY(EditAnywhere)
	bool bIsStackable;
};

USTRUCT()
struct FItemAssetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* Icon;

	// Mesh
	// SOund Effect
};

USTRUCT()
struct FInventoryItemData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Item Data")
	FName ID;

	UPROPERTY(EditAnywhere, Category = "Item Data")
	EItemType ItemType;

	UPROPERTY(EditAnywhere, Category = "Item Data")
	EItemQuality ItemQuality;

	UPROPERTY(EditAnywhere, Category = "Item Data")
	FItemStatistics ItemStatistics;

	UPROPERTY(EditAnywhere, Category = "Item Data")
	FItemTextData TextData;

	UPROPERTY(EditAnywhere, Category = "Item Data")
	FItemNumericData NumericData;
	
	UPROPERTY(EditAnywhere, Category = "Item Data")
	FItemAssetData AssetData;
};