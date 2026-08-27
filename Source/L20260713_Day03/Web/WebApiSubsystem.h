// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "WebApiSubsystem.generated.h"

class FJsonObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWebApiResultSignature, const bool, bInSuccess, const FString&, InMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FWebApiServerFoundSignature, const bool, bInSuccess, const FString&, InMessage, const FString&, InServerIP, const int32, InPort, const FString&, InOwnerNickname);

/**
 * 웹서버와의 HTTP 통신을 전담한다. 결과는 델리게이트로만 알린다.
 *
 * 서버 등록 상태와 heartbeat 타이머를 여기에 둔다. GameInstance 수명이라
 * ALobbyGM::StartGame()의 ServerTravel로 레벨이 넘어가도 살아남는다.
 * GameMode에 두면 로비→게임 전환 시점에 살아 있는 서버가 등록 해제된다.
 */
UCLASS()
class L20260713_DAY03_API UWebApiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiResultSignature OnLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiResultSignature OnSignUpResult;

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiResultSignature OnRegisterServerResult;

	UPROPERTY(BlueprintAssignable, Category = "WebApi")
	FWebApiServerFoundSignature OnLatestServerResult;

	void RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword);

	void RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword);

	/** 자기 주소를 웹서버에 등록하고 heartbeat를 시작한다. 서버로 시작했을 때만 부른다. */
	void StartServerRegistration(const FString& InWebServerIP, const int32 InOwnerIdx, const int32 InGamePort);

	/** heartbeat를 멈추고 등록을 해제한다. 응답을 기다리지 않는다. */
	void StopServerRegistration();

	/** 접속 가능한 최신 서버 하나를 조회한다. */
	void RequestLatestServer(const FString& InWebServerIP);

private:

	void SendAuthRequest(const FString& InServerIP, const FString& InPath,
		const FString& InUserID, const FString& InPassword,
		FWebApiResultSignature& InDelegate, const bool bInIsLogin);

	void HandleAuthResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
		FWebApiResultSignature& InDelegate, const bool bInIsLogin);

	/** 등록 계열 POST 요청 한 건을 만든다. 본문은 {owner_idx, port}. */
	FHttpRequestRef MakeServerRequest(const FString& InPath) const;

	void SendHeartbeat();

	void HandleRegisterResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully);

	void HandleHeartbeatResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully);

	void HandleLatestServerResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully);

	FString BuildUrl(const FString& InServerIP, const FString& InPath) const;

	/** HTTP 200 + 본문 result 플래그로 성공을 판정한다. 실패 사유는 OutMessage로 준다. */
	static bool ParseResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
		TSharedPtr<FJsonObject>& OutJson, FString& OutMessage);

	// 등록 상태. 레벨 이동에 살아남아야 하므로 GameMode가 아니라 여기에 둔다.
	FString RegisteredWebServerIP;
	int32 RegisteredOwnerIdx = 0;
	int32 RegisteredGamePort = 0;
	bool bServerRegistrationActive = false;

	FTimerHandle HeartbeatTimerHandle;
};
