// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Weapon/BaseWeapon.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (UCameraComponent* FollowCamera = Character->GetFollowCamera())
		{
			DefaultFOV = FollowCamera->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarryAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarryAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}
 
void UCombatComponent::EquipWeapon(ABaseWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip)
	{
		if (EquippedWeapon)
		{
			EquippedWeapon->DropWeapon();
		}
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		EquippedWeapon->SetOwner(Character);
		EquippedWeapon->SetHUDAmmo();

		if (CarryAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarryAmmo = CarryAmmoMap[EquippedWeapon->GetWeaponType()];
		}

		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarryAmmo);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		bCanfire = true;
	}
}

void UCombatComponent::Reload()
{
	if (CarryAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		bCanfire = true;
	}
}

void  UCombatComponent::OnRep_CarryAmmo()
{
	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarryAmmo);
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
		case ECombatState::ECS_Unoccupied:
			if (bFireButtonPressed)
			{
				Fire();
			}
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && EquippedWeapon && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;

	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
			PlayerController,
			CrosshairLocation,
			CrosshairWorldPosition,
			CrosshairWorldDirection);

		if (bScreenToWorld)
		{
			FVector Start = CrosshairWorldPosition;

			if (Character)
			{
				float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
				Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
			}

			FVector End = Start + CrosshairWorldDirection * CrosshairTraceLength;

			if (UWorld* World = GetWorld())
			{
				World->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

				if (!TraceHitResult.bBlockingHit)
				{
					TraceHitResult.ImpactPoint = End;
				}

				if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
				{
					HUDPackage.CrosshairsColor = FLinearColor::Red;
					CrosshairEnemyFactor = 0.25f; // yes, I just hardcoded this value
				}
				else
				{
					HUDPackage.CrosshairsColor = FLinearColor::White;
					CrosshairEnemyFactor = 0.0f;// same story
				}
			}
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (!Character || !Character->Controller)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;

		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			//Calculate crosshair spread

			//[0, 600] -> [0, 1]
			if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
			{
				FVector2D WalkSpeedRange(0.f, MovementComponent->MaxWalkSpeed);
				FVector2D VelocityMultiplierRange(0.f, 1.f);
				FVector Velocity = Character->GetVelocity();

				Velocity.Z = 0.f;

				CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

				if (MovementComponent->IsFalling())
				{
					CrosshairAirFactor = FMath::FInterpTo(CrosshairAirFactor, 2.25f, DeltaTime, 2.25f);
				}
				else
				{
					CrosshairAirFactor = FMath::FInterpTo(CrosshairAirFactor, 0.0f, DeltaTime, 30.f);
				}
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread = 0.5 + CrosshairVelocityFactor + CrosshairAirFactor - CrosshairAimFactor - CrosshairEnemyFactor + CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!Character || !EquippedWeapon)
	{
		return;
	}

	CombatState = ECombatState::ECS_Reloading;

	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character != nullptr && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (!EquippedWeapon)
	{
		return int32();
	}

	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarryAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarryAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);

		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return int32();
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon)
	{
		return;
	}

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->ZoomedFOV, DeltaTime, EquippedWeapon->ZoomInterpSpeed);
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !Character)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	bCanfire = true;

	if (bFireButtonPressed && EquippedWeapon && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanfire = false;
		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
			StartFireTimer();
		}
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon != nullptr)
	{
		return !EquippedWeapon->IsEmpty() && bCanfire && CombatState == ECombatState::ECS_Unoccupied;
	}

	return false;
}

void UCombatComponent::InitializeCarryAmmo()
{
	CarryAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::UpdateAmmoValues()
{
	if (!Character || !EquippedWeapon)
	{
		return;
	}

	int32 ReloadAmount = AmountToReload();
	if (CarryAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarryAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarryAmmo = CarryAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarryAmmo);
	}

	EquippedWeapon->AddAmmo(-ReloadAmount);
}