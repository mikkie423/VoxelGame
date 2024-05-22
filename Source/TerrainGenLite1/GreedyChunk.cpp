// Fill out your copyright notice in the Description page of Project Settings.


#include "GreedyChunk.h"

#include "FastNoiseLite.h"
//#include "ProceduralMeshComponent.h"

void AGreedyChunk::Setup()
{
	// Initialize Blocks
	Blocks.SetNum(Size * Size * Size);
}

void AGreedyChunk::Generate2DHeightMap(const FVector Position)
{
	UE_LOG(LogTemp, Warning, TEXT("Generating 2D Height Map"));

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			const float Xpos = x + Position.X;
			const float Ypos = y + Position.Y;

			const int Height = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(Xpos, Ypos) + 1) * Size / 2), 0, Size);

			for (int z = 0; z < Size; ++z)
			{
				//if (z == 0) Blocks[GetBlockIndex(x, y, z)] = EBlock::Bedrock;
				//else
				if (z < Height - 3) Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
				else if (z < Height - 1) Blocks[GetBlockIndex(x, y, z)] = EBlock::Dirt;
				else if (z == Height - 1) Blocks[GetBlockIndex(x, y, z)] = EBlock::Grass;
				else Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
			}
		}
	}
}



void AGreedyChunk::Generate3DHeightMap(const FVector Position)
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



void AGreedyChunk::GenerateBiomeHeightMap(const FVector Position)
{
	UE_LOG(LogTemp, Warning, TEXT("Generating Biome Height Map"));
	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			// Calculate surface height for this column
			const double Xpos = x + Position.X;
			const double Ypos = y + Position.Y;
			const float SurfaceHeight = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(Xpos, Ypos) + 1) * Size / 2), 0, Size);


			for (int z = 0; z < Size; ++z)
			{
				const double Zpos = z + Position.Z;

				// Calculate noise value for cave generation
				const auto NoiseValue = Noise->GetNoise(Xpos, Ypos, Zpos);

				// Apply cave generation logic based on noise value
				if (NoiseValue <= 0)
				{
					Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
				}
				else
				{
					// Adjust block types based on surface height
					if (Zpos < SurfaceHeight - 3) Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
					else if (Zpos < SurfaceHeight - 1) Blocks[GetBlockIndex(x, y, z)] = EBlock::Dirt;
					else if (Zpos <= SurfaceHeight - 1) Blocks[GetBlockIndex(x, y, z)] = EBlock::Grass;
					else Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
				}
			}
		}
	}
}


void AGreedyChunk::GenerateMesh()
{
	// Sweep over each axis (X, Y, Z)
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		// 2 Perpendicular axis
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

		TArray<FMask> Mask;
		Mask.SetNum(Axis1Limit * Axis2Limit);

		// Check each slice of the chunk
		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			// Compute Mask
			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const auto CurrentBlock = GetBlock(ChunkItr);
					const auto CompareBlock = GetBlock(ChunkItr + AxisMask);

					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air;

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

			// Generate Mesh From Mask
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

						for (Width = 1; i + Width < Axis1Limit && CompareMask(Mask[N + Width], CurrentMask); ++Width)
						{
						}

						int Height;
						bool Done = false;

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

						DeltaAxis1[Axis1] = Width;
						DeltaAxis2[Axis2] = Height;

						CreateQuad(
							CurrentMask, AxisMask, Width, Height,
							ChunkItr,
							ChunkItr + DeltaAxis1,
							ChunkItr + DeltaAxis2,
							ChunkItr + DeltaAxis1 + DeltaAxis2
						);

						DeltaAxis1 = FIntVector::ZeroValue;
						DeltaAxis2 = FIntVector::ZeroValue;

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

void AGreedyChunk::CreateQuad(
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
	const auto Normal = FVector(AxisMask * Mask.Normal);
	auto Color = FColor(0, 0, 0, GetTextureIndex(Mask.Block, Normal));

	MeshData.Vertices.Append({
		FVector(V1) * 100,
		FVector(V2) * 100,
		FVector(V3) * 100,
		FVector(V4) * 100
		});

	MeshData.Triangles.Append({
		VertexCount,
		VertexCount + 2 + Mask.Normal,
		VertexCount + 2 - Mask.Normal,
		VertexCount + 3,
		VertexCount + 1 - Mask.Normal,
		VertexCount + 1 + Mask.Normal
		});

	MeshData.Normals.Append({
		Normal,
		Normal,
		Normal,
		Normal
		});


	MeshData.Colors.Append({
		Color,
		Color,
		Color,
		Color
		});


	if (Normal.X == 1 || Normal.X == -1)
	{
		MeshData.UV0.Append({
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0),
			});
	}
	else
	{
		MeshData.UV0.Append({
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0),
			});
	}

	VertexCount += 4;
}


void AGreedyChunk::ModifyVoxelData(const FIntVector Position, const EBlock Block)
{
	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);

	UE_LOG(LogTemp, Warning, TEXT("X: %d, Y: %d, Z: %d"), Position.X, Position.Y, Position.Z);
	Blocks[Index] = Block;
}

int AGreedyChunk::GetBlockIndex(const int X, const int Y, const int Z) const
{
	return Z * Size * Size + Y * Size + X;
}

EBlock AGreedyChunk::GetBlock(const FIntVector Index) const
{
	if (Index.X >= Size || Index.Y >= Size || Index.Z >= Size || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AGreedyChunk::CompareMask(const FMask M1, const FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
}

int AGreedyChunk::GetTextureIndex(const EBlock Block, const FVector Normal) const
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

void AGreedyChunk::GenerateTrees(TArray<FIntVector> LocalTreePositions)
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
		int LeafRadius = 2; // Adjust the radius of the leaf canopy
		for (int dz = TreeHeight - 1; dz <= TreeHeight + LeafRadius; ++dz) // Start leaves from just below the top of the trunk
		{
			for (int dx = -LeafRadius; dx <= LeafRadius; ++dx) // Randomize X placement within the leaf radius
			{
				for (int dy = -LeafRadius; dy <= LeafRadius; ++dy) // Randomize Y placement within the leaf radius
				{
					if (FMath::Abs(dx) + FMath::Abs(dy) <= LeafRadius) // Ensure leaf placement forms a circular shape
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

