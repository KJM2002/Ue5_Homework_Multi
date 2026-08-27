// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Perception/AIPerceptionTypes.h"
#include "Zombie.h"

#include "ZombieAIController.generated.h"


class UAIPerceptionComponent;
class UBehaviorTree;

/**
 *
 */
UCLASS()
class L20260713_DAY03_API AZombieAIController : public AAIController
{
	GENERATED_BODY()

public:

	AZombieAIController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UBehaviorTree> RunTree;

	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	UFUNCTION()
	void OnActorPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnActorPerceptionForgetUpdated(AActor* Actor);

	UFUNCTION()
	void OnPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo);

	UFUNCTION(BlueprintCallable)
	void SetState(EZombieState NewState);
};
