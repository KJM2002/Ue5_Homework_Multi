// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGM.generated.h"

/**
 * 
 */
UCLASS()
class L20260713_DAY03_API ALobbyGM : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;


	virtual void StartPlay() override;

	virtual void BeginPlay() override;

	FTimerHandle LeftTimeHandle;


	void CountConnection();

	void CountDownLeftTime();


	void StopTimer();

	void StartGame();

private:

	/**
	 * 자기 주소를 웹서버에 등록한다. GameMode는 서버에만 존재하므로
	 * 넷모드 분기는 Standalone 제외만 필요하다.
	 *
	 * 등록 상태와 heartbeat는 UWebApiSubsystem이 들고 있다. 여기서 들면
	 * StartGame()의 ServerTravel로 이 GameMode가 파괴될 때 살아 있는
	 * 서버가 등록 해제된다.
	 */
	void RegisterToWebServer();

};
