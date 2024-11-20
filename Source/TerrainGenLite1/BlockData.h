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
    EBlockCategory BlockCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    float BlockHardness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    int TextureIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    EBiome BiomeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    float Humidity;

    FBlockData()
        : Mask(), BlockCategory(EBlockCategory::Null), TextureIndex(-1), BiomeType(EBiome::Null), Humidity(0.5) {}

    FBlockData(const FMask& InMask, EBlockCategory InBlockCategory, int InTextureIndex, EBiome InBiomeType, int InHumidity)
        : Mask(InMask), BlockCategory(InBlockCategory), TextureIndex(InTextureIndex), BiomeType(InBiomeType), Humidity(InHumidity) {}
};



USTRUCT(BlueprintType)
struct FDecorationData
{
    GENERATED_BODY()

    // The biome this flora should be present in
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flora Generation")
    FIntVector Position;

    // The chance of spawning the flora in this biome
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flora Generation")
    EBlock DecorationBlockType;

    // The texture/material index for the flora (can map to specific texture/material in your setup)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flora Generation")
    int32 TextureIndex; // Represent the texture/material index
};


//USTRUCT(BlueprintType)
//struct FWaterBlockData : public FBlockData
//{
//    GENERATED_BODY()
//
//public:
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
//    float WaterOpacity;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
//    bool bHasFlow;
//
//    FWaterBlockData()
//        : FBlockData(), WaterOpacity(0.5f), bHasFlow(false) {}
//
//    FWaterBlockData(const FMask& InMask, bool InSolid, int InTextureIndex, EBiome InBiomeType, float InWaterOpacity, bool InHasFlow)
//        : FBlockData(InMask, InSolid, InTextureIndex, InBiomeType), WaterOpacity(InWaterOpacity), bHasFlow(InHasFlow) {}
//};
