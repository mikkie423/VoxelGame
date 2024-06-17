#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "BlockData.generated.h"

USTRUCT(BlueprintType)
struct FMask
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    EBlock BlockType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    int Normal;

    FMask()
        : BlockType(EBlock::Null), Normal(0) {}

    FMask(EBlock InBlockType, int InNormal)
        : BlockType(InBlockType), Normal(InNormal) {}
};

USTRUCT(BlueprintType)
struct FBlockData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    FMask Mask;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    bool bIsSolid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    int TextureIndex;

    FBlockData()
        : Mask(), bIsSolid(true), TextureIndex(-1) {}

    FBlockData(const FMask& InMask, bool InSolid, int InTextureIndex)
        : Mask(InMask), bIsSolid(InSolid), TextureIndex(InTextureIndex) {}
};



USTRUCT(BlueprintType)
struct FWaterBlockData : public FBlockData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    float WaterOpacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    bool bHasFlow;

    FWaterBlockData()
        : FBlockData(), WaterOpacity(0.5f), bHasFlow(false) {}

    FWaterBlockData(const FMask& InMask, bool InSolid, int InTextureIndex, float InWaterOpacity, bool InHasFlow)
        : FBlockData(InMask, InSolid, InTextureIndex), WaterOpacity(InWaterOpacity), bHasFlow(InHasFlow) {}
};
