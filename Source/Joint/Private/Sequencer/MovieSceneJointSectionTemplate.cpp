#include "Sequencer/MovieSceneJointSectionTemplate.h"

#include "JointActor.h"
#include "JointFunctionLibrary.h"
#include "JointLogChannels.h"
#include "JointManager.h"
#include "MovieSceneExecutionToken.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"
#include "Sequencer/MovieSceneJointTrack.h"

#include "Misc/EngineVersionComparison.h"

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
		
		// it will always refer to the asset node
		UJointNodeBase* AssetNode = InExecutionData.JointNode.Get();
		
		// early out if no asset node (it can be happened when the users forgot to set the node in the section)
		if ( !AssetNode ) return;
		
		if (AJointActor* JointActor = InExecutionData.ParentTrack->GetRuntimeJointActor())
		{
			// find the corresponding node for the Joint manager of the actor.
			FoundNode = UJointFunctionLibrary::GetCorrespondingJointNodeForJointManager(
				AssetNode,
				JointActor->GetJointManager()
			);
			
			if (!FoundNode) return;
			
			//Joint's node state is not reversible in any circumstances unless the node is reloaded.
			//So we don't have to care about checking the state of the node before calling the functions below.
			
			switch (InExecutionData.SectionType)
			{
			case EJointMovieSectionType::BeginPlay:
				FoundNode->RequestNodeBeginPlay(JointActor);	
				break;
			case EJointMovieSectionType::ActiveForRange:
				FoundNode->RequestNodeBeginPlay(JointActor);
				break;
			case EJointMovieSectionType::EndPlay:
				FoundNode->RequestNodeEndPlay();
				break;
			case EJointMovieSectionType::MarkAsPending:
				FoundNode->MarkNodePendingByForce();
				break;
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
	
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseLinearOnly() / Context.GetFrameRate();
#elif UE_VERSION_OLDER_THAN(5, 5, 0)
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseNoLooping() / Context.GetFrameRate();
#else
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().Inverse().AsLinear() / Context.GetFrameRate();
#endif

	if (const UMovieSceneJointSection* CastedSection = GetSourceSection() ? Cast<UMovieSceneJointSection>(GetSourceSection()) : nullptr)
	{
		if (TRange<FFrameNumber>::Intersection(CastedSection->GetRange(), SweptRange).Size<FFrameNumber>() > 0)
		{
			JointDataToExecute.Add(FMovieSceneJointExecutionData(
				ParentTrack.Get(),
				CastedSection->GetJointNodePointer().Node.Get(),
				CastedSection->SectionType,
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
	
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseLinearOnly() / Context.GetFrameRate();
#elif UE_VERSION_OLDER_THAN(5, 5, 0)
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().InverseNoLooping() / Context.GetFrameRate();
#else
	const float PositionInSeconds = Context.GetTime() * Context.GetRootToSequenceTransform().Inverse().AsLinear() / Context.GetFrameRate();
#endif
	
	//UE_LOG(LogJoint, Log, TEXT("FMovieSceneJointSectionTemplate::SourceSection: %s, Time: %f"), *GetSourceSection()->GetName(), PositionInSeconds);
	
	if (const UMovieSceneJointSection* CastedSection = GetSourceSection() ? Cast<UMovieSceneJointSection>(GetSourceSection()) : nullptr)
	{
		if (CastedSection->GetRange().Contains(Context.GetTime().FloorToFrame()))
		{
			JointDataToExecute.Add(FMovieSceneJointExecutionData(
				ParentTrack.Get(),
				CastedSection->GetJointNodePointer().Node.Get(),
				CastedSection->SectionType,
				PositionInSeconds)
			);
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
	
	const UMovieSceneJointSection* CastedSection = GetSourceSection() ? Cast<UMovieSceneJointSection>(GetSourceSection()) : nullptr;
	
	if (!CastedSection) return;
	
	if (CastedSection->SectionType == EJointMovieSectionType::ActiveForRange)
	{
		// it will always refer to the asset node
		UJointNodeBase* AssetNode = CastedSection->GetJointNodePointer().Node.Get();
		
		// early out if no asset node (it can be happened when the users forgot to set the node in the section)
		if ( !AssetNode ) return;
		
		if (AJointActor* JointActor = ParentTrack.Get()->GetRuntimeJointActor())
		{
			// find the corresponding node for the Joint manager of the actor.

			UJointNodeBase* FoundNode = UJointFunctionLibrary::GetCorrespondingJointNodeForJointManager(
				AssetNode,
				JointActor->GetJointManager()
			);
			
			if (!FoundNode) return;
			
			FoundNode->RequestNodeEndPlay();
			
		}
	}
}
