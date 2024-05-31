#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkMeshData.h"
#include "Enums.h"
#include "BlockData.h"
#include "ChunkBase.generated.h"



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
	int Size = 64;

	TObjectPtr<UMaterialInterface> LandMaterial;
	TObjectPtr<UMaterialInterface> LiquidMaterial;
	float Frequency;
	int ZRepeat;
	int DrawDistance;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void ModifyVoxel(const FIntVector Position, const EBlock Block);

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	EBlock GetBlock(const FIntVector Index) const;

	void GenerateMesh();


protected:
	// Called when the game starts or when spawned
	void BeginPlay() ;

	void Generate3DHeightMap(const FVector Position);

	void ModifyVoxelData(const FIntVector Position, const EBlock Block);

	TObjectPtr<UProceduralMeshComponent> Mesh;
	FastNoiseLite* Noise;
	FChunkMeshData MeshData;
	int VertexCount = 0;

private:
	void ApplyMesh() const;
	void ClearMesh();
	void GenerateHeightMap();

	TArray<FBlockData> Blocks;

	void CreateQuad( const FBlockData BlockData, const FIntVector AxisMask, int Width, int Height, const FIntVector V1, const FIntVector V2, const FIntVector V3, const FIntVector V4 );

	int GetBlockIndex(int X, int Y, int Z) const;


	bool CompareMask(FMask M1, FMask M2) const;
	int GetTextureIndex(EBlock Block, FVector Normal) const;

	TArray<FIntVector> TreePositions;
	void GenerateTrees(TArray<FIntVector> LocalTreePositions);
};
