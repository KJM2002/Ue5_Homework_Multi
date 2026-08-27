// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_CheckDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Zombie.h"

UBTTask_CheckDistance::UBTTask_CheckDistance()
{
	NodeName = TEXT("CheckDistance");
}

EBTNodeResult::Type UBTTask_CheckDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AActor* Player = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TEXT("Player")));
	AZombie* Zombie = Cast<AZombie>(OwnerComp.GetAIOwner()->GetPawn());

	if (Zombie && Player)
	{
		FVector ZombieLocation = Zombie->GetActorLocation();
		FVector PlayerLocation = Player->GetActorLocation();
		float Distance = FVector::Distance(ZombieLocation, PlayerLocation);
		
		switch (TargetCondition)
		{
			case ECondition::GreaterThan:
			{
				if (Distance > TargetDistance)
				{
					Zombie->SetState(TargetState);
					OwnerComp.GetBlackboardComponent()->SetValueAsEnum(GetSelectedBlackboardKey(), (uint8)TargetState);
				}
			}
			break;
			case ECondition::LessThan:
			{
				if (Distance < TargetDistance)
				{
					Zombie->SetState(TargetState);
					OwnerComp.GetBlackboardComponent()->SetValueAsEnum(GetSelectedBlackboardKey(), (uint8)TargetState);
				}
			}
		}
	}

	return EBTNodeResult::Succeeded;
}
