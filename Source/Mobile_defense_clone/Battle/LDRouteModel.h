#pragma once

#include "CoreMinimal.h"

// Canonical geometry only. Neither viewport orientation nor actor identity belongs in path arithmetic.
class MOBILE_DEFENSE_CLONE_API FLDRouteModel
{
public:
	bool TryInitialize(const TArray<FVector>& Points, FString& OutError);
	bool TrySample(double TotalDistanceCm, FVector& OutPosition, FVector& OutTangent, uint64& OutLap) const;
	bool IsInitialized() const;
	double GetLengthCm() const;
	const TArray<FVector>& GetPoints() const;

	// A bounded display estimate never updates the authoritative distance or clock.
	static bool TryPredictPresentationDistance(double SampleDistanceCm, double SampleServerSeconds,
	                                           double SpeedCmPerSecond, bool bActive, double ViewServerSeconds,
	                                           double& OutDistanceCm);
	static constexpr double MaximumPresentationPredictionSeconds = 0.25;

private:
	TArray<FVector> RoutePoints;
	TArray<double> SegmentEndDistancesCm;
	double LengthCm = 0;
};
