#pragma once
#include "CoreMinimal.h"
#include "BruteForceSolver.generated.h"

UENUM()
enum EGradientDescentMethod
{
	Binomial,
	FixedEpsilon,
	Exhaustive
};

USTRUCT(BlueprintType)
struct NOXEL_API FTRVector
{
	GENERATED_BODY()

	static const FTRVector ZeroVector;
	static const FTRVector OneVector;
	
	UPROPERTY(BlueprintReadWrite)
	FVector Translation;
	UPROPERTY(BlueprintReadWrite)
	FVector Rotation;

	FTRVector()
		:Translation(FVector::ZeroVector),
		Rotation(FVector::ZeroVector)
	{}

	FTRVector(FVector InTranslation, FVector InRotation)
		:Translation(InTranslation), Rotation(InRotation)
	{}

	FTRVector(float tx, float ty, float tz, float rx, float ry, float rz)
		:Translation(tx, ty, tz), Rotation( rx, ry, rz)
	{}

	float& GetComponent(int i);

	FORCEINLINE FTRVector operator+(const FTRVector& V) const;

	FORCEINLINE FTRVector operator-(const FTRVector& V) const;
	
	FORCEINLINE FTRVector operator*(const float& x) const;

	FORCEINLINE FTRVector operator*(const FTRVector& x) const;

	FORCEINLINE FTRVector operator/(const FTRVector& x) const;
	
	FORCEINLINE FTRVector operator+=(const FTRVector& V);

	float GetSizeSquared();
	
	float GetSize();
	
	FTRVector GetUnsafeNormal();
	
	static float Distance(FTRVector A, FTRVector B);

	float Sum() const;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct NOXEL_API FForceSource
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FTRVector ForceAndTorque;

	UPROPERTY(BlueprintReadWrite)
	float RangeMin;
	UPROPERTY(BlueprintReadWrite)
	float RangeMax;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct NOXEL_API FOutputColumn
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FTRVector OptimisedDirection;
	UPROPERTY(BlueprintReadWrite)
	TArray<float> InputCoefficients;

	TArray<float> GetOutputValues(float input) const;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct NOXEL_API FOutputMatrix
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FOutputColumn> InputCoefficients; // An array of k by N coefficients

	TArray<float> GetOutputValues(TArray<float> input) const;

	FTRVector GetInputVector(TArray<float> input) const;

	FString ToString() const;
};

class NOXEL_API FBruteForceRunnerBase : public FRunnable
{
	
public:
	virtual float GetProgress() const = 0;
	virtual int GetIteration() const = 0;
	virtual bool IsDone() const = 0;
	virtual FOutputColumn GetOutput() const = 0;
};

class NOXEL_API FExhaustiveRunner : public FBruteForceRunnerBase
{
	TArray<FForceSource> Sources;
	int Cuts;
	FTRVector Direction;
	
	FOutputColumn BestState;
	float BestScore;
	
	bool done;
	long i, top;

private:
	FRunnableThread* Thread;
	bool StopFlag;
public:
	
	FExhaustiveRunner(const TArray<FForceSource>& InSources, 
	const FTRVector InDirection, int InCuts);

	TArray<float> GetAlpha(int SliceIndex, int NumSlices);

	virtual ~FExhaustiveRunner() override;

	virtual uint32 Run() override;
	virtual void Stop() override;

	virtual float GetProgress() const override;

	virtual int GetIteration() const override;

	virtual bool IsDone() const override;

	virtual FOutputColumn GetOutput() const override;
};

class NOXEL_API FGradientDescentRunner : public FBruteForceRunnerBase
{
	TArray<FForceSource> Sources;
	int MaxIterations;
	FTRVector Direction;
	EGradientDescentMethod DescentMethod;
	
	FOutputColumn State;
	TArray<float> Width;

	int Iteration;
	bool done;

private:
	FRunnableThread* Thread;
	bool StopFlag;
public:
	
	FGradientDescentRunner(const TArray<FForceSource>& InSources, 
	const FTRVector InDirection, int InMaxIterations, EGradientDescentMethod InDescentMethod);

	virtual ~FGradientDescentRunner() override;

	virtual uint32 Run() override;
	virtual void Stop() override;

	virtual float GetProgress() const override;

	virtual int GetIteration() const override;

	virtual bool IsDone() const override;

	virtual FOutputColumn GetOutput() const override;
};

UCLASS(BlueprintType)
class NOXEL_API UBruteForceSolver : public UObject
{
	GENERATED_BODY()
	
private:
	TArray<FBruteForceRunnerBase*> Runners;
	
public:

	~UBruteForceSolver();

	UFUNCTION(BlueprintCallable)
	static TArray<FForceSource> MakeTestCube(FVector extents);

	UFUNCTION(BlueprintCallable)
	static FTRVector GetOutputVector(UPARAM(ref) TArray<FForceSource>& sources, UPARAM(ref) FOutputColumn& state, bool WithSaturation);

	//Returns a score, where higher is best (not linear AT ALL)
	UFUNCTION()
	static float GetScore(UPARAM(ref) FTRVector& input, UPARAM(ref) FTRVector& output);

	//Returns an array of score gradients for each source axis
	UFUNCTION()
	static TArray<float> ComputeGradient(UPARAM(ref) TArray<FForceSource>& sources,
	                                     UPARAM(ref) FOutputColumn& state, TArray<float> epsilon);

	UFUNCTION(BlueprintCallable)
	static TArray<float> Desaturate(UPARAM(ref) TArray<FForceSource>& sources, TArray<float> inputs);

	UFUNCTION(BlueprintCallable)
	int StartSolveInputs(UPARAM(ref) TArray<FForceSource>& sources, FTRVector InputToOptimise, int MaxIterations);

	int GetNumRunners() const;
	
	int GetRunnerIteration(int RunnerIdx);
	
	UFUNCTION(BlueprintCallable)
	float GetRunnerProgress(int RunnerIdx);

    UFUNCTION(BlueprintCallable)
	bool IsDone(int RunnerIdx);

	UFUNCTION(BlueprintCallable)
	FOutputColumn GetOutput(int RunnerIdx);

	UFUNCTION(BlueprintCallable)
	void ClearRunners();
	
};
