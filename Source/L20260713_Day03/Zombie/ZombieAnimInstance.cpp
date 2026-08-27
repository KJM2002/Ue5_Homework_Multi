// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void UZombieAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AZombie* Zombie = Cast<AZombie>(TryGetPawnOwner());
	if (Zombie)
	{
		Speed = Zombie->GetCharacterMovement()->Velocity.Size2D();
		CurrentState = Zombie->CurrentState;
	}
}
