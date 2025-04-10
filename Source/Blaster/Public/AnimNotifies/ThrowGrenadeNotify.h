// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ThrowGrenadeNotify.generated.h"

/**
 * 
 */

UCLASS(meta = (DisplayName = "Throw Grenade c++"))
class BLASTER_API UThrowGrenadeNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSequance) override;
	
};
