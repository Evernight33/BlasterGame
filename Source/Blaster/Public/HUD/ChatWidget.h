// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

class UEditableTextBox;
class UScrollBox;
class UBorder;

USTRUCT()
struct FChatMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FString Sender;

	UPROPERTY()
	FString Text;
};

UCLASS()
class BLASTER_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual bool Initialize() override;

	UPROPERTY()
	TArray<FChatMessage> Messages;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ChatInput;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	class UBorder* Border;

public: 
	void AddChatMessage(const FString& Sender, const FString& Message);

	FORCEINLINE UEditableTextBox* GetChatInput() const { return ChatInput; }

	UFUNCTION()
	void ActivateChat();

	UFUNCTION()
	void DeactivateChat();

	UFUNCTION()
	void OnTextCommited(const FText& Text, ETextCommit::Type CommitMethod);
};
