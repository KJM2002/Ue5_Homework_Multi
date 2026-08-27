// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerCameraManager.h"
#include "MyCharacter.h"

AMyPlayerCameraManager::AMyPlayerCameraManager()
{
}

void AMyPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);

	AMyCharacter* Player = Cast<AMyCharacter>(GetOwningPlayerController()->GetPawn());
	if (Player)
	{
		float TargetFOV = Player->bZoom ? ZoomFOV : NormalFOV;
		float CurrentFOV = FMath::FInterpTo(GetFOVAngle(), TargetFOV, DeltaTime, ZoomSpeed);
		SetFOV(CurrentFOV);
	}
}
