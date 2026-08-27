// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGM.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "LobbyGS.h"
#include "LobbyPC.h"
#include "../DataGameInstanceSubsystem.h"
#include "../Web/WebApiSubsystem.h"

void ALobbyGM::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::PreLogin Begin"));

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::PreLogin End"));
}

APlayerController* ALobbyGM::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::Login Begin"));

	APlayerController* PC =  Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::Login End"));

	return PC;
}

void ALobbyGM::PostLogin(APlayerController* NewPlayer)
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::PostLogin Begin"));

	Super::PostLogin(NewPlayer);

	CountConnection();
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::PostLogin End"));

}

void ALobbyGM::Logout(AController* Exiting)
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::Logout Begin"));
	
	Super::Logout(Exiting);

	CountConnection();

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::Logout End"));
}

void ALobbyGM::StartPlay()
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::StartPlay Begin"));

	Super::StartPlay();

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyGM::StartPlay End"));
}

void ALobbyGM::BeginPlay()
{
	Super::BeginPlay();

	RegisterToWebServer();


	GetWorld()->GetTimerManager().SetTimer(
		LeftTimeHandle,
		FTimerDelegate::CreateLambda([this]() {
			CountDownLeftTime();
		}),
		1.0f,
		true,
		0.0f
	);
}

void ALobbyGM::RegisterToWebServer()
{
	// 혼자 플레이는 서버가 아니다. 리슨/데디케이티드만 등록한다.
	if (GetNetMode() == NM_Standalone)
	{
		return;
	}

	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	if (!GameInstance)
	{
		return;
	}

	UDataGameInstanceSubsystem* Data = GameInstance->GetSubsystem<UDataGameInstanceSubsystem>();
	UWebApiSubsystem* WebApi = GameInstance->GetSubsystem<UWebApiSubsystem>();
	if (!Data || !WebApi)
	{
		return;
	}

	// 웹서버 주소와 로그인한 회원 idx는 타이틀 화면에서 이미 저장했다.
	// 게임서버 포트는 리슨 서버로 열린 실제 포트를 쓴다.
	WebApi->StartServerRegistration(Data->ServerIP, Data->Idx, GetWorld()->URL.Port);
}

void ALobbyGM::CountConnection()
{
	int Count = GetNumPlayers();
	//for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	//{
	//	Count++;
	//}

	ALobbyGS* GS = GetGameState<ALobbyGS>();
	if (GS)
	{
		GS->ConnectionCount = Count;

		//ReplicatedUsing������ C++������ ȣ���� �ȵ�.
		GS->OnRep_ConnectionCount();
	}
}

void ALobbyGM::CountDownLeftTime()
{

	ALobbyGS* GS = GetGameState<ALobbyGS>();
	if (GS)
	{
		GS->LeftTime--;
		GS->LeftTime = FMath::Clamp(GS->LeftTime, 0, 60);

		//ReplicatedUsing������ C++������ ȣ���� �ȵ�.
		GS->OnRep_LeftTime();

		if (GS->LeftTime <= 0)
		{
			StartGame();
		}
	}
}

void ALobbyGM::StopTimer()
{
	GetWorldTimerManager().ClearTimer(
		LeftTimeHandle
	);
}

void ALobbyGM::StartGame()
{
	StopTimer();
	for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		ALobbyPC* PC = Cast<ALobbyPC>(*Iter);
		if (PC)
		{
			PC->S2C_ShowLoadingScreen();
		}
	}


	GetWorld()->ServerTravel(TEXT("Lvl_ThirdPerson"));


}
