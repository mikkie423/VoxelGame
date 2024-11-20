#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkMeshData.h"
#include "Enums.h"
#include "BlockData.h"
#include "FastNoiseLite.h"
#include "ProceduralMeshComponent.h"
#include "Math/Vector2D.h"
#include <array>
#include "ChunkBase.generated.h"


// Define  custom channels
#define ECC_LandMesh ECC_GameTraceChannel2
#define ECC_WaterMesh ECC_GameTraceChannel3

class FastNoiseLite;
class UProceduralMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChunkMeshUpdated);

UCLASS()
class TERRAINGENLITE1_API AChunkBase: public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AChunkBase();

	// Delegate for notifying mesh updates
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnChunkMeshUpdated OnChunkMeshUpdated;

	// Call this when the mesh is updated
	void NotifyMeshUpdated();

	UPROPERTY(EditDefaultsOnly, Category = "Chunk")
	int ChunkSize = 32;

	TObjectPtr<UMaterialInterface> LandMaterial;
	TObjectPtr<UMaterialInterface> LiquidMaterial;

	int WorldSeed;
	float Frequency;
	int ZRepeat;
	int DrawDistance;
	int BlockSize;

	UPROPERTY(EditInstanceOnly, Category = "World")
	int WaterLevel = 15;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	EBlock GetBlockType(const FIntVector Index) const;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	float GetBlockHardnessScale(const FIntVector Index) const;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	FBlockData GetBlockData(const FIntVector Index) const;

	int GetBlockIndex(int X, int Y, int Z) const;

	void GenerateMesh(bool isLandMesh);

	FMask DetermineMask(const FBlockData& CurrentBlock, const FBlockData& CompareBlock, bool CurrentIsOpaque, bool CompareIsOpaque, bool CurrentIsLiquid, bool CompareIsLiquid, bool CurrentIsNonSolid, bool CompareIsNonSolid);

	bool IsOpaque(EBlockCategory BlockCategory);

	bool IsNonSolid(EBlockCategory BlockCategory);


	bool IsLiquid(EBlockCategory BlockCategory);

	void ClearMaskInBlockData(TArray<FBlockData>& BlockData, int N, int Width, int Height, int Axis1Limit);

	int DetermineWidth(const TArray<FBlockData>& BlockData, int N, const FMask& CurrentMask, int Axis1Limit);

	int DetermineHeight(const TArray<FBlockData>& BlockData, int N, const FMask& CurrentMask, int Width, int Axis1Limit, int Axis2Limit, int j);

	// Sets the biome information for a specific block
	void SetBiome(int32 X, int32 Y, int32 Z, EBiome BiomeType, float Humidity);

	TArray<FBlockData> Blocks;

	TArray<FIntVector> WaterBlockPositions;

	void GenerateTrees(TArray<FIntVector> LocalTreePositions);

	TArray<FDecorationData> GetFloraPositions() const;
	//void GenerateFlora(TArray<FIntVector> LocalFloraPositions);

	void RegenerateChunkBlockTextures();
	int GetTextureIndex(EBlock Block, FVector Normal) const;



protected:
	// Called when the game starts or when spawned
	void BeginPlay() ;

	void GenerateHeightMap(const FVector Position);
	void GenerateWaterAndHumidity(const FVector Position);



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

	void CreateQuad(const FBlockData BlockData, const FIntVector AxisMask, int Width, int Height, const FIntVector V1, const FIntVector V2, const FIntVector V3, const FIntVector V4, FChunkMeshData& MeshData, int& VertexCount);

	std::array<FVector2D, 4> GetUVMapping(const FBlockData& BlockData, const FVector& NormalVector, int Width, int Height);

	bool CompareMask(FMask M1, FMask M2) const;

	TArray<FIntVector> TreePositions;
	TArray<FDecorationData> FloraPositions;

	void PrintMeshData(bool isLandMesh) const;
	void UpdateWaterMesh();

};

