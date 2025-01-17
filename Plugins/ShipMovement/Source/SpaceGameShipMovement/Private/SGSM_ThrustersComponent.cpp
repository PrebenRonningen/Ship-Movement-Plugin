// Copyright Distant Light Games, Inc. All Rights Reserved.

#include "SGSM_ThrustersComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Physics/PhysicsInterfaceCore.h"


USGSM_ThrustersComponent::USGSM_ThrustersComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	SetAsyncPhysicsTickEnabled(true);
}

void USGSM_ThrustersComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		OwnerRootMesh = Owner->GetComponentByClass<UStaticMeshComponent>();
		ensureAlwaysMsgf(OwnerRootMesh, TEXT("Failed to get Owner Root Static Mesh Component"));

		PrimitiveComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		ensureAlwaysMsgf(PrimitiveComponent, TEXT("Failed to get Owner Root Component as Primitive Component"));
	}
}

void USGSM_ThrustersComponent::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickComponent(DeltaTime, SimTime);

	if (PrimitiveComponent && !PrimitiveComponent->IsSimulatingPhysics())
	{
		return;
	}

	if (ThrusterInput.bLinearBrake && !bLinearThrustActive)
	{
		PhysicsTickLinearBrake(DeltaTime);
	}

	if (!ThrusterInput.LinearThrustDirection.IsNearlyZero())
	{
		PhysicsTickLinearThrust(DeltaTime);
	}

	if (ThrusterInput.bAngularBrake && !bAngularThrustActive)
	{
		PhysicsTickAngularBrake(DeltaTime);
	}

	if (!ThrusterInput.AngularThrustDirection.IsNearlyZero())
	{
		if (ThrusterInput.bAlternativeTurning)
		{
			PhysicsTickScreenRelativeAngularThrust(DeltaTime);
		}
		else
		{
			PhysicsTickAngularThrust(DeltaTime);
		}
	}
}

void USGSM_ThrustersComponent::PhysicsTickLinearThrust(float DeltaTime)
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	const FVector Input = ThrusterInput.LinearThrustDirection;

	const FVector CurrentThrustOutputVecor = GetCurrentThrustOutput(Input);

	const FVector AppliedThrust = CurrentThrustOutputVecor;

	RigidBodyHandle->AddForce(AppliedThrust, true);
	bLinearThrustActive = true;
	LinearThrustVector = AppliedThrust;
}

void USGSM_ThrustersComponent::EndLinearThrust()
{
	bLinearThrustActive = false;
	ThrusterInput.LinearThrustDirection = FVector::ZeroVector;
	LinearThrustVector = FVector::ZeroVector;
}

void USGSM_ThrustersComponent::PhysicsTickLinearBrake(float DeltaTime)
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	const FVector LinearVelocity = RigidBodyHandle->GetV();

	if (LinearVelocity.IsNearlyZero())
	{
		RigidBodyHandle->SetV(FVector::ZeroVector);
		LinearThrustVector = FVector::ZeroVector;
		return;
	}

	const FVector Direction = LinearVelocity.GetSafeNormal2D();

	const double MaxForceMagnitude = GetMaxThrustOutput(-Direction).Length();

	const double StoppingForce = SGSM_Utils::GetKiloNewtonToCentiNewtons(LinearVelocity.Length() * 10);
	const double ForceUsed = FMath::Min(StoppingForce, MaxForceMagnitude);

	const FVector ThrustVector = ForceUsed * -Direction;
	const FVector AppliedThrust = ThrustVector;


	RigidBodyHandle->AddForce(AppliedThrust, true);
	LinearThrustVector = AppliedThrust;
}

void USGSM_ThrustersComponent::PhysicsTickAngularThrust(float DeltaTime)
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("AngularThrust - Mass : %f"), RigidBodyHandle->M());

	const double InputX = ThrusterInput.AngularThrustDirection.X;

	const FVector RigidBodyInertia = static_cast<FVector>(RigidBodyHandle->I());
	double Inertia = RigidBodyInertia.GetMax();

	const FVector Torque = FVector::UpVector * GetMaxAngularCentinewtons() * InputX;

	const FVector AngularVelocity = RigidBodyHandle->GetW();
	const double AngularVelocityRad = AngularVelocity.Z;

	const FVector ExpectedAcceleration = (Torque / Inertia) * DeltaTime;
	const double ExpectedAccRad = ExpectedAcceleration.Z;

	FVector FinalTorque = Torque;

	if (FMath::Abs(AngularVelocityRad + ExpectedAccRad) > FMath::DegreesToRadians(MaxRotationDegPerSec))
	{
		// Reduce torque with the needed amount to not go higher than MaxRotationDegPerSec
		const double ExcessVelocity = (FMath::Abs(AngularVelocityRad + ExpectedAccRad) - FMath::DegreesToRadians(MaxRotationDegPerSec));
		const double ExcessTorque = (ExcessVelocity / ExpectedAccRad);
		FinalTorque += ExcessTorque * Torque * FMath::Sign(-InputX);
	}

	if (FMath::Abs(AngularVelocityRad) - FMath::DegreesToRadians(MaxRotationDegPerSec) > 0.0001)
	{
		return;
	}

	RigidBodyHandle->AddTorque(FinalTorque, true);

	bAngularThrustActive = true;
	CurrentYawTorque = FinalTorque.Z;
}

