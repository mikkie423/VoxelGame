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

				if (z == 0 && ZRepeat == BaseZ) Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Bedrock;

				// Apply cave generation logic based on noise value
				else if (NoiseValue >= 0 && Zpos <= SurfaceHeight - 7)
				{
					Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Air;
				}
				else
				{
					// Adjust block types based on surface height
					if (Zpos < SurfaceHeight - 3) Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Stone;
					else if (Zpos < SurfaceHeight - 1)  Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Dirt;
					else if (Zpos == SurfaceHeight - 1)
					{
						int randNum = FMath::FRandRange(1, 51);
						//UE_LOG(LogTemp, Warning, TEXT("RandNum = %i"), randNum);
						if (randNum == 1 && Zpos > 15)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Dirt;
							TreePositions.Add(FIntVector(x, y, z));
						}
						else
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Grass;
						}
					}
					else Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Air;
					if (Zpos < 15)
					{
						if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Air)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::ShallowWater;
						}
						if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass && Zpos > 10)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Sand;
						}
						else if (Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Dirt || Blocks[GetBlockIndex(x, y, z)].Mask.BlockType == EBlock::Grass)
						{
							Blocks[GetBlockIndex(x, y, z)].Mask.BlockType = EBlock::Gravel;
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
                    int CurrentBlockIndex = -1;
                    int CompareBlockIndex = -1;

                    if (ChunkItr.X >= 0 && ChunkItr.Y >= 0 && ChunkItr.Z >= 0 &&
                        ChunkItr.X < Size && ChunkItr.Y < Size && ChunkItr.Z < Size)
                    {
                        CurrentBlockIndex = GetBlockIndex(ChunkItr.X, ChunkItr.Y, ChunkItr.Z);
                    }

                    if (ChunkItr.X + AxisMask.X >= 0 && ChunkItr.Y + AxisMask.Y >= 0 && ChunkItr.Z + AxisMask.Z >= 0 &&
                        ChunkItr.X + AxisMask.X < Size && ChunkItr.Y + AxisMask.Y < Size && ChunkItr.Z + AxisMask.Z < Size)
                    {
                        CompareBlockIndex = GetBlockIndex(ChunkItr.X + AxisMask.X, ChunkItr.Y + AxisMask.Y, ChunkItr.Z + AxisMask.Z);
                    }

                    const FBlockData* CurrentBlock = (CurrentBlockIndex >= 0) ? &Blocks[CurrentBlockIndex] : nullptr;
                    const FBlockData* CompareBlock = (CompareBlockIndex >= 0) ? &Blocks[CompareBlockIndex] : nullptr;

                    const bool CurrentBlockOpaque = CurrentBlock && CurrentBlock->bIsSolid && CurrentBlock->Mask.BlockType != EBlock::Air;
                    const bool CompareBlockOpaque = CompareBlock && CompareBlock->bIsSolid && CompareBlock->Mask.BlockType != EBlock::Air;

                    if (CurrentBlockOpaque == CompareBlockOpaque)
                    {
                        BlockData[N++].Mask = FMask{ EBlock::Null, 0 };
                    }
                    else if (CurrentBlockOpaque)
                    {
                        BlockData[N++].Mask = FMask{ CurrentBlock->Mask.BlockType, 1 };
                    }
                    else
                    {
                        BlockData[N++].Mask = FMask{ CompareBlock->Mask.BlockType, -1 };
                    }
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

                        CreateQuad(
                            CurrentMask,
                            AxisMask,
                            Width,
                            Height,
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
    const FIntVector V4
)
{
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

    const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
    if (Blocks[Index].Mask.BlockType != Block)
    {
        ModifyVoxelData(Position, Block);
        ClearMesh();
        GenerateMesh();
        ApplyMesh();
    }
}

void AChunkBase::ModifyVoxelData(const FIntVector Position, const EBlock Block)
{
    const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
    Blocks[Index].Mask.BlockType = Block;
}

int AChunkBase::GetBlockIndex(const int X, const int Y, const int Z) const
{
    return Z * Size * Size + Y * Size + X;
}

FBlockData AChunkBase::GetBlock(const FIntVector Index) const
{
    if (Index.X >= Size || Index.Y >= Size || Index.Z >= Size || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
        return FBlockData{ FMask{ EBlock::Air, 0 }, false, false, 0 }; // Return default block data for out-of-bounds indices
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
                        }
                    }
                }
            }
        }
    }
}
