#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "BlockData.h"
#include "ChunkMeshData.generated.h"

USTRUCT()
struct FChunkMeshData
{
	GENERATED_BODY();

public:
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
	TArray<FColor> Colors;
	TArray<FVector2D> UV0;
	TArray<FBlockData> BlockData;

	void Clear();
};

inline void FChunkMeshData::Clear()
{
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	Colors.Empty();
	UV0.Empty();
	BlockData.Empty();
}