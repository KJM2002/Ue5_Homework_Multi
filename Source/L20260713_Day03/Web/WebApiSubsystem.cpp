// Fill out your copyright notice in the Description page of Project Settings.


#include "WebApiSubsystem.h"
#include "HttpModule.h"
#include "Engine/GameInstance.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "../DataGameInstanceSubsystem.h"

namespace
{
	constexpr int32 WebServerPort = 8080;

	// 언리얼 리슨 서버 기본 포트.
	constexpr int32 DefaultGamePort = 7777;

	// 웹서버의 활성 판정 창이 30초다. 그 1/3로 보내 두 번 놓쳐도 버틴다.
	constexpr float HeartbeatIntervalSeconds = 10.0f;
}

void UWebApiSubsystem::Deinitialize()
{
	// 정상 종료 경로. 여기서 못 보내도 웹서버가 30초 뒤 만료 처리한다.
	StopServerRegistration();

	Super::Deinitialize();
}

void UWebApiSubsystem::RequestLogin(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/login"), InUserID, InPassword, OnLoginResult, true);
}

void UWebApiSubsystem::RequestSignUp(const FString& InServerIP, const FString& InUserID, const FString& InPassword)
{
	SendAuthRequest(InServerIP, TEXT("/signup"), InUserID, InPassword, OnSignUpResult, false);
}

void UWebApiSubsystem::SendAuthRequest(const FString& InServerIP, const FString& InPath,
	const FString& InUserID, const FString& InPassword,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("user_id"), InUserID);
	JsonObject->SetStringField(TEXT("passwd"), InPassword);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const FString Url = BuildUrl(InServerIP, InPath);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Url);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	// 응답이 도착하기 전에 GameInstance가 정리될 수 있으므로 약참조로 잡는다.
	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);
	FWebApiResultSignature* DelegatePtr = &InDelegate;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, DelegatePtr, bInIsLogin](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleAuthResponse(InResponse, bInConnectedSuccessfully, *DelegatePtr, bInIsLogin);
		});

	Request->ProcessRequest();
}

void UWebApiSubsystem::HandleAuthResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
	FWebApiResultSignature& InDelegate, const bool bInIsLogin)
{
	if (!bInConnectedSuccessfully || !InResponse.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("서버에 연결할 수 없습니다"));
		return;
	}

	const int32 ResponseCode = InResponse->GetResponseCode();
	if (ResponseCode != 200)
	{
		InDelegate.Broadcast(false, FString::Printf(TEXT("요청을 처리할 수 없습니다 (코드 %d)"), ResponseCode));
		return;
	}

	const FString ResponseBody = InResponse->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		InDelegate.Broadcast(false, TEXT("응답을 해석할 수 없습니다"));
		return;
	}

	if (!JsonObject->GetBoolField(TEXT("result")))
	{
		InDelegate.Broadcast(false, JsonObject->GetStringField(TEXT("message")));
		return;
	}

	if (bInIsLogin)
	{
		UDataGameInstanceSubsystem* Data = GetGameInstance()->GetSubsystem<UDataGameInstanceSubsystem>();
		if (Data)
		{
			Data->Idx = JsonObject->GetIntegerField(TEXT("idx"));
			Data->Nickname = JsonObject->GetStringField(TEXT("nickname"));
			Data->Level = JsonObject->GetIntegerField(TEXT("level"));
			Data->bLoggedIn = true;
		}
	}

	InDelegate.Broadcast(true, TEXT(""));
}

void UWebApiSubsystem::StartServerRegistration(const FString& InWebServerIP, const int32 InOwnerIdx, const int32 InGamePort)
{
	if (InWebServerIP.IsEmpty())
	{
		OnRegisterServerResult.Broadcast(false, TEXT("웹서버 주소가 없습니다"));
		return;
	}

	RegisteredWebServerIP = InWebServerIP;
	RegisteredOwnerIdx = InOwnerIdx;
	RegisteredGamePort = InGamePort > 0 ? InGamePort : DefaultGamePort;
	bServerRegistrationActive = true;

	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);

	FHttpRequestRef Request = MakeServerRequest(TEXT("/server/register"));
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleRegisterResponse(InResponse, bInConnectedSuccessfully);
		});
	Request->ProcessRequest();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		// GameInstance의 타이머 매니저를 쓴다. World의 것을 쓰면 ServerTravel에서 끊긴다.
		GameInstance->GetTimerManager().SetTimer(
			HeartbeatTimerHandle,
			this,
			&UWebApiSubsystem::SendHeartbeat,
			HeartbeatIntervalSeconds,
			true
		);
	}
}

void UWebApiSubsystem::StopServerRegistration()
{
	if (!bServerRegistrationActive)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
	}

	// 요청을 만든 뒤에 상태를 지운다. MakeServerRequest가 상태를 읽는다.
	FHttpRequestRef Request = MakeServerRequest(TEXT("/server/unregister"));
	Request->ProcessRequest();

	bServerRegistrationActive = false;
	RegisteredWebServerIP.Empty();
	RegisteredOwnerIdx = 0;
	RegisteredGamePort = 0;
}

