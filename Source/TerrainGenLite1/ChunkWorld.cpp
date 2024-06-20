#include "ChunkWorld.h"
#include "ChunkBase.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
    PrimaryActorTick.bCanEverTick = false;

    // Initialize BiomeNoise
    BiomeNoise = MakeUnique<FastNoiseLite>();
}

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
    Super::BeginPlay();

    Generate3DWorld();
    UE_LOG(LogTemp, Warning, TEXT("%d Chunks Created"), ChunkCount);
}


void AChunkWorld::Generate3DWorld()
{
    UE_LOG(LogTemp, Warning, TEXT("Generate 3D World"));

    BiomeNoise->SetFrequency(0.3);
    BiomeNoise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    BiomeNoise->SetFractalType(FastNoiseLite::FractalType_FBm);

    for (int x = -DrawDistance; x <= DrawDistance; x++)
    {
        for (int y = -DrawDistance; y <= DrawDistance; ++y)
        {
            for (int z = -DrawDistance; z <= DrawDistance; ++z)
            {
                auto Transform = FTransform(
                    FRotator::ZeroRotator,
                    FVector(x * Size * 100, y * Size * 100, z * Size * 100),
                    FVector::OneVector
                );

                auto Chunk = GetWorld()->SpawnActorDeferred<AChunkBase>(
                    ChunkType,
                    Transform,
                    this
                );

                Chunk->Frequency = Frequency;
                Chunk->LandMaterial = LandMaterial;
                Chunk->LiquidMaterial = LiquidMaterial;
                Chunk->Size = Size;
                Chunk->DrawDistance = DrawDistance;
                Chunk->ZRepeat = z;

                UGameplayStatics::FinishSpawningActor(Chunk, Transform);

                SetBiomeForChunk(Chunk, x, y, z);

                ChunkCount++;
                UE_LOG(LogTemp, Warning, TEXT("Finished chunk?"));

            }
        }
    }
}

void AChunkWorld::SetBiomeForChunk(AChunkBase* Chunk, int32 ChunkX, int32 ChunkY, int32 ChunkZ)
{
    UE_LOG(LogTemp, Warning, TEXT("Set Biome For Chunk"));

    for (int32 bx = 0; bx < Size; ++bx)
    {
        for (int32 by = 0; by < Size; ++by)
        {
            for (int32 bz = 0; bz < Size; ++bz)
            {
                float Xpos = bx + ChunkX * Size;
                float Ypos = by + ChunkY * Size;
                float Zpos = bz + ChunkZ * Size;

                float NoiseValue = BiomeNoise->GetNoise(Xpos, Ypos, Zpos);

                // Calculate humidity based on distance to nearest water source
                float Humidity = CalculateHumidity(Chunk, bx, by, bz);

                EBiome BiomeType = GetBiomeType(NoiseValue, Humidity);

                Chunk->SetBiome(bx, by, bz, BiomeType, Humidity);
            }
        }
    }
    Chunk->RegenerateChunkBlockTextures();

}



// Function to map noise value to EBiome enum
EBiome AChunkWorld::GetBiomeType(float NoiseValue, float Humidity) const
{
    // Adjust the range and thresholds based on desired noise and humidity values
    if (NoiseValue < -0.6f)
    {
        if (Humidity < 0.3f)
        {
            return EBiome::Desert;
        }
        else if (Humidity >= 0.3f && Humidity < 0.7f)
        {
            return EBiome::Plains;
        }
        else
        {
            return EBiome::Swamp;
        }
    }
    else if (NoiseValue < -0.2f)
    {
        if (Humidity < 0.5f)
        {
            return EBiome::Desert;
        }
        else if (Humidity >= 0.5f && Humidity < 0.7f)
        {
            return EBiome::Swamp;
        }
        else
        {
            return EBiome::Tundra;
        }
    }
    else if (NoiseValue < 0.2f)
    {
        if (Humidity < 0.3f)
        {
            return EBiome::Plains;
        }
        else if (Humidity >= 0.3f && Humidity < 0.7f)
        {
            return EBiome::Taiga;
        }
        else
        {
            return EBiome::Swamp;
        }
    }
    else if (NoiseValue < 0.6f)
    {
        if (Humidity < 0.5f)
        {
            return EBiome::Taiga;
        }
        else if (Humidity >= 0.5f && Humidity < 0.8f)
        {
            return EBiome::Tundra;
        }
        else
        {
            return EBiome::Swamp;
        }
    }
    else
    {
        if (Humidity < 0.6f)
        {
            return EBiome::Tundra;
        }
        else if (Humidity >= 0.6f && Humidity < 0.8f)
        {
            return EBiome::Swamp;
        }
        else
        {
            return EBiome::Taiga;
        }
    }
}


float AChunkWorld::CalculateHumidity(AChunkBase* Chunk, int32 bx, int32 by, int32 bz)
{
    FVector BlockPosition = Chunk->GetActorLocation() + FVector(bx, by, bz) * BlockSize;
    FVector WaterSourcePosition = GetNearestWaterSource(BlockPosition);

    float MaxDistance = 1000.0f;
    float DistanceToWater = FVector::Dist(WaterSourcePosition, BlockPosition);
    float Humidity = FMath::Clamp(1.0f - (DistanceToWater / MaxDistance), 0.0f, 1.0f);

    return Humidity;
}

FVector AChunkWorld::GetNearestWaterSource(const FVector& Position)
{
    FVector NearestWaterSource = FVector::ZeroVector;
    float NearestDistanceSquared = FLT_MAX;

    // Iterate over all chunks
    for (AChunkBase* Chunk : Chunks)
    {
        // Iterate over all blocks in the chunk
        for (int bx = 0; bx < Size; ++bx)
        {
            for (int by = 0; by < Size; ++by)
            {
                for (int bz = 0; bz < Size; ++bz)
                {
                    int BlockIndex = Chunk->GetBlockIndex(bx, by, bz);
                    EBlock BlockType = Chunk->Blocks[BlockIndex].Mask.BlockType;

                    // Check if the block is shallow or deep water
                    if (BlockType == EBlock::ShallowWater || BlockType == EBlock::DeepWater)
                    {
                        // Calculate water block position based on chunk and block indices
                        FVector WaterBlockPosition = Chunk->GetActorLocation() + FVector(bx, by, bz) * BlockSize;

                        // Calculate squared distance to the provided Position
                        float DistanceSquared = FVector::DistSquared(WaterBlockPosition, Position);

                        // Update nearest water source if closer than current nearest
                        if (DistanceSquared < NearestDistanceSquared)
                        {
                            NearestDistanceSquared = DistanceSquared;
                            NearestWaterSource = WaterBlockPosition;
                        }
                    }
                }
            }
        }
    }

    // Return the nearest water source position found
    return NearestWaterSource;
}


