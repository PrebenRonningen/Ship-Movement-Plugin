// Copyright Distant Light Games, Inc. All Rights Reserved.

#include "SGSM_PropulsionBrain.h"
#include "SGSM_ThrustersComponent.h"
#include "SGSM_RocketComponent.h"
#include "SGSM_LogCategory.h"


USGSM_PropulsionBrain::USGSM_PropulsionBrain(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_StartPhysics;
}

void USGSM_PropulsionBrain::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = GetOwner<APawn>();
	ensureAlwaysMsgf(OwnerPawn, TEXT("Failed to get Owner Pawn"));

	if (OwnerPawn)
	{
		ThrustersComponent = OwnerPawn->GetComponentByClass<USGSM_ThrustersComponent>();
		if (ensureAlwaysMsgf(ThrustersComponent, TEXT("Failed to get Thrusters Component")))
		{
			ThrustersComponent->SetThrusterSpecifications(ThrusterSpecs);
		}

		TArray<USGSM_RocketComponent*> RocketComponents;
		OwnerPawn->GetComponents<USGSM_RocketComponent>(RocketComponents, true);

		for (USGSM_RocketComponent* const Rocket : RocketComponents)
		{
			AddRocketComponent(Rocket);
		}

		SetupRockets();

		PawnRootMesh = OwnerPawn->GetComponentByClass<UStaticMeshComponent>();
		ensureAlwaysMsgf(PawnRootMesh, TEXT("Failed to get Static Mesh Component"));
	}
}

double USGSM_PropulsionBrain::CalculateRocketEngagementValue(const USGSM_RocketComponent* InThruster, const FVector& InDirection)
{
	if (!InThruster)
	{
		return 0.0;
	}

	const FVector F = InThruster->GetForwardVector();
	const FVector R = (InDirection.Cross(F).Z < 0) ? InThruster->GetRightVector() : -InThruster->GetRightVector();

	const FVector A = F - R;
	const FVector B = InDirection;
	const FVector C = B - R;

	double DotProduct = FVector::DotProduct(C, A);
	double A_Squared = FVector::DotProduct(A, A);

	const double t = FMath::Clamp(DotProduct / A_Squared, 0, 1);

	return t;
}

void USGSM_PropulsionBrain::TickLinearThrust(const FVector& InValue)
{
	if (InValue.IsNearlyZero())
	{
		return;
	}

	LinearThrustDirection = FVector(FMath::Clamp(InValue.X, -1, 1), FMath::Clamp(InValue.Y, -1, 1), FMath::Clamp(InValue.Z, -1, 1));

	if (ThrustersComponent)
	{
		ThrustersComponent->SetLinearThrustDirection(LinearThrustDirection);
	}
}

void USGSM_PropulsionBrain::EndLinearThrust()
{
	if (ThrustersComponent)
	{
		LinearThrustDirection = FVector::ZeroVector;
		ThrustersComponent->EndLinearThrust();
	}
}

void USGSM_PropulsionBrain::TickAngularThrust(const FVector& InValue)
{
	if (InValue.IsNearlyZero())
	{
		return;
	}

	AngularThrustDirection = FVector(FMath::Clamp(InValue.X, -1, 1), FMath::Clamp(InValue.Y, -1, 1), FMath::Clamp(InValue.Z, -1, 1));

	if (ThrustersComponent)
	{
		ThrustersComponent->SetAngularThrustDirection(AngularThrustDirection);
	}
}

void USGSM_PropulsionBrain::EndAngularThrust()
{
	if (ThrustersComponent)
	{
		AngularThrustDirection = FVector::ZeroVector;
		ThrustersComponent->EndAngularThrust();
	}
}

