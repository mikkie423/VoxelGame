// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Enums.h"
#include "FastNoiseLite.h"


#include "ChunkWorld.generated.h"

class AChunkBase;
class FastNoiseLite;


UCLASS()
class AChunkWorld final : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditInstanceOnly, Category = "World")
    TSubclassOf<AChunkBase> ChunkType;

    UPROPERTY(EditInstanceOnly, Category = "World")
    int DrawDistance = 5;

    UPROPERTY(EditInstanceOnly, Category = "Chunk")
    TObjectPtr<UMaterialInterface> LandMaterial;

    UPROPERTY(EditInstanceOnly, Category = "Chunk")
    TObjectPtr<UMaterialInterface> LiquidMaterial;

    UPROPERTY(EditInstanceOnly, Category = "Chunk")
    int Size = 32;

    int BlockSize = 100;

    UPROPERTY(EditInstanceOnly, Category = "Height Map")
    float Frequency = 0.03f;

    // Sets default values for this actor's properties
    AChunkWorld();

    TUniquePtr<FastNoiseLite> BiomeNoise;
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    int ChunkCount;

    void Generate3DWorld();

    EBiome GetBiomeType(float NoiseValue, float Humidity) const;

    void SetBiomeForChunk(AChunkBase* Chunk, int32 ChunkX, int32 ChunkY, int32 ChunkZ);
    float CalculateHumidity(AChunkBase* Chunk, int32 bx, int32 by, int32 bz);
    FVector GetNearestWaterSource(const FVector& Position);



    TArray<AChunkBase*> Chunks;
};
