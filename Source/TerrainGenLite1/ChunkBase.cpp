// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkBase.h"

#include "FastNoiseLite.h"
#include "CollisionQueryParams.h"
#include "Engine/CollisionProfile.h"
#include "VoxelFunctionLibrary.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AChunkBase::AChunkBase()
	: LandMesh(CreateDefaultSubobject<UProceduralMeshComponent>("LandMesh")),
	LiquidMesh(CreateDefaultSubobject<UProceduralMeshComponent>("LiquidMesh")),
	Noise(MakeUnique<FastNoiseLite>())
{
	PrimaryActorTick.bCanEverTick = false;  // Set the tick behavior

	SetRootComponent(LandMesh);
	LiquidMesh->SetupAttachment(LandMesh);
}



// Called when the game starts or when spawned
void AChunkBase::BeginPlay()
{
	Super::BeginPlay();

	Noise->SetFrequency(Frequency);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);

	// Initialize Blocks
	Blocks.SetNum(Size * Size * Size);

	GenerateChunk();

}

void AChunkBase::GenerateChunk()
{
	//Generate Land
	GenerateHeightMap(GetActorLocation() / 100);
	GenerateMesh(true);
	UE_LOG(LogTemp, Warning, TEXT("Land Vertex Count : %d"), LandVertexCount);
	ApplyMesh(true);
	//PrintMeshData(true); // Print land mesh data after generation

	//Generate Liquid
	GenerateWater(GetActorLocation() / 100);
	GenerateMesh(false);
	UE_LOG(LogTemp, Warning, TEXT("Liquid Vertex Count : %d"), LiquidVertexCount);
	ApplyMesh(false);
	//PrintMeshData(false); // Print liquid mesh data after generation
}


void AChunkBase::GenerateWater(const FVector Position)
{
	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			bool foundWaterSurface = false;  // Track if we've found the water surface for this column
			int waterDepth = 0;  // Track the depth of the current water column

			for (int z = 0; z < Size; ++z)
			{
				const double Zpos = z + Position.Z;

				if (Zpos < 15)
				{
					// Check if the block is air and within certain Z range
					if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Air && z < WaterLevel)
					{


						if (!foundWaterSurface)
						{
							foundWaterSurface = true;  // Mark that we found the surface for this column
						}

						waterDepth++;  // Increment water depth

						if (waterDepth >= 5)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::DeepWater;  // Set deep water type
						}
						else
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::ShallowWater;  // Set shallow water type
						}

						Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;

					}
					else
					{
						foundWaterSurface = false;  // Reset if we encounter a non-water block
						waterDepth = 0;  // Reset water depth
					}
				}
				else
				{
					foundWaterSurface = false;  // Reset if we are above the water threshold
					waterDepth = 0;  // Reset water depth
				}
			}
		}
	}
}



void AChunkBase::GenerateHeightMap(const FVector Position)
{
	UE_LOG(LogTemp, Warning, TEXT("Generating 3D Height Map"));

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			const float Xpos = x + Position.X;
			const float Ypos = y + Position.Y;
			const float SurfaceHeight = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(Xpos, Ypos) + 1) * Size / 2), 0, Size);

			for (int z = 0; z < Size; ++z)
			{
				const double Zpos = z + Position.Z;

				const auto NoiseValue = Noise->GetNoise(x + Position.X, y + Position.Y, Zpos);
				int BaseZ = DrawDistance - (DrawDistance * 2);

				if (z == 0 && ZRepeat == BaseZ)
				{
					Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Bedrock;
					Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
				}
				else if (NoiseValue >= 0 && Zpos <= SurfaceHeight - 7)
				{
					Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Air;
					Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;
				}
				else
				{
					if (Zpos < SurfaceHeight - 3)
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Stone;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
					}
					else if (Zpos < SurfaceHeight - 1)
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Dirt;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
					}
					else if (Zpos == SurfaceHeight - 1)
					{
						int randNum = FMath::FRandRange(1, 51);
						if (randNum == 1 && Zpos > 15)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Dirt;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
							TreePositions.Add(FIntVector(x, y, z));
						}
						else
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Grass;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
						}
					}
					else
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Air;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;
					}
					if (Zpos < 15)
					{
						/*if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Air)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::ShallowWater;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;
						}*/
						if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Dirt && Zpos > 10)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Sand;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
						}
						else if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Dirt || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Gravel;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
						}
					}
				}
			}
		}
	}
	GenerateTrees(TreePositions);
}



/**
 * @brief Generates a mesh using the greedy meshing algorithm.
 *
 * This function iterates over each axis of the chunk (X, Y, Z) and creates
 * quads for the visible faces of the blocks. It reduces the number of polygons
 * by merging adjacent blocks that share the same properties.
 */
