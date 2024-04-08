// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Chunk.generated.h"

enum class EBlock;
enum class EDirection;
class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class TERRAINGENLITE1_API AChunk : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChunk();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Chunk")
	//Size of the chunk in x,y,z
	int Size = 32;

	UPROPERTY(EditAnywhere, Category = "Chunk")
	//Scale of the faces drawn in a chunk
	int Scale = 1;

private:
	TObjectPtr<UProceduralMeshComponent> Mesh;
	TObjectPtr<FastNoiseLite> Noise;

	TArray<EBlock> Blocks;

	TArray<FVector> VertexData;
	TArray<int> TriangleData;
	TArray<FVector2D> UVData;

	int VertexCount = 0;


	const FVector BlockVertexData[8] = {
	  FVector(100,100,100),
	  FVector(100,0,100),
	  FVector(100,0,0),
	  FVector(100,100,0),
	  FVector(0,0,100),
	  FVector(0,100,100),
	  FVector(0,100,0),
	  FVector(0,0,0)
	};

	const int BlockTriangleData[24] = {
	  0,1,2,3, // Forward
	  5,0,3,6, // Right
	  4,5,6,7, // Back
	  1,4,7,2, // Left
	  5,4,1,0, // Up
	  3,2,7,6  // Down
	};

	//Populate blocks array according to heightmap generated by noise library
	void GenerateBlocks();

	//Populate vertex data, create the Mesh
	void GenerateMesh();

	//Takes vertex and index data, passes to procedural mesh component (rendering happens here)
	void ApplyMesh();

	//Checks if a block is solid or transparent
	bool Check(FVector Position) const;

	//Adding vertex and Index data, to create Face
	void CreateFace(EDirection Direction, FVector Position);

	//Given Face, Direction and Position and uses the lookup table (BlockVertexData) to get the four (vertices) corners of a face 
	TArray<FVector> GetFaceVertices(EDirection Direction, FVector Position) const;

	//Finds an adjacent Position to the given one in the specified Direction
	FVector GetPositionInDirection(EDirection Direction, FVector Position) const;

	//Flattens the 3D(x,y,z coord.) information into a 1D index for Blocks array, returns correct array index for specifed Block
	int GetBlockIndex(int X, int Y, int Z) const;
};
