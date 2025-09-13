// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/DamageMultiplier.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"

void ADamageMultiplier::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffDamageMultiplier(DamageMultiplierBuffTime);
		}		
	}
	
	Destroy();
}