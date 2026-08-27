// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"


// Sets default values
AProjectileBase::AProjectileBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	RootComponent = Sphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->MaxSpeed = 10000.0f;
	Movement->InitialSpeed = 10000.0f;

	SetReplicates(true);
	SetReplicateMovement(true);
	bNetLoadOnClient = true;
	bNetUseOwnerRelevancy = true;
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	Sphere->OnComponentHit.AddDynamic(this, &AProjectileBase::ProcessHit);
	
}

// Called every frame
void AProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectileBase::ProcessHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit %s"), *OtherActor->GetName());

	UDecalComponent* MadeDecal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),
		Decal,
		FVector(5, 5, 5),
		Hit.ImpactPoint,
		Hit.ImpactNormal.Rotation(),
		5.0f
	);

	MadeDecal->SetFadeScreenSize(0.005f);


	if (HasAuthority())
	{
		//맞았을때
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(),
			1.0f,
			-Hit.ImpactNormal,
			Hit,
			nullptr,
			nullptr,
			DamageType
		);
	}


	Destroy();
}

