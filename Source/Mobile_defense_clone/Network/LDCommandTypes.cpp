#include "Network/LDCommandTypes.h"

bool FLDCommand::IsValidPayload() const
{
	const bool bValidRevision =
	    CommandType == ELDCommandType::Upgrade ? ExpectedBoardRevision == -1 : ExpectedBoardRevision >= 0;
	if (ConnectionEpoch == 0 || RequestId == 0 || !bValidRevision ||
	    static_cast<uint8>(CommandType) > static_cast<uint8>(ELDCommandType::Target))
	{
		return false;
	}
	const bool bNoConsumed = ConsumedInstanceId0 == 0 && ConsumedInstanceId1 == 0 && ConsumedInstanceId2 == 0;
	const bool bNoSummon = Source == 0 && RouletteGrade == 0;
	const bool bNoMove = DestinationCellId == INDEX_NONE && MoveMode == 0;
	switch (CommandType)
	{
	case ELDCommandType::Summon:
		return InstanceId == 0 && bNoConsumed && bNoMove && Source <= 1 && RouletteGrade <= 3 &&
		       (Source != 0 || RouletteGrade == 0);
	case ELDCommandType::Merge:
		return InstanceId != 0 && bNoSummon && bNoMove && ConsumedInstanceId0 != 0 && ConsumedInstanceId1 != 0 &&
		       ConsumedInstanceId2 != 0 && ConsumedInstanceId0 != ConsumedInstanceId1 &&
		       ConsumedInstanceId0 != ConsumedInstanceId2 && ConsumedInstanceId1 != ConsumedInstanceId2 &&
		       (InstanceId == ConsumedInstanceId0 || InstanceId == ConsumedInstanceId1 ||
		        InstanceId == ConsumedInstanceId2);
	case ELDCommandType::Sell:
		return InstanceId != 0 && bNoConsumed && bNoSummon && bNoMove;
	case ELDCommandType::Move:
		return InstanceId != 0 && bNoConsumed && bNoSummon && MoveMode == 0 && DestinationCellId >= 0;
	default:
		// Reserved capabilities carry no accepted P0 payload and never execute.
		return InstanceId == 0 && bNoConsumed && bNoSummon && bNoMove;
	}
}

FLDCommand FLDCommand::Normalized() const
{
	FLDCommand Result = *this;
	if (Result.CommandType == ELDCommandType::Merge)
	{
		uint64 Ids[] = {ConsumedInstanceId0, ConsumedInstanceId1, ConsumedInstanceId2};
		if (Ids[0] > Ids[1])
		{
			Swap(Ids[0], Ids[1]);
		}
		if (Ids[1] > Ids[2])
		{
			Swap(Ids[1], Ids[2]);
		}
		if (Ids[0] > Ids[1])
		{
			Swap(Ids[0], Ids[1]);
		}
		Result.ConsumedInstanceId0 = Ids[0];
		Result.ConsumedInstanceId1 = Ids[1];
		Result.ConsumedInstanceId2 = Ids[2];
	}
	return Result;
}

bool FLDCommand::HasSameContent(const FLDCommand& Other) const
{
	return CommandType == Other.CommandType && ExpectedBoardRevision == Other.ExpectedBoardRevision &&
	       Source == Other.Source && RouletteGrade == Other.RouletteGrade && InstanceId == Other.InstanceId &&
	       ConsumedInstanceId0 == Other.ConsumedInstanceId0 && ConsumedInstanceId1 == Other.ConsumedInstanceId1 &&
	       ConsumedInstanceId2 == Other.ConsumedInstanceId2 && DestinationCellId == Other.DestinationCellId &&
	       MoveMode == Other.MoveMode;
}
