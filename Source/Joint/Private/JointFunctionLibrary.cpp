//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointFunctionLibrary.h"

#include "JointManager.h"
#include "MovieScene.h"
#include "MovieSceneSequence.h"
#include "SharedType/JointSharedTypes.h"
#include "Components/RichTextBlock.h"
#include "Components/Widget.h"
#include "Engine/DataTable.h"
#include "Framework/Text/RichTextMarkupProcessing.h"
#include "Node/JointNodeBase.h"
#include "Sequencer/MovieSceneJointTrack.h"

#include "Misc/EngineVersionComparison.h"



bool UJointFunctionLibrary::IsWidgetFocusable(UWidget* TargetWidget)
{
	if (TargetWidget)
	{
		if (TargetWidget->GetCachedWidget())
		{
			return TargetWidget->GetCachedWidget()->SupportsKeyboardFocus();
		}
	}

	return false;
}

FText UJointFunctionLibrary::FormatTextWith(FText InText, FString Target, FText Format)
{
	FFormatNamedArguments FormatArgs;

	FormatArgs.Add(Target, Format);

	return FText::Format(InText, FormatArgs);
}

TArray<FInt32Range> UJointFunctionLibrary::GetTextContentRange(const FText InText)
{
	TArray<FInt32Range> OutRange;

	const TSharedRef<FDefaultRichTextMarkupParser> Parser = FDefaultRichTextMarkupParser::Create();

	TArray<FTextLineParseResults> Results;

	FString OutputString;

	Parser->Process(Results, InText.ToString(), OutputString);

	int LineIndex = 0;

	for (FTextLineParseResults& Result : Results)
	{
		for (FTextRunParseResults& Run : Result.Runs)
		{
			if (Run.ContentRange.Len() != 0)
			{
				FInt32Range Range = FInt32Range(Run.ContentRange.BeginIndex + LineIndex * 2,
				                                Run.ContentRange.EndIndex + 1 + LineIndex * 2);

				OutRange.Add(Range);
			}else if(Run.Name == "" && Run.OriginalRange.Len() != 0) // if this is an empty run...
			{
				FInt32Range Range = FInt32Range(Run.OriginalRange.BeginIndex + LineIndex * 2,
											Run.OriginalRange.EndIndex + 1 + LineIndex * 2);

				OutRange.Add(Range);
			}
		}

		LineIndex++;
	}

	return OutRange;
}

TArray<FInt32Range> UJointFunctionLibrary::GetDecoratorSymbolRange(const FText InText)
{
	TArray<FInt32Range> OutRange;

	//First, grab the content range.
	TArray<FInt32Range> ContentRange = GetTextContentRange(InText);

	int LastSeenRangeEnd = 0;

	//And subtract from the original range.
	for (FInt32Range Range : ContentRange)
	{
		//If the doesn't start from the 0, add it to the array.
		if (Range.GetLowerBound().GetValue() != 0)
		{
			OutRange.Add(FInt32Range(LastSeenRangeEnd, Range.GetLowerBound().GetValue() - 1));
		}

		LastSeenRangeEnd = Range.GetUpperBound().GetValue() - 1;
	}

	//Add the tail part if it doesn't end with content.
	if (LastSeenRangeEnd != InText.ToString().Len() - 1)
	{
		OutRange.Add(FInt32Range(LastSeenRangeEnd, InText.ToString().Len() - 1));
	}

	return OutRange;
}

TArray<FInt32Range> UJointFunctionLibrary::GetDecoratedTextContentRange(const FText InText)
{
	TArray<FInt32Range> OutRange;

	const TSharedRef<FDefaultRichTextMarkupParser> Parser = FDefaultRichTextMarkupParser::Create();

	TArray<FTextLineParseResults> Results;

	FString OutputString;

	Parser->Process(Results, InText.ToString(), OutputString);

	int LineIndex = 0;

	for (FTextLineParseResults& Result : Results)
	{
		for (FTextRunParseResults& Run : Result.Runs)
		{
			if (Run.ContentRange.Len() != 0)
			{
				FInt32Range Range = FInt32Range(Run.ContentRange.BeginIndex + LineIndex * 2,
												Run.ContentRange.EndIndex + 1 + LineIndex * 2);

				OutRange.Add(Range);
			}
		}

		LineIndex++;
	}

	return OutRange;
}


