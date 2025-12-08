// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Weapon/BaseWeapon.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/Character/BlasterAnimInstance.h"
#include "Blaster/Public/Weapon/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "Weapon/HitScanWeapon.h"
#include "Weapon/Shotgun.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	ShowAttachedKnife(false);

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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarryAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
	DOREPLIFETIME(UCombatComponent, bDamageMultiplied);
}
 
void UCombatComponent::EquipWeapon(ABaseWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip && CombatState == ECombatState::ECS_Unoccupied)
	{
		if (EquippedWeapon && SecondaryWeapon == nullptr)
		{
			EquipSecondaryWeapon(WeaponToEquip);
		}
		else
		{
			EquipPrimaryWeapon(WeaponToEquip);
		}		

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		bCanfire = true;		
	}
}

void UCombatComponent::SwapWeapons()
{
	if (Character == nullptr || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	ABaseWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));
	EquippedWeapon->SetHUDAmmo();
	UpdateCarryAmmo();
	PlayEquippedWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToSocket(SecondaryWeapon, FName("BackpackSocket"));
}

void UCombatComponent::Reload()
{
	if (CarryAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReload();
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr || Grenades == 0)
	{
		return;
	}
	
	if (Character)
	{
		if (Character->HasAuthority())
		{
			CombatState = ECombatState::ECS_ThrowingGrenade;

			Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
			if (Controller)
			{
				Controller->ControllerGrenades -= 1;
				Grenades = Controller->ControllerGrenades;
				UpdateHudGrenades();
			}
		}

		if (Character->IsLocallyControlled())
		{
			ShowAttachedGrenade(true);

			Character->PlayThrowGrenadeMontage();
			if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
			{
				AttachActorToSocket(EquippedWeapon, FName("LeftHandPistolSocket"));
			}
			else
			{
				AttachActorToSocket(EquippedWeapon, FName("LeftHandSocket"));
			}
		}		
	}

	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade(HitTarget);
	}
}

void UCombatComponent::KnifeStab()
{
	if (CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	if (Character)
	{
		if (!Character->GetEquippedWeapon())
		{
			return;
		}

		if (Character->HasAuthority())
		{
			CombatState = ECombatState::ECS_KnifeStabbing;
			Character->GetWorldTimerManager().SetTimer(
				KnifeStabTimer,
				this,
				&UCombatComponent::KnifeStabTimerFunction,
				0.05f,
				true);
		}
		else
		{
			ServerKnifeStab();
		}

		if (Character->IsLocallyControlled())
		{
			ShowAttachedKnife(true);
			Character->PlayKnifeStabMontage();			
		}
	}
}

void UCombatComponent::KnifeStabTimerFunction()
{
	if (Character)
	{
		Character->PerformKnifeStab();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotunAmmoValues();
	}	
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();

	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bVisible)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bVisible);
	}
}

void UCombatComponent::EquipPrimaryWeapon(ABaseWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)
	{
		return;
	}

	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	UpdateCarryAmmo();
	PlayEquippedWeaponSound(WeaponToEquip);

	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(ABaseWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)
	{
		return;
	}

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToSocket(WeaponToEquip, FName("BackpackSocket"));
	PlayEquippedWeaponSound(WeaponToEquip);
	SecondaryWeapon->SetOwner(Character);
}

void UCombatComponent::ShowAttachedKnife(bool bVisible)
{
	if (Character && Character->GetAttachedKnife())
	{
		Character->GetAttachedKnife()->SetVisibility(bVisible);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (!Character || !EquippedWeapon)
	{
		return;
	}

	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
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

		AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));

		PlayEquippedWeaponSound(EquippedWeapon);

		EquippedWeapon->SetHUDAmmo();

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		bCanfire = true;

		Character->GetWorldTimerManager().SetTimer(
			EquipTimer,
			this,
			&UCombatComponent::SetHudText,
			TextDelay);
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToSocket(SecondaryWeapon, FName("BackpackSocket"));
	}
}

void  UCombatComponent::OnRep_CarryAmmo()
{
	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarryAmmo);
	}

	if (CombatState == ECombatState::ECS_Reloading && EquippedWeapon != nullptr 
		&& EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun && CarryAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	if (!EquippedWeapon)
	{
		return;
	}

	switch (CombatState)
	{
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
		case ECombatState::ECS_Unoccupied:
			AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));
			ShowAttachedGrenade(false);
			ShowAttachedKnife(false);
			if (bFireButtonPressed)
			{
				Fire();
			}			
			break;
		case ECombatState::ECS_ThrowingGrenade:
			if (Character && !Character->IsLocallyControlled())
			{
				if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
				{
					AttachActorToSocket(EquippedWeapon, FName("LeftHandPistolSocket"));
				}
				else
				{
					AttachActorToSocket(EquippedWeapon, FName("LeftHandSocket"));
				}
				
				ShowAttachedGrenade(true);
				Character->PlayThrowGrenadeMontage();
			}
		case ECombatState::ECS_KnifeStabbing:
			ShowAttachedKnife(true);
			Character->PlayKnifeStabMontage();
			break;
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
	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
	{
		AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));
	}
	else
	{
		AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));
	}

	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && EquippedWeapon)
	{
		if (CombatState == ECombatState::ECS_Unoccupied || 
			(CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun))
		{
			if (CombatState == ECombatState::ECS_Reloading)
			{
				CombatState = ECombatState::ECS_Unoccupied;
			}

			Character->PlayFireMontage(bAiming);
			EquippedWeapon->Fire(TraceHitTarget);
		}		
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
	if (!Character || !EquippedWeapon || Grenades == 0)
	{
		return;
	}

	CombatState = ECombatState::ECS_Reloading;

	HandleReload();
}

