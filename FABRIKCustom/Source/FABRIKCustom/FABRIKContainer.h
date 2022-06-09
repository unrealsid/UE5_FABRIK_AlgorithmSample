// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FABRIKConstraint.h"
#include "CinematicCamera/Public/CameraRig_Rail.h"
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
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FFABRIKConstraint Constraints;

	//Stores a ref to the rig rail
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ACameraRig_Rail* CameraRail;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float CurrentPositionOnRail;

protected:
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
	
private:
	FVector PreviousTargetLocation;

	FVector CurrentTargetLocation;
	
	void DrawHelpers();

	void Init();

	void ResolveIK(float DeltaTime);

	FRotator GetClampedBoneRotation(const AActor* Parent, FRotator UnclampedRotator, int32 BoneIdx);

	FVector GetTargetVelocity(float DeltaTime);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;



public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
