// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FABRIKContainer.generated.h"

UCLASS()
class FABRIKCUSTOM_API AFABRIKContainer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFABRIKContainer();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 ChainLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Iterations;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double Delta;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SnapbackStrength;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* Target;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* Pole;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bActivateComputation;

private:
	void DrawHelpers();

	void Init();

	void ResolveIK();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Target To Origin
	UPROPERTY()
	TArray<double> BonesLength;

	double CompleteLength;

	UPROPERTY()
	TArray<AActor*> Bones;

	TArray<FVector> Positions;

	TArray<FVector> StartDirectionSucc;

	TArray<FRotator> StartRotationBone;

	FRotator StartRotationTarget;

	FRotator StartRotationRoot;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
