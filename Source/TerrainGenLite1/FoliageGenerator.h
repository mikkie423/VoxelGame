// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoliageGenerator.generated.h"

UCLASS()
class TERRAINGENLITE1_API AFoliageGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFoliageGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foliage")
	UStaticMeshComponent* GrassMesh1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foliage")
	UStaticMeshComponent* GrassMesh2;

	UFUNCTION()
	void SetTexture(UTexture2D* Texture);
};

