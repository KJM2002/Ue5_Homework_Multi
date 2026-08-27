// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleWidgetBase.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "../DataGameInstanceSubsystem.h"
#include "../Web/WebApiSubsystem.h"

void UTitleWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();


	StartServerButton = Cast<UButton>(GetWidgetFromName(TEXT("StartServerButton")));
	if (StartServerButton)
	{
		StartServerButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::StartServer);
		StartServerButton->SetIsEnabled(false);
	}

	if (ConnectServerButton)
	{
		ConnectServerButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::ConnectServer);
		ConnectServerButton->SetIsEnabled(false);
	}

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::Login);
	}

	if (SignUpButton)
	{
		SignUpButton->OnClicked.AddDynamic(this, &UTitleWidgetBase::SignUp);
	}

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		WebApi->OnLoginResult.AddUniqueDynamic(this, &UTitleWidgetBase::ProcessLoginResult);
		WebApi->OnSignUpResult.AddUniqueDynamic(this, &UTitleWidgetBase::ProcessSignUpResult);
		WebApi->OnLatestServerResult.AddUniqueDynamic(this, &UTitleWidgetBase::ProcessLatestServerResult);
	}
}

void UTitleWidgetBase::StartServer()
{
	if (!IsLoggedIn())
	{
		SetInfoText(TEXT("먼저 로그인해 주세요"));
		return;
	}

	SaveData();

	UGameplayStatics::OpenLevel(GetWorld(),
		TEXT("Lobby"),
		true,
		TEXT("Listen")
	);
}

void UTitleWidgetBase::ConnectServer()
{
	if (!IsLoggedIn())
	{
		SetInfoText(TEXT("먼저 로그인해 주세요"));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UDataGameInstanceSubsystem* Data = GI ? GI->GetSubsystem<UDataGameInstanceSubsystem>() : nullptr;
	if (!Data || !Data->bServerFound || Data->GameServerIP.IsEmpty())
	{
		SetInfoText(TEXT("접속 가능한 서버가 없습니다"));
		return;
	}

	SaveData();

	// 접속 대상은 웹서버에서 받은 주소다. ServerIP 입력칸은 웹서버 주소이지
	// 게임서버 주소가 아니다.
	const FString Destination = FString::Printf(TEXT("%s:%d"), *Data->GameServerIP, Data->GameServerPort);

	UGameplayStatics::OpenLevel(GetWorld(),
		FName(*Destination),
		true,
		TEXT("Key=100")
	);
}

void UTitleWidgetBase::SaveData()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	if (GI)
	{
		UDataGameInstanceSubsystem* MySubSystem = GI->GetSubsystem<UDataGameInstanceSubsystem>();
		if (MySubSystem)
		{
			MySubSystem->UserID = UserID->GetText().ToString();
			MySubSystem->Password = Password->GetText().ToString();
			MySubSystem->ServerIP = ServerIP->GetText().ToString();
		}
	}
}

void UTitleWidgetBase::Login()
{
	if (!ValidateInput())
	{
		return;
	}

	if (bRequestInFlight)
	{
		return;
	}

	ClearLoginState();

	SaveData();

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		SetInfoText(TEXT("로그인 중..."));
		WebApi->RequestLogin(ServerIP->GetText().ToString(),
			UserID->GetText().ToString(),
			Password->GetText().ToString());
		bRequestInFlight = true;
	}
}

void UTitleWidgetBase::SignUp()
{
	if (!ValidateInput())
	{
		return;
	}

	if (bRequestInFlight)
	{
		return;
	}

	ClearLoginState();

	SaveData();

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		SetInfoText(TEXT("가입 중..."));
		WebApi->RequestSignUp(ServerIP->GetText().ToString(),
			UserID->GetText().ToString(),
			Password->GetText().ToString());
		bRequestInFlight = true;
	}
}

