// Fill out your copyright notice in the Description page of Project Settings.


#include "FABRIKContainer.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AFABRIKContainer::AFABRIKContainer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ChainLength = 2;
	Target = Pole = nullptr;
	Iterations = 10;
	Delta = 0.01;
	SnapbackStrength = 1.0;
}

void AFABRIKContainer::DrawHelpers()
{
	AActor* Current = this;
	AActor* Parent = GetAttachParentActor();
	
	for (int i = 0; (i < ChainLength) && (Current != nullptr) && (Current->GetAttachParentActor() != nullptr); i++)
	{
		//double Distance = FVector::Distance(Current->GetActorLocation(), Current->GetAttachParentActor()->GetActorLocation());
		DrawDebugLine(GetWorld(),
		              Current->GetActorLocation(),
		              Current->GetAttachParentActor()->GetActorLocation(),
		              FColor::Red,
		              false,
		              0,
		              0,
		              3);
		
		Current = Current->GetAttachParentActor();
	}
}

void AFABRIKContainer::Init()
{
	Bones.AddDefaulted(ChainLength + 1);
	Positions.AddDefaulted(ChainLength + 1);
	BonesLength.AddDefaulted(ChainLength);
	StartDirectionSucc.AddDefaulted(ChainLength + 1);
	StartRotationBone.AddDefaulted(ChainLength + 1);
	CompleteLength = 0;

	StartRotationTarget = Target->GetActorRotation();

	//Init data
	AActor* Current = this;
	for (int i = Bones.Num() - 1; i >= 0; i--)
	{
		Bones[i] = Current;
		StartRotationBone[i] = Current->GetActorRotation();

		if(i == Bones.Num() - 1)
		{
			//Leaf
			StartDirectionSucc[i] = Target->GetActorLocation() - Current->GetActorLocation();
		}
		else
		{
			StartDirectionSucc[i] = Bones[i + 1]->GetActorLocation() - Current->GetActorLocation();
			BonesLength[i] = FVector::Distance(Bones[i + 1]->GetActorLocation(), Current->GetActorLocation());
			CompleteLength += BonesLength[i];
		}

		Current = Current->GetAttachParentActor();
	}
}

void AFABRIKContainer::ResolveIK()
{
	if(Target == nullptr)
	{
		return;
	}

	if(BonesLength.Num() != ChainLength)
	{
		Init();
	}

	//Get Position
	for (int i = 0; i < Bones.Num(); ++i)
	{
		Positions[i] = Bones[i]->GetActorLocation();
	}

	FRotator RootRotation = Bones[0]->GetAttachParentActor() != nullptr ? Bones[0]->GetAttachParentActor()->GetActorRotation() : FRotator(0, 0, 0);
	auto RootRotationDifference = UKismetMathLibrary::NormalizedDeltaRotator(RootRotation, StartRotationRoot);

	//Computation
	double Distance = FVector::Distance(Target->GetActorLocation(), Bones[0]->GetActorLocation()) ;
	if(Distance >= CompleteLength) 
	{
		//Stretch it
		FVector Direction = (Target->GetActorLocation() - Positions[0]).GetSafeNormal();

		for (int i = 1; i < Positions.Num(); i++)
		{
			Positions[i] = Positions[i - 1] + Direction * BonesLength[i - 1];
		}
	}
	else
	{
		for (int iterations = 0; iterations < Iterations ; iterations++)
		{
			//Back
			for (int i = Positions.Num() - 1; i > 0; i--)
			{
				if(i == Positions.Num() - 1)
				{
					Positions[i] = Target->GetActorLocation();
				}
				else
				{
					Positions[i] = Positions[i + 1] + (Positions[i] - Positions[i + 1]).GetSafeNormal() * BonesLength[i]; //Set in line on distance
				}
			}

			//Forward
			for (int i = 1; i < Positions.Num(); ++i)
			{
				Positions[i] = Positions[i - 1] + (Positions[i] - Positions[i - 1]).GetSafeNormal() * BonesLength[i - 1]; //Set in line on distance
			}
			
			//Close enough?
			if(FVector::Distance(Positions[Positions.Num() - 1], Target->GetActorLocation()) < Delta)
			{
				break;
			}
		}
	}

	//Set Position and rotation
	for (int i = 0; i < Positions.Num(); ++i)
	{
		FQuat Rotation;
		if(i == Positions.Num() - 1)
		{
			//Last Bone
			Rotation = Target->GetActorRotation().Quaternion() * StartRotationTarget.Quaternion().Inverse() * StartRotationBone[i].Quaternion();
			
			if(Constraints.bUseRotationConstraints)
			{
				auto RelativeRotation = UKismetMathLibrary::InverseTransformRotation(Bones[i - 1]->GetActorTransform(), Bones[i]->GetActorRotation());
				UE_LOG(LogTemp, Warning, TEXT("Rotation: %s"), *RelativeRotation.ToString());
			}

			Bones[i]->SetActorRotation(Rotation);
		}
		else
		{
			//Every other bone
			Rotation = FQuat::FindBetweenVectors(StartDirectionSucc[i], Positions[i + 1] - Positions[i]) * StartRotationBone[i].Quaternion();
			Bones[i]->SetActorRotation(Rotation);
		}
		
		Bones[i]->SetActorLocation(Positions[i]);
	}
}

// Called when the game starts or when spawned
void AFABRIKContainer::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AFABRIKContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bActivateComputation)
	{
		DrawHelpers();
		ResolveIK();
	}
}

