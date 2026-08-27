// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPC.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LobbyWidgetBase.h"
#include "Kismet/GameplayStatics.h"
#include "LobbyGM.h"

//연속된 메모리 저장?
//임의 접근의 시간복잡도가 상수시간.
//추가, 삭제가 오래 걸림. [][][][] -> [][][][][]
//#include <vector>
//// 검색이 느리다.
//// 추가, 삭제가 빠르다.
//#include <list>


void ALobbyPC::BeginPlay()
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyPC::BeginPlay Begin"));

	Super::BeginPlay();

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyPC::BeginPlay End"));

	//listen서버이고 방장일때만
	if (HasAuthority() && LobbyWidgetObject)
	{
		ALobbyGM* GM = Cast<ALobbyGM>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GM)
		{
			GM->CountConnection();
		}
	}
}

void ALobbyPC::OnPossess(APawn* aPawn)
{
	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyPC::OnPossess Begin"));

	Super::OnPossess(aPawn);

	//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("ALobbyPC::OnPossess End"));
}

bool ALobbyPC::C2S_SendMessage_Validate(const FText& Message)
{
	return true;
}

void ALobbyPC::C2S_SendMessage_Implementation(const FText& Message)
{

	//for (int i = 0; i < 10; ++i)
	//{
	//	Data[i];
	//}

	//std::list<int> Data2;
	////for (int i = 0; i < 10; ++i)
	////{
	////	Data2[i];
	////}

	//std::vector<int> Data;

	//for (auto iter = Data.rbegin(); iter != Data.rend(); ++iter)
	//{
	//	//Data2[i];
	//}
	//for (auto iter = Data2.rbegin(); iter != Data2.rend(); ++iter)
	//{
	//	//Data2[i];
	//}

	//for (auto Value : Data2)
	//{

	//}



	
	for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		ALobbyPC* PC = Cast<ALobbyPC>(*Iter);
		if (PC)
		{
			//send client
			PC->S2C_SendMessage(Message);
		}
			
	}
}

void ALobbyPC::S2C_SendMessage_Implementation(const FText& Message)
{
	//execute client
	if (LobbyWidgetObject)
	{
		LobbyWidgetObject->AddMessage(Message);
	}
}

void ALobbyPC::S2C_ShowLoadingScreen_Implementation()
{
	if (LoadingScreenWidgetTemplate)
	{
		LoadingScreenWidgetObject = CreateWidget<UUserWidget>(this, LoadingScreenWidgetTemplate);
		LoadingScreenWidgetObject->AddToViewport();
	}
}
