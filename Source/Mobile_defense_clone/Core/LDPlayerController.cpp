#include "Core/LDPlayerController.h"

#include "HAL/PlatformTime.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"

void ALDPlayerController::InitializeServerSession(const FLDParticipantContext& Context, ULDCommandProcessor& Processor)
{
	if (!HasAuthority() || !Context.IsValid())
	{
		return;
	}
	ServerContext = Context;
	CommandProcessor = &Processor;
	if (ConnectionEpoch != Context.ConnectionEpoch || CurrentMatchId != Context.MatchId)
	{
		CurrentMatchId = Context.MatchId;
		ConnectionEpoch = Context.ConnectionEpoch;
		OnRep_ConnectionEpoch();
	}
	ForceNetUpdate();
}

void ALDPlayerController::ShutdownServerSession()
{
	CommandProcessor = nullptr;
	ServerContext = {};
	PendingCommand.Reset();
}

void ALDPlayerController::OnRep_ConnectionEpoch()
{
	// Never replay an uncertain request into a different server connection epoch.
	PendingCommand.Reset();
	NextRequestId = 1;
	LastResult = {};
}

bool ALDPlayerController::SubmitLocalCommand(FLDCommand Command)
{
	if (!IsLocalController() || !CurrentMatchId.IsValid() || ConnectionEpoch == 0 || PendingCommand.IsSet() ||
	    NextRequestId == MAX_uint32)
	{
		return false;
	}
	Command.ConnectionEpoch = ConnectionEpoch;
	Command.RequestId = NextRequestId;
	if (!Command.IsValidPayload())
	{
		return false;
	}
	++NextRequestId;
	PendingCommand = Command;
	ServerRequestCommand(Command);
	return true;
}

bool ALDPlayerController::RetryPendingCommand()
{
	if (!IsLocalController() || !PendingCommand.IsSet())
	{
		return false;
	}
	ServerRequestCommand(PendingCommand.GetValue());
	return true;
}

void ALDPlayerController::ServerRequestCommand_Implementation(const FLDCommand& Command)
{
	if (!CommandProcessor || !ServerContext.IsValid())
	{
		FLDCommandResult Result;
		Result.MatchId = CurrentMatchId;
		Result.ConnectionEpoch = Command.ConnectionEpoch;
		Result.RequestId = Command.RequestId;
		Result.ResultCode = ELDCommandResultCode::PhaseNotAllowed;
		ClientCommandResult(Result);
		return;
	}
	const FLDCommandResult Result = CommandProcessor->Submit(ServerContext, Command);
	// A flood may delay a response, but must never replace an already cached outcome.
	if (CommandProcessor->CanSendResponse(ServerContext, FPlatformTime::Seconds()))
	{
		ClientCommandResult(Result);
	}
}

void ALDPlayerController::ClientCommandResult_Implementation(const FLDCommandResult& Result)
{
	if (Result.MatchId != CurrentMatchId || Result.ConnectionEpoch != ConnectionEpoch || !PendingCommand.IsSet() ||
	    Result.RequestId != PendingCommand->RequestId)
	{
		return;
	}
	LastResult = Result;
	if (Result.ResultCode != ELDCommandResultCode::Pending)
	{
		PendingCommand.Reset();
	}
	OnCommandCompleted.Broadcast(Result);
}

bool ALDPlayerController::HasPendingCommand() const
{
	return PendingCommand.IsSet();
}

const FLDCommandResult& ALDPlayerController::GetLastResult() const
{
	return LastResult;
}

void ALDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownServerSession();
	OnCommandCompleted.Clear();
	Super::EndPlay(EndPlayReason);
}

void ALDPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ALDPlayerController, CurrentMatchId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ALDPlayerController, ConnectionEpoch, COND_OwnerOnly);
}
