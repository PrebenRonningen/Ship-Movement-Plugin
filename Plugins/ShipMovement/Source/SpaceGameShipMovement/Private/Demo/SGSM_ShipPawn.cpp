// Copyright Distant Light Games, Inc. All Rights Reserved.

#include "Demo/SGSM_ShipPawn.h"

// input system
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CommonInputSubsystem.h"

// components
#include "SGSM_PropulsionBrain.h"
#include "Camera/CameraComponent.h"


ASGSM_ShipPawn::ASGSM_ShipPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InputType{ECommonInputType::MouseAndKeyboard}
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ASGSM_ShipPawn::BeginPlay()
{
	Super::BeginPlay();

	PropulsionBrain = GetComponentByClass<USGSM_PropulsionBrain>();
	if (ensureAlways(PropulsionBrain))
	{
		PropulsionBrain->SetLinearBraking(true);
		PropulsionBrain->SetAngularBraking(true);
	}

	FollowCamera = GetComponentByClass<UCameraComponent>();
	ensureAlways(FollowCamera);

	UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(GetRootComponent());

	if (ensureAlwaysMsgf(PrimitiveComponent, TEXT("Failed to get Owner Root Component as Primitive Component")))
	{
		FBodyInstance& BodyInstance = PrimitiveComponent->BodyInstance;

		const FVector InertiaTensorScale = BodyInstance.InertiaTensorScale;
		const FVector Scale = GetActorScale();

		BodyInstance.InertiaTensorScale = InertiaTensorScale / Scale;
		BodyInstance.UpdateMassProperties();
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UCommonInputSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::InputMethodeChanged);
		}
	}
}

void ASGSM_ShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InputType == ECommonInputType::MouseAndKeyboard)
	{
		if (PropulsionBrain && PropulsionBrain->IsAlternativeTurning())
		{
			PropulsionBrain->TickAngularThrust(GetMouseDirection());
		}
	}
}

void ASGSM_ShipPawn::TickLinearThrustInput(const FInputActionValue& InValue)
{
	if (PropulsionBrain)
	{
		const FVector2D& Value = InValue.Get<FVector2D>();

		const FVector LinearThrust = FVector(Value.X, Value.Y, 0);

		PropulsionBrain->TickLinearThrust(LinearThrust);
	}
}

void ASGSM_ShipPawn::EndLinearThrustInput(const FInputActionValue& InValue)
{
	if (PropulsionBrain)
	{
		PropulsionBrain->EndLinearThrust();
	}
}

void ASGSM_ShipPawn::TickAngularThrustInput(const FInputActionValue& InValue)
{
	if (InputType == ECommonInputType::MouseAndKeyboard)
	{
		if (PropulsionBrain->IsAlternativeTurning())
		{
			return;
		}
	}

	if (PropulsionBrain)
	{
		const FVector2D& Value = InValue.Get<FVector2D>();

		const FVector AngularThrust = FVector(Value.X, Value.Y, 0);

		PropulsionBrain->TickAngularThrust(AngularThrust);
	}
}

void ASGSM_ShipPawn::EndAngularThrustInput(const FInputActionValue& InValue)
{
	if (InputType == ECommonInputType::MouseAndKeyboard)
	{
		if (PropulsionBrain->IsAlternativeTurning())
		{
			return;
		}
	}

	if (PropulsionBrain)
	{
		PropulsionBrain->EndAngularThrust();
	}
}

void ASGSM_ShipPawn::TickBoostInput(const FInputActionValue& InValue)
{
	if (PropulsionBrain)
	{
		const float Value = InValue.Get<float>();

		PropulsionBrain->TickBoosting(Value);
	}
}

void ASGSM_ShipPawn::EndBoostInput()
{
	if (PropulsionBrain)
	{
		PropulsionBrain->EndBoosting();
	}
}

void ASGSM_ShipPawn::ToggleLinearBraking()
{
	if (PropulsionBrain)
	{
		PropulsionBrain->ToggleLinearBraking();
	}
}

void ASGSM_ShipPawn::ToggleAngularBraking()
{
	if (PropulsionBrain)
	{
		PropulsionBrain->ToggleAngularBraking();
	}
}

void ASGSM_ShipPawn::ToggleAltTurning()
{
	if (PropulsionBrain)
	{
		PropulsionBrain->ToggleAltTurning();
		PropulsionBrain->SetAngularBraking(!PropulsionBrain->IsAlternativeTurning());
	}
}

void ASGSM_ShipPawn::InputMethodeChanged(ECommonInputType NewInputType)
{
	InputType = NewInputType;

	switch (NewInputType)
	{
	case ECommonInputType::MouseAndKeyboard:
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerController->SetShowMouseCursor(true);
		}
		break;
	case ECommonInputType::Gamepad:
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerController->SetShowMouseCursor(false);
		}
		break;
	default:
		break;
	}
}

void ASGSM_ShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			EnhancedInputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::TickLinearThrustInput);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::EndLinearThrustInput);

		// Boost
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Triggered, this, &ThisClass::TickBoostInput);
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Completed, this, &ThisClass::EndBoostInput);

		// Turn
		EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ThisClass::TickAngularThrustInput);
		EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Completed, this, &ThisClass::EndAngularThrustInput);

		// Glide
		EnhancedInputComponent->BindAction(GlideToggleAction, ETriggerEvent::Completed, this, &ThisClass::ToggleLinearBraking);
		
		// Alternative Turning
		EnhancedInputComponent->BindAction(AltTurningToggleAction, ETriggerEvent::Completed, this, &ThisClass::ToggleAltTurning);
	}
}

FVector ASGSM_ShipPawn::GetMouseDirection() const
{
	const APlayerController* const PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return FVector::ZeroVector;
	}

	FVector2D MousePosition;
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);

	FVector2D ActorScreenLocation;
	PlayerController->ProjectWorldLocationToScreen(GetActorLocation(), ActorScreenLocation);

	return -FVector(ActorScreenLocation - MousePosition, 0).GetSafeNormal();
}
