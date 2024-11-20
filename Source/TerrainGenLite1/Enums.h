#pragma once

UENUM(BlueprintType)
enum class EDirection : uint8
{
    Forward, Right, Back, Left, Up, Down
};

UENUM(BlueprintType)
enum class EBlock : uint8
{
   Seeds, Torch, Grass, ShortGrass, DryDirt, WetDirt, WetFarmland, DryFarmland, Stone, Bedrock, Log, WoodPlanks, Leaves, Sand, Gravel, ShallowWater, DeepWater, Swamp, Taiga, Tundra, Ice, Air, Null
};

UENUM(BlueprintType)
enum class EBiome : uint8
{
    Null,
    Desert,
    Swamp,
    Tundra,
    Taiga,
    Plains
};

UENUM(BlueprintType)
enum class EBlockCategory : uint8
{
    Null,
    Solid,
    Liquid,
    NonSolid
};