void USGSM_ThrustersComponent::PhysicsTickAngularBrake(float DeltaTime)
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	const FVector AngularVelocity = RigidBodyHandle->GetW();

	if (AngularVelocity.IsNearlyZero())
	{
		RigidBodyHandle->SetW(FVector::ZeroVector);
		CurrentYawTorque = 0.0;
		return;
	}

	const double MaxTorque = GetMaxAngularCentinewtons();
	const FVector RigidBodyInertia = static_cast<FVector>(RigidBodyHandle->I());
	double Inertia = RigidBodyInertia.GetMax();


	const double DesiredDeceleration = AngularVelocity.Z / DeltaTime;
	const double DesiredTorque = FMath::Abs(Inertia * DesiredDeceleration);
	const double TorqueToApply = FMath::Min(DesiredTorque, MaxTorque);

	const FVector TorqueUse = FVector::UpVector * (FMath::Sign(-AngularVelocity.Z) * TorqueToApply);

	RigidBodyHandle->AddTorque(TorqueUse, true);
	CurrentYawTorque = TorqueUse.Z;
}

void USGSM_ThrustersComponent::PhysicsTickScreenRelativeAngularThrust(float DeltaTime)
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	const FVector ForwardVector = RigidBodyHandle->R().Vector();
	const FVector DirectionVector = FVector(-ThrusterInput.AngularThrustDirection.Y, ThrusterInput.AngularThrustDirection.X, 0);

	const FVector CurrentAngularVelocity = RigidBodyHandle->W();

	if (DirectionVector.IsNearlyZero())
	{
		return;
	}

	const double AngularVelocity = CurrentAngularVelocity.Z;
	const double MaxTorque = GetMaxAngularCentinewtons();
	const FVector RigidBodyInertia = static_cast<FVector>(RigidBodyHandle->I());
	double Inertia = RigidBodyInertia.GetMax();

	const double MaxTickAcceleration = (MaxTorque / Inertia) * DeltaTime;

	constexpr double Tolerance = 0.001;
	if (ForwardVector.Equals(DirectionVector, Tolerance) && FMath::Abs(AngularVelocity) <= MaxTickAcceleration)
	{
		if (FMath::Abs(AngularVelocity / DeltaTime) > 0)
		{
			const int8 Sign = FMath::Sign(AngularVelocity);
			const double Torque = SGSM_Utils::GetNewtonToCentiNewtons(FMath::Abs(AngularVelocity * Inertia));
			const FVector TorqueUsed = FMath::Min(MaxTorque, Torque) * FVector::UpVector * -Sign;

			if (Torque < (MaxTorque / 1000.0))
			{
				if (FMath::Abs(AngularVelocity / DeltaTime) > FLT_EPSILON)
				{
					RigidBodyHandle->SetW(FVector::ZeroVector, true);
					CurrentYawTorque = 0;
				}
			}
			else
			{
				RigidBodyHandle->AddTorque(TorqueUsed, true);
				CurrentYawTorque = TorqueUsed.Z;
			}
		}
		return;
	}

	const FVector CrossProduct = FVector::CrossProduct(ForwardVector, DirectionVector);
	const double Magnitude = CrossProduct.Size();
	const double DotProduct = FVector::DotProduct(ForwardVector, DirectionVector);

	const double AngleRadians = FMath::Atan2(Magnitude, DotProduct);

	{
		const double MaxAngularAcceleration = (MaxTorque / Inertia);
		const double BreakingDistance = (AngularVelocity * AngularVelocity) / (2 * MaxAngularAcceleration);
		const double BreakingRevolutions = BreakingDistance / UE_TWO_PI;
		const double Torque = FMath::Min(MaxTorque, SGSM_Utils::GetNewtonToCentiNewtons(FMath::Abs(Inertia) * FMath::Abs(AngularVelocity + FMath::Abs(AngleRadians))));

		if (FMath::IsNearlyZero(AngularVelocity))
		{
			const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(CrossProduct.Z);

			RigidBodyHandle->AddTorque(AppliedTorque, true);

			bAngularThrustActive = true;
			CurrentYawTorque = AppliedTorque.Z;
			return;
		}

		if ((FMath::Sign(AngularVelocity) == FMath::Sign(CrossProduct.Z)) && BreakingDistance < AngleRadians)
		{
			const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(CrossProduct.Z);

			RigidBodyHandle->AddTorque(AppliedTorque, true);

			bAngularThrustActive = true;
			CurrentYawTorque = AppliedTorque.Z;
			return;
		}

		if ((FMath::Sign(AngularVelocity) == FMath::Sign(CrossProduct.Z)) && BreakingDistance > AngleRadians && FMath::Abs(BreakingDistance - AngleRadians) < FMath::DegreesToRadians(MissTolerance))
		{
			const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(-CrossProduct.Z);

			RigidBodyHandle->AddTorque(AppliedTorque, true);

			bAngularThrustActive = true;
			CurrentYawTorque = AppliedTorque.Z;
			return;
		}

		if ((FMath::Sign(-AngularVelocity) == FMath::Sign(CrossProduct.Z)))
		{
			const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(CrossProduct.Z);

			RigidBodyHandle->AddTorque(AppliedTorque, true);

			bAngularThrustActive = true;
			CurrentYawTorque = AppliedTorque.Z;
			return;
		}

		if (BreakingDistance > AngleRadians)
		{
			const double MissDistance = (FMath::Frac(BreakingRevolutions) * UE_TWO_PI) - AngleRadians;

			if (MissDistance < 0 && MissDistance > -UE_HALF_PI)
			{
				const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(AngularVelocity);

				RigidBodyHandle->AddTorque(AppliedTorque, true);

				bAngularThrustActive = true;
				CurrentYawTorque = AppliedTorque.Z;
				return;
			}

			if (MissDistance < 0 && MissDistance < -UE_HALF_PI)
			{
				const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(AngularVelocity);

				RigidBodyHandle->AddTorque(AppliedTorque, true);

				bAngularThrustActive = true;
				CurrentYawTorque = AppliedTorque.Z;
				return;
			}

			if (MissDistance > 0 && MissDistance < UE_HALF_PI)
			{
				const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(-AngularVelocity);

				RigidBodyHandle->AddTorque(AppliedTorque, true);

				bAngularThrustActive = true;
				CurrentYawTorque = AppliedTorque.Z;
				return;
			}

			if (MissDistance > 0 && MissDistance > UE_HALF_PI)
			{
				const FVector AppliedTorque = FVector::UpVector * Torque * FMath::Sign(AngularVelocity);

				RigidBodyHandle->AddTorque(AppliedTorque, true);
				bAngularThrustActive = true;
				CurrentYawTorque = AppliedTorque.Z;
				return;
			}
		}
	}
}

