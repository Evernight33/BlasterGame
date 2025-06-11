// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "KnifeStabRestrictAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UKnifeStabRestrictAnimNotify : public UAnimNotify
{
	GENERATED_BODY()
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSequance) override;
};
