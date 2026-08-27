// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_Sample.h"

FString UAnimNotifyState_Sample::GetNotifyName_Implementation() const
{
	return TEXT("Sample");
}

void UAnimNotifyState_Sample::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UE_LOG(LogTemp, Warning, TEXT("NotifyBegin"));
}

void UAnimNotifyState_Sample::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	UE_LOG(LogTemp, Warning, TEXT("NotifyTick"));
}

void UAnimNotifyState_Sample::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UE_LOG(LogTemp, Warning, TEXT("NotifyEnd"));
}
