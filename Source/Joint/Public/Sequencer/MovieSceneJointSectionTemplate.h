// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneJointSection.h"
#include "UObject/ObjectMacros.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "MovieSceneJointSectionTemplate.generated.h"

enum class EJointMovieSectionType : uint8;
class UJointNodeBase;
class UMovieSceneJointTrack;
class UMovieSceneJointSection;

USTRUCT()
struct JOINT_API FMovieSceneJointSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneJointSectionTemplate() {}
	FMovieSceneJointSectionTemplate(const UMovieSceneJointSection& Section, const UMovieSceneJointTrack& Track);

public:
	
	UPROPERTY()
	TWeakObjectPtr<const UMovieSceneJointTrack> ParentTrack = nullptr;
	
private:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void EvaluateSwept(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const TRange<FFrameNumber>& SweptRange, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	
};