void USGSM_PropulsionBrain::TickBoosting(const float& InValue)
{
	if (!OwnerPawn || Rockets.IsEmpty())
	{
		return;
	}

	for (USGSM_RocketComponent* const Rocket : Rockets)
	{
		if (!Rocket)
		{
			continue;
		}

		double ActivationPercentage;
		if (LinearThrustDirection != FVector::ZeroVector)
		{
			ActivationPercentage = CalculateRocketEngagementValue(Rocket, LinearThrustDirection) * LinearThrustDirection.Length();
		}
		else
		{
			ActivationPercentage = CalculateRocketEngagementValue(Rocket, OwnerPawn->GetActorForwardVector());
		}
		const double FinalThrustValue = InValue * ActivationPercentage;

		Rocket->TickThrust(FMath::Clamp(FinalThrustValue, 0, 1));
	}

	if (ThrustersComponent)
	{
		ThrustersComponent->SetBoosting(true);
		ThrustersComponent->SetBoostAmount(InValue);

		const FVector ThrustDirection = (LinearThrustDirection != FVector::ZeroVector) ? LinearThrustDirection : OwnerPawn->GetActorForwardVector();
		ThrustersComponent->SetLinearThrustDirection(ThrustDirection);
	}
}

void USGSM_PropulsionBrain::EndBoosting()
{
	if (ThrustersComponent)
	{
		ThrustersComponent->SetBoosting(false);
		ThrustersComponent->SetBoostAmount(0);
		ThrustersComponent->EndLinearThrust();
	}

	if (!Rockets.IsEmpty())
	{
		for (USGSM_RocketComponent* const Rocket : Rockets)
		{
			if (Rocket)
			{
				Rocket->EndThrust();
			}
		}
	}
}

bool USGSM_PropulsionBrain::IsBoosting() const
{
	if (ThrustersComponent)
	{
		return ThrustersComponent->IsBoosting();
	}
	return false;
}

