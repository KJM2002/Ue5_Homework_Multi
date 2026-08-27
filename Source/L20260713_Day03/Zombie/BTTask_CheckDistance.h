// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "Zombie.h"

#include "BTTask_CheckDistance.generated.h"

UENUM(BlueprintType)
enum class ECondition : uint8
{
	LessThan = 0				UMETA(DisplayName = "<"),
	GreaterThan = 10			UMETA(DisplayName = ">"),
};

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API UBTTask_CheckDistance : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_CheckDistance();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	ECondition TargetCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	EZombieState TargetState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	float TargetDistance;
};
