#pragma once

UENUM(BlueprintType)
enum class EDirection : uint8
{
    Forward, Right, Back, Left, Up, Down
};

UENUM(BlueprintType)
enum class EBlock : uint8
{
    Null, Air, Bedrock, Stone, Dirt, Grass, Log, Leaves, Sand, Gravel, ShallowWater, DeepWater
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