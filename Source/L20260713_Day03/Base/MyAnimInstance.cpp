// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimInstance.h"
#include "MyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AMyCharacter* Character = Cast<AMyCharacter>(TryGetPawnOwner());
	if (Character)
	{
		//Syncronize Pawn data.
		Speed = Character->GetCharacterMovement()->Velocity.Size2D();
		Direction = UKismetAnimationLibrary::CalculateDirection(Character->GetCharacterMovement()->Velocity, Character->GetActorRotation());
		bArmed = Character->bArmed;

		AimPitch = Character->GetAimOffset().Pitch;

		bZoom = Character->bZoom;
		LeanValue = Character->LeanValue;
	}
}

//Skeleton Notify
void UMyAnimInstance::AnimNotify_ReloadComplete2(UAnimNotify* Notify)
{
	UE_LOG(LogTemp, Warning, TEXT("AnimNotify_ReloadComplete2"));
}
