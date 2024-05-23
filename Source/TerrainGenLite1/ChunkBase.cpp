// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkBase.h"

#include "FastNoiseLite.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AChunkBase::AChunkBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Noise = new FastNoiseLite();

	//Mesh Settings
	Mesh->SetCastShadow(false);

	//Set Mesh as root
	SetRootComponent(Mesh);

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

	GenerateHeightMap();

	ClearMesh();

	GenerateMesh();

	UE_LOG(LogTemp, Warning, TEXT("Vertex Count : %d"), VertexCount);

	ApplyMesh();

}

void AChunkBase::GenerateHeightMap()
{
	Generate3DHeightMap(GetActorLocation() / 100);
}

void AChunkBase::Generate3DHeightMap(const FVector Position)
{
	UE_LOG(LogTemp, Warning, TEXT("Generating 3D Height Map"));

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			// Calculate surface height for this column
			const float Xpos = x + Position.X;
			const float Ypos = y + Position.Y;
			const float SurfaceHeight = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(Xpos, Ypos) + 1) * Size / 2), 0, Size);


			for (int z = 0; z < Size; ++z)
			{
				const double Zpos = z + Position.Z;

				// Calculate noise value for cave generation
				const auto NoiseValue = Noise->GetNoise(x + Position.X, y + Position.Y, Zpos);
				int BaseZ = DrawDistance - (DrawDistance * 2);

				if (z == 0 && ZRepeat == BaseZ) Blocks[GetBlockIndex(x, y, z)] = EBlock::Bedrock;

				// Apply cave generation logic based on noise value
				else if (NoiseValue >= 0 && Zpos <= SurfaceHeight - 7)
				{
					Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
				}
				else
				{
					// Adjust block types based on surface height
					if (Zpos < SurfaceHeight - 3) Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
					else if (Zpos < SurfaceHeight - 1)  Blocks[GetBlockIndex(x, y, z)] = EBlock::Dirt;
					else if (Zpos == SurfaceHeight - 1)
					{
						int randNum = FMath::FRandRange(1, 51);
						//UE_LOG(LogTemp, Warning, TEXT("RandNum = %i"), randNum);
						if (randNum == 1 && Zpos > 15)
						{
							Blocks[GetBlockIndex(x, y, z)] = EBlock::Dirt;
							TreePositions.Add(FIntVector(x, y, z));
						}
						else
						{
							Blocks[GetBlockIndex(x, y, z)] = EBlock::Grass;
						}
					}
					else Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
					if (Zpos < 15)
					{
						if (Blocks[GetBlockIndex(x, y, z)] == EBlock::Air)
						{
							Blocks[GetBlockIndex(x, y, z)] = EBlock::ShallowWater;
						}
						if (Blocks[GetBlockIndex(x, y, z)] == EBlock::Grass && Zpos > 10)
						{
							Blocks[GetBlockIndex(x, y, z)] = EBlock::Sand;
						}
						else if (Blocks[GetBlockIndex(x, y, z)] == EBlock::Dirt || Blocks[GetBlockIndex(x, y, z)] == EBlock::Grass)
						{
							Blocks[GetBlockIndex(x, y, z)] = EBlock::Gravel;
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
void AChunkBase::GenerateMesh()
{
	// Iterate over each axis (X, Y, Z) to process each plane of the chunk
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		// Define the perpendicular axes to the current axis
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		// Set the limits for iteration based on the chunk size
		const int MainAxisLimit = Size;
		const int Axis1Limit = Size;
		const int Axis2Limit = Size;

		// Initialize delta vectors for defining quads
		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		// Iterator for traversing the chunk
		auto ChunkItr = FIntVector::ZeroValue;
		// Mask to select the current axis
		auto AxisMask = FIntVector::ZeroValue;

		AxisMask[Axis] = 1;

		// Mask array to keep track of the faces to render
		TArray<FMask> Mask;
		Mask.SetNum(Axis1Limit * Axis2Limit);

		// Iterate through each slice of the chunk along the current axis
		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			// Compute the mask for the current slice
			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					// Get the current block and the adjacent block along the current axis
					const auto CurrentBlock = GetBlock(ChunkItr);
					const auto CompareBlock = GetBlock(ChunkItr + AxisMask);

					// Determine if the current and adjacent blocks are opaque
					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air;

					// Populate the mask based on the opacity comparison
					if (CurrentBlockOpaque == CompareBlockOpaque)
					{
						Mask[N++] = FMask{ EBlock::Null, 0 };
					}
					else if (CurrentBlockOpaque)
					{
						Mask[N++] = FMask{ CurrentBlock, 1 };
					}
					else
					{
						Mask[N++] = FMask{ CompareBlock, -1 };
					}
				}
			}

			++ChunkItr[Axis];
			N = 0;

			// Generate the mesh from the computed mask
			for (int j = 0; j < Axis2Limit; ++j)
			{
				for (int i = 0; i < Axis1Limit;)
				{
					if (Mask[N].Normal != 0)
					{
						const auto CurrentMask = Mask[N];
						ChunkItr[Axis1] = i;
						ChunkItr[Axis2] = j;

						int Width;

						// Determine the width of the quad
						for (Width = 1; i + Width < Axis1Limit && CompareMask(Mask[N + Width], CurrentMask); ++Width)
						{
						}

						int Height;
						bool Done = false;

						// Determine the height of the quad
						for (Height = 1; j + Height < Axis2Limit; ++Height)
						{
							for (int k = 0; k < Width; ++k)
							{
								if (CompareMask(Mask[N + k + Height * Axis1Limit], CurrentMask)) continue;

								Done = true;
								break;
							}

							if (Done) break;
						}

						// Define the quad using the delta vectors
						DeltaAxis1[Axis1] = Width;
						DeltaAxis2[Axis2] = Height;

						// Create the quad for the current face
						CreateQuad(
							CurrentMask, AxisMask, Width, Height,
							ChunkItr,
							ChunkItr + DeltaAxis1,
							ChunkItr + DeltaAxis2,
							ChunkItr + DeltaAxis1 + DeltaAxis2
						);

						// Reset the delta vectors
						DeltaAxis1 = FIntVector::ZeroValue;
						DeltaAxis2 = FIntVector::ZeroValue;

						// Clear the mask for the created quad
						for (int l = 0; l < Height; ++l)
						{
							for (int k = 0; k < Width; ++k)
							{
								Mask[N + k + l * Axis1Limit] = FMask{ EBlock::Null, 0 };
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
	const FMask Mask,
	const FIntVector AxisMask,
	const int Width,
	const int Height,
	const FIntVector V1,
	const FIntVector V2,
	const FIntVector V3,
	const FIntVector V4
)
{
	// Calculate the normal vector for the quad based on the axis mask and mask normal
	const auto Normal = FVector(AxisMask * Mask.Normal);
	// Determine the color for the quad, setting the texture index in the alpha channel
	auto Color = FColor(0, 0, 0, GetTextureIndex(Mask.Block, Normal));

	// Append the vertices for the quad to the mesh data, scaled by 100 to account for UE5 scale
	MeshData.Vertices.Append({
		FVector(V1) * 100,
		FVector(V2) * 100,
		FVector(V3) * 100,
		FVector(V4) * 100
		});

	// Append the triangle indices for the quad to the mesh data
	// Two triangles are formed for each quad, resulting in a total of 6 indices
	MeshData.Triangles.Append({
		VertexCount,
		VertexCount + 2 + Mask.Normal,
		VertexCount + 2 - Mask.Normal,
		VertexCount + 3,
		VertexCount + 1 - Mask.Normal,
		VertexCount + 1 + Mask.Normal
		});

	// Append the normal vectors for each vertex of the quad to the mesh data
	MeshData.Normals.Append({
		Normal,
		Normal,
		Normal,
		Normal
		});

	// Append the color for each vertex of the quad to the mesh data
	MeshData.Colors.Append({
		Color,
		Color,
		Color,
		Color
		});

	// Determine the UV coordinates for the quad based on the normal direction
	// If the normal is along the X axis, use Width and Height directly
	if (Normal.X == 1 || Normal.X == -1)
	{
		MeshData.UV0.Append({
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0),
			});
	}
	// Otherwise, swap Width and Height for the UV coordinates
	else
	{
		MeshData.UV0.Append({
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0),
			});
	}

	// Increment the vertex count by 4 since a quad consists of 4 vertices
	VertexCount += 4;
}


void AChunkBase::ApplyMesh() const
{
	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection(
		0,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.Colors,
		TArray<FProcMeshTangent>(),
		true
	);
}


void AChunkBase::ClearMesh()
{
	VertexCount = 0;
	MeshData.Clear();
}

void AChunkBase::ModifyVoxel(const FIntVector Position, const EBlock Block)
{
	if (Position.X >= Size || Position.Y >= Size || Position.Z >= Size || Position.X < 0 || Position.Y < 0 || Position.Z < 0) return;

	ModifyVoxelData(Position, Block);

	ClearMesh();

	GenerateMesh();

	ApplyMesh();

}


void AChunkBase::ModifyVoxelData(const FIntVector Position, const EBlock Block)
{
	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);

	UE_LOG(LogTemp, Warning, TEXT("X: %d, Y: %d, Z: %d"), Position.X, Position.Y, Position.Z);
	Blocks[Index] = Block;
}

int AChunkBase::GetBlockIndex(const int X, const int Y, const int Z) const
{
	return Z * Size * Size + Y * Size + X;
}

EBlock AChunkBase::GetBlock(const FIntVector Index) const
{
	if (Index.X >= Size || Index.Y >= Size || Index.Z >= Size || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AChunkBase::CompareMask(const FMask M1, const FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
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
				Blocks[GetBlockIndex(X, Y, Z + i)] = EBlock::Log;
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
							Blocks[GetBlockIndex(X + dx, Y + dy, Z + dz)] = EBlock::Leaves;
						}
					}
				}
			}
		}
	}
}