FText UJointFunctionLibrary::FormatTextWithMap(FText InText, TMap<FString, FText> Map)
{
	FFormatNamedArguments FormatArgs;

	for (TTuple<FString, FText> Result : Map)
	{
		FormatArgs.Add(Result.Key, Result.Value);
	}

	return FText::Format(InText, FormatArgs);
}

UDataTable* UJointFunctionLibrary::MergeTextStyleDataTables(TSet<UDataTable*> TablesToMerge)
{
	UDataTable* NewTable = nullptr;

	NewTable = NewObject<UDataTable>();
	NewTable->RowStruct = FRichTextStyleRow::StaticStruct();

	static const FString ContextString(TEXT("UJointFunctionLibrary::MergeTextStyleDataTables"));

	TSet<FName> AddedNames;

	AddedNames.Empty();

	for (UDataTable* Table : TablesToMerge)
	{
		if (Table == nullptr) continue;

		TArray<FName> RowNames = Table->GetRowNames();

		for (FName RowName : RowNames)
		{
			FRichTextStyleRow* Row = Table->FindRow<FRichTextStyleRow>(RowName, ContextString);

			if (!AddedNames.Contains(RowName))
			{
				NewTable->AddRow(RowName, *Row);
				AddedNames.Add(RowName);
			}
		}
	}

	return NewTable;
}

TArray<FJointEdPinData> UJointFunctionLibrary::ImplementPins(const TArray<FJointEdPinData>& ExistingPins,const TArray<FJointEdPinData>& NeededPinSignature)
{


	TArray<FJointEdPinData> TotalPins = ExistingPins;
	TArray<FJointEdPinData> CachedNeededPinSignature = NeededPinSignature;
	
	for (int i = TotalPins.Num() - 1; i >= 0; --i)
	{
		const FJointEdPinData& ExistingPin = ExistingPins[i];

		int bFoundSignatureIndex = INDEX_NONE;
		
		for (int j = CachedNeededPinSignature.Num() - 1; j >= 0; --j)
		{
			const FJointEdPinData& NeededPinSignaturePin = CachedNeededPinSignature[j];

			if(AreBothPinHaveSameSignature(ExistingPin, NeededPinSignaturePin))
			{
				bFoundSignatureIndex = j;
				
				break;
			}
		}

		if(bFoundSignatureIndex != INDEX_NONE) // Found
		{
			TotalPins[i].CopyPropertiesFrom(CachedNeededPinSignature[bFoundSignatureIndex]);
			CachedNeededPinSignature.RemoveAt(bFoundSignatureIndex);
		}else
		{
			TotalPins.RemoveAt(i);
		}

	}

	TotalPins.Append(CachedNeededPinSignature);

	return TotalPins;
}

const bool UJointFunctionLibrary::AreBothPinHaveSameSignature(const FJointEdPinData& A, const FJointEdPinData& B)
{
	return A.HasSameSignature(B);
}

UJointNodeBase* UJointFunctionLibrary::GetCorrespondingJointNodeForJointManager(UJointNodeBase* SearchFor, UJointManager* TargetManager)
{
	if (SearchFor != nullptr && SearchFor->GetJointManager() != nullptr)
	{
		UJointManager* JointManager = SearchFor->GetJointManager();
		
		//if this node is from the target Joint manager : return itself.
		if (JointManager == TargetManager) return SearchFor;

		if (!TargetManager) return nullptr;
		
		// JointManager->Nodes contains only the base node on the graph, not sub nodes. So we need to iterate through all nodes to find the matching one - but in a clever way.
		// Cache the hierarchy paths of the provided node of the attachment tree, from base node to itself.
		TArray<FString> InJointNodeHierarchyPaths;
		
		UJointNodeBase* CurrentNode = SearchFor;
		while (CurrentNode != nullptr)
		{
			FString CurrentPath = CurrentNode->GetPathName(JointManager);
			InJointNodeHierarchyPaths.Insert(CurrentPath, 0); // insert at the beginning to maintain order from base to leaf.

			CurrentNode = CurrentNode->GetParentNode();
		}
		
		// Now iterate the actual nodes on the asset side and 'step-in' for the hierarchy path stages to find the matching node.
		
		TArray<UJointNodeBase*> CandidateNodes = TargetManager->Nodes;
		while (CandidateNodes.Num() > 0 && InJointNodeHierarchyPaths.Num() > 0)
		{
			FString TargetPath = InJointNodeHierarchyPaths[0];
			InJointNodeHierarchyPaths.RemoveAt(0);

			TArray<UJointNodeBase*> NextCandidateNodes;

			for (UJointNodeBase* Node : CandidateNodes)
			{
				if (Node == nullptr) continue;

				FString NodePath = Node->GetPathName(Node->GetJointManager());

				if (NodePath == TargetPath)
				{
					// if this is the last stage, we found it.
					if (InJointNodeHierarchyPaths.Num() == 0)
					{
						return Node;
					}
					else
					{
						// step into children for next stage.
						TArray<UJointNodeBase*> ChildNodes = Node->SubNodes;
						NextCandidateNodes.Append(ChildNodes);
					}
				}
			}

			CandidateNodes = NextCandidateNodes;
		}
	}
	
	return nullptr;
}

