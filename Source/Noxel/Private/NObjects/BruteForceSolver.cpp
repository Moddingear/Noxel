#include "NObjects/BruteForceSolver.h"

NOXEL_API const FTRVector FTRVector::ZeroVector(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
NOXEL_API const FTRVector FTRVector::OneVector(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

float& FTRVector::GetComponent(int i)
{
	check(i<6 && i>=0);
	if (i <3)
	{
		return Translation.Component(i);
	}
	else
	{
		return Rotation.Component(i-3);
	}
}

FTRVector FTRVector::operator+(const FTRVector& V) const
{
	return FTRVector(Translation + V.Translation, Rotation + V.Rotation);
}

FTRVector FTRVector::operator-(const FTRVector& V) const
{
	return FTRVector(Translation - V.Translation, Rotation - V.Rotation);
}

FORCEINLINE FTRVector FTRVector::operator*(const float& x) const
{
	return FTRVector(Translation * x, Rotation * x);
}

FTRVector FTRVector::operator*(const FTRVector& V) const
{
	return FTRVector(Translation * V.Translation, Rotation * V.Rotation);
}

FTRVector FTRVector::operator/(const FTRVector& V) const
{
	return FTRVector(Translation / V.Translation, Rotation / V.Rotation);
}

FORCEINLINE FTRVector FTRVector::operator+=(const FTRVector& V)
{
	Translation += V.Translation;
	Rotation += V.Rotation;
	return *this;
}

float FTRVector::GetSizeSquared()
{
	return Translation.SizeSquared() + Rotation.SizeSquared();
}

float FTRVector::GetSize()
{
	return FMath::Sqrt(GetSizeSquared());
}

FTRVector FTRVector::GetUnsafeNormal()
{
	return *this * FMath::InvSqrt(GetSizeSquared());
}

float FTRVector::Distance(FTRVector A, FTRVector B)
{
	FTRVector C = A-B;
	const float SizeSquared = C.Translation.SizeSquared() + C.Rotation.SizeSquared();
	return FMath::Sqrt(SizeSquared);
}

float FTRVector::Sum() const
{
	return Translation.X + Translation.Y + Translation.Z + Rotation.X + Rotation.Y + Rotation.Z;
}

FString FTRVector::ToString() const
{
	return TEXT("T:") + Translation.ToString() + TEXT(" / R:") + Rotation.ToString();
}

FString FForceSource::ToString() const
{
	return FString::Printf(TEXT("[%f:%f] "), RangeMin, RangeMax) + ForceAndTorque.ToString();
}

TArray<float> FOutputColumn::GetOutputValues(float input) const
{
	TArray<float> mul = InputCoefficients;
	for (int i = 0; i < InputCoefficients.Num(); ++i)
	{
		mul[i] *= input;
	}
	return mul;
}

FString FOutputColumn::ToString() const
{
	FString str = "";
	for (int i = 0; i < InputCoefficients.Num(); ++i)
	{
		str += FString::Printf(TEXT("%3.3f "), InputCoefficients[i]);
	}
	return OptimisedDirection.ToString() + TEXT(" fac:{") + str + TEXT("}");
}

TArray<float> FOutputMatrix::GetOutputValues(TArray<float> input) const
{
	TArray<float> sum;
	sum.SetNumZeroed(InputCoefficients[0].InputCoefficients.Num());
	checkf(input.Num() == InputCoefficients.Num(), TEXT("[FOutputMatrix::GetOutputValues] Input vector isn't the same size as the amount of columns in the matrix !"));
	for (int i = 0; i < input.Num(); ++i)
	{
		if (abs(input[i]) < SMALL_NUMBER)
		{
			continue;
		}
		TArray<float> curr = InputCoefficients[i].GetOutputValues(input[i]);
		for (int j = 0; j < curr.Num(); ++j)
		{
			sum[j]+=curr[j];
		}
	}
	return sum;
}

FTRVector FOutputMatrix::GetInputVector(TArray<float> input) const
{
	checkf(input.Num() == InputCoefficients.Num(), TEXT("[FOutputMatrix::GetInputVector] Input vector isn't the same size as the array of optimised directions !"));
	FTRVector acc;
	for (int i = 0; i < input.Num(); ++i)
	{
		if (abs(input[i]) < SMALL_NUMBER)
		{
			continue;
		}
		acc += InputCoefficients[i].OptimisedDirection * input[i];
	}
	return acc;
}

FString FOutputMatrix::ToString() const
{
	FString str = "";
	for (int i = 0; i < InputCoefficients.Num(); ++i)
	{
		str += FString::Printf(TEXT("%d "), i) + InputCoefficients[i].ToString() + TEXT("\r\n");
	}
	return str;
}

FExhaustiveRunner::FExhaustiveRunner(const TArray<FForceSource>& InSources, const FTRVector InDirection, int InCuts)
{
	Sources = InSources;
	Direction = InDirection;
	Cuts = InCuts;
	
	done = false;
	StopFlag = false;
	Thread = FRunnableThread::Create(this, TEXT("Exhaustive Runner Thread"));
}

TArray<float> FExhaustiveRunner::GetAlpha(int SliceIndex, int NumSlices)
{
	int NumSources = Sources.Num();
	TArray<float> alphas;
	alphas.SetNumZeroed(NumSources);
	int Slice = SliceIndex;
	for (int j = 0; j < NumSources; ++j)
	{
		int progress = Slice % NumSlices;
		Slice = Slice / NumSlices;
		alphas[j] = (float)progress / NumSlices;
	}
	return alphas;
}

FExhaustiveRunner::~FExhaustiveRunner()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

uint32 FExhaustiveRunner::Run()
{
	int NumSources = Sources.Num();
	BestState.OptimisedDirection = Direction;
	BestState.InputCoefficients.SetNumZeroed(NumSources);
	TArray<float> SliceExtents, SliceStarts;
	SliceExtents.SetNumZeroed(NumSources);
	SliceStarts.SetNumZeroed(NumSources);
	
	top = 1;
	for (int j = 0; j < NumSources; ++j)
	{
		float min = Sources[j].RangeMin, max = Sources[j].RangeMax;
		float extent = max - min;
		SliceStarts[j] = min;
		SliceExtents[j] = extent;
		top *= Cuts;
	}

	for (i = 0; i < top && !StopFlag; ++i)
	{
		TArray<float> alphas = GetAlpha(i, Cuts);
		for (int j = 0; j < NumSources; ++j)
		{
			alphas[j] = SliceStarts[j] + SliceExtents[j] * alphas[j];
		}
		FOutputColumn State;
		State.InputCoefficients = alphas;
		FTRVector Output = UBruteForceSolver::GetOutputVector(Sources, State, false);
		float Score = UBruteForceSolver::GetScore(Direction, Output);
		if (Score > BestScore)
		{
			BestState.InputCoefficients = alphas;
			BestScore = Score;
		}
	}

	done = true;
	return 0;
}

void FExhaustiveRunner::Stop()
{
	StopFlag = true;
	FRunnable::Stop();
}

float FExhaustiveRunner::GetProgress() const
{
	return (float)i/top;
}

int FExhaustiveRunner::GetIteration() const
{
	return i;
}

bool FExhaustiveRunner::IsDone() const
{
	return done;
}

FOutputColumn FExhaustiveRunner::GetOutput() const
{
	return BestState;
}

FGradientDescentRunner::FGradientDescentRunner(const TArray<FForceSource>& InSources, 
                                               const FTRVector InDirection, int InMaxIterations, EGradientDescentMethod InDescentMethod)
{
	Sources = InSources;
	Direction = InDirection;
	MaxIterations = InMaxIterations;
	DescentMethod = InDescentMethod;
	
	done = false;
	StopFlag = false;
	Thread = FRunnableThread::Create(this, TEXT("Gradient Descent Runner Thread"));
}

FGradientDescentRunner::~FGradientDescentRunner()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

uint32 FGradientDescentRunner::Run()
{
	int NumSources = Sources.Num();
	State.OptimisedDirection = Direction;
	State.InputCoefficients.SetNumZeroed(NumSources); Width.SetNumZeroed(NumSources);
	
	for (int j = 0; j < NumSources; ++j)
	{
		float min = Sources[j].RangeMin, max = Sources[j].RangeMax;
		float extent = max - min;
		
		switch (DescentMethod)
		{
		case Binomial:
			State.InputCoefficients[j] = min + extent*0.5;
			Width[j] = extent*0.25;
			break;
		case FixedEpsilon:
			State.InputCoefficients[j] = FMath::Clamp(0.f, min, max);
			Width[j] = extent/MaxIterations/NumSources;
			break;
		default:
			break;
		}
	}
	
	for (Iteration = 0; !StopFlag; ++Iteration)
	{
		TArray<float> gradient = UBruteForceSolver::ComputeGradient(Sources, State, Width);
		
		int bestGradient = 0;
		for (int j = 1; j < Sources.Num(); ++j)
		{
			if (abs(gradient[j]) > abs(gradient[bestGradient]))
			{
				if (gradient[j] > abs(gradient[bestGradient]) && State.InputCoefficients[j] < Sources[j].RangeMax) //above
				{
					bestGradient = j;
				}
				if (-gradient[j] > abs(gradient[bestGradient]) && State.InputCoefficients[j] > Sources[j].RangeMin) //below
				{
					bestGradient = j;
				}
			}
		}
		State.InputCoefficients[bestGradient] += Width[bestGradient] * (gradient[bestGradient] > 0 ? 1 : -1);
		switch (DescentMethod)
		{
		case Binomial:
			Width[bestGradient]*=0.5;
			break;
		case FixedEpsilon:
			break;
		default:
			break;
		}
		
	}
	done = true;
	return 0;
}

void FGradientDescentRunner::Stop()
{
	StopFlag = true;
	FRunnable::Stop();
}

float FGradientDescentRunner::GetProgress() const
{
	return (float)Iteration/MaxIterations;
}

int FGradientDescentRunner::GetIteration() const
{
	return Iteration;
}

bool FGradientDescentRunner::IsDone() const
{
	return done;
}

FOutputColumn FGradientDescentRunner::GetOutput() const
{
	return State;
}

UBruteForceSolver::~UBruteForceSolver()
{
	ClearRunners();
}

TArray<FForceSource> UBruteForceSolver::MakeTestCube(FVector extents)
{
	TArray<FVector> vertices;
	for (int i = 0; i < 8; ++i)
	{
		FVector vp;
		vp.X = i & 1 ? extents.X : -extents.X;
		vp.Y = i & 2 ? extents.Y : -extents.Y;
		vp.Z = i & 4 ? extents.Z : -extents.Z;
		vertices.Add(vp);
	}
	struct edge
	{
		FVector position;
		FVector direction;
	};
	TArray<edge> edges;
	for (FVector Vertex : vertices)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (Vertex.Component(i) < 0)
			{
				edge e;
				FVector axis = FVector::ZeroVector;
				axis.Component(i) = 1;
				e.position = Vertex + extents.Component(i) * axis;
				e.direction = axis;
				edges.Add(e);
			}
		}
	}

	TArray<FForceSource> forces;
	for (edge e : edges)
	{
		FForceSource force;
		force.ForceAndTorque.Translation = e.direction;
		force.ForceAndTorque.Rotation = e.position ^ force.ForceAndTorque.Translation;
		force.RangeMin = -1;
		force.RangeMax = 1;
		forces.Add(force);
	}
	return forces;
}

