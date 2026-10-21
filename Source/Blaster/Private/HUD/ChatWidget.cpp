// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ChatWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "BlasterPlayerController.h"

bool UChatWidget::Initialize()
{
	bool bSuccess = Super::Initialize();

	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UChatWidget::OnTextCommited);
	}

	Border->SetVisibility(ESlateVisibility::Collapsed);

	return bSuccess;
}

void UChatWidget::AddChatMessage(const FString& Sender, const FString& Message)
{
	if (!ChatScrollBox)
	{
		return;
	}

	FString FullMessage = FString::Printf(TEXT("%s: %s"), *Sender, *Message);
	UTextBlock* Text = NewObject<UTextBlock>(this);

	Text->SetText(FText::FromString(FullMessage));
	
	ChatScrollBox->AddChild(Text);
	ChatScrollBox->ScrollToEnd();
}

void UChatWidget::ActivateChat()
{
	if (!ChatInput)
	{
		return;
	}

	Border->SetVisibility(ESlateVisibility::Visible);
	ChatInput->SetVisibility(ESlateVisibility::Visible);

	ChatInput->SetKeyboardFocus();
}

void UChatWidget::DeactivateChat()
{
	if (!ChatInput)
	{
		return;
	}

	ChatInput->SetText(FText::GetEmpty());
	ChatInput->SetVisibility(ESlateVisibility::Collapsed);
}

void UChatWidget::OnTextCommited(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	FString Message = Text.ToString();

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetOwningPlayer());
	if (!BlasterPlayerController)
	{
		return;
	}

	BlasterPlayerController->ServerSendChatMessage(Message);
	BlasterPlayerController->CloseChat();
}

