#pragma once

UENUM(BlueprintType)
enum class EDirection : uint8
{
    Forward, Right, Back, Left, Up, Down
};

UENUM(BlueprintType)
enum class EBlock : uint8
{
   Grass, Dirt, Stone, Bedrock, Log, Leaves, Sand, Gravel, ShallowWater, DeepWater, Swamp, Taiga, Tundra, Air, Null
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