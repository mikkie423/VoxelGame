#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkMeshData.h"
#include "Enums.h"
#include "BlockData.h"
#include "FastNoiseLite.h"
#include "ChunkBase.generated.h"

// Define  custom channels
#define ECC_LandMesh ECC_GameTraceChannel2
#define ECC_WaterMesh ECC_GameTraceChannel3


class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class TERRAINGENLITE1_API AChunkBase: public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AChunkBase();

	UPROPERTY(EditDefaultsOnly, Category = "Chunk")
	int Size = 32;

	TObjectPtr<UMaterialInterface> LandMaterial;
	TObjectPtr<UMaterialInterface> LiquidMaterial;
	float Frequency;
	int ZRepeat;
	int DrawDistance;

	UPROPERTY(EditInstanceOnly, Category = "World")
	int WaterLevel = 20;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	EBlock GetBlock(const FIntVector Index) const;

	void GenerateMesh(bool isLandMesh);


protected:
	// Called when the game starts or when spawned
	void BeginPlay() ;

	void GenerateHeightMap(const FVector Position);
	void GenerateWater(const FVector Position);


	void ModifyVoxelData(const FIntVector Position, const EBlock Block);

	TObjectPtr<UProceduralMeshComponent> LandMesh;
	TObjectPtr<UProceduralMeshComponent> LiquidMesh;
	TUniquePtr<FastNoiseLite> Noise;
	FChunkMeshData LandMeshData;
	FChunkMeshData LiquidMeshData;
	int LandVertexCount = 0;
	int LiquidVertexCount = 0;

	FCollisionResponseContainer LandMeshResponse;
	FCollisionResponseContainer WaterMeshResponse;

private:
	void ApplyMesh(bool isLandMesh) const;
	void ClearMesh(bool isLandMesh);
	void GenerateChunk();

	TArray<FBlockData> Blocks;

	void CreateQuad( const FBlockData BlockData, const FIntVector AxisMask, int Width, int Height, const FIntVector V1, const FIntVector V2, const FIntVector V3, const FIntVector V4, FChunkMeshData& MeshData, int& VertexCount);

	int GetBlockIndex(int X, int Y, int Z) const;


	bool CompareMask(FMask M1, FMask M2) const;
	int GetTextureIndex(EBlock Block, FVector Normal) const;

	TArray<FIntVector> TreePositions;
	void GenerateTrees(TArray<FIntVector> LocalTreePositions);

	void PrintMeshData(bool isLandMesh) const;

};