FTRVector UBruteForceSolver::GetOutputVector(TArray<FForceSource>& sources, 
                                                FOutputColumn& state, bool WithSaturation)
{
	TArray<float>& outputs = state.InputCoefficients;
	FTRVector OutputForces;
	for (int i = 0; i < FMath::Min(outputs.Num(), sources.Num()); ++i)
	{
		float drive = WithSaturation ? FMath::Clamp(outputs[i], sources[i].RangeMin, sources[i].RangeMax) : outputs[i];
		OutputForces += sources[i].ForceAndTorque*drive;
	}
	return OutputForces;
}

float UBruteForceSolver::GetScore(FTRVector& input, FTRVector& output)
{
	return 1/(1+FTRVector::Distance(input, output));
	/*float wanted = 0;
	float unwanted = 1;
	for (int i = 0; i < 6; ++i)
	{
		if (abs(input.GetComponent(i)) > SMALL_NUMBER)
		{
			wanted += output.GetComponent(i);
		}
		else
		{
			unwanted += abs(output.GetComponent(i));
		}
	}
	float dotmul = 1;
	if (!input.Translation.IsNearlyZero() && !output.Translation.IsNearlyZero())
	{
		FVector outputWanted;
		for (int i = 0; i < 3; ++i)
		{
			if (abs(input.Translation.Component(i)) > SMALL_NUMBER)
			{
				outputWanted.Component(i) = output.Translation.Component(i);
			}
		}
		dotmul *= input.Translation.GetSafeNormal() | outputWanted.GetSafeNormal();
	}
	if (!input.Rotation.IsNearlyZero() && !output.Rotation.IsNearlyZero())
	{
		FVector outputWanted;
		for (int i = 0; i < 3; ++i)
		{
			if (abs(input.Rotation.Component(i)) > SMALL_NUMBER)
			{
				outputWanted.Component(i) = output.Rotation.Component(i);
			}
		}
		dotmul *= input.Rotation.GetSafeNormal() | outputWanted.GetSafeNormal();
	}
	
	return wanted / unwanted * dotmul;*/
}

