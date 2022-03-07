// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Tests/BruteForceTester.h"

#include "Noxel.h"
#include "Tests/BruteForceSolver.h"

// Sets default values
ABruteForceTester::ABruteForceTester()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrintedResults = false;
	
}

// Called when the game starts or when spawned
void ABruteForceTester::BeginPlay()
{
	Super::BeginPlay();
	Sources = UBruteForceSolver::MakeTestCube(FVector(80, 100, 120));
	Solver = NewObject<UBruteForceSolver>();
	TArray<FInputVector> Inputs;
	for (int i = 0; i < 6; ++i)
	{
		FInputVector vec;
		vec.GetComponent(i) = 1;
		Inputs.Add(vec);
	}
	Solver->StartSolveInputs(Sources, Inputs, 10*1000);
}

// Called every frame
void ABruteForceTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(!PrintedResults && Solver->IsDone())
	{
		PrintedResults = true;
		
		FOutputMatrix out = Solver->GetOutput();
		UE_LOG(Noxel, Log, TEXT("Runners done! Matrix :\r\n%s"), *out.ToString())
		
		for (int i = 0; i < Sources.Num(); ++i)
        {
            UE_LOG(Noxel, Log, TEXT("Source %d : %s"), i, *Sources[i].ToString());
        }
		for (int i = 0; i < out.InputCoefficients.Num(); ++i)
		{
			TArray<float> input;
			input.SetNumZeroed(out.InputCoefficients.Num());
			input[i] = 1;
			FInputVector outVec = UBruteForceSolver::GetOutputVector(Sources, input, out, false);
			UE_LOG(Noxel, Log, TEXT("Test %d, result %s"), i, *outVec.ToString());
		}
	}
	if (!PrintedResults)
	{
		TArray<float> progress = Solver->GetRunnerProgress();
		FString prog = "";
		for (int i = 0; i < progress.Num(); ++i)
		{
			prog += FString::Printf(TEXT("Runner %d : %f% "), i, progress[i]*100);
		}
		UE_LOG(Noxel, Log, TEXT("Runners' progress : %s"), *prog);
	}
}

