// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/Weapon/BaseWeapon.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Public/Character/BlasterAnimInstance.h"
#include "Blaster.h"
#include "BlasterPlayerController.h"
#include "TimerManager.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"

ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement())
	{
		CharacterMovementComponent->bOrientRotationToMovement = true;
	}

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	}
	
	if (GetCapsuleComponent() && GetMesh())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

		GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AttachedKnife = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Knife"));
	AttachedKnife->SetupAttachment(GetMesh(), FName("LeftHandKnifeSocket"));
	AttachedKnife->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedKnife->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	//AttachedKnife->OnComponentBeginOverlap.AddDynamic(this, &ABlasterCharacter::OnKnifeStabbed);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	

	TryToHideACamera();
	PollInit();
	RotateInPlace(DeltaTime);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
	DOREPLIFETIME(ABlasterCharacter, bCanStab);
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);

	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(ABaseWeapon* Weapon)
{ 
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon; 

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::PlayElimintationMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && EliminationMontage)
	{
		AnimInstance->Montage_Play(EliminationMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlayKnifeStabMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && KnifeStabMontage)
	{
		AnimInstance->Montage_Play(KnifeStabMontage);
	}
}

void ABlasterCharacter::Eliminate()
{
	if (!IsFirstEliminationCall)
	{
		return;
	}

	IsFirstEliminationCall = false;

	MulticastEliminate();

	GetWorldTimerManager().SetTimer(
		EliminateTimer,
		this,
		&ABlasterCharacter::EliminateTimerFinished,
		EliminateDelay);

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->DropWeapon();
		Combat->ShowAttachedKnife(false);
	}
}

void ABlasterCharacter::MulticastEliminate_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->ControllerGrenades = Combat->Grenades;
	}

	bEliminated = true;
	PlayElimintationMontage();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		if (GetMesh())
		{
			GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);

			StartDissolve();
		}

		if (Combat)
		{
			Combat->ShowAttachedKnife(false);
		}
	}

	// Disable movement, rotation and collision after death
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DisableMovement();
		MovementComponent->StopMovementImmediately();
	}

	bDisableGameplay = true;

	GetCharacterMovement()->DisableMovement();

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Spawn elimination bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), 
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()                                                                                   
		);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	if (IsLocallyControlled() && IsAiming() && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::MulticastThrowGrenade_Implementation()
{
	if (Combat && !HasAuthority())
	{
		Combat->ShowAttachedGrenade(false);
	}	
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	UE_LOG(LogTemp, Display, TEXT("AO_Yaw: %f"), AO_Yaw);

	if (AO_Yaw > 90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::TryToHideACamera()
{
	if (IsLocallyControlled())
	{
		if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
		{
			GetMesh()->SetVisibility(false);
			if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			{
				Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
			}
		}
		else
		{
			GetMesh()->SetVisibility(true);
			if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
			{
				Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
			}
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat)
	{
		return ECombatState::ECS_MAX;
	}

	return Combat->CombatState;
}

ABaseWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (!Combat)
	{
		return nullptr;
	}

	return Combat->EquippedWeapon;
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonRealeased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);

	PlayerInputComponent->BindAction("KnifeStab", IE_Pressed, this, &ABlasterCharacter::KnifeStabButtonPressed);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementRep = 0.0f;
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));

	if (Combat && Combat->EquippedWeapon && BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat || !Combat->EquippedWeapon)
	{
		return;
	}

	if (GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && FireWeaponMontage)
		{
			AnimInstance->Montage_Play(FireWeaponMontage);

			FName SectionName;
			SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void  ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon)
	{
		return;
	}

	if (GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && ReloadMontage && ReloadSniperRifleMontage)
		{
			if (Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
			{
				AnimInstance->Montage_Play(ReloadSniperRifleMontage);
				AnimInstance->Montage_JumpToSection(FName("SniperRifle"));
				return;
			}

			AnimInstance->Montage_Play(ReloadMontage);
			FName SectionName;

			switch (Combat->EquippedWeapon->GetWeaponType())
			{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("RocketLauncher");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_SubmachineGun:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Shotgun");
				break;
			case EWeaponType::EWT_GrenadeLauncher:
				SectionName = FName("Rifle");
				break;
			}

			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat || !Combat->EquippedWeapon)
	{
		return;
	}

	if (GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && HitReactMontage)
		{
			float var = AnimInstance->Montage_Play(HitReactMontage);

			FName SectionName;
			SectionName = FName("FromFront");
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}

	if (HasAuthority())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &ABlasterCharacter::OnMontageEnded);
		}
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f && !bDisableGameplay)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.0f && !bDisableGameplay)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	if (Value != 0)
	{
		AddControllerYawInput(Value);
	}
}

