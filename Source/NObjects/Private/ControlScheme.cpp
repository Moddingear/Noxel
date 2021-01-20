//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "ControlScheme.h"
#include "NObjects.h"

struct AngleWithReference
{
	float Angle;
	int32 Index;
	AngleWithReference()
	{}
	AngleWithReference(float AngleIn, int32 IndexIn)
		:Angle(AngleIn),
		Index(IndexIn)
	{}

	FORCEINLINE bool operator< (const AngleWithReference& other) const
	{
		return Angle < other.Angle;
	}
};

UControlScheme::UControlScheme()
{
}

UControlScheme::~UControlScheme()
{
}
AActor * UControlScheme::getOwnerActor()
{
	return (AActor*)GetOuter();
}
void UControlScheme::SetInputValues(FVector InputTranslation, FVector InputRotation)
{
	Translation = InputTranslation;
	Rotation = InputRotation;
}

void UControlScheme::SetForces(TArray<FTorsor> Forces)
{
	inputForces = Forces;
}

void UControlScheme::SetCOMAndMass(FVector InCOM, float InMass)
{
	COM = InCOM;
	Mass = InMass;
}

TArray<float> UControlScheme::Solve()
{
	return TArray<float>();
}

//Returns the angle occupied by the force relative to others, in radians. Up is not considered.
TArray<float> UControlScheme::VoronoiAngleFill(TArray<FVector>& ForcesLocation)
{
	int32 ArraySize = ForcesLocation.Num();
	TArray<AngleWithReference> Angles, Sizes;
	Angles.Reserve(ArraySize);
	Sizes.AddDefaulted(ArraySize);
	for (int32 i = 0; i < ArraySize; i++)
	{
		FVector ZRemoved = FVector(ForcesLocation[i].X, ForcesLocation[i].Y, 0);
		ZRemoved.Normalize();
		//UE_LOG(NObjects, Log, TEXT("ZRemoved %i is %s"), i, *ZRemoved.ToString());
		Angles.Emplace(FMath::Atan2(ZRemoved.Y, ZRemoved.X), i); //in radians
	}
	Angles.Sort();
	for (int32 i = 0; i < ArraySize; i++)
	{

		float size = Angles[(i + 1) % ArraySize].Angle - Angles[i].Angle;
		//UE_LOG(NObjects, Log, TEXT("[UControlScheme::VoronoiAngleFill] Angle[%d] : %f; Size = %f"), i, Angles[i].Angle, size);
		if (size < 0)
		{
			size += 2 * PI;
		}
		Sizes[i].Angle += size / 2.f;
		Sizes[i].Index = Angles[i].Index;
		Sizes[(i + 1) % ArraySize].Angle += size / 2.f;
	}
	Sizes.Sort([](const AngleWithReference& A, const AngleWithReference& B) {
		return A.Index < B.Index;
	});
	TArray<float> Floats;
	Floats.Reserve(Sizes.Num());
	for (AngleWithReference angle : Sizes)
	{
		//UE_LOG(NObjects, Log, TEXT("[UControlScheme::VoronoiAngleFill] Angle[%d] : %f"), angle.Index, angle.Angle);
		Floats.Add(angle.Angle);
		if (FMath::Abs(angle.Angle) > 2 * PI)
		{
			Floats[Floats.Num() - 1] = PI/2;
		}
	}
	return Floats;
}

//Computes the best combination to get a sum of normalized forces on a unit line, yielding a moment
TArray<float> UControlScheme::GetSumOnDirection(FVector Direction, FVector Normal, TArray<FVector>& ForcesLocation)
{
	FVector tangent = Normal ^ Direction;
	float rightScalar = 0, leftScalar = 0, lefttan = 0, righttan = 0;
	int32 bestright = -1, bestleft = -1;
	for (int32 i = 0; i < ForcesLocation.Num(); i++) //find a left and right pair
	{
		float scalar = Direction | ForcesLocation[i];
		float tansc = tangent | ForcesLocation[i];
		bool right = (tansc) > 0;
		if (right)
		{
			if (scalar > rightScalar)
			{
				bestright = i;
				rightScalar = scalar;
				righttan = tansc;
			}
		}
		else
		{
			if (scalar > leftScalar)
			{
				bestleft = i;
				leftScalar = scalar;
				lefttan = tansc;
			}
		}
	}
	if (bestleft == -1 || bestright == -1)
	{
		return TArray<float>();
	}
	TArray<float> retArray;
	retArray.Init(0.0f, ForcesLocation.Num());
	retArray[bestright] = 1 / righttan;
	retArray[bestleft] = 1 / lefttan;
	NormalizeArray(retArray);
	return retArray;
}

TArray<float>* UControlScheme::NormalizeArray(TArray<float>& Input)
{
	float Sum = 0;
	for (float value : Input)
	{
		Sum += value;
	}
	for (int32 i = 0; i < Input.Num(); i++)
	{
		Input[i] /= Sum;
	}
	return &Input;
}

bool UControlScheme::CheckSaturation(TArray<FTorsor>& Torsors, TArray<float>& Floats, TArray<bool>& Saturated)
{
	bool modified = false;
	int32 ArraySize = Torsors.Num();
	Floats.SetNum(ArraySize);
	Saturated.SetNum(ArraySize);
	for (int i = 0; i < Torsors.Num(); i++)
	{
		if (Torsors[i].IsSaturatedWith(Floats[i]) != Saturated[i])
		{
			modified = true;
			Saturated[i] = !Saturated[i];
		}
		Floats[i] = Torsors[i].ClampToSaturation(Floats[i]);
	}
	return modified;
}
