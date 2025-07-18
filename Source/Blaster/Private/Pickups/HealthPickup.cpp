// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"
#include "NiagaraFunctionLibrary.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;	
}

void AHealthPickup::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}

	Destroy();
}