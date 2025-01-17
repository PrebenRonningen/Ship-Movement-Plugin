// Copyright Distant Light Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SGSM_Utils.h"
#include "SGSM_ThrustersComponent.generated.h"

UCLASS( ClassGroup=(Custom), Meta=(BlueprintSpawnableComponent))
class SPACEGAMESHIPMOVEMENT_API USGSM_ThrustersComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	USGSM_ThrustersComponent(const FObjectInitializer& ObjectInitializer);

	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

protected:

	virtual void BeginPlay() override;

public:

	void EndLinearThrust();
	void EndAngularThrust();

	void SetBoosting(bool bState);
	bool IsBoosting() const;
	void SetBoostAmount(float Value);

	void ToggleAltTurning();
	void SetAlternativeTurning(bool bIsEnabled);

	void ToggleLinearBraking();
	void SetLinearBraking(bool bIsEnabled);

	void ToggleAngularBraking();
	void SetAngularBraking(bool bIsEnabled);

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	bool IsLinearThrustActive() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	FVector GetLinearThrustVector() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	void SetLinearThrustDirection(const FVector& InLinearThrustDirection);

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	bool IsAngularThrustActive() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	void SetAngularThrustDirection(const FVector& InAngularThrustDirection);

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	bool IsAlternativeTurning() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	bool IsLinearBraking() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	bool IsAngularBraking() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	FVector GetCurrentThrustOutput(const FVector& InDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	FVector GetMaxThrustOutput(const FVector& InDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	double GetMaxLinearCentinewtons() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	double GetMaxAngularCentinewtons() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	double GetCurrentYawTorqueNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Thrusters Component")
	void SetThrusterSpecifications(const FThrustersSpecifications& InThrusterSpecifications);

	FThrusterInput GetThrusterInput() const { return ThrusterInput; }

protected:

	// Physics
	void PhysicsTickLinearThrust(float DeltaTime);
	void PhysicsTickLinearBrake(float DeltaTime);

	void PhysicsTickAngularThrust(float DeltaTime);
	void PhysicsTickAngularBrake(float DeltaTime);

	void PhysicsTickAltAngularThrust(float DeltaTime);
	void PhysicsTickScreenRelativeAngularThrust(float DeltaTime);
private:

	FVector GetEngagementVector(const FVector& InDirection, const TMap<EDirection, double>& InDirectionMultiplier) const;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrusters Component")
	bool ShowDebugInfo = false;

private:

	UPROPERTY(VisibleAnywhere, Category = "Thrusters Component", Meta = (DisplayName = "Max Linear Thrust (kN)", ToolTip = "Max linear thrust in kilo newtons, used to calculate linear acceleration."))
	float MaxLinearKiloNewtons = 0;

	UPROPERTY(VisibleAnywhere, Category = "Thrusters Component", Meta = (DisplayName = "Max Angular Velocity (deg/s)", ToolTip = "Max angular velocity in degrees per second, used to clamp angular velocity."))
	double MaxRotationDegPerSec = 0;

	UPROPERTY(VisibleAnywhere, Category = "Thrusters Component", Meta = (DisplayName = "Max Yaw Torque (kNm)", ToolTip = "Max yaw torque in kilo newton meters, used to calculate angular acceleration - turn rate."))
	double MaxYawKiloNewtons = 0;

	double BoostPercent = 0;

	UPROPERTY(VisibleAnywhere, Category = "Thrusters Component", Meta = (Units = "Degrees"))
	double MisTolerance = 5;

	UPROPERTY(Transient)
	UStaticMeshComponent* OwnerRootMesh = nullptr;

	UPROPERTY(Transient)
	UPrimitiveComponent* PrimitiveComponent = nullptr;

	FThrusterInput ThrusterInput{};


	FVector PreviousLocationBeforeBraking = FVector::ZeroVector;
	FVector LinearThrustVector = FVector::ZeroVector;

	double CurrentYawTorque = 0.0;
	double MissTolerance = 5.0;

	bool bLinearThrustActive = false;
	bool bAngularThrustActive = false;
	bool bBoosting = false;

};
