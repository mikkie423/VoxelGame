// Fill out your copyright notice in the Description page of Project Settings.


#include "FoliageGenerator.h"

// Sets default values
AFoliageGenerator::AFoliageGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

    // Create two static mesh components for the grass planes
    GrassMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrassMesh1"));
    GrassMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrassMesh2"));

    // Attach meshes to the root component
    RootComponent = GrassMesh1;
    GrassMesh2->SetupAttachment(RootComponent);

    // Set the default scale and rotation for the meshes
    GrassMesh1->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.01f)); // Thin plane
    GrassMesh2->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.01f)); // Thin plane
    GrassMesh2->SetRelativeRotation(FRotator(0, 90, 0)); // Rotate to create a diagonal cross
}

void AFoliageGenerator::SetTexture(UTexture2D* Texture)
{
    if (GrassMesh1->GetMaterial(0) && GrassMesh2->GetMaterial(0))
    {
        UMaterialInstanceDynamic* DynamicMaterial1 = GrassMesh1->CreateAndSetMaterialInstanceDynamic(0);
        UMaterialInstanceDynamic* DynamicMaterial2 = GrassMesh2->CreateAndSetMaterialInstanceDynamic(0);

        DynamicMaterial1->SetTextureParameterValue(TEXT("Texture"), Texture);
        DynamicMaterial2->SetTextureParameterValue(TEXT("Texture"), Texture);
    }
}

// Called when the game starts or when spawned
void AFoliageGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}