TArray<float> UBruteForceSolver::ComputeGradient(TArray<FForceSource>& sources, 
                                                 FOutputColumn& state, TArray<float> epsilon)
{
	const int numsources = sources.Num();
	TArray<float> gradient;
	gradient.SetNumZeroed(numsources);
	const FTRVector Output = GetOutputVector(sources, state, false);
	FTRVector wanted = state.OptimisedDirection;
	for (int i = 0; i < numsources; ++i)
	{
		FTRVector sourceval = sources[i].ForceAndTorque;
		FTRVector outputmin = Output - sourceval*epsilon[i];
		FTRVector outputmax = Output + sourceval*epsilon[i];
		
		const float scoremin = GetScore(wanted, outputmin);
		const float scoremax = GetScore(wanted, outputmax);
		gradient[i] = scoremax-scoremin;
	}
	return gradient;
}

TArray<float> UBruteForceSolver::Desaturate(TArray<FForceSource>& sources, TArray<float> inputs)
{
	check(sources.Num() == inputs.Num());
	TArray<float> output;
	output.Reserve(sources.Num());
	float factor = 1;
	for (int i = 0; i < sources.Num(); ++i)
	{
		if (inputs[i] > sources[i].RangeMax*factor)
		{
			check(abs(sources[i].RangeMax) > SMALL_NUMBER);
			factor = inputs[i]/sources[i].RangeMax;
		}

		if (inputs[i] < sources[i].RangeMin*factor)
		{
			check(abs(sources[i].RangeMin) > SMALL_NUMBER);
			factor = inputs[i]/sources[i].RangeMin;
		}
	}
	for (int i = 0; i < sources.Num(); ++i)
	{
		output.Add(inputs[i]/factor);
	}
	return output;
}

