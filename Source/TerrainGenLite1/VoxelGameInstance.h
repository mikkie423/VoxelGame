// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VoxelGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TERRAINGENLITE1_API UVoxelGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UVoxelGameInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	int WorldSeed = 1337;
	
};
