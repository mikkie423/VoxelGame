// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"
#include "Enums.h"
#include "ChunkBase.h"
#include "GreedyChunk.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AChunkWorld::AChunkWorld()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}


// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();

	switch (GenerationType)
	{
	case EGenerationType::GT_3D:
		Generate3DWorld();
		break;
	case EGenerationType::GT_2D:
		Generate2DWorld();
		break;
	default:
		throw std::invalid_argument("Invalid Generation Type");
	}


	UE_LOG(LogTemp, Warning, TEXT("%d Chunks Created"), ChunkCount);
}


void AChunkWorld::Generate3DWorld()
{
	for (int x = -DrawDistance; x <= DrawDistance; x++)
	{
		for (int y = -DrawDistance; y <= DrawDistance; ++y)
		{
			for (int z = -DrawDistance; z <= DrawDistance; ++z)
			{
					UE_LOG(LogTemp, Warning, TEXT("Generate 3D World z part: %i"), z);

					auto transform = FTransform(
						FRotator::ZeroRotator,
						FVector(x * Size * 100, y * Size * 100, z * Size * 100),
						FVector::OneVector
					);

					const auto chunk = GetWorld()->SpawnActorDeferred<AChunkBase>(
						ChunkType,
						transform,
						this
					);

					chunk->GenerationType = EGenerationType::GT_3D;
					chunk->Frequency = Frequency;
					chunk->Material = Material;
					chunk->Size = Size;
					chunk->DrawDistance = DrawDistance;
					chunk->ZRepeat = z;


					UGameplayStatics::FinishSpawningActor(chunk, transform);

					ChunkCount++;				
			}
		}
	}
}


void AChunkWorld::Generate2DWorld()
{
	for (int x = -DrawDistance; x <= DrawDistance; x++)
	{
		for (int y = -DrawDistance; y <= DrawDistance; ++y)
		{
			auto transform = FTransform(
				FRotator::ZeroRotator,
				FVector(x * Size * 100, y * Size * 100, 0),
				FVector::OneVector
			);

			const auto chunk = GetWorld()->SpawnActorDeferred<AChunkBase>(
				ChunkType,
				transform,
				this
			);

			chunk->GenerationType = EGenerationType::GT_2D;
			chunk->Frequency = Frequency;
			chunk->Material = Material;
			chunk->Size = Size;

			UGameplayStatics::FinishSpawningActor(chunk, transform);

			ChunkCount++;

		}
	}
}


//void AChunkWorld::GenerateWaterFeatures()
//{
//	// Step 1: Fill land below a certain Y-level with water blocks
//	FillLakesAndOceans();
//
//	// Step 2: Identify deep water blocks
//	IdentifyDeepWaterBlocks();
//
//	// Step 3: Modify grass blocks adjacent to water
//	ModifyGrassBlocksAdjacentToWater();
//}


//void AChunkWorld::FillLakesAndOceans()
//{
//
//	UE_LOG(LogTemp, Warning, TEXT("Filling Lakes and Oceans"));
//
//	for (AChunkBase* Chunk : Chunks)
//	{
//		AGreedyChunk* GreedyChunk = Cast<AGreedyChunk>(Chunk);
//		if (GreedyChunk)
//		{
//			for (int x = 0; x < Size; ++x)
//			{
//				for (int y = 0; y < Size; ++y)
//				{
//					for (int z = 0; z < Size; ++z)
//					{
//						// Fill blocks below a certain Y-level with water
//						if (GreedyChunk->GetBlock(FIntVector(x, y, z)) == EBlock::Air && z < WaterLevel) 
//						{
//							GreedyChunk->ModifyVoxel(FIntVector(x, y, z), EBlock::ShallowWater);
//						}
//					}
//				}
//			}
//		}
//	}
//}
//
//void AChunkWorld::IdentifyDeepWaterBlocks()
//{
//	UE_LOG(LogTemp, Warning, TEXT("Identify Deep Water"));
//
//	// Iterate over all blocks in all chunks
//	// Check if each block is surrounded by water on all sides
//	// If so, mark it as a deep water block
//}
//
//void AChunkWorld::ModifyGrassBlocksAdjacentToWater()
//{
//	UE_LOG(LogTemp, Warning, TEXT("Modify Seabed"));
//
//	// Iterate over all blocks in all chunks
//	// Check if each grass block has water above it
//	// If so, replace it with gravel or dirt
//}