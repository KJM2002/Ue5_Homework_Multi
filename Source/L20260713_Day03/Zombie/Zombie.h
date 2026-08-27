// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Zombie.generated.h"

UENUM(BlueprintType)
enum class EZombieState : uint8
{
	Normal			= 0  UMETA(DisplayName = "Normal"),
	Chase			= 10  UMETA(DisplayName = "Chase"),
	Battle			= 20  UMETA(DisplayName = "Battle"),
	Dead			= 30  UMETA(DisplayName = "Dead")
};

UCLASS()
class L20260713_DAY03_API AZombie : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZombie();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	float CurrnetHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	EZombieState CurrentState;

	UFUNCTION(BlueprintCallable)
	void SetState(EZombieState NewState);


	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
};
