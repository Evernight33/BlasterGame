// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletShell.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

ABulletShell::ABulletShell()
{
	PrimaryActorTick.bCanEverTick = false;

	BulletShell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletShell"));
	SetRootComponent(BulletShell);

	BulletShell->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	BulletShell->SetSimulatePhysics(true);
	BulletShell->SetEnableGravity(true);
	BulletShell->SetSimulatePhysics(true);
	BulletShell->SetNotifyRigidBodyCollision(true);
}

void ABulletShell::BeginPlay()
{
	Super::BeginPlay();
	
	BulletShell->OnComponentHit.AddDynamic(this, &ABulletShell::OnHit);
	BulletShell->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	SetLifeSpan(2.5);
}

void ABulletShell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}
}