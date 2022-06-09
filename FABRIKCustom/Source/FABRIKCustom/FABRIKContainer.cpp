// Fill out your copyright notice in the Description page of Project Settings.


#include "FABRIKContainer.h"
#include "DrawDebugHelpers.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

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

void AFABRIKContainer::ResolveIK(float DeltaTime)
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

	//Computation
	const double Distance = FVector::Distance(Target->GetActorLocation(), Bones[0]->GetActorLocation()) ;

	//Exceed distance of arm length
	if(Distance >= CompleteLength) 
	{
		//Stretch it
		const FVector Direction = (Target->GetActorLocation() - Positions[0]).GetSafeNormal();

		for (int i = 1; i < Positions.Num(); i++)
		{
			Positions[i] = Positions[i - 1] + Direction * BonesLength[i - 1];
		}

		//Compute Rail Movement Direction
		if(CameraRail)
		{
			FVector RailForward = CameraRail->GetRailSplineComponent()->GetLocationAtTime(CurrentPositionOnRail,  ESplineCoordinateSpace::World,true);
			FVector TargetVelocity = GetTargetVelocity(DeltaTime);

			double Dot = RailForward.Dot(TargetVelocity.GetSafeNormal());

			if(Dot > 0.8)
			{
				CurrentPositionOnRail += (TargetVelocity.GetSafeNormal().Length() / DeltaTime);

				if(CurrentPositionOnRail > 1)
				{
					CurrentPositionOnRail = 1;
				}

				CameraRail->CurrentPositionOnRail = FMath::FInterpTo<float, float, float>(CameraRail->CurrentPositionOnRail, CurrentPositionOnRail, DeltaTime, 0.1);
				
			}

			if(Dot < -0.8)
			{
				CurrentPositionOnRail -= (TargetVelocity.Length() / DeltaTime);
			
				if(CurrentPositionOnRail < 0)
				{
					CurrentPositionOnRail = 0;
				}

				//CameraRail->CurrentPositionOnRail = CurrentPositionOnRail;
				
				CameraRail->CurrentPositionOnRail = FMath::FInterpTo<float, float, float>(CameraRail->CurrentPositionOnRail, CurrentPositionOnRail, DeltaTime, 0.1);
			}
			
			UE_LOG(LogTemp, Warning, TEXT("Velocity %s, Dot: %f, CurrentPositionOnRail: %f"), *TargetVelocity.ToString(), Dot, CurrentPositionOnRail);

			if(GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Velocity %s, Dot: %f"), *TargetVelocity.ToString(), Dot));
			}
                
		}
	}
	else
	{
		//Within distance of arm length
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
		// FQuat Rotation;
		//
		// if(i == Positions.Num() - 1)
		// {
		// 	//Last Bone
		// 	Rotation = Target->GetActorRotation().Quaternion() * StartRotationTarget.Quaternion().Inverse() * StartRotationBone[i].Quaternion();
		// 	
		// 	if(Constraints.bUseRotationConstraints)
		// 	{
		// 		FRotator ClampedRotation = GetClampedBoneRotation(Bones[i - 1], Rotation.Rotator(), i);
		// 		Bones[i]->SetActorRotation(ClampedRotation);
		// 	}
		// 	else
		// 	{
		// 		Bones[i]->SetActorRotation(Rotation);
		// 	}
		// 	
		// }
		// else
		// {
		// 	//Every other bone
		// 	Rotation = FQuat::FindBetweenVectors(StartDirectionSucc[i], Positions[i + 1] - Positions[i]) * StartRotationBone[i].Quaternion();
		//
		// 	if(Constraints.bUseRotationConstraints &&
		// 	   Bones.IsValidIndex(i-1))
		// 	{
		// 		FRotator ClampedRotation = GetClampedBoneRotation(Bones[i - 1], Rotation.Rotator(), i);
		// 		Bones[i]->SetActorRotation(ClampedRotation);
		// 	}
		// 	else
		// 	{
		// 		Bones[i]->SetActorRotation(Rotation);	
		// 	}
		// }
		
		Bones[i]->SetActorLocation(Positions[i]);
	}
}

FRotator AFABRIKContainer::GetClampedBoneRotation(const AActor* Parent, FRotator UnclampedRotator, int32 BoneIdx)
{
	if(Parent != nullptr)
	{
		//auto CurrentRelativeRotation = UKismetMathLibrary::InverseTransformRotation(Bones[i - 1]->GetActorTransform(), Bones[i]->GetActorRotation());
		auto CurrentRelativeRotation = UKismetMathLibrary::InverseTransformRotation(Parent->GetActorTransform(), UnclampedRotator);
		//UE_LOG(LogTemp, Warning, TEXT("Local Rotation: %s"), *CurrentRelativeRotation.ToString());

		CurrentRelativeRotation.Pitch = FMath::ClampAngle(CurrentRelativeRotation.Pitch, -Constraints.RotationConstraints[BoneIdx].Pitch, Constraints.RotationConstraints[BoneIdx].Pitch);
		CurrentRelativeRotation.Yaw = FMath::ClampAngle(CurrentRelativeRotation.Yaw, -Constraints.RotationConstraints[BoneIdx].Yaw, Constraints.RotationConstraints[BoneIdx].Yaw);
		CurrentRelativeRotation.Roll = FMath::ClampAngle(CurrentRelativeRotation.Roll, -Constraints.RotationConstraints[BoneIdx].Roll, Constraints.RotationConstraints[BoneIdx].Roll);

		const FRotator ClampedRotation = UKismetMathLibrary::TransformRotation(Parent->GetActorTransform(), CurrentRelativeRotation);
		//UE_LOG(LogTemp, Warning, TEXT("Global Rotation: %s"), *ClampedRotation.ToString());

		return ClampedRotation;
	}

	return FRotator();
}

FVector AFABRIKContainer::GetTargetVelocity(float DeltaTime)
{
	if(Target)
	{
		auto CurrentLocation = Target->GetActorLocation();
		const FVector TargetVelocity = (CurrentLocation - PreviousTargetLocation) / DeltaTime;
		PreviousTargetLocation = Target->GetActorLocation();

		return TargetVelocity;
	}

	return FVector::ZeroVector;
}

// Called when the game starts or when spawned
void AFABRIKContainer::BeginPlay()
{
	Super::BeginPlay();

	if(Target)
		PreviousTargetLocation = Target->GetActorLocation();
}

// Called every frame
void AFABRIKContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bActivateComputation)
	{
		DrawHelpers();
		ResolveIK(DeltaTime);
	}
}

