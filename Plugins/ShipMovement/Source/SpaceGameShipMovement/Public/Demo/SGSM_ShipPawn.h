// Copyright Distant Light Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SGSM_ShipPawn.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USGSM_PropulsionBrain;
struct FInputActionValue;

UCLASS()
class SPACEGAMESHIPMOVEMENT_API ASGSM_ShipPawn : public APawn
{
	GENERATED_BODY()

public:

	ASGSM_ShipPawn(const FObjectInitializer& ObjectInitializer);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void TickLinearThrustInput(const FInputActionValue& InValue);
	void EndLinearThrustInput(const FInputActionValue& InValue);

	void TickAngularThrustInput(const FInputActionValue& InValue);
	void EndAngularThrustInput(const FInputActionValue& InValue);

	void TickBoostInput(const FInputActionValue& InValue);
	void EndBoostInput();

	void ToggleLinearBraking();
	void ToggleAngularBraking();
	void ToggleAltTurning();

	UFUNCTION()
	void InputMethodeChanged(ECommonInputType NewInputType);

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* TurnAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BoostAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* GlideToggleAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AltTurningToggleAction = nullptr;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	USGSM_PropulsionBrain* PropulsionBrain = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Ship)
	UCameraComponent* FollowCamera = nullptr;

private:

	FVector GetMouseDirection() const;

	ECommonInputType InputType;
};
