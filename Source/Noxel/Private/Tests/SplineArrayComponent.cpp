// Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.


#include "Tests/SplineArrayComponent.h"


USplineArrayComponent::USplineArrayComponent()
{
	provider = CreateDefaultSubobject<USplineProvider>("SplineProvider");
}

USplineArrayComponent::~USplineArrayComponent()
{
}

void USplineArrayComponent::OnRegister()
{
	Super::OnRegister();
	Initialize(provider);
}
