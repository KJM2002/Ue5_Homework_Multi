// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_ReloadComplete.h"

FString UAnimNotify_ReloadComplete::GetNotifyName_Implementation() const
{
	return TEXT("ReloadComplete_CPP");
}

void UAnimNotify_ReloadComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	TEXT("ReloadComplete_CPP2");

}
