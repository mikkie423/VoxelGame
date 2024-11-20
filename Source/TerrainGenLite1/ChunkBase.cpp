// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkBase.h"

#include "FastNoiseLite.h"
#include "CollisionQueryParams.h"
#include "Engine/CollisionProfile.h"
#include "VoxelFunctionLibrary.h"
#include "Math/Vector2D.h"
#include <array>
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

void AChunkBase::NotifyMeshUpdated()
{
	OnChunkMeshUpdated.Broadcast();
}

// Called when the game starts or when spawned
void AChunkBase::BeginPlay()
{
	Super::BeginPlay();

	Noise->SetSeed(WorldSeed);
	Noise->SetFrequency(Frequency);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);

	// Initialize Blocks
	Blocks.SetNum(ChunkSize * ChunkSize * ChunkSize);

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
	//GenerateWaterAndHumidity(GetActorLocation() / 100);
	GenerateMesh(false);
	UE_LOG(LogTemp, Warning, TEXT("Liquid Vertex Count : %d"), LiquidVertexCount);
	ApplyMesh(false);
	//PrintMeshData(false); // Print liquid mesh data after generation
}


void AChunkBase::SetBiome(int32 X, int32 Y, int32 Z, EBiome BiomeType, float Humidity)
{
	// Set biome type for the block at (X, Y, Z)
	Blocks[GetBlockIndex(X, Y, Z)].BiomeType = BiomeType;
	Blocks[GetBlockIndex(X, Y, Z)].Humidity = Humidity;
	int randNum;


	switch (BiomeType)
	{
	case EBiome::Null:

		break;
	case EBiome::Desert:
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass || Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::DryDirt)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Sand;
		}

		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::ShallowWater || Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::DeepWater)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Sand;
		}
		break;
	case EBiome::Swamp:
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass)
		{
			randNum = FMath::FRandRange(1, 81);
			if (randNum == 1)
			{
				TreePositions.Add(FIntVector(X, Y, Z));
			}
		}
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Swamp;
		}
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Sand && Z < 15)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::WetDirt;
		}
		break;
	case EBiome::Tundra:
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass || Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Sand)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Tundra;
		}
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::ShallowWater || Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::DeepWater)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Ice;
		}
		break;
	case EBiome::Taiga:
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass)
		{
			randNum = FMath::FRandRange(1, 31);
			if (randNum == 1)
			{
				TreePositions.Add(FIntVector(X, Y, Z));
			}
		}
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Taiga;
		}
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Sand)
		{
			Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType = EBlock::Gravel;
		}
		break;
	case EBiome::Plains:
		if (Blocks[GetBlockIndex(X, Y, Z)].Mask.BlockType == EBlock::Grass)
		{
			randNum = FMath::FRandRange(1, 101);
			if (randNum == 1)
			{
				TreePositions.Add(FIntVector(X, Y, Z));
			}
			else
			{
				randNum = FMath::FRandRange(1, 8);
				if (randNum == 1 || randNum == 3 || randNum == 5)
				{
					
					FDecorationData DecorationData;
					DecorationData.Position = FIntVector(X, Y, Z + 1);
					DecorationData.DecorationBlockType = EBlock::ShortGrass;
					DecorationData.TextureIndex = 20;

					FloraPositions.Add(DecorationData);

					UE_LOG(LogTemp, Warning, TEXT("ChunkBase: Flora Position added at: %d, %d, %d"), X, Y, Z);
				}
			}
		}
		break;
	default:
		break;
	}
	GenerateTrees(TreePositions);
}



