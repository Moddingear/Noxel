#include "BruteForceSolver.h"

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

TArray<float> FOutputColumn::GetOutputValues(float input) const
{
	TArray<float> mul = InputCoefficients;
	for (int i = 0; i < InputCoefficients.Num(); ++i)
	{
		mul[i] *= input;
	}
	return mul;
}

TArray<float> FOutputMatrix::GetOutputValues(FInputVector input) const
{
	TArray<float> sum;
	for (int i = 0; i < 6; ++i)
	{
		if (abs(input.GetComponent(i)) < SMALL_NUMBER)
		{
			continue;
		}
		TArray<float> curr = InputCoefficients[i].GetOutputValues(input.GetComponent(i));
		if (sum.Num() < curr.Num())
		{
			sum.SetNumZeroed(curr.Num(), false);
		}
		for (int j = 0; j < curr.Num(); ++j)
		{
			sum[i]+=curr[i];
		}
	}
	return sum;
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

FInputVector UBruteForceSolver::GetOutputVector(TArray<FForceSource>& sources, FInputVector& input,
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
	FInputVector A = input.GetUnsafeNormal(), B = output.GetUnsafeNormal();
	return FInputVector::Distance(A, B);
}

TArray<float> UBruteForceSolver::ComputeGradient(TArray<FForceSource>& sources, FInputVector& input,
                                                 FOutputMatrix& state, int col, TArray<float> epsilon)
{
	int numsources = sources.Num();
	TArray<float> gradient;
	gradient.SetNumZeroed(numsources);
	FOutputColumn& column = state.InputCoefficients[col];
	const FInputVector output = GetOutputVector(sources, input, state, false);
	for (int i = 0; i < numsources; ++i)
	{
		float center = column.InputCoefficients[i];
		FInputVector sourceval = sources[i].ForceAndTorque;
		FInputVector outputzero = output-sourceval*center;
		FInputVector outputmin = outputzero + sourceval*(center - epsilon[i]);
		FInputVector outputmax = outputzero + sourceval*(center + epsilon[i]);
		const float scoremin = GetScore(input, outputmin);
		const float scoremax = GetScore(input, outputmax);
		gradient[i] = (scoremax-scoremin)/epsilon[i];
	}
	return gradient;
}

TArray<float> UBruteForceSolver::GradientDescent(TArray<FForceSource>& sources, FInputVector input, FOutputMatrix state,
                                        int col, int iterations, TArray<float> start, TArray<float> width)
{
	FOutputColumn& column = state.InputCoefficients[col];
	column.InputCoefficients = start;
	for (int i = 0; i < iterations; ++i)
	{
		TArray<float> gradient = ComputeGradient(sources, input, state, col, width);
		for (int j = 0; j < sources.Num(); ++j)
		{
			column.InputCoefficients[j] += width[j] * (gradient[j] > 0 ? -1 : 1);
			width[j]*=0.5;
		}
	}
	return column.InputCoefficients;
}
