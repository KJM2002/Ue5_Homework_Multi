// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ReloadComplete.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API UAnimNotify_ReloadComplete : public UAnimNotify
{
	GENERATED_BODY()

	virtual FString GetNotifyName_Implementation() const override;
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int MaxBullet = 30;
};
