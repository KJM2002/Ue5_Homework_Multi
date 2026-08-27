// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API UMyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Direction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AimPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AimYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	uint32 bArmed : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	uint32 bZoom : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float LeanValue = 0;

	UFUNCTION()
	void AnimNotify_ReloadComplete2(UAnimNotify* Notify);
};