bool UJointFunctionLibrary::DoesClassImplementInterface(TSubclassOf<UObject> ClassToCheck, TSubclassOf<UInterface> InterfaceToCheck)
{
	if (!ClassToCheck || !InterfaceToCheck) return false;

	return ClassToCheck->ImplementsInterface(InterfaceToCheck);
}

UJointNodeBase* UJointFunctionLibrary::CastAndResolveJointNodePointer(const FJointNodePointer& Pointer, const TSubclassOf<UJointNodeBase> CastClass)
{
	if (!Pointer.IsValid()) return nullptr;

	if (CastClass)
	{
		return RESOLVE_JOINT_POINTER(Pointer, UJointNodeBase) && Pointer.Node->IsA(CastClass) ? Pointer.Node.Get() : nullptr;
	}else
	{
		return Pointer.Node.Get();
	}
}

bool UJointFunctionLibrary::IsValid(const FJointNodePointer& Pointer)
{
	return Pointer.IsValid();
}

bool UJointFunctionLibrary::HasSameRestrictionsAs(const FJointNodePointer& A, const FJointNodePointer& B)
{
	return A.HasSameRestrictionsAs(B);
}

bool UJointFunctionLibrary::CheckMatchRestrictions(const FJointNodePointer& Pointer, TSet<TSubclassOf<UJointNodeBase>> AllowedClass, TSet<TSubclassOf<UJointNodeBase>> DisallowedClasses)
{
	return Pointer.CheckMatchRestrictions(AllowedClass, DisallowedClasses);
}

TArray<UMovieSceneJointTrack*> UJointFunctionLibrary::FindJointMovieTracks(UMovieSceneSequence* Sequence)
{
	auto FilterTracks = [](TArrayView<UMovieSceneTrack* const> InTracks, UClass* DesiredClass, bool bExactMatch) -> TArray<UMovieSceneJointTrack*>
	{
		TArray<UMovieSceneJointTrack*> Tracks;

		for (UMovieSceneTrack* Track : InTracks)
		{
			if (!Track) continue;
			UClass* TrackClass = Track->GetClass();

			if (TrackClass == DesiredClass || (!bExactMatch && TrackClass->IsChildOf(DesiredClass)))
			{
				Tracks.Add(Cast<UMovieSceneJointTrack>(Track));
			}
		}

		return Tracks;
	};
	
	UMovieScene* MovieScene   = Sequence ? Sequence->GetMovieScene() : nullptr;
	UClass*      DesiredClass = UMovieSceneJointTrack::StaticClass();

	if (MovieScene && DesiredClass)
	{
		bool bExactMatch = false;

#if UE_VERSION_OLDER_THAN(5, 3, 0)
		TArray<UMovieSceneJointTrack*> MatchedTracks = FilterTracks(MovieScene->GetMasterTracks(), DesiredClass, bExactMatch);
#else
		TArray<UMovieSceneJointTrack*> MatchedTracks = FilterTracks(MovieScene->GetTracks(), DesiredClass, bExactMatch);
#endif 
		return MatchedTracks;
	}

	return TArray<UMovieSceneJointTrack*>();
}

