#pragma once

#include "CoreMinimal.h"
#include "FABRIKConstraint.generated.h"

USTRUCT(BlueprintType)
struct FFABRIKConstraint
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> LocationConstraints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseLocationConstraints = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRotator> RotationConstraints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseRotationConstraints = false;
};
