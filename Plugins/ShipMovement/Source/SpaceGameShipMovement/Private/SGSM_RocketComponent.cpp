// Copyright Distant Light Games, Inc. All Rights Reserved.

#include "SGSM_RocketComponent.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"


USGSM_RocketComponent::USGSM_RocketComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	SetAsyncPhysicsTickEnabled(true);
}

void USGSM_RocketComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		PrimitiveComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		ensureAlwaysMsgf(PrimitiveComponent, TEXT("Failed to get Owner Root Component as Primitive Component"));

		OwnerRootMesh = GetRootMesh();
		ensureAlwaysMsgf(OwnerRootMesh, TEXT("Failed to get Owner Root Static Mesh Component"));
	}
}

void USGSM_RocketComponent::AsyncPhysicsTickComponent(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickComponent(DeltaTime, SimTime);

	if (PrimitiveComponent && !PrimitiveComponent->IsSimulatingPhysics())
	{
		return;
	}

	if (RocketInput.bRocketThrusting)
	{
		PhysicsTickThrust(DeltaTime);
	}
}

void USGSM_RocketComponent::OnAttachmentChanged()
{
	Super::OnAttachmentChanged();

	OwnerRootMesh = GetRootMesh();
}

void USGSM_RocketComponent::PhysicsTickThrust(float DeltaTime) const
{
	Chaos::FRigidBodyHandle_Internal* RigidBodyHandle = SGSM_Utils::GetRigidBodyHandle(PrimitiveComponent);
	if (!RigidBodyHandle)
	{
		return;
	}

	const FVector AppliedThrust = CurrentThrustVector;

	if (AppliedThrust.IsNearlyZero())
	{
		return;
	}

	RigidBodyHandle->AddForce(AppliedThrust, true);
}

UStaticMeshComponent* USGSM_RocketComponent::GetRootMesh() const
{
	UStaticMeshComponent* RootMesh = nullptr;
	if (const AActor* Owner = GetOwner())
	{
		RootMesh = Owner->GetComponentByClass<UStaticMeshComponent>();

		const UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

		if (RootComponent && RootComponent->GetAttachmentRootActor())
		{
			if (const AActor* RootOwner = RootComponent->GetAttachmentRootActor()->GetOwner())
			{
				if (UStaticMeshComponent* StaticMesh = RootOwner->GetComponentByClass<UStaticMeshComponent>())
				{
					RootMesh = StaticMesh;
				}
			}
		}
	}
	return RootMesh;
}

void USGSM_RocketComponent::TickThrust(const double InScale)
{
	if (OwnerRootMesh)
	{
		CurrentThrustVector = GetMaxThrustVector() * InScale;
		RocketInput.bRocketThrusting = true;
	}
}

void USGSM_RocketComponent::EndThrust()
{
	CurrentThrustVector = FVector::ZeroVector;
	RocketInput.bRocketThrusting = false;
}

FVector USGSM_RocketComponent::GetMaxThrustVector() const
{
	return GetMaxThrustPower() * GetForwardVector();
}

bool USGSM_RocketComponent::IsRocketThrusting() const
{
	return RocketInput.bRocketThrusting;
}

double USGSM_RocketComponent::GetCurrentThrustPower() const
{
	return CurrentThrustVector.Length();
}

FVector USGSM_RocketComponent::GetCurrentThrustVector() const
{
	return CurrentThrustVector;
}

double USGSM_RocketComponent::GetMaxThrustPower() const
{
	if (!ensure(RocketInterface)) return 0.0;
	const auto Multiplier = ISGSM_Rocket::Execute_GetCurrentEfficiencyMultiplier(RocketInterface.GetObject());
	return SGSM_Utils::GetKiloNewtonToCentiNewtons(MaxLinearKiloNewtons * Multiplier);
}

void USGSM_RocketComponent::SetRocketSpecifications(const FRocketSpecifications& InRocketSpecifications)
{
	RocketInput.BoostMultiplier = InRocketSpecifications.BoostMultiplier;

	const FVector ForwardVector = GetForwardVector();

	if (OwnerRootMesh)
	{
		const FVector ThrustMultiplier = SGSM_Utils::GetThrustEngagementVector(OwnerRootMesh->GetForwardVector(), ForwardVector, RocketInput.BoostMultiplier);

		MaxLinearKiloNewtons = InRocketSpecifications.LinearThrustKiloNewtons * ThrustMultiplier.Length();
	}
}
