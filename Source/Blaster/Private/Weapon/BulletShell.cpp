// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletShell.h"

ABulletShell::ABulletShell()
{
	PrimaryActorTick.bCanEverTick = false;

	BulletShell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletShell"));
	SetRootComponent(BulletShell);
}

void ABulletShell::BeginPlay()
{
	Super::BeginPlay();
	
}

