// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyWeaponBase.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;
class AProjectileBase;

UCLASS()
class L20260713_DAY03_API AMyWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int32 MaxBulletCount = 30;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int32 CurrentBulletCount = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UAnimMontage> FireAnimMontage;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UAnimMontage> ReloadAnimMontage;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	uint8 bFullAuto : 1 = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	uint8 bCanFire : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	float FireRate = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	float FiringRate = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", Replicated)
	float CoolDown = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TSubclassOf<AProjectileBase> ProjectileTemplate;
	

	UFUNCTION(BlueprintCallable)
	void StartFire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	UFUNCTION(Server, Reliable)
	void C2S_StartFire();
	void C2S_StartFire_Implementation();


	UFUNCTION(Server, Reliable)
	void C2S_StopFire();
	void C2S_StopFire_Implementation();

	UFUNCTION(Server, Reliable)
	void C2S_SpawnBullet(const FVector& Start, const FRotator& SpawnRotator);
	void C2S_SpawnBullet_Implementation(const FVector& Start, const FRotator& SpawnRotator);


	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	void MakeMuzzleFlash();

	UFUNCTION(BlueprintCallable)
	bool LineTrace(FHitResult& OutResult);

};
