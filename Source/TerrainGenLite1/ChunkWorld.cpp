#include "ChunkWorld.h"
#include "ChunkBase.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
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

void AChunkWorld::OnChunkMeshUpdated()
{
	UE_LOG(LogTemp, Warning, TEXT("Chunk mesh updated. Updating NavMesh..."));
	UpdateNavMeshBoundsVolume();
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

				Chunks.Add(Chunk);
				// Bind to the OnChunkMeshUpdated delegate
				Chunk->OnChunkMeshUpdated.AddDynamic(this, &AChunkWorld::OnChunkMeshUpdated);

				ChunkCount++;
				UE_LOG(LogTemp, Warning, TEXT("Finished chunk?"));

			}
		}
	}
	GenerateFlora();
	// Create or update NavMeshBoundsVolume
	UpdateNavMeshBoundsVolume();
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


void AChunkWorld::UpdateNavMeshBoundsVolume()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		APawn* PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn)
		{
			// Get player position
			FVector PlayerLocation = PlayerPawn->GetActorLocation();

			// Set WorldCenter to the player's location, adjusting as needed for the surrounding area
			FVector WorldCenter = PlayerLocation;

			// Adjust the extent to define the area around the player for the navmesh
			FVector WorldExtent = FVector(ChunkSize * DrawDistance, ChunkSize * DrawDistance, ChunkSize * DrawDistance * 2);

			// Find or spawn the NavMeshBoundsVolume
			ANavMeshBoundsVolume* NavMeshBounds = nullptr;
			TArray<AActor*> FoundNavMeshVolumes;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), FoundNavMeshVolumes);

			if (FoundNavMeshVolumes.Num() > 0)
			{
				NavMeshBounds = Cast<ANavMeshBoundsVolume>(FoundNavMeshVolumes[0]);
			}
			else
			{
				NavMeshBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>();
			}

			if (NavMeshBounds)
			{
				// Set the size and position of the NavMeshBoundsVolume around the player
				NavMeshBounds->SetActorLocation(WorldCenter);
				NavMeshBounds->SetActorScale3D(WorldExtent);

				// Force navigation rebuild
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
				if (NavSys)
				{
					// Notify the navigation system that the bounds have changed
					NavSys->OnNavigationBoundsUpdated(NavMeshBounds);

					// Optionally trigger a rebuild for immediate updates
					NavSys->Build(); // Use with caution as this can cause lag
				}
			}
		}
	}
}


void AChunkWorld::GenerateFlora()
{
	// Ensure the FloraBlueprint is valid
	if (!FloraBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("FloraBlueprint is not valid!"));
		return;
	}

	// Loop through all chunks
	for (AChunkBase* Chunk : Chunks)
	{
		// Ensure Chunk is valid
		if (!Chunk)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found invalid chunk. Skipping."));
			continue;
		}

		// Get the local flora data for this chunk
		TArray<FDecorationData> LocalFloraData = Chunk->GetFloraPositions();

		if (LocalFloraData.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("No flora data found for chunk at %s"), *Chunk->GetActorLocation().ToString());
		}

		// Loop through each decoration data entry
		for (const FDecorationData& DecorationData : LocalFloraData)
		{
			// Calculate spawn location based on chunk and decoration data
			FVector SpawnLocation = Chunk->GetActorLocation() + FVector(DecorationData.Position.X, DecorationData.Position.Y, DecorationData.Position.Z) * BlockSize;

			// Log the spawn location and texture index for debugging
			UE_LOG(LogTemp, Log, TEXT("Spawning flora at %s with TextureIndex: %d"), *SpawnLocation.ToString(), DecorationData.TextureIndex);

			// Set spawn parameters
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			// Spawn the Flora blueprint actor at the specified location
			AActor* SpawnedFlora = GetWorld()->SpawnActor<AActor>(FloraBlueprint, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

			// Check if the flora was spawned successfully
			if (SpawnedFlora)
			{
				// Log success
				UE_LOG(LogTemp, Log, TEXT("Successfully spawned flora actor at %s"), *SpawnLocation.ToString());

				// If the spawned actor is valid, call the SetDecorationData function in BP_Flora
				UFunction* SetDecorationDataFunc = SpawnedFlora->FindFunction(TEXT("SetDecorationData"));
				if (SetDecorationDataFunc)
				{
					// Log the function being called
					UE_LOG(LogTemp, Log, TEXT("Calling SetDecorationData on %s"), *SpawnedFlora->GetName());

					// Prepare the parameters for the function call (DecorationData struct)
					SpawnedFlora->ProcessEvent(SetDecorationDataFunc, (void*)&DecorationData);
				}
				else
				{
					// Log a warning if the function is not found
					UE_LOG(LogTemp, Warning, TEXT("SetDecorationData function not found on %s"), *SpawnedFlora->GetName());
				}
			}
			else
			{
				// Log an error if the flora couldn't be spawned
				UE_LOG(LogTemp, Error, TEXT("Failed to spawn flora actor at %s"), *SpawnLocation.ToString());
			}
		}
	}
}
