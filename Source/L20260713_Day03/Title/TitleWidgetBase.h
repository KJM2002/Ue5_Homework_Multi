// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidgetBase.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UWebApiSubsystem;


/**
 *
 */
UCLASS()
class L20260713_DAY03_API UTitleWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TObjectPtr<UButton> StartServerButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UButton> ConnectServerButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Password;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ServerIP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UButton> LoginButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UButton> SignUpButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (BindWidget))
	TObjectPtr<UTextBlock> InfoText;

	UFUNCTION()
	void StartServer();

	UFUNCTION()
	void ConnectServer();

	void SaveData();

	UFUNCTION()
	void Login();

	UFUNCTION()
	void SignUp();

	UFUNCTION()
	void ProcessLoginResult(const bool bInSuccess, const FString& InMessage);

	UFUNCTION()
	void ProcessSignUpResult(const bool bInSuccess, const FString& InMessage);

	UFUNCTION()
	void ProcessLatestServerResult(const bool bInSuccess, const FString& InMessage,
		const FString& InServerIP, const int32 InPort, const FString& InOwnerNickname);

private:

	UWebApiSubsystem* GetWebApi() const;

	bool IsLoggedIn() const;

	bool ValidateInput();

	void SetInfoText(const FString& InMessage);

	void ClearLoginState();

	/** "jaemin (Lv.1)" — 로그인 상태를 InfoText 앞에 붙일 때 쓴다. */
	FString GetLoginLabel() const;

	bool bRequestInFlight = false;

};
