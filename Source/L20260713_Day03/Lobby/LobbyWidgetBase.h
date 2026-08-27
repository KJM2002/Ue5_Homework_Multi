// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidgetBase.generated.h"


class UScrollBox;
class UTextBlock;
class UEditableTextBox;
class UButton;


/**
 * 
 */
UCLASS()
class L20260713_DAY03_API ULobbyWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:

	virtual void NativeConstruct() override;

	virtual void NativeOnInitialized() override;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UButton> ChatButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UEditableTextBox> ChatInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UTextBlock> LeftTimeText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UTextBlock> ConnectCountText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget", meta = (WidgetBind))
	TObjectPtr<UScrollBox> ChatScrollBox;

	UFUNCTION(BlueprintCallable)
	void ShowStartButton(bool IsShow);

	UFUNCTION(BlueprintNativeEvent)
	void PlayShowStartButton();
	void PlayShowStartButton_Implementation();

	UFUNCTION()
	void ProcessTextCommited(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void ProcessTextChanged(const FText& Text);

	UFUNCTION()
	void ProcessStartServer();

	UFUNCTION()
	void ProcessSendMessage();

	UFUNCTION(BlueprintCallable)
	void AddMessage(const FText& Text);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font")
	TObjectPtr<UDataTable> CharStyleSet;


	UFUNCTION()
	void ProcessChangeLeftTime(const int32 InLeftTime);

	UFUNCTION()
	void ProcessChangeConnectionCount(const int32 InConnectionCount);
};


