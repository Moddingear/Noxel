#include "Tests/BruteForceSolver.h"

float& FInputVector::GetComponent(int i)
{
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
                                               const FOutputMatrix InState, int InCol, int InMaxIterations, TArray<float> InStart, TArray<float> InWidth)
{
	Sources = (InSources);
	State = (InState);
	Col = (InCol);
	MaxIterations = (InMaxIterations);
	Start = (InStart);
	Width = (InWidth);
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
	FOutputColumn& column = State.InputCoefficients[Col];
	column.InputCoefficients = Start;
	TArray<float> input;
	input.SetNumZeroed(State.InputCoefficients.Num());
	input[Col] = 1;
	for (i = 0; (i < MaxIterations) && (!StopFlag); ++i)
	{
		TArray<float> gradient = UBruteForceSolver::ComputeGradient(Sources, input, State, Col, Width);
		
		int bestGradient = 0;
		for (int j = 1; j < Sources.Num(); ++j)
		{
			if (abs(gradient[j]) > abs(gradient[bestGradient]))
			{
				bestGradient = j;
			}
		}
		column.InputCoefficients[bestGradient] += Width[bestGradient] * (gradient[bestGradient] > 0 ? 1 : -1);
		Width[bestGradient]*=0.5;
		
		/*for (int j = 0; j < Sources.Num(); ++j)
		{
			column.InputCoefficients[j] += Width[j] * (gradient[j] > 0 ? 1 : -1) * (abs(gradient[j]) > SMALL_NUMBER ? 1 : 0);
			Width[j]*=0.5;
		}*/
	}
	done = !StopFlag;
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
	return (float)i/MaxIterations;
}

bool FGradientDescentRunner::IsDone() const
{
	return done;
}

FOutputColumn FGradientDescentRunner::GetOutput() const
{
	return State.InputCoefficients[Col];
}

int FGradientDescentRunner::GetColumn() const
{
	return Col;
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

FInputVector UBruteForceSolver::GetOutputVector(TArray<FForceSource>& sources, TArray<float>& input,
                                                FOutputMatrix& state, bool WithSaturation)
{
	TArray<float> outputs = state.GetOutputValues(input);
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
	float wanted =0;
	float unwanted =0;
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
	
	return wanted / (1+unwanted);
}

TArray<float> UBruteForceSolver::ComputeGradient(TArray<FForceSource>& sources, TArray<float>& input,
                                                 FOutputMatrix& state, int col, TArray<float> epsilon)
{
	int numsources = sources.Num();
	TArray<float> gradient;
	gradient.SetNumZeroed(numsources);
	FOutputColumn& column = state.InputCoefficients[col];
	const FInputVector output = GetOutputVector(sources, input, state, false);
	FInputVector wanted = state.GetInputVector(input).GetUnsafeNormal();
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

void UBruteForceSolver::StartSolveInputs(TArray<FForceSource>& sources, TArray<FInputVector> InputsToOptimise, int MaxIterations)
{
	FOutputMatrix matrix;
	int NumSources = sources.Num();
	int NumCols = InputsToOptimise.Num();
	matrix.InputCoefficients.SetNumZeroed(NumCols);
	for (int i = 0; i < NumCols; ++i)
	{
		matrix.InputCoefficients[i].InputCoefficients.SetNumZeroed(NumSources);
		matrix.InputCoefficients[i].OptimisedDirection = InputsToOptimise[i];
	}
	TArray<float> Starts, Widths;
	Starts.SetNumZeroed(NumSources); Widths.SetNumZeroed(NumSources);
	for (int i = 0; i < NumCols; ++i)
	{
		for (int j = 0; j < NumSources; ++j)
		{
			float min = sources[j].RangeMin, max = sources[j].RangeMax;
			float extent = max - min;
			Starts[j] = min + extent*0.5;
			Widths[j] = extent*0.25;
		}
		FGradientDescentRunner* Runner = new FGradientDescentRunner(sources, matrix, i, MaxIterations * NumSources, Starts, Widths);
		Runners.Add(Runner);
	}
}

TArray<float> UBruteForceSolver::GetRunnerProgress()
{
	TArray<float> results;
	for (FGradientDescentRunner* runner : Runners)
	{
		results.Add(runner->GetProgress());
	}
	return results;
}

bool UBruteForceSolver::IsDone()
{
	for (FGradientDescentRunner* runner : Runners)
	{
		if (!runner->IsDone())
		{
			return false;
		}
	}
	return true;
}

FOutputMatrix UBruteForceSolver::GetOutput()
{
	FOutputMatrix matrix;
	matrix.InputCoefficients.SetNumZeroed(Runners.Num());
	
	for (int i=0; i<Runners.Num(); ++i)
	{
		matrix.InputCoefficients[Runners[i]->GetColumn()] = Runners[i]->GetOutput();
	}
	return matrix;
}
