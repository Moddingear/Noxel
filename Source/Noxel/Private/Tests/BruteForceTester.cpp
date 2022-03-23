// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Tests/BruteForceTester.h"

#include "Noxel.h"
#include "NObjects/BruteForceSolver.h"

// Sets default values
ABruteForceTester::ABruteForceTester()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void ABruteForceTester::BeginPlay()
{
	Super::BeginPlay();
	Sources = UBruteForceSolver::MakeTestCube(FVector(80, 100, 120));
	Solver = NewObject<UBruteForceSolver>();
	for (int i = 0; i < 6; ++i)
	{
		FTRVector vec;
		vec.GetComponent(i) = i>=3 ? 100 : 1;
		PrintedResults.Add(false);
		Solver->StartSolveInputs(Sources, vec, 10*1000);
	}
	for (int i = 0; i < Sources.Num(); ++i)
	{
		UE_LOG(Noxel, Log, TEXT("Source %d : %s"), i, *Sources[i].ToString());
	}
}

// Called every frame
void ABruteForceTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int i = 0; i < 6; ++i)
	{
		if(!PrintedResults[i])
        {
        	PrintedResults[i] = true;
        	
        	FOutputColumn out = Solver->GetOutput(i);
        	UE_LOG(Noxel, Log, TEXT("Runner%d done (after %d iterations)! Matrix :\r\n%s"), i, Solver->GetRunnerIteration(i), *out.ToString());
        	
        	FTRVector outVec = UBruteForceSolver::GetOutputVector(Sources, out, false);
        	UE_LOG(Noxel, Log, TEXT("Test Runner%d, result %s"), i, *outVec.ToString());
        }
        if (!PrintedResults[i])
        {
        	float progress = Solver->GetRunnerProgress(i);
        	UE_LOG(Noxel, Log, TEXT("Runner%d's progress : %f"), i , progress);
        }
	}
	
	
}

