// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidgetBase.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "LobbyPC.h"
#include "../DataGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/RichTextBlock.h"
#include "LobbyGS.h"
#include "LobbyGM.h"


void ULobbyWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULobbyWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &ULobbyWidgetBase::ProcessStartServer);
	}

	if (ChatButton)
	{
		ChatButton->OnClicked.AddDynamic(this, &ULobbyWidgetBase::ProcessSendMessage);
	}

	if (ChatInput)
	{
		ChatInput->OnTextChanged.AddDynamic(this, &ULobbyWidgetBase::ProcessTextChanged);
		ChatInput->OnTextCommitted.AddDynamic(this, &ULobbyWidgetBase::ProcessTextCommited);
	}

	ALobbyGS* GS = Cast<ALobbyGS>(UGameplayStatics::GetGameState(GetWorld()));
	if (GS)
	{
		GS->OnChangeConnectionCount.AddDynamic(this, &ULobbyWidgetBase::ProcessChangeConnectionCount);
		GS->OnChangeLeftTime.AddDynamic(this, &ULobbyWidgetBase::ProcessChangeLeftTime);
	}

}

void ULobbyWidgetBase::ShowStartButton(bool IsShow)
{
	PlayShowStartButton();
}

void ULobbyWidgetBase::PlayShowStartButton_Implementation()
{
}

void ULobbyWidgetBase::ProcessTextCommited(const FText& Text, ETextCommit::Type CommitMethod)
{
	switch (CommitMethod)
	{
		case ETextCommit::OnEnter:
		{
			ALobbyPC* PC = Cast<ALobbyPC>(GetOwningPlayer());
			if (PC)
			{
				UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
				if (GI)
				{
					UDataGameInstanceSubsystem* MySubSystem = GI->GetSubsystem<UDataGameInstanceSubsystem>();
					if (MySubSystem)
					{
						FString Temp = FString::Printf(TEXT("%s : %s"), *MySubSystem->UserID, *Text.ToString());

						//call Local(Client)
						//Execute Remote(Server)
						PC->C2S_SendMessage(FText::FromString(Temp));
						ChatInput->SetText(FText::FromString(TEXT("")));
					}
				}


			}
		}
		break;

		case ETextCommit::OnCleared:
		{
			ChatInput->SetUserFocus(GetOwningPlayer());
		}
		break;
	}

}

void ULobbyWidgetBase::ProcessTextChanged(const FText& Text)
{
}

void ULobbyWidgetBase::ProcessStartServer()
{
	ALobbyGM* GM = Cast<ALobbyGM>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		GM->StartGame();
	}
}

void ULobbyWidgetBase::ProcessSendMessage()
{
	ALobbyPC* PC = Cast<ALobbyPC>(GetOwningPlayer());
	if (PC)
	{
		FText Text = ChatInput->GetText();
		UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
		if (GI)
		{
			UDataGameInstanceSubsystem* MySubSystem = GI->GetSubsystem<UDataGameInstanceSubsystem>();
			if (MySubSystem)
			{
				FString Temp = FString::Printf(TEXT("%s : %s"), *MySubSystem->UserID, *Text.ToString());

				//call Local(Client)
				//Execute Remote(Server)
				PC->C2S_SendMessage(FText::FromString(Temp));
				ChatInput->SetText(FText::FromString(TEXT("")));
			}
		}
	}
}

void ULobbyWidgetBase::AddMessage(const FText& Text)
{
	if (ChatScrollBox)
	{
		UTextBlock* NewMessageBlock = NewObject<UTextBlock>(ChatScrollBox);
		if (NewMessageBlock)
		{
			NewMessageBlock->SetText(Text);
			FSlateFontInfo FontInfo = NewMessageBlock->GetFont();
			FontInfo.Size = 20.0f;
			NewMessageBlock->SetFont(FontInfo);

			ChatScrollBox->AddChild(NewMessageBlock);
			ChatScrollBox->ScrollToEnd();
		}

		//URichTextBlock* NewMessageBlock = NewObject<URichTextBlock>(ChatScrollBox);
		//if (NewMessageBlock)
		//{
		//	FString Temp = FString::Printf(TEXT("<RichText.Normal>%s</RichText>"), *Text.ToString());
		//	NewMessageBlock->SetText(FText::FromString(Temp));
		//	NewMessageBlock->SetAutoWrapText(true);
		//	NewMessageBlock->SetWrapTextAt(ChatScrollBox->GetCachedGeometry().GetLocalSize().X);
		//	NewMessageBlock->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);

		//	if (CharStyleSet)
		//	{
		//		NewMessageBlock->SetTextStyleSet(CharStyleSet);
		//	}

		//	ChatScrollBox->AddChild(NewMessageBlock);
		//	ChatScrollBox->ScrollToEnd();
		//}
	}
}

void ULobbyWidgetBase::ProcessChangeLeftTime(const int32 InLeftTime)
{
	if (LeftTimeText)
	{
		FString Temp;
		if (InLeftTime <= 0)
		{
			Temp = FString::Printf(TEXT("시작됩니다."));
		}
		else
		{
			Temp = FString::Printf(TEXT("%d초 남았습니다."), InLeftTime);
		}

		LeftTimeText->SetText(FText::FromString(Temp));
	}
	
}

void ULobbyWidgetBase::ProcessChangeConnectionCount(const int32 InConnectionCount)
{
	if (ConnectCountText)
	{
		FString Temp;
		Temp = FString::Printf(TEXT("%d명 접속"), InConnectionCount);
		ConnectCountText->SetText(FText::FromString(Temp));
	}
}

