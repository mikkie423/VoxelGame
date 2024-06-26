#include "ChunkWorld.h"
#include "ChunkBase.h"
#include "Kismet/GameplayStatics.h"
#include "VoxelGameInstance.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
	PrimaryActorTick.bCanEverTick = false;

	// Initialize BiomeNoise
	BiomeNoise = MakeUnique<FastNoiseLite>();
	HumidityNoise = MakeUnique<FastNoiseLite>();
}

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();


	UGameInstance* BaseGameInstance = GetGameInstance();
	if (BaseGameInstance)
	{

		UVoxelGameInstance* GameInstance = Cast<UVoxelGameInstance>(BaseGameInstance);
		if (GameInstance)
		{

			WorldSeed = GameInstance->WorldSeed;
			UE_LOG(LogTemp, Warning, TEXT("World Seed: %d"), WorldSeed);

			Generate3DWorld();
			UE_LOG(LogTemp, Warning, TEXT("%d Chunks Created"), ChunkCount);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to cast to UVoxelGameInstance"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Game Instance found!"));
	}
}


void AChunkWorld::Generate3DWorld()
{
	UE_LOG(LogTemp, Warning, TEXT("Generate 3D World"));

	BiomeNoise->SetSeed(WorldSeed);
	BiomeNoise->SetFrequency(0.010); // Lower frequency for smoother transitions
	BiomeNoise->SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	BiomeNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
	BiomeNoise->SetFractalOctaves(3); // More octaves for detail
	BiomeNoise->SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);

	HumidityNoise->SetSeed(WorldSeed);
	HumidityNoise->SetFrequency(0.005); // Matching frequency for aligned transitions
	HumidityNoise->SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	HumidityNoise->SetFractalType(FastNoiseLite::FractalType_FBm);
	HumidityNoise->SetFractalOctaves(3);
	HumidityNoise->SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
	HumidityNoise->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
	HumidityNoise->SetCellularJitter(2.5f);


	for (int x = -DrawDistance; x <= DrawDistance; x++)
	{
		for (int y = -DrawDistance; y <= DrawDistance; ++y)
		{
			for (int z = 0; z < 1; ++z)
			{
				auto Transform = FTransform(
					FRotator::ZeroRotator,
					FVector(x * ChunkSize * 100, y * ChunkSize * 100, z * ChunkSize * 100),
					FVector::OneVector
				);

				auto Chunk = GetWorld()->SpawnActorDeferred<AChunkBase>(
					ChunkType,
					Transform,
					this
				);

				Chunk->WorldSeed = WorldSeed;
				Chunk->Frequency = Frequency;
				Chunk->LandMaterial = LandMaterial;
				Chunk->LiquidMaterial = LiquidMaterial;
				Chunk->ChunkSize = ChunkSize;
				Chunk->DrawDistance = DrawDistance;
				Chunk->BlockSize = BlockSize;
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

	for (int32 bx = 0; bx < ChunkSize; ++bx)
	{
		for (int32 by = 0; by < ChunkSize; ++by)
		{
			for (int32 bz = 0; bz < ChunkSize; ++bz)
			{
				float Xpos = bx + ChunkX * ChunkSize;
				float Ypos = by + ChunkY * ChunkSize;
				float Zpos = bz + ChunkZ * ChunkSize;

				// Calculate local coordinates within the chunk
				int32 LocalX = bx;
				int32 LocalY = by;
				int32 LocalZ = bz;

				// Sample noise for biome generation
				float NoiseValue = BiomeNoise->GetNoise(Xpos, Ypos);
				float HumidityValue = HumidityNoise->GetNoise(Xpos, Ypos);

				// Determine biome type based on noise values
				EBiome BiomeType = GetBiomeType(NoiseValue, HumidityValue);

				// Set biome type and humidity for the block in the chunk
				Chunk->SetBiome(LocalX, LocalY, LocalZ, BiomeType, HumidityValue);
			}
		}
	}
	Chunk->RegenerateChunkBlockTextures();

}



// Function to map noise value to EBiome enum
// -1 Noise == Coldest, +1 Noise == Hottest
EBiome AChunkWorld::GetBiomeType(float NoiseValue, float Humidity) const
{
	// Ensure NoiseValue and Humidity are within [-1, 1]
	NoiseValue = FMath::Clamp(NoiseValue, -1.0f, 1.0f);
	Humidity = FMath::Clamp(Humidity, -1.0f, 1.0f);

	// Adjust thresholds for smooth transitions
	if (NoiseValue < -0.6f)
	{
		if (Humidity < -0.6f)
		{
			return EBiome::Tundra;   // Cold and dry
		}
		else if (Humidity < 0.4f)
		{
			return EBiome::Taiga;    // Cold and moderately wet
		}
		else
		{
			return EBiome::Swamp;    // Cold and wet
		}
	}
	else if (NoiseValue < 0.0f)
	{
		if (Humidity < -0.2f)
		{
			return EBiome::Plains;   // Moderate and dry
		}
		else if (Humidity < 0.4f)
		{
			return EBiome::Taiga;    // Moderate and moderately wet
		}
		else
		{
			return EBiome::Swamp;    // Moderate and wet
		}
	}
	else if (NoiseValue < 0.4f)
	{
		if (Humidity < -0.2f)
		{
			return EBiome::Plains;   // Warm and dry
		}
		else if (Humidity < 0.4f)
		{
			return EBiome::Swamp;    // Warm and moderately wet
		}
		else
		{
			return EBiome::Taiga;    // Warm and wet
		}
	}
	else
	{
		if (Humidity < 0.0f)
		{
			return EBiome::Desert;   // Hot and dry
		}
		else if (Humidity < 0.4f)
		{
			return EBiome::Plains;   // Hot and moderately wet
		}
		else
		{
			return EBiome::Swamp;    // Hot and wet
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
		for (int bx = 0; bx < ChunkSize; ++bx)
		{
			for (int by = 0; by < ChunkSize; ++by)
			{
				for (int bz = 0; bz < ChunkSize; ++bz)
				{
					int BlockIndex = Chunk->GetBlockIndex(bx, by, bz);
					EBlock BlockType = Chunk->Blocks[BlockIndex].Mask.BlockType;

					// Check if the block is shallow or deep water
					if (BlockType == EBlock::ShallowWater || BlockType == EBlock::DeepWater || BlockType == EBlock::Ice)
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