void AChunkBase::GenerateMesh(bool isLandMesh)
{
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		const int MainAxisLimit = Size;
		const int Axis1Limit = Size;
		const int Axis2Limit = Size;

		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		auto ChunkItr = FIntVector::ZeroValue;
		auto AxisMask = FIntVector::ZeroValue;

		AxisMask[Axis] = 1;

		TArray<FBlockData> BlockData;
		BlockData.SetNum(Axis1Limit * Axis2Limit);

		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const auto CurrentBlock = GetBlock(ChunkItr);
					const auto CompareBlock = GetBlock(ChunkItr + AxisMask);

					/*
					*
					* Checks if Block faces air, another opaque block or water to ensure proper face display. Doesnt lose whole chunk.
					*
					*/

					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air && CurrentBlock != EBlock::ShallowWater && CurrentBlock != EBlock::DeepWater;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air && CompareBlock != EBlock::ShallowWater && CompareBlock != EBlock::DeepWater;

					const bool CurrentBlockIsLiquid = CurrentBlock == EBlock::ShallowWater || CurrentBlock == EBlock::DeepWater;
					const bool CompareBlockIsLiquid = CompareBlock == EBlock::ShallowWater || CompareBlock == EBlock::DeepWater;

					/*UE_LOG(LogTemp, Log, TEXT("CurrentBlock: %d, CompareBlock: %d"), static_cast<int>(CurrentBlock), static_cast<int>(CompareBlock));
					UE_LOG(LogTemp, Log, TEXT("CurrentBlockOpaque: %s, CompareBlockOpaque: %s"), CurrentBlockOpaque ? TEXT("true") : TEXT("false"), CompareBlockOpaque ? TEXT("true") : TEXT("false"));
					UE_LOG(LogTemp, Log, TEXT("CurrentBlockIsLiquid: %s, CompareBlockIsLiquid: %s"), CurrentBlockIsLiquid ? TEXT("true") : TEXT("false"), CompareBlockIsLiquid ? TEXT("true") : TEXT("false"));*/

					if (CurrentBlockOpaque == CompareBlockOpaque && CurrentBlockIsLiquid == CompareBlockIsLiquid)
					{
						BlockData[N++].Mask = FMask{ EBlock::Null, 0 };
					}
					else if (CurrentBlockOpaque)
					{
						BlockData[N++].Mask = FMask{ CurrentBlock, 1 };
					}
					else if (CurrentBlockIsLiquid)
					{
						BlockData[N++].Mask = FMask{ CurrentBlock, 1 };
					}
					else
					{
						//UE_LOG(LogTemp, Error, TEXT("BlockData is NULL"));
						BlockData[N++].Mask = FMask{ CompareBlock, -1 };
					}
					/*
					* 
					* End Point
					* 
					*/
				}
			}

			++ChunkItr[Axis];
			N = 0;

			for (int j = 0; j < Axis2Limit; ++j)
			{
				for (int i = 0; i < Axis1Limit;)
				{
					if (BlockData[N].Mask.Normal != 0)
					{
						const auto& CurrentMask = BlockData[N];
						ChunkItr[Axis1] = i;
						ChunkItr[Axis2] = j;

						int Width;

						for (Width = 1; i + Width < Axis1Limit && CompareMask(BlockData[N + Width].Mask, CurrentMask.Mask); ++Width);

						int Height;
						bool Done = false;

						for (Height = 1; j + Height < Axis2Limit; ++Height)
						{
							for (int k = 0; k < Width; ++k)
							{
								if (CompareMask(BlockData[N + k + Height * Axis1Limit].Mask, CurrentMask.Mask)) continue;

								Done = true;
								break;
							}

							if (Done) break;
						}

						DeltaAxis1[Axis1] = Width;
						DeltaAxis2[Axis2] = Height;

						/*
						* 
						* Ensures the Land and Liquid Mesh dont get each others blocks, dont get duplicated
						* 
						*/
						FChunkMeshData& MeshData = isLandMesh ? LandMeshData : LiquidMeshData;
						int& VertexCount = isLandMesh ? LandVertexCount : LiquidVertexCount;

						// Check if the block is water
						bool isWaterBlock = CurrentMask.Mask.BlockType == EBlock::ShallowWater || CurrentMask.Mask.BlockType == EBlock::DeepWater;

						// Check if the mesh type matches the block type
						if ((isWaterBlock && !isLandMesh) || (!isWaterBlock && isLandMesh))
						{
							CreateQuad(
								CurrentMask,
								AxisMask,
								Width,
								Height,
								ChunkItr,
								ChunkItr + DeltaAxis1,
								ChunkItr + DeltaAxis2,
								ChunkItr + DeltaAxis1 + DeltaAxis2,
								MeshData,
								VertexCount
							);
						}
						/*
						*
						* End Point
						*
						*/

						DeltaAxis1 = FIntVector::ZeroValue;
						DeltaAxis2 = FIntVector::ZeroValue;

						for (int l = 0; l < Height; ++l)
						{
							for (int k = 0; k < Width; ++k)
							{
								BlockData[N + k + l * Axis1Limit].Mask = FMask{ EBlock::Null, 0 };
							}
						}

						i += Width;
						N += Width;
					}
					else
					{
						i++;
						N++;
					}
				}
			}
		}
	}
}




