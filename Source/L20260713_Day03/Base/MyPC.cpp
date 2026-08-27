// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPC.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "MyPlayerCameraManager.h"

AMyPC::AMyPC()
{
	PlayerCameraManagerClass = AMyPlayerCameraManager::StaticClass();
}

void AMyPC::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
			}
		}

	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}