int UBruteForceSolver::StartSolveInputs(TArray<FForceSource>& sources, FTRVector InputToOptimise, int MaxIterations)
{
	
	FGradientDescentRunner* Runner = new FGradientDescentRunner(sources, InputToOptimise, MaxIterations, EGradientDescentMethod::Binomial);
	//nbiter = cuts^numsources = exp(numsources * log(cuts))
	//log(nbiter) = numsources * log(cuts)
	//log(cuts) = log(nbiter)/numsources
	//cuts = nbiter / exp(numsoures)
	//cuts = nbiter * exp(-numsources)
	//const float cuts = MaxIterations * FMath::Exp(-sources.Num());
	//FExhaustiveRunner* Runner = new FExhaustiveRunner(sources, InputToOptimise, floor(cuts));
	return Runners.Add(Runner);
}

int UBruteForceSolver::GetNumRunners() const
{
	return Runners.Num();
}

int UBruteForceSolver::GetRunnerIteration(int RunnerIdx)
{
	check(Runners.IsValidIndex(RunnerIdx));
	return Runners[RunnerIdx]->GetIteration();
}

float UBruteForceSolver::GetRunnerProgress(int RunnerIdx)
{
	check(Runners.IsValidIndex(RunnerIdx));
	return Runners[RunnerIdx]->GetProgress();
}

bool UBruteForceSolver::IsDone(int RunnerIdx)
{
	check(Runners.IsValidIndex(RunnerIdx));
	return Runners[RunnerIdx]->IsDone();
}

FOutputColumn UBruteForceSolver::GetOutput(int RunnerIdx)
{
	check(Runners.IsValidIndex(RunnerIdx));
	Runners[RunnerIdx]->Stop();
	while (!Runners[RunnerIdx]->IsDone())
	{
		
	}
	return Runners[RunnerIdx]->GetOutput();
}

void UBruteForceSolver::ClearRunners()
{
	for (int i = Runners.Num() - 1; i >= 0; --i)
	{
		delete Runners[i];
	}
	Runners.Reset();
}