FVector USGSM_ThrustersComponent::GetEngagementVector(const FVector& InDirection, const TMap<EDirection, double>& InDirectionMultiplier) const
{

	FVector ForwardVector = FVector::ZeroVector;
	if (!IsInGameThread())
	{
		Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
		if (!ensureAlways(RigidBodyHandle))
		{
			return ForwardVector;
		}
		ForwardVector = RigidBodyHandle->R().GetForwardVector();
	}
	else if (IsInGameThread())
	{
		if (!ensureAlways(OwnerRootMesh))
		{
			return ForwardVector;
		}
		ForwardVector = OwnerRootMesh->GetForwardVector();
	}

	if (!ensureAlways(!ForwardVector.Equals(FVector::ZeroVector)))
	{
		return ForwardVector;
	}

	const FVector DirectionVector = SGSM_Utils::GetThrustEngagementVector(ForwardVector, InDirection, InDirectionMultiplier);
	return DirectionVector;
}

void USGSM_ThrustersComponent::EndAngularThrust()
{
	bAngularThrustActive = false;
	ThrusterInput.AngularThrustDirection = FVector::ZeroVector;
	CurrentYawTorque = 0.0;
}

bool USGSM_ThrustersComponent::IsLinearThrustActive() const
{
	return bLinearThrustActive;
}

bool USGSM_ThrustersComponent::IsAngularThrustActive() const
{
	return bAngularThrustActive;
}

void USGSM_ThrustersComponent::SetAngularThrustDirection(const FVector& InAngularThrustDirection)
{
	ThrusterInput.AngularThrustDirection = InAngularThrustDirection;
}

FVector USGSM_ThrustersComponent::GetLinearThrustVector() const
{
	return LinearThrustVector;
}

void USGSM_ThrustersComponent::SetLinearThrustDirection(const FVector& InLinearThrustDirection)
{
	ThrusterInput.LinearThrustDirection = InLinearThrustDirection;
}

