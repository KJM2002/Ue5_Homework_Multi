// Fill out your copyright notice in the Description page of Project Settings.


#include "TitlePC.h"
#include "TitleWidgetBase.h"


void ATitlePC::BeginPlay()
{
	Super::BeginPlay();

	//path
	FSoftClassPath TitleWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/Blueprints/Title/UI/WBP_Title.WBP_Title_C'"));

	//#include
	UClass* WidgetClass = TitleWidgetClass.TryLoadClass<UTitleWidgetBase>();
	if (WidgetClass)
	{
		//new
		TitleWidgetInstance = CreateWidget<UTitleWidgetBase>(this, WidgetClass);
		if (TitleWidgetInstance)
		{
			TitleWidgetInstance->AddToViewport();
		}
	}

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());

	UE_LOG(LogTemp, Warning, TEXT("Title %d"), GetNetMode());
}
