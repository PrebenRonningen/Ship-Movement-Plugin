#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISGSM_Rocket.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USGSM_Rocket : public UInterface
{
	GENERATED_BODY()
};

class SPACEGAMESHIPMOVEMENT_API ISGSM_Rocket
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SGSM Rocket Interface")
	double GetCurrentEfficiencyMultiplier() const;

	UFUNCTION()
	virtual double GetCurrentEfficiencyMultiplierPure() const = 0;

};
