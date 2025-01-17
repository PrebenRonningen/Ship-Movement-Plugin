// Copyright Distant Light Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "SGSM_Utils.generated.h"

UENUM()
enum class EDirection : uint8
{
	None UMETA(Hide),
	Front,
	Back,
	Left,
	Right,
	Derrived,
	Count UMETA(Hide)
};
ENUM_RANGE_BY_COUNT(EDirection, EDirection::Count);

USTRUCT(BlueprintType)
struct FThrustersSpecifications
{
	GENERATED_BODY()

	// Linear Thrust
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrusters Component - Linear", Meta = (DisplayName = "Linear Thrust (kN)", ToolTip = "Max linear thrust in kilo newtons, used to calculate linear acceleration."))
	double LinearThrustKiloNewtons = 0;

	// Angular Thrust
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrusters Component - Angular", Meta = (DisplayName = "Torque (kNm)", ToolTip = "Max torque in kilo newton meters, used to calculate angular acceleration - turn rate."))
	double TorqueKiloNewtons = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrusters Component - Angular", Meta = (Units = "DegreesPerSecond", DisplayName = "Max Angular Velocity (deg/s)", ToolTip = "Max angular velocity in degrees per second, used to clamp angular velocity."))
	double MaxAngularVelocity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Directional Multiplier", meta = (InvalidEnumValues = "Count"))
	TMap<EDirection, double> ThrustMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Directional Multiplier", meta = (InvalidEnumValues = "Count"))
	TMap<EDirection, double> BoostMultiplier;
};

USTRUCT(BlueprintType)
struct FRocketSpecifications
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rocket Component - Linear", Meta = (DisplayName = "Max Engine Thrust (kN)", ToolTip = "Max rocket engine thrust in kilo newtons, used to calculate linear acceleration."))
	double LinearThrustKiloNewtons = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Directional Multiplier", meta = (InvalidEnumValues = "Count"))
	TMap<EDirection, double> BoostMultiplier;
};

USTRUCT()
struct FThrusterInput
{
	GENERATED_BODY()
	FVector AngularThrustDirection = FVector::ZeroVector;
	FVector LinearThrustDirection = FVector::ZeroVector;
	bool bLinearBrake = false;
	bool bAngularBrake = false;
	bool bAlternativeTurning = false;

	TMap<EDirection, double> ThrustMultiplier;
	TMap<EDirection, double> BoostMultiplier;
};

USTRUCT()
struct FRocketInput
{
	GENERATED_BODY()
	bool bRocketThrusting = false;

	FVector ThrustMultiplier;

	TMap<EDirection, double> BoostMultiplier;
};

class SPACEGAMESHIPMOVEMENT_API SGSM_Utils
{
public:

	static Chaos::FRigidBodyHandle_Internal* GetRigidBodyHandle(const UPrimitiveComponent* const InPrimitiveComponent);

	static double GetNewtonToCentiNewtons(const double& InCentiNewton);
	static double GetKiloNewtonToCentiNewtons(const double& InCentiNewton);
	static double GetMegaNewtonToCentiNewtons(const double& InCentiNewton);

	static int32 GetCentinewtonsPerNewton() { return CentinewtonsPerNewton; }
	static int32 GetCentinewtonsPerKiloNewton() { return CentinewtonsPerKiloNewton; }
	static int32 GetCentinewtonsPerMegaNewton() { return CentinewtonsPerMegaNewton; }

	static FVector GetThrustEngagementVector(const FVector& ForwardVector, const FVector& InDirection, const TMap<EDirection, double>& InDirectionMultiplier);


protected:

private:

	SGSM_Utils();
	~SGSM_Utils();

public:

protected:

private:

	static int32 CentinewtonsPerNewton;
	static int32 CentinewtonsPerKiloNewton;
	static int32 CentinewtonsPerMegaNewton;

};
