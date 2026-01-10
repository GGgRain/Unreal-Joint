//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SharedType/JointSharedTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JointFunctionLibrary.generated.h"

class UJointManager;
class UMovieSceneJointTrack;
class UMovieSceneSequence;
class UMovieSceneTrack;
class UWidget;
/**
 * 
 */
UCLASS()
class JOINT_API UJointFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	/**
	 * Return the corresponding Joint node from the provided Joint node for the target Joint manager.
	 * @param SearchFor Key object to use
	 * @param TargetManager Target Joint manager to find the corresponding node for.
	 * @return Corresponding Joint Graph Node.
	 */
	static UJointNodeBase* GetCorrespondingJointNodeForJointManager(UJointNodeBase* SearchFor, UJointManager* TargetManager);

public:
	
	/*
	 * Check if the provided widget is focusable.
	 */
	UFUNCTION(BlueprintCallable, Category="Condition")
	static bool IsWidgetFocusable(UWidget* TargetWidget);


	/*
	 * Format the provided text with the provided format argument map 
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static FText FormatTextWithMap(FText InText, TMap<FString, FText> Map);
	
	/*
	 * Format the provided text with the provided format argument. 
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static FText FormatTextWith(FText InText, FString Target, FText Format);


	
	/**
	 * Get every range that the content texts take place, including empty run's content.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static TArray<FInt32Range> GetTextContentRange(FText InText);

	/**
	 * Get every range that the decorator symbols take place. The result will be a complement of the GetTextContentRange() ranges.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static TArray<FInt32Range> GetDecoratorSymbolRange(FText InText);
	
	/**
	 * Get every range that the decorated run's content texts take places. This excludes the empty run's content.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static TArray<FInt32Range> GetDecoratedTextContentRange(FText InText);
	
	/**
	 * Merge text style data tables. If there is redundant row name, it will use the first case.
	 * It only works with the table with FRichTextStyleRow row struct. if you use some custom type of it then You must try to make one custom version of this function for your project.
	 * Note: We Highly recommended to set the row name with specific label on them.
	 * for example, RichText.Cute.Default, RichText.Cute.Row1, RichText.Cute.Row2, RichText.Cute.Row3... like this.
	 * @param TablesToMerge tables to merge together.
	 * @return A merged table instance. Notice this instance will be transient, can not be stored and serialized.
	 */
	UFUNCTION(BlueprintCallable, Category="Joint Text Utilities")
	static UDataTable* MergeTextStyleDataTables(TSet<UDataTable*> TablesToMerge);

public:

	//Pin Related

	/**
	 * Implement pin while trying to maintain existing pins.
	 * @param ExistingPins Already existing pins. If NeededPinSignature array has pins that has the same signatures, they will be maintained, otherwise, discarded.
	 * @param NeededPinSignature Total list of Pins we need to implement. if ExistingPins array already has pins that we need, it will use that instead of implementing it newly.
	 * @return New Total pins.
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Text Utilities")
	static TArray<FJointEdPinData> ImplementPins(const TArray<FJointEdPinData>& ExistingPins,const TArray<FJointEdPinData>& NeededPinSignature);


	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Text Utilities")
	static const bool AreBothPinHaveSameSignature(const FJointEdPinData& A, const FJointEdPinData& B);

public:

	
	/**
	 * Check whether the provided class implements the provided interface.
	 * Note: Why doesn't Unreal Engine provide this.....?
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Function Library")
	static bool DoesClassImplementInterface(TSubclassOf<UObject> ClassToCheck, TSubclassOf<UInterface> InterfaceToCheck);

public:

	/*
	 * Resolve the Joint Node Pointer to get the actual Joint Node instance.
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Node Pointer", meta=(DeterminesOutputType="CastClass"))
	static UJointNodeBase* CastAndResolveJointNodePointer(const FJointNodePointer& Pointer, TSubclassOf<UJointNodeBase> CastClass);

	/**
	 * Check whether the Joint Node Pointer is valid (has valid object reference)
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Node Pointer")
	static bool IsValid(const FJointNodePointer& Pointer);

	/**
	 * Check whether two Joint Node Pointers has the same restrictions (Allowed and Disallowed Classes)
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Node Pointer")
	static bool HasSameRestrictionsAs(const FJointNodePointer& A, const FJointNodePointer& B);

	/**
	 * Check whether the Joint Node Pointer has the same restrictions (Allowed and Disallowed Classes) as the provided ones.
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Node Pointer")
	static bool CheckMatchRestrictions(const FJointNodePointer& Pointer, TSet<TSubclassOf<UJointNodeBase>> AllowedClass, TSet<TSubclassOf<UJointNodeBase>> DisallowedClasses);


public:

	/**
	 * Get the Joint Movie Tracks (UMovieSceneJointTrack)
	 * This function is introduced due to the version compatibility issues: the signature of the Find Track function has been changed over the versions.
	 * This will abstract that issue away.
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="Joint Movie Track (UMovieSceneJointTrack)")
	static TArray<UMovieSceneJointTrack*> FindJointMovieTracks(UMovieSceneSequence* Sequence);
};
