// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "InputAction.h"
#include "GenericTeamAgentInterface.h"

#include "MyCharacter.generated.h"




class USpringArmComponent;
class UCameraComponent;
class UChildActorComponent;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class L20260713_DAY03_API AMyCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UChildActorComponent> Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

	




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Zoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Lean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Fire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> IA_Reload;

	UFUNCTION(BlueprintCallable)
	void StartFire();

	UFUNCTION(BlueprintCallable)
	void StopFire();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status", Replicated)
	uint32 bFire : 1 = false;


	void Look(const FInputActionValue& Value);
	
	void Move(const FInputActionValue& Value);

	void StartZoom();

	void StopZoom();

	void Reload();

	UFUNCTION(Server, Reliable)
	void C2S_StartZoom();
	void C2S_StartZoom_Implementation();


	UFUNCTION(Server, Reliable)
	void C2S_StopZoom();
	void C2S_StopZoom_Implementation();



	void Lean(const FInputActionValue& Value);

	//void StopLean(const FInputActionValue& Value);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat", Replicated)
	uint32 bArmed : 1 = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat", Replicated)
	uint32 bZoom : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat", Replicated)
	float LeanValue = 0;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHP = 100.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UParticleSystem> HitEffect;

	UFUNCTION()
	void SpawnHitEffect(const FHitResult& InResult);

	UFUNCTION(NetMulticast, Unreliable)
	void S2A_SpawnHitEffect(const FHitResult& InResult);
	void S2A_SpawnHitEffect_Implementation(const FHitResult& InResult);



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UAnimMontage> HitReactionAnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UAnimMontage> ReloadAnimMontage; 


	FGenericTeamId TeamID;


	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override
	{
		TeamID = InTeamID;
	}

	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return TeamID;
	}

	/** Retrieved owner attitude toward given Other object */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override
	{
		const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
		return OtherTeamAgent ? FGenericTeamId::GetAttitude(GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId())
			: ETeamAttitude::Hostile;
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FRotator GetAimOffset() const;

	void EquipItem();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TSubclassOf<class AMyWeaponBase> WeaponTemplate;

};


