// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "../Base/MyCharacter.h"
#include "Zombie.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Hearing.h"


AZombieAIController::AZombieAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception.Get());

	UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius = 500.0f;
	Sight->LoseSightRadius = 600.0f;
	Sight->PeripheralVisionAngleDegrees = 60.0f;
	Sight->DetectionByAffiliation.bDetectEnemies = true;
	Sight->DetectionByAffiliation.bDetectNeutrals = false;
	Sight->DetectionByAffiliation.bDetectFriendlies = false;
	Perception->ConfigureSense(*Sight);
	Perception->SetDominantSense(*Sight->GetSenseImplementation());
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (RunTree)
	{
		RunBehaviorTree(RunTree);
	}

	if (Perception)
	{
		//Perception->OnTargetPerceptionForgotten.AddDynamic(this, &AZombieAIController::OnActorPerceptionForgetUpdated);

		Perception->OnPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnPerceptionUpdated);

		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnActorPerceptionUpdated);

		//Perception->OnTargetPerceptionInfoUpdated.AddDynamic(this, &AZombieAIController::OnPerceptionInfoUpdated);
	}


	SetGenericTeamId(3);


}

void AZombieAIController::OnUnPossess()
{
	BrainComponent->StopLogic(TEXT("UnPossess"));

	Super::OnUnPossess();
}

void AZombieAIController::BeginPlay()
{
	Super::BeginPlay();


}

void AZombieAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	UE_LOG(LogTemp, Warning, TEXT("OnPerceptionUpdated Update"));

}

void AZombieAIController::OnActorPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UE_LOG(LogTemp, Warning, TEXT("OnActorPerceptionUpdated Update %s"), *Actor->GetName());

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			AMyCharacter* Player = Cast<AMyCharacter>(Actor);
			AZombie* Zombie = Cast<AZombie>(GetPawn());
			if (Zombie && Player)
			{
				Blackboard->SetValueAsObject(FName(TEXT("Player")), Player);
				SetState(EZombieState::Chase);

				UE_LOG(LogTemp, Warning, TEXT("OnActorPerceptionUpdated EZombieState::Chase"));

			}
		}
		else
		{
			AMyCharacter* Player = Cast<AMyCharacter>(Actor);
			AZombie* Zombie = Cast<AZombie>(GetPawn());
			if (Zombie && Player)
			{
				Blackboard->SetValueAsObject(FName(TEXT("Player")), Player);
				SetState(EZombieState::Normal);
			}
		}
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnActorPerceptionUpdated Hearing %s"), *Actor->GetName());
 
		if (Stimulus.WasSuccessfullySensed())
		{
		}
	}
}

void AZombieAIController::OnActorPerceptionForgetUpdated(AActor* Actor)
{
	UE_LOG(LogTemp, Warning, TEXT("Perception Forgotten %s"), *Actor->GetName());
}

void AZombieAIController::OnPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("OnPerceptionInfoUpdated %s"), *UpdateInfo.Stimulus.Type.Name.ToString());

}


void AZombieAIController::SetState(EZombieState NewState)
{
	AZombie* Zombie = Cast<AZombie>(GetPawn());
	if (Zombie)
	{
		Zombie->CurrentState = NewState;
		Blackboard->SetValueAsEnum(FName(TEXT("CurrentState")), (uint8)NewState);

	}
}