void AChunkBase::GenerateHeightMap(const FVector Position)
{
	UE_LOG(LogTemp, Warning, TEXT("Generating 3D Height Map"));

	for (int x = 0; x < ChunkSize; ++x)
	{
		for (int y = 0; y < ChunkSize; ++y)
		{
			const float Xpos = x + Position.X;
			const float Ypos = y + Position.Y;
			const float SurfaceHeight = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(Xpos, Ypos) + 1) * ChunkSize / 2), 0, ChunkSize);

			for (int z = 0; z < ChunkSize; ++z)
			{
				const double Zpos = z + Position.Z;

				const auto NoiseValue = Noise->GetNoise(x + Position.X, y + Position.Y, Zpos);
				int BaseZ = DrawDistance - (DrawDistance * 2);

				if (z == 0)
				{
					Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Bedrock;
					Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
					Blocks[GetBlockIndex(x, y, z)].BlockHardness = 1.0f;
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
						Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.5f;
					}
					else if (Zpos < SurfaceHeight - 1)
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::DryDirt;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
						Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.2f;

					}
					else if (Zpos == SurfaceHeight - 1)
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Grass;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
						Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.2f;
					}
					else
					{
						Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Air;
						Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;
					}
					if (Zpos < WaterLevel)
					{
						// Check if the block is air and within certain Z range
						if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Air)
						{
							if (z < WaterLevel && z >= WaterLevel - 5)
							{
								Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::ShallowWater;
							}
							else if (z < WaterLevel - 5)
							{
								Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::DeepWater;
							}

							Blocks[GetBlockIndex(x, y, z)].bIsSolid = false;
							Blocks[GetBlockIndex(x, y, z)].Humidity = 1.0f;
							WaterBlockPositions.Add(FIntVector(x, y, z));
						}
						if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::DryDirt && Zpos > 10)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Sand;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
							Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.2f;
						}
						else if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::DryDirt || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Gravel;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
							Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.2f;

						}
						else if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::DryDirt || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::WetDirt;
							Blocks[GetBlockIndex(x, y, z)].bIsSolid = true;
							Blocks[GetBlockIndex(x, y, z)].BlockHardness = 0.2f;

						}
					}
				}
			}
		}
	}
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
	UE_LOG(LogTemp, Warning, TEXT("Generating Mesh"));

	// Loop through the three axes
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		const int MainAxisLimit = ChunkSize;
		const int Axis1Limit = ChunkSize;
		const int Axis2Limit = ChunkSize;

		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		auto ChunkItr = FIntVector::ZeroValue;
		auto AxisMask = FIntVector::ZeroValue;
		AxisMask[Axis] = 1;

		// Iterate through each block in the chunk
		TArray<FBlockData> BlockData;
		BlockData.SetNum(Axis1Limit * Axis2Limit);

		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			// Iterate through Axis2 and Axis1
			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const auto CurrentBlock = GetBlockType(ChunkItr);
					const auto CompareBlock = GetBlockType(ChunkItr + AxisMask);

					// Determine if the current and compare blocks are opaque or liquid
					const bool CurrentBlockIsOpaque = CurrentBlock != EBlock::Air && CurrentBlock != EBlock::ShallowWater && CurrentBlock != EBlock::DeepWater;
					const bool CompareBlockIsOpaque = CompareBlock != EBlock::Air && CompareBlock != EBlock::ShallowWater && CompareBlock != EBlock::DeepWater;

					const bool CurrentBlockIsLiquid = CurrentBlock == EBlock::ShallowWater || CurrentBlock == EBlock::DeepWater;
					const bool CompareBlockIsLiquid = CompareBlock == EBlock::ShallowWater || CompareBlock == EBlock::DeepWater;

					if (CurrentBlockIsOpaque && CompareBlockIsLiquid)
					{
						// Current block is land, compare block is water: prioritize land
						BlockData[N++].Mask = FMask{ CurrentBlock, 1 };
					}
					else if (CurrentBlockIsLiquid && CompareBlockIsOpaque)
					{
						// Current block is water, compare block is land: prioritize land
						BlockData[N++].Mask = FMask{ CompareBlock, -1 };
					}
					else if (CurrentBlockIsOpaque == CompareBlockIsOpaque && CurrentBlockIsLiquid == CompareBlockIsLiquid)
					{
						// Both blocks are of the same type (either both land or both water)
						BlockData[N++].Mask = FMask{ EBlock::Null, 0 };
					}
					else if (CurrentBlockIsOpaque)
					{
						// Only current block is opaque
						BlockData[N++].Mask = FMask{ CurrentBlock, 1 };
					}
					else if (CurrentBlockIsLiquid)
					{
						// Only current block is liquid
						BlockData[N++].Mask = FMask{ CurrentBlock, 1 };
					}
					else
					{
						// Default case: use the compare block type
						BlockData[N++].Mask = FMask{ CompareBlock, -1 };
					}
				}
			}

			// Increment ChunkItr[Axis] and reset N
			++ChunkItr[Axis];
			N = 0;

			// Loop through Axis2Limit and Axis1Limit
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

						for (Width = 1; i + Width < Axis1Limit && CompareMask(BlockData[N + Width].Mask, CurrentMask.Mask); ++Width)
						{
						}

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

						// Determine if the block is water
						bool isWaterBlock = CurrentMask.Mask.BlockType == EBlock::ShallowWater || CurrentMask.Mask.BlockType == EBlock::DeepWater;

						// Check if the mesh type matches the block type
						if (isWaterBlock && isLandMesh)
						{
							UE_LOG(LogTemp, Warning, TEXT("Skip quad creation for water block in land mesh"));
						}
						else
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
								isLandMesh ? LandMeshData : LiquidMeshData,
								isLandMesh ? LandVertexCount : LiquidVertexCount
							);
						}

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
	// Skip empty or non-existent blocks
	if (BlockData.Mask.BlockType == EBlock::Air || BlockData.Mask.BlockType == EBlock::Null)
	{
		return;
	}

	// Calculate the normal vector based on the axis mask
	const auto NormalVector = FVector(AxisMask * BlockData.Mask.Normal);
	auto Color = FColor(0, 0, 0, GetTextureIndex(BlockData.Mask.BlockType, NormalVector));


		MeshData.Vertices.Append({
			FVector(V1) * 100,
			FVector(V2) * 100,
			FVector(V3) * 100,
			FVector(V4) * 100
			});

		// Define triangles
		MeshData.Triangles.Append({
			VertexCount,
			VertexCount + 2 + BlockData.Mask.Normal,
			VertexCount + 2 - BlockData.Mask.Normal,
			VertexCount + 3,
			VertexCount + 1 - BlockData.Mask.Normal,
			VertexCount + 1 + BlockData.Mask.Normal
			});

		// Apply normals and colors
		MeshData.Normals.Append({
			NormalVector, NormalVector, NormalVector, NormalVector
			});

		MeshData.Colors.Append({
			Color, Color, Color, Color
			});

		auto UVs = GetUVMapping(BlockData, NormalVector, Width, Height);
		MeshData.UV0.Append(UVs.data(), UVs.size());

		// Append block data
		MeshData.BlockData.Append({
			BlockData,
			BlockData,
			BlockData,
			BlockData
			});

		VertexCount += 4; // Increment for 4 new vertices added
}


