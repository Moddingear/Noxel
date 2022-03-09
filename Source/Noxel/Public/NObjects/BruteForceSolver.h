#pragma once
#include "CoreMinimal.h"

#include "BruteForceSolver.generated.h"



USTRUCT(BlueprintType)
struct FInputVector
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector Translation;
	UPROPERTY(BlueprintReadWrite)
	FVector Rotation;

	FInputVector()
		:Translation(FVector::ZeroVector),
		Rotation(FVector::ZeroVector)
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

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct FForceSource
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FInputVector ForceAndTorque;

	UPROPERTY(BlueprintReadWrite)
	float RangeMin;
	UPROPERTY(BlueprintReadWrite)
	float RangeMax;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct FOutputColumn
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FInputVector OptimisedDirection;
	UPROPERTY(BlueprintReadWrite)
	TArray<float> InputCoefficients;

	TArray<float> GetOutputValues(float input) const;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct FOutputMatrix
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FOutputColumn> InputCoefficients; // An array of k by N coefficients

	TArray<float> GetOutputValues(TArray<float> input) const;

	FInputVector GetInputVector(TArray<float> input) const;

	FString ToString() const;
};


class FGradientDescentRunner : public FRunnable
{
	TArray<FForceSource> Sources;
	int MaxIterations;
	FInputVector Direction;
	
	FOutputColumn State;
	TArray<float> Width;

	int Iteration;
	bool done;

private:
	FRunnableThread* Thread;
	bool StopFlag;
public:
	
	FGradientDescentRunner(const TArray<FForceSource>& InSources, 
	const FInputVector InDirection, int InMaxIterations);

	~FGradientDescentRunner();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	float GetProgress() const;

	int GetIteration() const;

	bool IsDone() const;

	FOutputColumn GetOutput() const;
};

UCLASS(BlueprintType)
class UBruteForceSolver : public UObject
{
	GENERATED_BODY()
	
private:
	TArray<FGradientDescentRunner*> Runners;
	
public:

	~UBruteForceSolver();

	UFUNCTION(BlueprintCallable)
	static TArray<FForceSource> MakeTestCube(FVector extents);

	UFUNCTION(BlueprintCallable)
	static FInputVector GetOutputVector(UPARAM(ref) TArray<FForceSource>& sources, UPARAM(ref) FOutputColumn& state, bool WithSaturation);

	//Returns a score, where higher is best (not linear AT ALL)
	UFUNCTION()
	static float GetScore(UPARAM(ref) FInputVector& input, UPARAM(ref) FInputVector& output);

	//Returns an array of score gradients for each source axis
	UFUNCTION()
	static TArray<float> ComputeGradient(UPARAM(ref) TArray<FForceSource>& sources,
	                                     UPARAM(ref) FOutputColumn& state, TArray<float> epsilon);

	UFUNCTION(BlueprintCallable)
	static TArray<float> Desaturate(UPARAM(ref) TArray<FForceSource>& sources, TArray<float> inputs);

	UFUNCTION(BlueprintCallable)
	int StartSolveInputs(UPARAM(ref) TArray<FForceSource>& sources, FInputVector InputToOptimise, int MaxIterations);

	int GetRunnerIteration(int RunnerIdx);
	
	UFUNCTION(BlueprintCallable)
	float GetRunnerProgress(int RunnerIdx);

    UFUNCTION(BlueprintCallable)
	bool IsDone(int RunnerIdx);

	UFUNCTION(BlueprintCallable)
	FOutputColumn GetOutput(int RunnerIdx);
	
};