void USGSM_PropulsionBrain::ToggleLinearBraking()
{
	if (ThrustersComponent)
	{
		ThrustersComponent->ToggleLinearBraking();
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Toggled Glide to %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(ThrustersComponent->IsLinearBraking() ? TEXT("True") : TEXT("False")),
			*FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

void USGSM_PropulsionBrain::SetLinearBraking(bool bIsEnabled)
{
	if (ThrustersComponent)
	{
		ThrustersComponent->SetLinearBraking(bIsEnabled);
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Set Glide to: %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(bIsEnabled ? TEXT("True") : TEXT("False")), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

bool USGSM_PropulsionBrain::IsLinearBraking() const
{
	if (ThrustersComponent)
	{
		return ThrustersComponent->IsLinearBraking();
	}
	return false;
}

void USGSM_PropulsionBrain::ToggleAngularBraking()
{
	if (ThrustersComponent)
	{
		ThrustersComponent->ToggleAngularBraking();
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Toggled Angular Brake to %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(ThrustersComponent->IsAngularBraking() ? TEXT("True") : TEXT("False")),
			*FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

void USGSM_PropulsionBrain::SetAngularBraking(bool bIsEnabled)
{
	if (ThrustersComponent)
	{
		ThrustersComponent->SetAngularBraking(bIsEnabled);
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Set Angular Brake to: %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(bIsEnabled ? TEXT("True") : TEXT("False")), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

bool USGSM_PropulsionBrain::IsAngularBraking() const
{
	if (ThrustersComponent)
	{
		return ThrustersComponent->IsAngularBraking();
	}
	return false;
}

void USGSM_PropulsionBrain::ToggleAltTurning()
{
	if (ThrustersComponent)
	{
		ThrustersComponent->ToggleAltTurning();
		EndAngularThrust();
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Toggled Alt-Turning to %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(ThrustersComponent->IsAlternativeTurning() ? TEXT("True") : TEXT("False")),
			*FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

void USGSM_PropulsionBrain::SetAlternativeTurning(bool bIsEnabled)
{
	if (ThrustersComponent)
	{
		ThrustersComponent->SetAlternativeTurning(bIsEnabled);
		EndAngularThrust();
		UE_CLOG(GetWorld(), SMLogBrain, Verbose, TEXT("\"%s\" Set Alt-Turning to %s at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*FString(bIsEnabled ? TEXT("True") : TEXT("False")), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

bool USGSM_PropulsionBrain::IsAlternativeTurning() const
{
	if (ThrustersComponent)
	{
		return ThrustersComponent->IsAlternativeTurning();
	}
	return false;
}

void USGSM_PropulsionBrain::SetThrustersSpecifications(const FThrustersSpecifications& InThrustersSpecifications)
{
	ThrusterSpecs = InThrustersSpecifications;
	if (ThrustersComponent)
	{
		ThrustersComponent->SetThrusterSpecifications(ThrusterSpecs);
	}
}

void USGSM_PropulsionBrain::SetRocketSpecifications(const FRocketSpecifications& InRocketSpecifications)
{
	RocketSpecs = InRocketSpecifications;
	if (!Rockets.IsEmpty())
	{
		SetupRockets();
	}
}

void USGSM_PropulsionBrain::AddRocketComponent(USGSM_RocketComponent* const NewRocketComponent)
{
	if (!Rockets.Contains(NewRocketComponent))
	{
		Rockets.Emplace(NewRocketComponent);
		UE_CLOG(GetWorld() && NewRocketComponent, SMLogBrain, Verbose, TEXT("\"%s\" Added Rocket Engine \"%s\" at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*GetFNameSafe(NewRocketComponent->GetOwner()).ToString(), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
	else
	{
		UE_CLOG(GetWorld() && NewRocketComponent, SMLogBrain, Warning, TEXT("\"%s\" Failed to Add Rocket Engine \"%s\" to container as it already exists at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*GetFNameSafe(NewRocketComponent->GetOwner()).ToString(), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

void USGSM_PropulsionBrain::RemoveRocketComponent(USGSM_RocketComponent* const InRocketComponent)
{
	if (Rockets.Contains(InRocketComponent))
	{
		Rockets.Remove(InRocketComponent);
		UE_CLOG(GetWorld() && InRocketComponent, SMLogBrain, Verbose, TEXT("\"%s\" Removed Rocket Engine \"%s\" at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*GetFNameSafe(InRocketComponent->GetOwner()).ToString(), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
	else
	{
		UE_CLOG(GetWorld() && InRocketComponent, SMLogBrain, Warning, TEXT("\"%s\" Failed to Remove Rocket Engine \"%s\" because it was not found in the container at: %s"), *GetFNameSafe(OwnerPawn).ToString(),
			*GetFNameSafe(InRocketComponent->GetOwner()).ToString(), *FString::SanitizeFloat(GetWorld()->GetRealTimeSeconds()));
	}
}

double USGSM_PropulsionBrain::GetAverageRocketPower() const
{
	if (Rockets.IsEmpty()) return 0.0;

	double Sum = 0.0;

	for (USGSM_RocketComponent* const Rocket : Rockets)
	{
		if (!Rocket || Rocket->GetMaxThrustPower() <= 0.0) continue;

		Sum += Rocket->GetCurrentThrustPower() / Rocket->GetMaxThrustPower();
	}

	return Sum / Rockets.Num();
}

FVector USGSM_PropulsionBrain::GetCurrentCombinedRocketsThrustVector() const
{
	FVector CombinedThrustVector = FVector::ZeroVector;

	for (const auto RocketComponent : Rockets)
	{
		if (!ensure(RocketComponent)) continue;

		CombinedThrustVector += RocketComponent->GetCurrentThrustVector();
	}

	return CombinedThrustVector;
}

void USGSM_PropulsionBrain::SetupRockets()
{
	if (Rockets.IsEmpty())
	{
		return;
	}

	for (USGSM_RocketComponent* const Rocket : Rockets)
	{
		if (Rocket)
		{
			Rocket->SetRocketSpecifications(RocketSpecs);
		}
	}
}

FVector USGSM_PropulsionBrain::GetCurrentLinearThrustNormal() const
{
	if (ThrustersComponent)
	{
		const FVector CurrentLinearThrust = ThrustersComponent->GetLinearThrustVector();

		if (CurrentLinearThrust.IsNearlyZero())
		{
			return FVector::ZeroVector;
		}

		const double CurrentLinearThrustLength = CurrentLinearThrust.Length();
		const FVector ThrustNormal = CurrentLinearThrust.GetSafeNormal();
		const double MaxLinearThrust = ThrustersComponent->GetMaxLinearCentinewtons();

		if (MaxLinearThrust <= 0)
		{
			return FVector::ZeroVector;
		}

		const double ThrustRatio = CurrentLinearThrustLength / MaxLinearThrust;
		const FVector Result = ThrustNormal * ThrustRatio;

		return Result;
	}
	return FVector::ZeroVector;
}
