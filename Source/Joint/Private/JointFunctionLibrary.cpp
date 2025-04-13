//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointFunctionLibrary.h"

#include "JointSharedTypes.h"
#include "Components/RichTextBlock.h"
#include "Components/Widget.h"
#include "Engine/DataTable.h"
#include "Framework/Text/RichTextMarkupProcessing.h"


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