std::array<FVector2D, 4> AChunkBase::GetUVMapping(const FBlockData& BlockData, const FVector& NormalVector, int Width, int Height)
{
	std::array<FVector2D, 4> UVs;

	if (NormalVector.X == 1 || NormalVector.X == -1)
	{
		UVs = {
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0)
		};
	}
	else
	{
		UVs = {
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0)
		};
	}

	return UVs;
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
	if (SectionIndex == 1)
	{
		MeshComponent->SetCollisionProfileName(TEXT("WaterMesh"));
		MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
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
	if (Position.X >= ChunkSize || Position.Y >= ChunkSize || Position.Z >= ChunkSize || Position.X < 0 || Position.Y < 0 || Position.Z < 0)
		return;

	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
	if (Blocks[Index].Mask.BlockType != Block)
	{
		// Only modify if the block type is different
		ModifyVoxelData(Position, Block);

		RegenerateChunkBlockTextures();

		// Notify that the chunk's mesh has been updated
		NotifyMeshUpdated();
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
	return Z * ChunkSize * ChunkSize + Y * ChunkSize + X;
}

EBlock AChunkBase::GetBlockType(const FIntVector Index) const
{
	if (Index.X >= ChunkSize || Index.Y >= ChunkSize || Index.Z >= ChunkSize || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)].Mask.BlockType;
}

float AChunkBase::GetBlockHardnessScale(const FIntVector Index) const
{
	if (Index.X >= ChunkSize || Index.Y >= ChunkSize || Index.Z >= ChunkSize || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return 0.0f;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)].BlockHardness;
}

FBlockData AChunkBase::GetBlockData(const FIntVector Index) const
{
	if (Index.X >= ChunkSize || Index.Y >= ChunkSize || Index.Z >= ChunkSize || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return FBlockData();
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
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
		if (Normal == FVector::DownVector) return 2;
		return 1;
	}
	case EBlock::DryDirt: return 2;
	case EBlock::Stone: return 3;
	case EBlock::Bedrock: return 4;
	case EBlock::Log:
		if (Normal == FVector::UpVector || Normal == FVector::DownVector) return 6;
		return 5;
	case EBlock::WoodPlanks: return 7;
	case EBlock::Leaves:return 8;
	case EBlock::Sand:return 9;
	case EBlock::Gravel:return 10;
	case EBlock::ShallowWater:return 11;
	case EBlock::DeepWater:return 12;
	case EBlock::Swamp:return 13;
	case EBlock::Taiga:return 14;
	case EBlock::Tundra:return 15;
	case EBlock::Ice:return 16;
	case EBlock::WetDirt: return 17;
	case EBlock::WetFarmland:
	{
		if (Normal == FVector::UpVector) return 18;
		return 17; // wetdirt sides and base
	}
	case EBlock::DryFarmland:
	{
		if (Normal == FVector::UpVector) return 19;
		return 2; // drydirt sides and base
	}
	case EBlock::ShortGrass: return 20;
	case EBlock::Seeds: return 21;
	case EBlock::Torch: return 22;
	default: return 255;
	}
}

