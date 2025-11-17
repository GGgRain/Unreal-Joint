#include "Sequencer/MovieSceneJointSectionTemplate.h"

#include "JointActor.h"
#include "JointManager.h"
#include "MovieSceneExecutionToken.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"
#include "Sequencer/MovieSceneJointTrack.h"

DECLARE_CYCLE_STAT(
	TEXT("Joint Track Token Execute"),
	MovieSceneEval_JointTrack_TokenExecute,
	STATGROUP_MovieSceneEval
);

struct FMovieSceneJointExecutionData
{
	FMovieSceneJointExecutionData(
		TObjectPtr<const UMovieSceneJointTrack> InParentTrack,
		TWeakObjectPtr<UJointNodeBase> InJointNode,
		EJointMovieSectionType InSectionType,
		float InGlobalPosition)
		:
		ParentTrack(InParentTrack),
		JointNode(InJointNode),
		SectionType(InSectionType),
		GlobalPosition(InGlobalPosition)
	{
	}

public:

	TWeakObjectPtr<const UMovieSceneJointTrack> ParentTrack;
	TWeakObjectPtr<UJointNodeBase> JointNode;
	EJointMovieSectionType SectionType;
	float GlobalPosition;
};

/** A movie scene execution token that stores a specific transform, and an operand */
struct FJointTrackExecutionToken : IMovieSceneExecutionToken
{
	FJointTrackExecutionToken(TArray<FMovieSceneJointExecutionData> InJointData) : JointExecutionData(MoveTemp(InJointData))
	{
	}

	/** Execute this token, operating on all objects referenced by 'Operand' */
	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_JointTrack_TokenExecute)

		TArray<float> PerformanceCaptureEventPositions;

		for (FMovieSceneJointExecutionData& ExecutionData : JointExecutionData)
		{
			TriggerNode(ExecutionData, Player);
		}
	}

	void TriggerNode(FMovieSceneJointExecutionData& InExecutionData, IMovieScenePlayer& Player)
	{
		if (!InExecutionData.ParentTrack.IsValid() || !InExecutionData.JointNode.IsValid()) return;

		UJointNodeBase* FoundNode = nullptr;

		UJointNodeBase* AssetNode = InExecutionData.JointNode.Get();
		
		if (AJointActor* JointActor = InExecutionData.ParentTrack->GetJointActor())
		{
			if (UJointManager* JointManager = JointActor->GetJointManager())
			{
				FoundNode = JointManager->FindFragmentWithGuid(AssetNode->NodeGuid);

				if (!FoundNode)
				{
					FoundNode = JointManager->FindBaseNodeWithGuid(AssetNode->NodeGuid);
				}
			}

			if (FoundNode)
			{
				FoundNode->RequestNodeBeginPlay(JointActor);
				
				UE_LOG(LogTemp, Warning, TEXT("FJointTrackExecutionToken::TriggerNode: Triggering Joint Node %s at time %f, Actor %s"), *FoundNode->GetName(), InExecutionData.GlobalPosition, *JointActor->GetName());
			}
		}
	}

	TArray<FMovieSceneJointExecutionData> JointExecutionData;
};


FMovieSceneJointSectionTemplate::FMovieSceneJointSectionTemplate(
	const UMovieSceneJointSection& Section,
	const UMovieSceneJointTrack& Track)
{
	EnableOverrides(RequiresTearDownFlag);
	
	FJointMovieSectionPayload Payload = FJointMovieSectionPayload();

	Payload.ParentTrack = &Track;
	Payload.JointNodePointer = Section.GetJointNodePointer();
	Payload.SectionRange = Section.GetRange();
	Payload.SectionType = Section.SectionType;

	Payloads.Add(Payload);

	ParentTrack = &Track;
}

void FMovieSceneJointSectionTemplate::EvaluateSwept(
	const FMovieSceneEvaluationOperand& Operand,
	const FMovieSceneContext& Context,
	const TRange<FFrameNumber>& SweptRange,
	const FPersistentEvaluationData& PersistentData,
	FMovieSceneExecutionTokens& ExecutionTokens) const
{
	// Don't allow events to fire when playback is in a stopped state. This can occur when stopping 
	// playback and returning the current position to the start of playback. It's not desireable to have 
	// all the events from the last playback position to the start of playback be fired.
	if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent()) return;
	
	TArray<FMovieSceneJointExecutionData> JointDataToExecute;
	
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseLinearOnly() / Context.GetFrameRate();

	for (const FJointMovieSectionPayload& Payload : Payloads)
	{
		// Does it overlap?
		if (TRange<FFrameNumber>::Intersection(Payload.SectionRange, SweptRange).Size<FFrameNumber>() > 0)
		{
			JointDataToExecute.Add(FMovieSceneJointExecutionData(
				Payload.ParentTrack.Get(),
				Payload.JointNodePointer.Node.Get(),
				Payload.SectionType,
				PositionInSeconds)
			);
		}
	}

	if (JointDataToExecute.Num())
	{
		ExecutionTokens.Add(FJointTrackExecutionToken(MoveTemp(JointDataToExecute)));
	}
}

void FMovieSceneJointSectionTemplate::Evaluate(
	const FMovieSceneEvaluationOperand& Operand,
	const FMovieSceneContext& Context,
	const FPersistentEvaluationData& PersistentData,
	FMovieSceneExecutionTokens& ExecutionTokens) const
{
	// Don't allow events to fire when playback is in a stopped state. This can occur when stopping 
	// playback and returning the current position to the start of playback. It's not desireable to have 
	// all the events from the last playback position to the start of playback be fired.
	if (Context.GetStatus() == EMovieScenePlayerStatus::Stopped || Context.IsSilent()) return;
	
	TArray<FMovieSceneJointExecutionData> JointDataToExecute;
	
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseLinearOnly() / Context.GetFrameRate();

	for (const FJointMovieSectionPayload& Payload : Payloads)
	{
		if (Payload.bEverTriggered) continue;
		// Does it overlap?
		if (Payload.SectionRange.Contains(Context.GetTime().FloorToFrame()))
		{
			JointDataToExecute.Add(FMovieSceneJointExecutionData(
				Payload.ParentTrack.Get(),
				Payload.JointNodePointer.Node.Get(),
				Payload.SectionType,
				PositionInSeconds)
			);

			Payload.bEverTriggered = true;
			
			//UE_LOG(LogTemp, Warning, TEXT("FMovieSceneJointSectionTemplate::Evaluate: Triggering Joint Node %s at time %f"), Payload.JointNodePointer.Node ? *Payload.JointNodePointer.Node->GetName() : *FString("INVALID"), PositionInSeconds);
		}
	}

	if (JointDataToExecute.Num())
	{
		ExecutionTokens.Add(FJointTrackExecutionToken(MoveTemp(JointDataToExecute)));
	}
}

void FMovieSceneJointSectionTemplate::TearDown(
	FPersistentEvaluationData& PersistentData,
	IMovieScenePlayer& Player) const
{
	for (const FJointMovieSectionPayload& Payload : Payloads)
	{
		if (Payload.bEverTriggered && Payload.SectionType == EJointMovieSectionType::Range)
		{
			if (UJointNodeBase* Node = Payload.JointNodePointer.Node.Get())
			{
				Node->RequestNodeEndPlay();
			}
		}
	}
}
