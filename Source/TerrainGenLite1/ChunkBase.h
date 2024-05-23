#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkMeshData.h"
#include "Enums.h"
#include "ChunkBase.generated.h"

class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class TERRAINGENLITE1_API AChunkBase : public AActor
{
	GENERATED_BODY()

	struct FMask
	{
		EBlock Block;
		int Normal;
	};

public:
	// Sets default values for this actor's properties
	AChunkBase();

	UPROPERTY(EditDefaultsOnly, Category = "Chunk")
	int Size = 64;

	TObjectPtr<UMaterialInterface> Material;
	float Frequency;
	int ZRepeat;
	int DrawDistance;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	EBlock GetBlock(FIntVector Index) const;

protected:
	// Called when the game starts or when spawned
	void BeginPlay() ;

	void Generate3DHeightMap(const FVector Position);
	void GenerateMesh();

	void ModifyVoxelData(const FIntVector Position, const EBlock Block);

	TObjectPtr<UProceduralMeshComponent> Mesh;
	FastNoiseLite* Noise;
	FChunkMeshData MeshData;
	int VertexCount = 0;

private:
	void ApplyMesh() const;
	void ClearMesh();
	void GenerateHeightMap();

	TArray<EBlock> Blocks;

	void CreateQuad(FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4);
	int GetBlockIndex(int X, int Y, int Z) const;


	bool CompareMask(FMask M1, FMask M2) const;
	int GetTextureIndex(EBlock Block, FVector Normal) const;

	TArray<FIntVector> TreePositions;
	void GenerateTrees(TArray<FIntVector> LocalTreePositions);
};
