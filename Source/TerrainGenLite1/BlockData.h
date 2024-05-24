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
    bool bTransparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block Properties")
    int TextureIndex;

    FBlockData()
        : Mask(), bIsSolid(false), bTransparent(false), TextureIndex(-1) {}

    FBlockData(const FMask& InMask, bool InSolid, bool InTransparent, int InTextureIndex)
        : Mask(InMask), bIsSolid(InSolid), bTransparent(InTransparent), TextureIndex(InTextureIndex) {}
};