void UCombatComponent::ServerThrowGrenade_Implementation(const FVector_NetQuantize& Target)
{
	HitTarget = Target;

	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun)
	{
		AttachActorToSocket(EquippedWeapon, FName("LeftHandPistolSocket"));
	}
	else
	{
		AttachActorToSocket(EquippedWeapon, FName("LeftHandSocket"));
	}

	ShowAttachedGrenade(true);

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
	}

	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->ControllerGrenades -= 1;
		Grenades = Controller->ControllerGrenades;
	}	
}

void UCombatComponent::ServerKnifeStab_Implementation()
{
	ShowAttachedKnife(true);
	CombatState = ECombatState::ECS_KnifeStabbing;

	if (Character)
	{
		Character->PlayKnifeStabMontage();
		Character->GetWorldTimerManager().SetTimer(
			KnifeStabTimer,
			this,
			&UCombatComponent::KnifeStabTimerFunction,
			0.05f,
			true);
	}		
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

void UCombatComponent::ThrowGrenadeFinished()
{
	if (Character && EquippedWeapon && Character->HasAuthority())
	{
		ShowAttachedGrenade(false);
		AttachActorToSocket(EquippedWeapon, FName("RightHandSocket"));
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::KnifeStabFinished()
{
	if (Character && Character->HasAuthority())
	{
		Character->GetWorldTimerManager().ClearTimer(KnifeStabTimer);
		ShowAttachedKnife(false);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::DropEquippedWeapon() const
{
	if (EquippedWeapon)
	{
		EquippedWeapon->DropWeapon();
	}
}

void UCombatComponent::AttachActorToSocket(AActor* ActorToAttach, const FName& HandName)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach)
	{
		return;
	}

	const USkeletalMeshSocket* Socket = Character->GetMesh()->GetSocketByName(HandName);

	if (Socket)
	{
		Socket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarryAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (CarryAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarryAmmo = CarryAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller && EquippedWeapon)
	{
		Controller->SetHUDCarriedAmmo(CarryAmmo);
		Controller->SetTextWeaponType(EquippedWeapon->GetWeaponType());
	}
}

void UCombatComponent::PlayEquippedWeaponSound(ABaseWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarryAmmoMap.Contains(WeaponType))
	{
		CarryAmmoMap[WeaponType] += FMath::Clamp(CarryAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarryAmmo);

		UpdateCarryAmmo();
	}

	if (EquippedWeapon->GetWeaponType() == WeaponType && EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanSwapWeapons()
{
	return EquippedWeapon != nullptr && SecondaryWeapon != nullptr;
}

void UCombatComponent::ServerSetDamageMultiplied_Implementation(bool IsMultiplied)
{
	bDamageMultiplied = IsMultiplied;
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

void UCombatComponent::OnRep_Grenades()
{
	UpdateHudGrenades();
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

	ReloadEmptyWeapon();
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanfire = false;

		if (EquippedWeapon)
		{
			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			}

			CrosshairShootingFactor = 0.75f;
			StartFireTimer();
		}
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireShotgun()
{	
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun)
	{
		TArray<FVector> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon != nullptr)
	{
		return !EquippedWeapon->IsEmpty() &&  
			(CombatState == ECombatState::ECS_Unoccupied && bCanfire || 
		    (CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun));
	}

	return false;
}

void UCombatComponent::InitializeCarryAmmo()
{
	CarryAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarryAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
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

void UCombatComponent::UpdateShotunAmmoValues()
{
	if (!Character || !EquippedWeapon)
	{
		return;
	}

	if (CarryAmmoMap.Contains(EWeaponType::EWT_Shotgun))
	{
		CarryAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarryAmmo = CarryAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarryAmmo);
	}

	EquippedWeapon->AddAmmo(-1);

	if (EquippedWeapon->IsFull() || CarryAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::SetHudText()
{
	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller && EquippedWeapon)
	{
		Controller->SetTextWeaponType(EquippedWeapon->GetWeaponType());
	}
}

void UCombatComponent::UpdateHudGrenades()
{
	Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller && EquippedWeapon)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}