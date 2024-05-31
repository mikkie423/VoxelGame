// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"
#include "Enums.h"
#include "ChunkBase.h"
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

	Generate3DWorld();

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

					chunk->Frequency = Frequency;
					chunk->LandMaterial = LandMaterial;
					chunk->LiquidMaterial = LiquidMaterial;
					chunk->Size = Size;
					chunk->DrawDistance = DrawDistance;
					chunk->ZRepeat = z;


					UGameplayStatics::FinishSpawningActor(chunk, transform);

					ChunkCount++;				
			}
		}
	}
}