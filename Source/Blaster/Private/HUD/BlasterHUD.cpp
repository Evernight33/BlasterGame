// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/CharacterOverlay.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();	

	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
	}
}

void ABlasterHUD::AddCharacterOverlay()
{
	if (CharacterOverlayClass)
	{		
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

		FVector2D Spread(0.0f, 0.0f);
		float SpreadScale = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairsCenter)
		{
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsBottom)
		{
			Spread.Y = SpreadScale;
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsTop)
		{
			Spread.Y = -SpreadScale;
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		Spread.Y = 0.0f;

		if (HUDPackage.CrosshairsLeft)
		{
			Spread.X = -SpreadScale;
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsRight)
		{
			Spread.X = SpreadScale;
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2d ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D TextureDrawPoint(ViewportCenter.X - TextureWidth / 2.0f + Spread.X, ViewportCenter.Y - TextureHeight / 2.0f + Spread.Y);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0f, 0.0f, 1.0f, 1.0f, CrosshairsColor);
}