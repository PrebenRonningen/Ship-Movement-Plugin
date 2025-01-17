// Copyright Distant Light Games, Inc. All Rights Reserved.


#include "SGSM_Utils.h"
#include "SGSM_LogCategory.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"


int32 SGSM_Utils::CentinewtonsPerNewton = 100;
int32 SGSM_Utils::CentinewtonsPerKiloNewton = 100000;
int32 SGSM_Utils::CentinewtonsPerMegaNewton = 100000000;

SGSM_Utils::SGSM_Utils()
{
}

SGSM_Utils::~SGSM_Utils()
{
}

Chaos::FRigidBodyHandle_Internal* SGSM_Utils::GetRigidBodyHandle(const UPrimitiveComponent* const InPrimitiveComponent)
{
	if (!InPrimitiveComponent)
	{
		UE_LOG(SMLogUtils, Error, TEXT("ERROR - Failed to get RigidBodyHandle!"));
		return nullptr;
	}

	FBodyInstance* BI = InPrimitiveComponent->GetBodyInstance();
	if (!BI)
	{
		UE_LOG(SMLogUtils, Error, TEXT("ERROR - Failed to get BodyInstance of \"%s\"!"), *GetFNameSafe(InPrimitiveComponent).ToString());
		return nullptr;
	}

	FPhysicsActorHandle Handle = BI->GetPhysicsActorHandle();

	return Handle->GetPhysicsThreadAPI();
}

double SGSM_Utils::GetNewtonToCentiNewtons(const double& InNewton)
{
	return InNewton * CentinewtonsPerNewton;
}

double SGSM_Utils::GetKiloNewtonToCentiNewtons(const double& InKiloNewton)
{
	return InKiloNewton * CentinewtonsPerKiloNewton;
}
double SGSM_Utils::GetMegaNewtonToCentiNewtons(const double& InMegaNewton)
{
	return InMegaNewton * CentinewtonsPerMegaNewton;
}

FVector SGSM_Utils::GetThrustEngagementVector(const FVector& ForwardVector, const FVector& InDirection, const TMap<EDirection, double>& InDirectionMultiplier)
{
	if (InDirectionMultiplier.IsEmpty())
	{
		return FVector::ZeroVector;
	}

	FVector ThrustEngagmentVector = ForwardVector.Rotation().GetInverse().RotateVector(InDirection);

	if (ThrustEngagmentVector.X < 0 && InDirectionMultiplier.Contains(EDirection::Front))
	{
		ThrustEngagmentVector.X *= InDirectionMultiplier[EDirection::Front];
	}
	else if (ThrustEngagmentVector.X > 0 && InDirectionMultiplier.Contains(EDirection::Back))
	{
		ThrustEngagmentVector.X *= InDirectionMultiplier[EDirection::Back];
	}

	if (ThrustEngagmentVector.Y < 0 && InDirectionMultiplier.Contains(EDirection::Right))
	{
		ThrustEngagmentVector.Y *= InDirectionMultiplier[EDirection::Right];
	}
	else if (ThrustEngagmentVector.Y > 0 && InDirectionMultiplier.Contains(EDirection::Left))
	{
		ThrustEngagmentVector.Y *= InDirectionMultiplier[EDirection::Left];
	}

	ThrustEngagmentVector = ForwardVector.Rotation().RotateVector(ThrustEngagmentVector);

	return ThrustEngagmentVector;
}