void ABlasterCharacter::LookUp(float Value)
{
	if (Value != 0)
	{
		AddControllerPitchInput(Value);
	}	
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat && !bDisableGameplay)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}	
		else
		{			
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched && !bDisableGameplay)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat && !bDisableGameplay)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonRealeased()
{
	if (Combat && !bDisableGameplay)
	{
		Combat->SetAiming(false);
	}
}


void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270; 360) to (-90; 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}		
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon)
	{
		float Speed = CalculateSpeed();

		bool bIsInAir = GetCharacterMovement()->IsFalling();

		if (Speed == 0.f && !bIsInAir)
		{
			bRotateRootBone = true;
			FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);

			AO_Yaw = DeltaAimRotation.Yaw;

			if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
			{
				InterpAO_Yaw = AO_Yaw;
			}

			bUseControllerRotationYaw = true;

			TurnInPlace(DeltaTime);
		}

		if (Speed > 0.f || bIsInAir)
		{
			bRotateRootBone = false;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			AO_Yaw = 0.f;

			bUseControllerRotationYaw = true;

			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}

		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon)
	{
		return;
	}

	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotationCurrentFrame;
	ProxyRotationCurrentFrame = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotationCurrentFrame, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
	}
	else
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bEliminated)
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	UpdateHUDHealth();
	PlayHitReactMontage();	
	
	if (Health == 0.0f && GetWorld())
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode && IsFirstEliminationCall)
		{
			BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* BlasterInstigatorController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, BlasterInstigatorController);
		}
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.0f, true);
			BlasterPlayerState->AddToDefeats(0);
			BlasterPlayerState->ElimTextVisibility(false);

			BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

			if (BlasterPlayerController)
			{
				BlasterPlayerController->SetTextWeaponTypeInvisible();

				if (GetCombat())
				{
					GetCombat()->SetGrenades(BlasterPlayerController->ControllerGrenades);
					BlasterPlayerController->SetHUDGrenades(BlasterPlayerController->ControllerGrenades);
				}
			}			
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementRep += DeltaTime;
		if (TimeSinceLastMovementRep > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat && !bDisableGameplay)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat && !bDisableGameplay)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (Combat && !bDisableGameplay)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::KnifeStabButtonPressed()
{
	if (Combat)
	{
		AttachedKnife->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AttachedKnife->SetGenerateOverlapEvents(true);
		Combat->KnifeStab();
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}	
}

void  ABlasterCharacter::OnRep_OverlappingWeapon(ABaseWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::EliminateTimerFinished()
{
	if (GetWorld())
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();

		if (BlasterGameMode)
		{
			BlasterGameMode->RequestRespawn(this, Controller);
		}		

		if (ElimBotComponent)
		{
			ElimBotComponent->DestroyComponent();
		}

		IsFirstEliminationCall = true;
	}
}

void ABlasterCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == ThrowGrenadeMontage && Combat)
	{
		Combat->ThrowGrenadeFinished();
	}

	if (Montage == KnifeStabMontage && Combat)
	{
		Combat->KnifeStabFinished();
	}
}

void ABlasterCharacter::PerformKnifeStab()
{
	FVector Start = AttachedKnife->GetComponentLocation();
	FVector End = Start + AttachedKnife->GetUpVector() * 50.f;

	FVector KnifeBox(5.f, 5.f, 0.5f);

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	FHitResult KnifeHit;
	bool bHit = UKismetSystemLibrary::BoxTraceSingle(
	this, 
	Start,
	End,
	KnifeBox,
	AttachedKnife->GetComponentRotation(),
	ETraceTypeQuery::TraceTypeQuery1,
	false,
	IgnoredActors,
	EDrawDebugTrace::None,
	KnifeHit,
	true);

	if (bCanStab && bHit && Cast<ABlasterCharacter>(KnifeHit.GetActor()) && KnifeHit.GetActor() != this)
	{
		UGameplayStatics::ApplyDamage(
			KnifeHit.GetActor(),
			100.0,
			Controller,
			this,
			UDamageType::StaticClass()
		);

		bCanStab = false;
	}
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat)
	{
		return FVector();
	}

	return Combat->HitTarget;
}