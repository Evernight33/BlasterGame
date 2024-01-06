// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletShell.generated.h"

class USoundCue;

UCLASS()
class BLASTER_API ABulletShell : public AActor
{
	GENERATED_BODY()
	
public:	
	ABulletShell();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BulletShell;

	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse = 10.f;

	UPROPERTY(EditAnywhere)
	USoundCue* ShellSound;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:	

};
