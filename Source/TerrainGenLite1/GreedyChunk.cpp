// Fill out your copyright notice in the Description page of Project Settings.


#include "GreedyChunk.h"

#include "Enums.h"
#include "FastNoiseLite.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AGreedyChunk::AGreedyChunk()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;


    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
    Noise = new FastNoiseLite();

    if (Mesh != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Mesh created successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Mesh creation failed: Mesh is null."));
    }

    if (Noise != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Noise created successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Noise creation failed: Noise is null."));
    }

    Noise->SetFrequency(0.03f);
    Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    Noise->SetFractalType(FastNoiseLite::FractalType_FBm);

    //Initializes Blocks
    Blocks.SetNum(Size.X * Size.Y * Size.Z);


    //Mesh Settings
    Mesh->SetCastShadow(false);

    //Set Mesh as root
    SetRootComponent(Mesh);

}

// Called when the game starts or when spawned
void AGreedyChunk::BeginPlay()
{
	Super::BeginPlay();
	
    GenerateBlocks();

    GenerateMesh();

    UE_LOG(LogTemp, Warning, TEXT("Vertext Count : %d"), VertexCount);

    ApplyMesh();
}

void AGreedyChunk::GenerateBlocks()
{
    FVector Location = GetActorLocation();

    for (int x = 0; x < Size.X; ++x)
    {
        for (int y = 0; y < Size.Y; ++y)
        {
            const float Xpos = (x * 100 + Location.X) / 100;
            const float Ypos = (y * 100 + Location.Y) / 100;

            float NoiseValue = Noise->GetNoise(Xpos, Ypos);
            int Height = FMath::RoundToInt((NoiseValue + 1) * Size.Z / 2);
            Height = FMath::Clamp(Height, 0, Size.Z);


            for (int z = 0; z < Height; ++z)
            {
                Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
            }

            for (int z = Height; z < Size.Z; ++z)
            {
                Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
            }
        }
    }
}

void AGreedyChunk::ApplyMesh()
{
    Mesh->CreateMeshSection(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UV0, TArray<FColor>(),TArray<FProcMeshTangent>(), false);

}

void AGreedyChunk::GenerateMesh()
{
    for (int Axis = 0; Axis < 3; ++Axis)
    {
        const int Axis1 = (Axis + 1) % 3;
        const int Axis2 = (Axis + 2) % 3;

        const int MainAxisLimit = Size[Axis];
        int Axis1Limit = Size[Axis1];
        int Axis2Limit = Size[Axis2];

        auto DeltaAxis1 = FIntVector::ZeroValue;
        auto DeltaAxis2 = FIntVector::ZeroValue;

        auto ChunkItr = FIntVector::ZeroValue;
        auto AxisMask = FIntVector::ZeroValue;

        AxisMask[Axis] = 1;

        TArray<FMask> Mask;
        Mask.SetNum(Axis1Limit * Axis2Limit);

        //check each slice
        for (ChunkItr[Axis] = 0; ChunkItr[Axis] < MainAxisLimit;)
        {
            //Index inside the mask
            int N = 0;

            for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
            {
                for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
                {
                    const auto CurrentBlock = GetBlock(ChunkItr);
                    const auto CompareBlock = GetBlock(ChunkItr + AxisMask);

                    const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
                    const bool CompareBlockOpaque = CurrentBlock != EBlock::Air;

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

            //Generate mesh from the mask
            for (int j = 0; j < Axis2Limit; ++j)
            {
                for (int i = 0; i < Axis1Limit; ++i)
                {
                    if (Mask[N].Normal != 0) 
                    {
                        const auto CurrentMask = Mask[N];
                        ChunkItr[Axis1] = i;
                        ChunkItr[Axis2] = j;

                        int width;

                        for (width = 1; i + width < Axis1Limit && CompareMask(Mask[N + width], CurrentMask); ++width)
                        {

                        }

                        int height;
                        bool done = false;

                        for (height = 1; j + height < Axis2Limit; ++height)
                        {
                            for (int k = 0; k < width; ++k)
                            {
                                if (CompareMask(Mask[N + k + height * Axis1Limit], CurrentMask)) continue;

                                done = true;
                                break;
                            }

                            if (done) break;
                        }

                        DeltaAxis1[Axis1] = width;
                        DeltaAxis2[Axis2] = height;

                        CreateQuad(CurrentMask, AxisMask,
                            ChunkItr,
                            ChunkItr + DeltaAxis1,
                            ChunkItr + DeltaAxis2,
                            ChunkItr + DeltaAxis1 + DeltaAxis2
                        );

                        DeltaAxis1 = FIntVector::ZeroValue;
                        DeltaAxis2 = FIntVector::ZeroValue;


                        for (int l = 0; l < height; ++l)
                        {
                            for (int k = 0; k < width; ++k)
                            {
                                Mask[N + k + l * Axis1Limit] = FMask{ EBlock::Null, 0 };
                            }
                        }


                        i += width;
                        N += width;
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

void AGreedyChunk::CreateQuad(FMask Mask, FIntVector AxisMask, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4)
{
    const auto Normal = FVector(AxisMask * Mask.Normal);

    MeshData.Vertices.Add(FVector(V1) * 100);
    MeshData.Vertices.Add(FVector(V2) * 100);
    MeshData.Vertices.Add(FVector(V3) * 100);
    MeshData.Vertices.Add(FVector(V4) * 100);

    MeshData.Triangles.Add(VertexCount);
    MeshData.Triangles.Add(VertexCount + 2 + Mask.Normal);
    MeshData.Triangles.Add(VertexCount + 2 - Mask.Normal);
    MeshData.Triangles.Add(VertexCount + 3);
    MeshData.Triangles.Add(VertexCount + 1 - Mask.Normal);
    MeshData.Triangles.Add(VertexCount + 1 + Mask.Normal);


    MeshData.UV0.Add(FVector2D(0, 0));
    MeshData.UV0.Add(FVector2D(0, 1));
    MeshData.UV0.Add(FVector2D(1, 0));
    MeshData.UV0.Add(FVector2D(1, 1));

    MeshData.Normals.Add(Normal);
    MeshData.Normals.Add(Normal);
    MeshData.Normals.Add(Normal);
    MeshData.Normals.Add(Normal);

    VertexCount += 4;
}

int AGreedyChunk::GetBlockIndex(int X, int Y, int Z) const
{
    return Z * Size.X * Size.Y + Y * Size.X + X;

}

EBlock AGreedyChunk::GetBlock(FIntVector Index) const
{
    if (Index.X >= Size.X || Index.Y >= Size.Y || Index.Z >= Size.Z || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
    {
        return EBlock::Air;
    }
    return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AGreedyChunk::CompareMask(FMask M1, FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
}

