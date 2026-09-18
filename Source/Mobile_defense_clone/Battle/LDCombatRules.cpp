#include "Battle/LDCombatRules.h"

#include "Data/LDGameData.h"

namespace
{
	bool IsFinitePosition(const FVector& Position)
	{
		return FMath::IsFinite(Position.X) && FMath::IsFinite(Position.Y) && FMath::IsFinite(Position.Z);
	}
} // namespace

bool FLDCombatRules::TryCalculateDamage(const FLDUnitRow& Unit, double Armor, double MagicResistance, int32& OutDamage)
{
	if (!FMath::IsFinite(Unit.BaseAttack) || Unit.BaseAttack <= 0 || !FMath::IsFinite(Armor) ||
	    !FMath::IsFinite(MagicResistance))
	{
		return false;
	}
	double Multiplier = 0;
	if (Unit.DamageType == TEXT("Physical"))
	{
		Multiplier = 100.0 / (100.0 + FMath::Max(Armor, -50.0));
	}
	else if (Unit.DamageType == TEXT("Magic"))
	{
		Multiplier = 1.0 - FMath::Clamp(MagicResistance, 0.0, 0.75);
	}
	else
	{
		return false;
	}
	const double Rounded = FMath::FloorToDouble(Unit.BaseAttack * Multiplier + 0.5);
	if (!FMath::IsFinite(Rounded) || Rounded < 0 || Rounded > MAX_int32)
	{
		return false;
	}
	OutDamage = static_cast<int32>(Rounded);
	return true;
}

int32 FLDCombatRules::SelectTarget(const FVector& CanonicalOrigin, double RangeCm,
                                   const TArray<FLDTargetCandidate>& Candidates)
{
	if (!IsFinitePosition(CanonicalOrigin) || !FMath::IsFinite(RangeCm) || RangeCm <= 0)
	{
		return INDEX_NONE;
	}
	const double RangeSquared = RangeCm * RangeCm;
	int32 Selected = INDEX_NONE;
	double SelectedDistanceSquared = 0;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FLDTargetCandidate& Candidate = Candidates[Index];
		if (!Candidate.bAlive || Candidate.EnemyId == 0 || Candidate.SpawnSerial == 0 ||
		    !IsFinitePosition(Candidate.CanonicalPosition))
		{
			continue;
		}
		const double DistanceSquared = FVector::DistSquaredXY(CanonicalOrigin, Candidate.CanonicalPosition);
		if (!FMath::IsFinite(DistanceSquared) || DistanceSquared > RangeSquared)
		{
			continue;
		}
		if (Selected == INDEX_NONE || DistanceSquared < SelectedDistanceSquared ||
		    (DistanceSquared == SelectedDistanceSquared &&
		     (Candidate.SpawnSerial < Candidates[Selected].SpawnSerial ||
		      (Candidate.SpawnSerial == Candidates[Selected].SpawnSerial &&
		       Candidate.EnemyId < Candidates[Selected].EnemyId))))
		{
			Selected = Index;
			SelectedDistanceSquared = DistanceSquared;
		}
	}
	return Selected;
}
