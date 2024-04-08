// Fill out your copyright notice in the Description page of Project Settings.


#include "Chunk.h"
#include "Enums.h"
#include "FastNoiseLite.h"
#include "ProceduralMeshComponent.h"



// Sets default values
AChunk::AChunk()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;


    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
    Noise = new FastNoiseLite();

    Noise->SetFrequency(0.03f);
    Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    Noise->SetFractalType(FastNoiseLite::FractalType_FBm);

    Blocks.SetNum(Size * Size * Size);

    Mesh->SetCastShadow(false);

}


// Called when the game starts or when spawned
void AChunk::BeginPlay()
{
    Super::BeginPlay();

    GenerateBlocks();

    GenerateMesh();

    UE_LOG(LogTemp, Warning, TEXT("Vertext Count : %d"), VertexCount);

    ApplyMesh();
}


void AChunk::GenerateBlocks()
{
    FVector Location = GetActorLocation();

    for (int x = 0; x < Size; ++x)
    {
        for (int y = 0; y < Size; ++y)
        {
            const float Xpos = (x * 100 + Location.X) / 100;
            const float Ypos = (y * 100 + Location.Y) / 100;

            float NoiseValue = Noise->GetNoise(Xpos, Ypos);
            int Height = FMath::RoundToInt((NoiseValue + 1) * Size / 2);
            Height = FMath::Clamp(Height, 0, Size);


            for (int z = 0; z < Height; ++z)
            {
                Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
            }

            for (int z = Height; z < Size; ++z)
            {
                Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
            }
        }
    }
}

void AChunk::GenerateMesh()
{
    for (int x = 0; x < Size; ++x)
    {
        for (int y = 0; y < Size; ++y)
        {
            for (int z = 0; z < Size; ++z)
            {
                if (Blocks[GetBlockIndex(x, y, z)] != EBlock::Air)
                {
                    const auto Position = FVector(x, y, z);

                    for (auto Direction : { EDirection::Forward, EDirection::Right, EDirection::Back, EDirection::Left, EDirection::Up, EDirection::Down })
                    {
                        if (Check(GetPositionInDirection(Direction, Position)))
                        {
                            CreateFace(Direction, Position * 100);
                        }
                    }
                }
            }
        }
    }
}

void AChunk::ApplyMesh()
{
    Mesh->CreateMeshSection(0, VertexData, TriangleData, TArray<FVector>(), UVData, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
}

bool AChunk::Check(FVector Position) const
{
    if (Position.X >= Size || Position.Y >= Size || Position.Z >= Size || Position.X < 0 || Position.Y < 0 || Position.Z < 0)
    {
        return true;
    }
    return Blocks[GetBlockIndex(Position.X, Position.Y, Position.Z)] == EBlock::Air;
}

void AChunk::CreateFace(EDirection Direction, FVector Position)
{
    VertexData.Append(GetFaceVertices(Direction, Position));
    UVData.Append({ FVector2D(1, 1), FVector2D(1, 0), FVector2D(0, 0), FVector2D(0, 1) });
    TriangleData.Append({ VertexCount + 3, VertexCount + 2, VertexCount, VertexCount + 2, VertexCount + 1, VertexCount });
    VertexCount += 4;
}

TArray<FVector> AChunk::GetFaceVertices(EDirection Direction, FVector Position) const
{
    TArray<FVector> Vertices;

    for (int i = 0; i < 4; ++i)
    {
        Vertices.Add(BlockVertexData[BlockTriangleData[i + static_cast<int>(Direction) * 4]] * Scale + Position);
    }
    return Vertices;
}

FVector AChunk::GetPositionInDirection(EDirection Direction, FVector Position) const
{
    switch (Direction)
    {
    case EDirection::Forward: return Position + FVector::ForwardVector;
    case EDirection::Right: return Position + FVector::RightVector;
    case EDirection::Back: return Position + FVector::BackwardVector;
    case EDirection::Left: return Position + FVector::LeftVector;
    case EDirection::Up: return Position + FVector::UpVector;
    case EDirection::Down: return Position + FVector::DownVector;
    default: throw std::invalid_argument("Invalid direction");

    }

    return FVector();
}

int AChunk::GetBlockIndex(int X, int Y, int Z) const
{
    return Z * Size * Size + Y * Size + X;
}