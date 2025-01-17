// Copyright Distant Light Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISGSM_Rocket.h"
#include "Components/ActorComponent.h"
#include "SGSM_Utils.h"
#include "SGSM_RocketComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPACEGAMESHIPMOVEMENT_API USGSM_RocketComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	USGSM_RocketComponent(const FObjectInitializer& ObjectInitializer);

protected:

	virtual void BeginPlay() override;
	virtual void AsyncPhysicsTickComponent(float DeltaTime, float SimTime) override;

	virtual void OnAttachmentChanged() override;

public:

	UPROPERTY(BlueprintReadWrite, Category = "SGSM Rocket Component")
	TScriptInterface<ISGSM_Rocket> RocketInterface;

	void TickThrust(const double InScale);
	void EndThrust();

	FVector GetMaxThrustVector() const;
	bool IsRocketThrusting() const;

	UFUNCTION(BlueprintCallable, Category = "SGSM Rocket Component")
	double GetCurrentThrustPower() const;

	UFUNCTION(BlueprintCallable, Category = "SGSM Rocket Component")
	FVector GetCurrentThrustVector() const;

	UFUNCTION(BlueprintCallable, Category = "SGSM Rocket Component")
	double GetMaxThrustPower() const;

	UFUNCTION(BlueprintCallable, Category = "Rocket Component")
	void SetRocketSpecifications(const FRocketSpecifications& InRocketSpecifications);

protected:

	//Physics
	void PhysicsTickThrust(float DeltaTime) const;

private:

	UStaticMeshComponent* GetRootMesh() const;

	UPROPERTY(VisibleAnywhere, Category = "Rocket Component", Meta = (DisplayName = "Max Linear Thrust (kN)", ToolTip = "Max linear thrust in mega newtons, used to calculate linear acceleration."))
	float MaxLinearKiloNewtons = 0;

	UPROPERTY(Transient)
	UStaticMeshComponent* OwnerRootMesh = nullptr;

	FRocketInput RocketInput;

	UPROPERTY(Transient)
	UPrimitiveComponent* PrimitiveComponent = nullptr;

	FVector CurrentThrustVector = FVector::ZeroVector;

};
