// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API UDataGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString Password;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString ServerIP;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	bool bLoggedIn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Idx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	int32 Level = 0;


	// 웹서버에서 받아온 접속 대상 게임서버. 사용자가 입력하지 않는다.

	UPROPERTY(BlueprintReadOnly, Category = "GameServer")
	bool bServerFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "GameServer")
	FString GameServerIP;

	UPROPERTY(BlueprintReadOnly, Category = "GameServer")
	int32 GameServerPort = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GameServer")
	FString GameServerOwner;

};
