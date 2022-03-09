#include "NObjects/BruteForceSolver.h"

float& FInputVector::GetComponent(int i)
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

FInputVector FInputVector::operator+(const FInputVector& V) const
{
	return FInputVector(Translation + V.Translation, Rotation + V.Rotation);
}

FInputVector FInputVector::operator-(const FInputVector& V) const
{
	return FInputVector(Translation - V.Translation, Rotation - V.Rotation);
}

FORCEINLINE FInputVector FInputVector::operator*(const float& x) const
{
	return FInputVector(Translation * x, Rotation * x);
}

FORCEINLINE FInputVector FInputVector::operator+=(const FInputVector& V)
{
	Translation += V.Translation;
	Rotation += V.Rotation;
	return *this;
}

float FInputVector::GetSizeSquared()
{
	return Translation.SizeSquared() + Rotation.SizeSquared();
}

float FInputVector::GetSize()
{
	return FMath::Sqrt(GetSizeSquared());
}

FInputVector FInputVector::GetUnsafeNormal()
{
	return *this * FMath::InvSqrt(GetSizeSquared());
}

float FInputVector::Distance(FInputVector A, FInputVector B)
{
	FInputVector C = A-B;
	float SizeSquared = C.Translation.SizeSquared() + C.Rotation.SizeSquared();
	return FMath::Sqrt(SizeSquared);
}

FString FInputVector::ToString() const
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

FInputVector FOutputMatrix::GetInputVector(TArray<float> input) const
{
	checkf(input.Num() == InputCoefficients.Num(), TEXT("[FOutputMatrix::GetInputVector] Input vector isn't the same size as the array of optimised directions !"));
	FInputVector acc;
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

FGradientDescentRunner::FGradientDescentRunner(const TArray<FForceSource>& InSources, 
                                               const FInputVector InDirection, int InMaxIterations)
{
	Sources = (InSources);
	Direction = (InDirection);
	MaxIterations = (InMaxIterations);
	done = (false);
	StopFlag = (false);
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

bool FGradientDescentRunner::Init()
{
	return FRunnable::Init();
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
		State.InputCoefficients[j] = min + extent*0.5;
		Width[j] = extent*0.25;
	}
	
	for (Iteration = 0; (Iteration < MaxIterations) && (!StopFlag); ++Iteration)
	{
		TArray<float> gradient = UBruteForceSolver::ComputeGradient(Sources, State, Width);
		
		int bestGradient = 0;
		for (int j = 1; j < Sources.Num(); ++j)
		{
			if (abs(gradient[j]) > abs(gradient[bestGradient]))
			{
				bestGradient = j;
			}
		}
		State.InputCoefficients[bestGradient] += Width[bestGradient] * (gradient[bestGradient] > 0 ? 1 : -1);
		Width[bestGradient]*=0.5;
		
		/*for (int j = 0; j < Sources.Num(); ++j)
		{
			column.InputCoefficients[j] += Width[j] * (gradient[j] > 0 ? 1 : -1) * (abs(gradient[j]) > SMALL_NUMBER ? 1 : 0);
			Width[j]*=0.5;
		}*/
	}
	done = true;
	return 0;
}

void FGradientDescentRunner::Stop()
{
	StopFlag = true;
	FRunnable::Stop();
}

void FGradientDescentRunner::Exit()
{
	FRunnable::Exit();
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
	for (int i = 0; i < Runners.Num(); ++i)
	{
		delete Runners[i];
	}
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

FInputVector UBruteForceSolver::GetOutputVector(TArray<FForceSource>& sources, 
                                                FOutputColumn& state, bool WithSaturation)
{
	TArray<float>& outputs = state.InputCoefficients;
	FInputVector OutputForces;
	for (int i = 0; i < FMath::Min(outputs.Num(), sources.Num()); ++i)
	{
		float drive = WithSaturation ? FMath::Clamp(outputs[i], sources[i].RangeMin, sources[i].RangeMax) : outputs[i];
		OutputForces += sources[i].ForceAndTorque*drive;
	}
	return OutputForces;
}

float UBruteForceSolver::GetScore(FInputVector& input, FInputVector& output)
{
	return 1/(1+FInputVector::Distance(input, output));
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
	int numsources = sources.Num();
	TArray<float> gradient;
	gradient.SetNumZeroed(numsources);
	FOutputColumn& column = state;
	const FInputVector output = GetOutputVector(sources, state, false);
	FInputVector wanted = state.OptimisedDirection;
	for (int i = 0; i < numsources; ++i)
	{
		float center = column.InputCoefficients[i];
		FInputVector sourceval = sources[i].ForceAndTorque;
		FInputVector outputzero = output-sourceval*center;
		FInputVector outputmin = outputzero + sourceval*(center - epsilon[i]);
		FInputVector outputmax = outputzero + sourceval*(center + epsilon[i]);
		
		const float scoremin = GetScore(wanted, outputmin);
		const float scoremax = GetScore(wanted, outputmax);
		gradient[i] = (scoremax-scoremin);
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

int UBruteForceSolver::StartSolveInputs(TArray<FForceSource>& sources, FInputVector InputToOptimise, int MaxIterations)
{
	const int NumSources = sources.Num();
	
	FGradientDescentRunner* Runner = new FGradientDescentRunner(sources, InputToOptimise, MaxIterations * NumSources);
	return Runners.Add(Runner);
	
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
