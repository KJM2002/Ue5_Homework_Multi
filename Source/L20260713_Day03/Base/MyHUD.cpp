// Fill out your copyright notice in the Description page of Project Settings.


#include "MyHUD.h"
#include "Engine/Canvas.h"
#include "MyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void AMyHUD::DrawHUD()
{
	Super::DrawHUD();

	int32 Unit = Canvas->SizeX / 100;

	int32 CenterX = Canvas->SizeX / 2;
	int32 CenterY = Canvas->SizeY / 2;
	int32 DrawSize = Unit * 2;

	float CurrentSpeed = 0.f;
	float MaxSpeed = 0.0f;
	float GapRatio = 0.0f;
	int32 Gap = Unit * 3;


	ACharacter* Pawn = Cast<ACharacter>(GetOwningPawn());

	if (Pawn)
	{
		MaxSpeed = Pawn->GetCharacterMovement()->GetMaxSpeed();
		CurrentSpeed = Pawn->GetCharacterMovement()->Velocity.Size2D();
		GapRatio = CurrentSpeed / MaxSpeed;
	}
	Gap = (int32)((float)Gap * GapRatio);


	Draw2DLine(CenterX - Unit - Gap,
		CenterY,
		CenterX - Gap,
		CenterY,
		FColor::Red);

	Draw2DLine(CenterX + Gap,
		CenterY,
		CenterX + Unit + Gap,
		CenterY,
		FColor::Red);

	Draw2DLine(CenterX,
		CenterY - Unit - Gap,
		CenterX,
		CenterY - Gap,
		FColor::Red);

	Draw2DLine(CenterX,
		CenterY + Gap,
		CenterX,
		CenterY + Unit + Gap,
		FColor::Red);
}