void UTitleWidgetBase::ProcessLoginResult(const bool bInSuccess, const FString& InMessage)
{
	bRequestInFlight = false;

	if (!bInSuccess)
	{
		SetInfoText(InMessage);
		return;
	}

	// 서버를 직접 여는 건 로그인만 되면 가능하다.
	if (StartServerButton)
	{
		StartServerButton->SetIsEnabled(true);
	}

	// 접속은 등록된 서버가 있어야 가능하다. 조회 결과가 올 때까지 잠가둔다.
	if (ConnectServerButton)
	{
		ConnectServerButton->SetIsEnabled(false);
	}

	SetInfoText(FString::Printf(TEXT("%s · 서버 조회 중..."), *GetLoginLabel()));

	if (UWebApiSubsystem* WebApi = GetWebApi())
	{
		WebApi->RequestLatestServer(ServerIP->GetText().ToString());
	}
}

void UTitleWidgetBase::ProcessLatestServerResult(const bool bInSuccess, const FString& InMessage,
	const FString& InServerIP, const int32 InPort, const FString& InOwnerNickname)
{
	if (!bInSuccess)
	{
		if (ConnectServerButton)
		{
			ConnectServerButton->SetIsEnabled(false);
		}

		SetInfoText(FString::Printf(TEXT("%s · %s"), *GetLoginLabel(), *InMessage));
		return;
	}

	if (ConnectServerButton)
	{
		ConnectServerButton->SetIsEnabled(true);
	}

	const FString OwnerLabel = InOwnerNickname.IsEmpty() ? TEXT("알 수 없음") : InOwnerNickname;

	SetInfoText(FString::Printf(TEXT("%s · 접속 가능: %s:%d (%s)"),
		*GetLoginLabel(), *InServerIP, InPort, *OwnerLabel));
}

void UTitleWidgetBase::ProcessSignUpResult(const bool bInSuccess, const FString& InMessage)
{
	bRequestInFlight = false;

	SetInfoText(bInSuccess ? TEXT("가입이 완료되었습니다. 로그인해 주세요.") : InMessage);
}

UWebApiSubsystem* UTitleWidgetBase::GetWebApi() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWebApiSubsystem>() : nullptr;
}

bool UTitleWidgetBase::IsLoggedIn() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	UDataGameInstanceSubsystem* Data = GI->GetSubsystem<UDataGameInstanceSubsystem>();
	return Data && Data->bLoggedIn;
}

void UTitleWidgetBase::ClearLoginState()
{
	UGameInstance* GI = GetGameInstance();
	UDataGameInstanceSubsystem* Data = GI ? GI->GetSubsystem<UDataGameInstanceSubsystem>() : nullptr;
	if (Data)
	{
		Data->bLoggedIn = false;
		Data->Idx = 0;
		Data->Nickname.Empty();
		Data->Level = 0;

		Data->bServerFound = false;
		Data->GameServerIP.Empty();
		Data->GameServerPort = 0;
		Data->GameServerOwner.Empty();
	}

	if (StartServerButton)
	{
		StartServerButton->SetIsEnabled(false);
	}

	if (ConnectServerButton)
	{
		ConnectServerButton->SetIsEnabled(false);
	}
}

FString UTitleWidgetBase::GetLoginLabel() const
{
	UGameInstance* GI = GetGameInstance();
	UDataGameInstanceSubsystem* Data = GI ? GI->GetSubsystem<UDataGameInstanceSubsystem>() : nullptr;
	if (!Data)
	{
		return FString();
	}

	return FString::Printf(TEXT("%s (Lv.%d)"), *Data->Nickname, Data->Level);
}

void UTitleWidgetBase::SetInfoText(const FString& InMessage)
{
	if (InfoText)
	{
		InfoText->SetText(FText::FromString(InMessage));
	}
}

bool UTitleWidgetBase::ValidateInput()
{
	if (!UserID || !Password || !ServerIP)
	{
		return false;
	}

	if (UserID->GetText().IsEmptyOrWhitespace() || Password->GetText().IsEmptyOrWhitespace())
	{
		SetInfoText(TEXT("아이디와 비밀번호를 입력해 주세요"));
		return false;
	}

	if (ServerIP->GetText().IsEmptyOrWhitespace())
	{
		SetInfoText(TEXT("서버 주소를 입력해 주세요"));
		return false;
	}

	UserID->SetText(UserID->GetText().TrimPrecedingAndTrailing(UserID->GetText()));
	Password->SetText(Password->GetText().TrimPrecedingAndTrailing(Password->GetText()));
	ServerIP->SetText(ServerIP->GetText().TrimPrecedingAndTrailing(ServerIP->GetText()));

	return true;
}