/**
 * @brief Creates a quad and adds it to the mesh data.
 *
 * This function handles the vertices, normals, colors, and UV coordinates for
 * the quad based on the specified dimensions and vertex positions.
 *
 * @param Mask The mask containing block type and normal direction.
 * @param AxisMask The mask indicating the current axis being processed.
 * @param Width The width of the quad.
 * @param Height The height of the quad.
 * @param V1 The first vertex position of the quad.
 * @param V2 The second vertex position of the quad.
 * @param V3 The third vertex position of the quad.
 * @param V4 The fourth vertex position of the quad.
 */
void AChunkBase::CreateQuad(
	const FBlockData BlockData,
	const FIntVector AxisMask,
	int Width,
	int Height,
	const FIntVector V1,
	const FIntVector V2,
	const FIntVector V3,
	const FIntVector V4,
	FChunkMeshData& MeshData,
	int& VertexCount
)
{
	if (BlockData.Mask.BlockType == EBlock::Air || BlockData.Mask.BlockType == EBlock::Null)
	{
		return; // Skip empty or non-existent blocks
	}

	const auto NormalVector = FVector(AxisMask * BlockData.Mask.Normal);
	auto Color = FColor(0, 0, 0, GetTextureIndex(BlockData.Mask.BlockType, NormalVector));


	MeshData.Vertices.Append({
		FVector(V1) * 100,
		FVector(V2) * 100,
		FVector(V3) * 100,
		FVector(V4) * 100
		});

	MeshData.Triangles.Append({
	   VertexCount,
	   VertexCount + 2 + BlockData.Mask.Normal,
	   VertexCount + 2 - BlockData.Mask.Normal,
	   VertexCount + 3,
	   VertexCount + 1 - BlockData.Mask.Normal,
	   VertexCount + 1 + BlockData.Mask.Normal
		});

	MeshData.Normals.Append({
		NormalVector,
		NormalVector,
		NormalVector,
		NormalVector
		});

	MeshData.Colors.Append({
		Color,
		Color,
		Color,
		Color
		});

	if (NormalVector.X == 1 || NormalVector.X == -1)
	{
		MeshData.UV0.Append({
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0)
			});
	}
	else
	{
		MeshData.UV0.Append({
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0)
			});
	}

	MeshData.BlockData.Append({
		BlockData,
		BlockData,
		BlockData,
		BlockData
		});

	VertexCount += 4;

}


void AChunkBase::ApplyMesh(bool isLandMesh) const
{
	const FChunkMeshData& MeshData = isLandMesh ? LandMeshData : LiquidMeshData;
	UProceduralMeshComponent* MeshComponent = isLandMesh ? LandMesh : LiquidMesh;
	int SectionIndex = isLandMesh ? 0 : 1;

	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Mesh component is null for %s mesh"), isLandMesh ? TEXT("Land") : TEXT("Liquid"));
		return;
	}

	// Create mesh section
	MeshComponent->CreateMeshSection(
		SectionIndex,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.Colors,
		TArray<FProcMeshTangent>(),
		true
	);

	// Set material (assuming you have different materials for land and liquid meshes)
	UMaterialInterface* MeshMaterial = isLandMesh ? LandMaterial : LiquidMaterial;
	MeshComponent->SetMaterial(SectionIndex, MeshMaterial);


	// Set collision settings for the mesh sections
	if (!isLandMesh)
	{
		MeshComponent->SetCollisionProfileName(TEXT("WaterMesh"));
		MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap); 
	}
	else
	{
		MeshComponent->SetCollisionProfileName(TEXT("LandMesh"));
	}
}




void AChunkBase::ClearMesh(bool isLandMesh)
{
	FChunkMeshData& MeshData = isLandMesh ? LandMeshData : LiquidMeshData;
	int& VertexCount = isLandMesh ? LandVertexCount : LiquidVertexCount;

	VertexCount = 0;
	MeshData.Clear();
}


