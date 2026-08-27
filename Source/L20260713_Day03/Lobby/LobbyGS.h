// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGS.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeConnectCountSignature, const int32, InConnectionCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLeftTimeSignature, const int32, InLeftTime);

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API ALobbyGS : public AGameStateBase
{
	GENERATED_BODY()
public:

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", ReplicatedUsing="OnRep_ConnectionCount")
	int32 ConnectionCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", ReplicatedUsing="OnRep_LeftTime")
	int32 LeftTime = 60;

	UFUNCTION()
	void OnRep_ConnectionCount();

	UFUNCTION()
	void OnRep_LeftTime();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	UPROPERTY(BlueprintAssignable)
	FChangeLeftTimeSignature OnChangeLeftTime;

	UPROPERTY(BlueprintAssignable)
	FChangeLeftTimeSignature OnChangeConnectionCount;
};
