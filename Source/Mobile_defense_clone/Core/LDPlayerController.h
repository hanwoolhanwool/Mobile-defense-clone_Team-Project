#pragma once

#include "CoreMinimal.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/PlayerController.h"
#include "Network/LDCommandTypes.h"
#include "LDPlayerController.generated.h"

class ULDCommandProcessor;

DECLARE_MULTICAST_DELEGATE_OneParam(FLDCommandCompleted, const FLDCommandResult&);

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void InitializeServerSession(const FLDParticipantContext& Context, ULDCommandProcessor& Processor);
	void ShutdownServerSession();
	bool SubmitLocalCommand(FLDCommand Command);
	bool RetryPendingCommand();
	bool HasPendingCommand() const;
	const FLDCommandResult& GetLastResult() const;
	FLDCommandCompleted OnCommandCompleted;

	UFUNCTION(Server, Reliable)
	void ServerRequestCommand(const FLDCommand& Command);
	UFUNCTION(Client, Reliable)
	void ClientCommandResult(const FLDCommandResult& Result);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_ConnectionEpoch();
	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor;
	FLDParticipantContext ServerContext;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	FGuid CurrentMatchId;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	uint64 ConnectionEpoch = 0;
	uint32 NextRequestId = 1;
	TOptional<FLDCommand> PendingCommand;
	FLDCommandResult LastResult;
};
