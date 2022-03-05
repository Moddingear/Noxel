#pragma once
#include "CoreMinimal.h"

#include "BruteForceSolver.generated.h"



USTRUCT()
struct FInputVector
{
	GENERATED_BODY()
	FVector Translation;
	FVector Rotation;

	FInputVector()
		:Translation(),
		Rotation()
	{}

	FInputVector(FVector InTranslation, FVector InRotation)
		:Translation(InTranslation), Rotation(InRotation)
	{}

	float& GetComponent(int i);

	FORCEINLINE FInputVector operator+(const FInputVector& V) const;

	FORCEINLINE FInputVector operator-(const FInputVector& V) const;
	
	FORCEINLINE FInputVector operator*(const float& x) const;
	
	FORCEINLINE FInputVector operator+=(const FInputVector& V);

	float GetSizeSquared();
	
	float GetSize();
	
	FInputVector GetUnsafeNormal();
	
	static float Distance(FInputVector A, FInputVector B);
};

USTRUCT(BlueprintType)
struct FForceSource
{
	GENERATED_BODY()
	FInputVector ForceAndTorque;
	float RangeMin;
	float RangeMax;
};

USTRUCT()
struct FOutputColumn
{
	GENERATED_BODY()
	TArray<float> InputCoefficients;

	TArray<float> GetOutputValues(float input) const;
};

USTRUCT(BlueprintType)
struct FOutputMatrix
{
	GENERATED_BODY()
	TArray<FOutputColumn> InputCoefficients; // An array of 6 by N coefficients

	TArray<float> GetOutputValues(FInputVector input) const;
};



UCLASS()
class UBruteForceSolver : public UObject
{
	GENERATED_BODY()
public:

	UFUNCTION()
	static TArray<FForceSource> MakeTestCube(FVector extents);

	UFUNCTION()
	static FInputVector GetOutputVector(UPARAM(ref) TArray<FForceSource>& sources, UPARAM(ref) FInputVector& input, UPARAM(ref) FOutputMatrix& state, bool WithSaturation);

	//Returns a score between 0 and 2, where 0 is best
	UFUNCTION()
	static float GetScore(UPARAM(ref) FInputVector& input, UPARAM(ref) FInputVector& output);

	//Returns an array of score gradients for each source axis
	UFUNCTION()
	static TArray<float> ComputeGradient(UPARAM(ref) TArray<FForceSource>& sources, UPARAM(ref) FInputVector& input,
		UPARAM(ref) FOutputMatrix& state, int col, TArray<float> epsilon);

	//Tries to find the lowest score from a starting point for an input vector to exchange a column of the state
	UFUNCTION()
	static TArray<float> GradientDescent(UPARAM(ref) TArray<FForceSource>& sources, FInputVector input,
	                            FOutputMatrix state, int col, int iterations, TArray<float> start, TArray<float> width);
};
