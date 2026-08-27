// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MyPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API AMyPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AMyPlayerCameraManager();

	virtual void UpdateCamera(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float NormalFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float ZoomFOV = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float ZoomSpeed = 15.f;
};