void UWebApiSubsystem::RequestLatestServer(const FString& InWebServerIP)
{
	if (InWebServerIP.IsEmpty())
	{
		OnLatestServerResult.Broadcast(false, TEXT("웹서버 주소를 입력해 주세요"), TEXT(""), 0, TEXT(""));
		return;
	}

	const FString Url = BuildUrl(InWebServerIP, TEXT("/server/latest"));
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Url);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));

	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleLatestServerResponse(InResponse, bInConnectedSuccessfully);
		});

	Request->ProcessRequest();
}

FHttpRequestRef UWebApiSubsystem::MakeServerRequest(const FString& InPath) const
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetNumberField(TEXT("owner_idx"), RegisteredOwnerIdx);
	JsonObject->SetNumberField(TEXT("port"), RegisteredGamePort);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const FString Url = BuildUrl(RegisteredWebServerIP, InPath);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Url);

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	return Request;
}

void UWebApiSubsystem::SendHeartbeat()
{
	if (!bServerRegistrationActive)
	{
		return;
	}

	TWeakObjectPtr<UWebApiSubsystem> WeakThis(this);

	FHttpRequestRef Request = MakeServerRequest(TEXT("/server/heartbeat"));
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr, FHttpResponsePtr InResponse, bool bInConnectedSuccessfully)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->HandleHeartbeatResponse(InResponse, bInConnectedSuccessfully);
		});
	Request->ProcessRequest();
}

void UWebApiSubsystem::HandleRegisterResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> JsonObject;
	FString Message;
	if (!ParseResponse(InResponse, bInConnectedSuccessfully, JsonObject, Message))
	{
		OnRegisterServerResult.Broadcast(false, Message);
		return;
	}

	const FString RegisteredIP = JsonObject->GetStringField(TEXT("server_ip"));
	const int32 RegisteredPort = JsonObject->GetIntegerField(TEXT("port"));

	UE_LOG(LogTemp, Warning, TEXT("서버 등록 완료: %s:%d"), *RegisteredIP, RegisteredPort);

	OnRegisterServerResult.Broadcast(true, FString::Printf(TEXT("%s:%d"), *RegisteredIP, RegisteredPort));
}

void UWebApiSubsystem::HandleHeartbeatResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> JsonObject;
	FString Message;
	if (ParseResponse(InResponse, bInConnectedSuccessfully, JsonObject, Message))
	{
		return;
	}

	if (!bServerRegistrationActive)
	{
		return;
	}

	// 웹서버가 우리를 모른다면(DB 초기화·행 삭제) 다시 등록한다.
	// 연결 자체가 실패한 경우는 다음 heartbeat가 재시도한다.
	if (bInConnectedSuccessfully)
	{
		UE_LOG(LogTemp, Warning, TEXT("heartbeat 실패(%s) — 재등록한다"), *Message);
		StartServerRegistration(RegisteredWebServerIP, RegisteredOwnerIdx, RegisteredGamePort);
	}
}

void UWebApiSubsystem::HandleLatestServerResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully)
{
	UDataGameInstanceSubsystem* Data = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDataGameInstanceSubsystem>()
		: nullptr;

	TSharedPtr<FJsonObject> JsonObject;
	FString Message;
	if (!ParseResponse(InResponse, bInConnectedSuccessfully, JsonObject, Message))
	{
		if (Data)
		{
			Data->bServerFound = false;
			Data->GameServerIP.Empty();
			Data->GameServerPort = 0;
			Data->GameServerOwner.Empty();
		}

		OnLatestServerResult.Broadcast(false, Message, TEXT(""), 0, TEXT(""));
		return;
	}

	const FString FoundIP = JsonObject->GetStringField(TEXT("server_ip"));
	const int32 FoundPort = JsonObject->GetIntegerField(TEXT("port"));
	const FString OwnerNickname = JsonObject->GetStringField(TEXT("nickname"));

	if (Data)
	{
		Data->bServerFound = true;
		Data->GameServerIP = FoundIP;
		Data->GameServerPort = FoundPort;
		Data->GameServerOwner = OwnerNickname;
	}

	OnLatestServerResult.Broadcast(true, TEXT(""), FoundIP, FoundPort, OwnerNickname);
}

FString UWebApiSubsystem::BuildUrl(const FString& InServerIP, const FString& InPath) const
{
	return FString::Printf(TEXT("http://%s:%d%s"), *InServerIP, WebServerPort, *InPath);
}

bool UWebApiSubsystem::ParseResponse(FHttpResponsePtr InResponse, const bool bInConnectedSuccessfully,
	TSharedPtr<FJsonObject>& OutJson, FString& OutMessage)
{
	if (!bInConnectedSuccessfully || !InResponse.IsValid())
	{
		OutMessage = TEXT("서버에 연결할 수 없습니다");
		return false;
	}

	const int32 ResponseCode = InResponse->GetResponseCode();
	if (ResponseCode != 200)
	{
		OutMessage = FString::Printf(TEXT("요청을 처리할 수 없습니다 (코드 %d)"), ResponseCode);
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InResponse->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, OutJson) || !OutJson.IsValid())
	{
		OutMessage = TEXT("응답을 해석할 수 없습니다");
		return false;
	}

	if (!OutJson->GetBoolField(TEXT("result")))
	{
		OutMessage = OutJson->GetStringField(TEXT("message"));
		return false;
	}

	return true;
}
