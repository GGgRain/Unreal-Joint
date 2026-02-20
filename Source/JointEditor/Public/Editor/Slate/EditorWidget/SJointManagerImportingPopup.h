//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "SGraphActionMenu.h"
#include "Misc/EngineVersionComparison.h"


class UJointScriptParser;
class UJointManager;
class SEditableTextBox;
class SGraphActionMenu;
class UJointEdGraphNode;
class UJointNodeBase;


class JOINTEDITOR_API SJointManagerImportingPopup: public SCompoundWidget
{
	// a dialog widget to create a node preset from the selected nodes in the graph editor
	
public:
	
	enum class EJointImportMode : uint8
	{
		ToSpecifiedJointManager,
		AsIndividualJointManagers
	};
	
	class FParserCandidateItem : public TSharedFromThis<FParserCandidateItem>, public FGCObject
	{
	public:
		
		FParserCandidateItem();
		
	public:
		
		TObjectPtr<UJointScriptParser> Parser;
		FString Name;
		FString Path;
		
	public:
		
		// FGCObject interface
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Collector.AddReferencedObject(Parser);
		}
		
		virtual FString GetReferencerName() const override
		{
			return TEXT("FParserCandidateItem");
		}
	};
	
	class SParserCandidateRow : public SMultiColumnTableRow<TSharedPtr<FParserCandidateItem>>
	{
	public:
		SLATE_BEGIN_ARGS(SParserCandidateRow);
		SLATE_ARGUMENT(TSharedPtr<FParserCandidateItem>, Item)
		SLATE_END_ARGS()
	public:
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
		
	private:
		
		TSharedPtr<FParserCandidateItem> Item;
	};
	
public:
	
	SJointManagerImportingPopup();
	
public:
	void ConstructLayout();

public:
	
	SLATE_BEGIN_ARGS(SJointManagerImportingPopup)
		:
		_bIsReimporting(false),
		_OptionalJointManagerToImport(nullptr)
		{}
	SLATE_ARGUMENT(bool, bIsReimporting)
	SLATE_ARGUMENT(EJointImportMode, InitialImportMode)
	SLATE_ARGUMENT(FSoftObjectPath, OptionalJointManagerToImport)
	SLATE_ARGUMENT(TArray<FString>, ExternalFilePaths)
	SLATE_ARGUMENT(UClass*, InitiallySelectedParserClass)
	SLATE_ARGUMENT(TArray<uint8>, InitiallySelectedParserInstanceData)
	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );
	
private:
	
	/**
	 * Optional Joint manager to import.
	 */
	FSoftObjectPath OptionalJointManagerToImport;
	
	FString GetOptionalJointManagerPath() const;
	void OnOptionalJointManagerChanged(const FAssetData& AssetData);
	
private:

	TWeakPtr<SWindow> ParentWindow;
	
	TSharedPtr<SListView<TSharedPtr<FParserCandidateItem>>> ParserListView;
	TArray<TSharedPtr<FParserCandidateItem>> ParserCandidateItems;
	TWeakPtr<FParserCandidateItem> SelectedParserItem;
	
	TSharedPtr<SBox> ParserSettingsBox;
	
public:
	
	void UpdateParserSettingsWidget();
	
private:
	
	TSharedRef<SWidget> CreateParserSettingsWidget();
	
private:

	FName ParserCurrentSortColumn = "Match";
	EColumnSortMode::Type ParserCurrentSortMode = EColumnSortMode::Descending;

private:

	TArray<FString> ExternalFilePaths;
	
	// A path that will be used for importing if user choose "AsIndividualJointManagers" mode. it will be the path of the folder that contains all the imported Joint Managers.
	FString SelectedContentPath = "/Game/JointManager/Imported";
	
public:
	
	EJointImportMode CurrentImportMode = EJointImportMode::ToSpecifiedJointManager;
	EJointImportMode GetCurrentImportMode() const;
	void OnImportModeChanged(EJointImportMode JointImportMode);

public:
		
	void OnColumnSort(EColumnSortPriority::Type, const FName& ColumnId, EColumnSortMode::Type SortMode);
	EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
	
public:
	
	//callbacks for slate events
	
	FReply OnSelectFileButtonClicked();
	FReply OnProceedButtonClicked();
	
private:
	
	
	bool bIsReimporting = false;


	
};