void AChunkBase::GenerateTrees(TArray<FIntVector> LocalTreePositions)
{
	// Defaults
	int DefaultTreeHeight = 5;
	EBlock DefaultLog = EBlock::Log;
	EBlock DefaultLeaves = EBlock::Leaves;

	for (const FIntVector& Position : LocalTreePositions)
	{
		int X = Position.X;
		int Y = Position.Y;
		int Z = Position.Z;

		EBiome CurrentBiome = Blocks[GetBlockIndex(X, Y, Z)].BiomeType;

		int TreeHeight = DefaultTreeHeight;
		EBlock Log = DefaultLog;
		EBlock Leaves = DefaultLeaves;


		switch (CurrentBiome)
		{
		case EBiome::Null:
			break;
		case EBiome::Desert:
			break;
		case EBiome::Swamp:
			TreeHeight = 7;
			Log = EBlock::Log;
			Leaves = EBlock::Leaves;
			break;
		case EBiome::Tundra:
			break;
		case EBiome::Taiga:
			TreeHeight = 10;
			Log = EBlock::Log;
			Leaves = EBlock::Leaves;
			break;
		case EBiome::Plains:
			break;
		default:
			break;
		}


		/********************************** Generating Trees **********************************************/
		// Place the trunk
		for (int i = 0; i < TreeHeight; ++i)
		{
			if (Z + i < ChunkSize)
			{
				const int Index = GetBlockIndex(X, Y, Z + i);
				Blocks[Index].Mask.BlockType = EBlock::Log;
				Blocks[Index].bIsSolid = true;
				Blocks[Index].BlockHardness = 0.3f;

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
						if (X + dx >= 0 && X + dx < ChunkSize && Y + dy >= 0 && Y + dy < ChunkSize && Z + dz >= 0 && Z + dz < ChunkSize)
						{
							const int Index = GetBlockIndex(X + dx, Y + dy, Z + dz);
							Blocks[Index].Mask.BlockType = EBlock::Leaves;
							Blocks[Index].bIsSolid = true;
							Blocks[Index].BlockHardness = 0.1f;
						}
					}
				}
			}
		}

	}
}

TArray<FDecorationData> AChunkBase::GetFloraPositions() const
{
	return FloraPositions; // Assuming FloraPositions is populated during biome assignment
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

void AChunkBase::RegenerateChunkBlockTextures()
{
	UE_LOG(LogTemp, Warning, TEXT("Regenerating Block Textures"));

	ClearMesh(true);
	ClearMesh(false);
	UpdateWaterMesh();
	GenerateMesh(true);
	GenerateMesh(false);
	ApplyMesh(true);
	ApplyMesh(false);
}

void AChunkBase::UpdateWaterMesh()
{
	UE_LOG(LogTemp, Warning, TEXT("UpdateWaterMesh"));

	for (int x = 0; x < ChunkSize; ++x)
	{
		for (int y = 0; y < ChunkSize; ++y)
		{
			for (int z = 0; z < ChunkSize; ++z)
			{
				const auto BlockType = Blocks[GetBlockIndex(x, y, z)].Mask.BlockType;

				if (BlockType == EBlock::ShallowWater || BlockType == EBlock::DeepWater)
				{
					// Check surrounding blocks
					for (int dx = -1; dx <= 1; ++dx)
					{
						for (int dy = -1; dy <= 1; ++dy)
						{
							for (int dz = -1; dz <= 1; ++dz)
							{
								if (dx == 0 && dy == 0 && dz == 0)
									continue;

								int nx = x + dx;
								int ny = y + dy;
								int nz = z + dz;

								if (nx >= 0 && ny >= 0 && nz >= 0 && nx < ChunkSize && ny < ChunkSize && nz < ChunkSize)
								{
									auto NeighborBlockType = Blocks[GetBlockIndex(nx, ny, nz)].Mask.BlockType;

									if (NeighborBlockType == EBlock::Air && (nz + 1) <= WaterLevel)
									{
										// Convert air pocket to water
										Blocks[GetBlockIndex(nx, ny, nz)].Mask.BlockType = BlockType;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