void AChunkBase::ModifyVoxel(const FIntVector Position, const EBlock Block)
{
	if (Position.X >= Size || Position.Y >= Size || Position.Z >= Size || Position.X < 0 || Position.Y < 0 || Position.Z < 0)
		return;

	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
	if (Blocks[Index].Mask.BlockType != Block)
	{
		// Only modify if the block type is different
		ModifyVoxelData(Position, Block);

		// Regenerate land mesh
		ClearMesh(true);
		GenerateMesh(true);
		ApplyMesh(true);


		
		ClearMesh(false);
		GenerateMesh(false);
		ApplyMesh(false);
	}
}





void AChunkBase::ModifyVoxelData(const FIntVector Position, const EBlock Block)
{
	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
	UE_LOG(LogTemp, Warning, TEXT("X: %d, Y: %d, Z: %d"), Position.X, Position.Y, Position.Z);
	Blocks[Index].Mask.BlockType = Block;
}

int AChunkBase::GetBlockIndex(const int X, const int Y, const int Z) const
{
	return Z * Size * Size + Y * Size + X;
}

EBlock AChunkBase::GetBlock(const FIntVector Index) const
{
	if (Index.X >= Size || Index.Y >= Size || Index.Z >= Size || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)].Mask.BlockType;
}


bool AChunkBase::CompareMask(const FMask M1, const FMask M2) const
{
	return M1.BlockType == M2.BlockType && M1.Normal == M2.Normal;
}

int AChunkBase::GetTextureIndex(const EBlock Block, const FVector Normal) const
{
	switch (Block) {
	case EBlock::Grass:
	{
		if (Normal == FVector::UpVector) return 0;
		return 1;
	}
	case EBlock::Dirt: return 2;
	case EBlock::Stone: return 3;
	case EBlock::Bedrock: return 4;
	case EBlock::Log:return 5;
	case EBlock::Leaves:return 6;
	case EBlock::Sand:return 7;
	case EBlock::Gravel:return 8;
	case EBlock::ShallowWater:return 9;
	case EBlock::DeepWater:return 10;
	default: return 255;
	}
}

void AChunkBase::GenerateTrees(TArray<FIntVector> LocalTreePositions)
{
	// Define tree height
	int TreeHeight = 5;

	for (const FIntVector& Position : LocalTreePositions)
	{
		int X = Position.X;
		int Y = Position.Y;
		int Z = Position.Z;

		// Place the trunk
		for (int i = 0; i < TreeHeight; ++i)
		{
			if (Z + i < Size)
			{
				const int Index = GetBlockIndex(X, Y, Z + i);
				Blocks[Index].Mask.BlockType = EBlock::Log;
				Blocks[Index].bIsSolid = true;
			}
		}

		// Place the leaves

		// Adjust the radius of the leaf canopy
		int LeafRadius = 2;

		// Start leaves from just below the top of the trunk
		for (int dz = TreeHeight - 1; dz <= TreeHeight + LeafRadius; ++dz)
		{
			// Randomize X and Y placement within the leaf radius
			for (int dx = -LeafRadius; dx <= LeafRadius; ++dx)
			{
				for (int dy = -LeafRadius; dy <= LeafRadius; ++dy)
				{
					// Ensure leaf placement forms a circular shape
					if (FMath::Abs(dx) + FMath::Abs(dy) <= LeafRadius)
					{
						if (X + dx >= 0 && X + dx < Size && Y + dy >= 0 && Y + dy < Size && Z + dz >= 0 && Z + dz < Size)
						{
							const int Index = GetBlockIndex(X + dx, Y + dy, Z + dz);
							Blocks[Index].Mask.BlockType = EBlock::Leaves;
							Blocks[Index].bIsSolid = true;

						}
					}
				}
			}
		}
	}
}


void AChunkBase::PrintMeshData(bool isLandMesh) const
{
	const FChunkMeshData& MeshData = isLandMesh ? LandMeshData : LiquidMeshData;

	// Log mesh data details
	UE_LOG(LogTemp, Warning, TEXT("Printing Mesh Data for %s:"), isLandMesh ? TEXT("Land Mesh") : TEXT("Liquid Mesh"));

	// Log vertices
	UE_LOG(LogTemp, Warning, TEXT("Vertices (%d):"), MeshData.Vertices.Num());
	for (const FVector& Vertex : MeshData.Vertices)
	{
		UE_LOG(LogTemp, Warning, TEXT("Vertex: (%f, %f, %f)"), Vertex.X, Vertex.Y, Vertex.Z);
	}

	// Log block data
	UE_LOG(LogTemp, Warning, TEXT("Block Data (%d):"), MeshData.BlockData.Num());
	for (const FBlockData& Block : MeshData.BlockData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Block Type: %d, Normal: %d"), static_cast<int32>(Block.Mask.BlockType), Block.Mask.Normal);
	}
}
