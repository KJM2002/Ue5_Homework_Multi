// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGS.h"
#include "Net/UnrealNetwork.h"


//ConnectionCount값이 바뀌면 호출 됨
//C++에서는 서버에서는 호출 안됨
//Client에서만 호출됨
//BP에서는 둘다 됨.
void ALobbyGS::OnRep_ConnectionCount()
{
	//UI 업데이트 
	//UI 찾아서 위젯 값 넣어주고 화면 갱신
	OnChangeConnectionCount.Broadcast(ConnectionCount);
}

void ALobbyGS::OnRep_LeftTime()
{
	OnChangeLeftTime.Broadcast(LeftTime);
}

void ALobbyGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGS, ConnectionCount);
	DOREPLIFETIME(ALobbyGS, LeftTime);
}
