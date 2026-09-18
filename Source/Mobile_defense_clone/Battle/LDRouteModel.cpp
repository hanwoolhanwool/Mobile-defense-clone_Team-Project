#include "Battle/LDRouteModel.h"

namespace
{
	bool IsFiniteVector(const FVector& Vector)
	{
		return FMath::IsFinite(Vector.X) && FMath::IsFinite(Vector.Y) && FMath::IsFinite(Vector.Z);
	}
} // namespace

bool FLDRouteModel::TryInitialize(const TArray<FVector>& Points, FString& OutError)
{
	OutError.Reset();
	if (Points.Num() < 3)
	{
		OutError = TEXT("A closed route requires at least three distinct corners");
		return false;
	}
	TArray<double> CandidateEnds;
	double CandidateLength = 0;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FVector& Start = Points[Index];
		const FVector& End = Points[(Index + 1) % Points.Num()];
		if (!IsFiniteVector(Start) || !IsFiniteVector(End))
		{
			OutError = TEXT("Route contains a non-finite coordinate");
			return false;
		}
		const double SegmentLength = FVector::Distance(Start, End);
		if (!FMath::IsFinite(SegmentLength) || SegmentLength <= UE_SMALL_NUMBER)
		{
			OutError = TEXT("Route contains a zero-length or invalid segment");
			return false;
		}
		CandidateLength += SegmentLength;
		if (!FMath::IsFinite(CandidateLength))
		{
			OutError = TEXT("Route length overflow");
			return false;
		}
		CandidateEnds.Add(CandidateLength);
	}
	RoutePoints = Points;
	SegmentEndDistancesCm = MoveTemp(CandidateEnds);
	LengthCm = CandidateLength;
	return true;
}

bool FLDRouteModel::TrySample(double TotalDistanceCm, FVector& OutPosition, FVector& OutTangent, uint64& OutLap) const
{
	if (!IsInitialized() || !FMath::IsFinite(TotalDistanceCm) || TotalDistanceCm < 0)
	{
		return false;
	}
	const double Lap = FMath::FloorToDouble(TotalDistanceCm / LengthCm);
	// Keep conversion defined even for malformed distances far beyond any possible match duration.
	if (Lap >= static_cast<double>(MAX_uint64))
	{
		return false;
	}
	const double RouteDistance = FMath::Fmod(TotalDistanceCm, LengthCm);
	double SegmentStart = 0;
	for (int32 Index = 0; Index < RoutePoints.Num(); ++Index)
	{
		if (RouteDistance < SegmentEndDistancesCm[Index])
		{
			const FVector& Start = RoutePoints[Index];
			const FVector& End = RoutePoints[(Index + 1) % RoutePoints.Num()];
			const double SegmentLength = SegmentEndDistancesCm[Index] - SegmentStart;
			const double Alpha = (RouteDistance - SegmentStart) / SegmentLength;
			OutPosition = FMath::Lerp(Start, End, Alpha);
			OutTangent = (End - Start) / SegmentLength;
			OutLap = static_cast<uint64>(Lap);
			return true;
		}
		SegmentStart = SegmentEndDistancesCm[Index];
	}
	return false;
}

bool FLDRouteModel::IsInitialized() const
{
	return LengthCm > 0 && RoutePoints.Num() >= 3 && RoutePoints.Num() == SegmentEndDistancesCm.Num();
}

double FLDRouteModel::GetLengthCm() const
{
	return LengthCm;
}

const TArray<FVector>& FLDRouteModel::GetPoints() const
{
	return RoutePoints;
}

bool FLDRouteModel::TryPredictPresentationDistance(double SampleDistanceCm, double SampleServerSeconds,
                                                   double SpeedCmPerSecond, bool bActive, double ViewServerSeconds,
                                                   double& OutDistanceCm)
{
	if (!FMath::IsFinite(SampleDistanceCm) || SampleDistanceCm < 0 || !FMath::IsFinite(SampleServerSeconds) ||
	    !FMath::IsFinite(ViewServerSeconds) || !FMath::IsFinite(SpeedCmPerSecond) || SpeedCmPerSecond <= 0)
	{
		return false;
	}
	const double PredictionSeconds =
	    bActive ? FMath::Clamp(ViewServerSeconds - SampleServerSeconds, 0.0, MaximumPresentationPredictionSeconds)
	            : 0.0;
	const double CandidateDistance = SampleDistanceCm + PredictionSeconds * SpeedCmPerSecond;
	if (!FMath::IsFinite(CandidateDistance))
	{
		return false;
	}
	OutDistanceCm = CandidateDistance;
	return true;
}