void USGSM_ThrustersComponent::SetBoosting(bool bState)
{
	bBoosting = bState;
}

bool USGSM_ThrustersComponent::IsBoosting() const
{
	return bBoosting;
}

void USGSM_ThrustersComponent::SetBoostAmount(float Value)
{
	BoostPercent = Value;
}

void USGSM_ThrustersComponent::ToggleAltTurning()
{
	ThrusterInput.bAlternativeTurning = !ThrusterInput.bAlternativeTurning;
}

void USGSM_ThrustersComponent::SetAlternativeTurning(bool bIsEnabled)
{
	ThrusterInput.bAlternativeTurning = bIsEnabled;
}

bool USGSM_ThrustersComponent::IsAlternativeTurning() const
{
	return ThrusterInput.bAlternativeTurning;
}

void USGSM_ThrustersComponent::SetLinearBraking(bool bIsEnabled)
{
	ThrusterInput.bLinearBrake = bIsEnabled;
}

void USGSM_ThrustersComponent::ToggleAngularBraking()
{
	ThrusterInput.bAngularBrake = !ThrusterInput.bAngularBrake;
}

void USGSM_ThrustersComponent::SetAngularBraking(bool bIsEnabled)
{
	ThrusterInput.bAngularBrake = bIsEnabled;
}

void USGSM_ThrustersComponent::ToggleLinearBraking()
{
	ThrusterInput.bLinearBrake = !ThrusterInput.bLinearBrake;
}

bool USGSM_ThrustersComponent::IsLinearBraking() const
{
	return ThrusterInput.bLinearBrake;
}

bool USGSM_ThrustersComponent::IsAngularBraking() const
{
	return ThrusterInput.bAngularBrake;
}

double USGSM_ThrustersComponent::GetMaxLinearCentinewtons() const
{
	return SGSM_Utils::GetKiloNewtonToCentiNewtons(MaxLinearKiloNewtons);
}

FVector USGSM_ThrustersComponent::GetCurrentThrustOutput(const FVector& InDirection) const
{
	const FVector ThrustersEngagmentVector = GetEngagementVector(InDirection, ThrusterInput.ThrustMultiplier);
	const FVector BoostEngagmentVector = GetEngagementVector(InDirection, ThrusterInput.BoostMultiplier) * (bBoosting ? BoostPercent : 0);

	const FVector ThrustOutput = (BoostEngagmentVector + ThrustersEngagmentVector);

	const FVector ThrustOutputVector = ThrustOutput * GetMaxLinearCentinewtons();

	return ThrustOutputVector;
}

FVector USGSM_ThrustersComponent::GetMaxThrustOutput(const FVector& InDirection) const
{
	const FVector Direction = FVector(FMath::Sign(InDirection.X), FMath::Sign(InDirection.Y), FMath::Sign(InDirection.Z));

	const FVector ThrustersEngagmentVector = GetEngagementVector(Direction, ThrusterInput.ThrustMultiplier);

	const FVector BoostEngagmentVector = GetEngagementVector(Direction, ThrusterInput.BoostMultiplier) * (bBoosting ? BoostPercent : 0);

	const FVector MaxThrustOutput = (BoostEngagmentVector + ThrustersEngagmentVector);

	const FVector MaxThrustOutputVector = MaxThrustOutput * GetMaxLinearCentinewtons();

	return MaxThrustOutputVector;
}

double USGSM_ThrustersComponent::GetMaxAngularCentinewtons() const
{
	return SGSM_Utils::GetKiloNewtonToCentiNewtons(MaxYawKiloNewtons);
}

double USGSM_ThrustersComponent::GetCurrentYawTorqueNormalized() const
{
	const double MaxYawCentinewtons = SGSM_Utils::GetKiloNewtonToCentiNewtons(MaxYawKiloNewtons);

	return UKismetMathLibrary::MapRangeClamped(
		CurrentYawTorque, -MaxYawCentinewtons, MaxYawCentinewtons, -1.0f, 1.0f);
}

void USGSM_ThrustersComponent::SetThrusterSpecifications(const FThrustersSpecifications& InThrusterSpecifications)
{
	MaxLinearKiloNewtons = InThrusterSpecifications.LinearThrustKiloNewtons;
	MaxRotationDegPerSec = InThrusterSpecifications.MaxAngularVelocity;
	MaxYawKiloNewtons = InThrusterSpecifications.TorqueKiloNewtons;

	ThrusterInput.ThrustMultiplier = InThrusterSpecifications.ThrustMultiplier;
	ThrusterInput.BoostMultiplier = InThrusterSpecifications.BoostMultiplier;
}
