// Copyright Distant Light Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SGSM_Utils.h"
#include "SGSM_PropulsionBrain.generated.h"

class USGSM_RocketComponent;
class USGSM_ThrustersComponent;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPACEGAMESHIPMOVEMENT_API USGSM_PropulsionBrain : public UActorComponent
{
	GENERATED_BODY()

public:

	USGSM_PropulsionBrain(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Propulsion Brain")
	USGSM_ThrustersComponent* ThrustersComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Propulsion Brain - Rockets")
	TArray<USGSM_RocketComponent*> Rockets;

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Rockets")
	void AddRocketComponent(USGSM_RocketComponent* const NewRocketComponent);

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Rockets")
	void RemoveRocketComponent(USGSM_RocketComponent* const InRocketComponent);

	/**
	 * Returns a value between 0 and 1 representing the total rocket power being used.
	 * This can be useful when controlling a rocket sound effect that represents all rockets at once.
	 */
	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Rockets")
	double GetAverageRocketPower() const;

	/*
	 * Returns the combined thrust vectors of all rocket components owned by this propulsion brain.
	 */
	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Thrusters")
	FVector GetCurrentCombinedRocketsThrustVector() const;

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Rockets")
	void SetupRockets();

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Thrusters")
	FVector GetCurrentLinearThrustNormal() const;

	void TickLinearThrust(const FVector& InValue);
	void EndLinearThrust();

	void TickAngularThrust(const FVector& InValue);
	void EndAngularThrust();

	void TickBoosting(const float& InValue);
	void EndBoosting();
	bool IsBoosting() const;

	void ToggleLinearBraking();
	void SetLinearBraking(bool bIsEnabled);
	bool IsLinearBraking() const;

	void ToggleAngularBraking();
	void SetAngularBraking(bool bIsEnabled);
	bool IsAngularBraking() const;

	void ToggleAltTurning();
	void SetAlternativeTurning(bool IsEnabled);
	bool IsAlternativeTurning() const;

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Thrusters")
	void SetThrustersSpecifications(const FThrustersSpecifications& InThrustersSpecifications);

	UFUNCTION(BlueprintCallable, Category = "Propulsion Brain - Rockets")
	void SetRocketSpecifications(const FRocketSpecifications& InRocketSpecifications);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Propulsion Brain - Thrusters")
	FThrustersSpecifications ThrusterSpecs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Propulsion Brain - Rockets")
	FRocketSpecifications RocketSpecs;

	static double CalculateRocketEngagementValue(const USGSM_RocketComponent* InThruster, const FVector& InDirection);

	UPROPERTY(BlueprintReadOnly, Category = "Propulsion Brain")
	APawn* OwnerPawn = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Propulsion Brain")
	UStaticMeshComponent* PawnRootMesh = nullptr;

	FVector LinearThrustDirection = FVector::ZeroVector;
	FVector AngularThrustDirection = FVector::ZeroVector;

};
