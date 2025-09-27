// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditorWidget/SJointGraphPalette.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"

#include "BlueprintActionMenuItem.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintDragDropMenuItem.h"
#include "BlueprintEditor.h"
#include "BlueprintEditorSettings.h"

#include "BlueprintNamespaceUtilities.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintPaletteFavorites.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "Containers/UnrealString.h"
#include "CoreGlobals.h"
#include "Delegates/Delegate.h"
#include "Dialogs/Dialogs.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Engine/Blueprint.h"
#include "Engine/MemberReference.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/SlateDelegates.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/Platform.h"
#include "IAssetTools.h"
#include "IDocumentation.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Variable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Layout/Children.h"
#include "Layout/ChildrenBase.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Math/Color.h"
#include "Misc/AssertionMacros.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "SGraphActionMenu.h"
#include "SMyBlueprint.h"
#include "SPinTypeSelector.h"
#include "ScopedTransaction.h"
#include "SlateOptMacros.h"
#include "SlotBase.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleDefaults.h"
#include "Templates/Casts.h"
#include "Templates/SubclassOf.h"
#include "Textures/SlateIcon.h"
#include "TutorialMetaData.h"

#include "Types/ISlateMetaData.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Package.h"
#include "UObject/Script.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealNames.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/IToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"

class FDragDropEvent;
struct FGeometry;
struct FSlateBrush;

#define LOCTEXT_NAMESPACE "JointEditorPalette"

/*******************************************************************************
* Static File Helpers
*******************************************************************************/

/** namespace'd to avoid collisions during unified builds */
namespace JointEditorPalette
{
	static FString const ConfigSection("BlueprintEditor.Palette");
	static FString const FavoritesHeightConfigKey("FavoritesHeightRatio");
	static FString const LibraryHeightConfigKey("LibraryHeightRatio");
}



/**
 * A utility function intended to aid the construction of a specific blueprint 
 * palette item (specifically FEdGraphSchemaAction_K2Graph palette items). Based 
 * off of the sub-graph's type, this gets an icon representing said sub-graph.
 * 
 * @param  ActionIn		The FEdGraphSchemaAction_K2Graph action that the palette item represents.
 * @param  BlueprintIn	The blueprint currently being edited (that the action is for).
 * @param  IconOut		An icon denoting the sub-graph's type.
 * @param  ColorOut		An output color, further denoting the specified action.
 * @param  ToolTipOut	The tooltip to display when the icon is hovered over (describing the sub-graph type).
 */

/**
 * A utility function intended to aid the construction of a specific blueprint 
 * palette item. This looks at the item's associated action, and based off its  
 * type, retrieves an icon, color and tooltip for the slate widget.
 * 
 * @param  ActionIn		The action associated with the palette item you want an icon for.
 * @param  BlueprintIn	The blueprint currently being edited (that the action is for).
 * @param  BrushOut		An output of the icon, best representing the specified action.
 * @param  ColorOut		An output color, further denoting the specified action.
 * @param  ToolTipOut	An output tooltip, best describing the specified action type.
 */
static void GetPaletteItemIcon(TSharedPtr<FEdGraphSchemaAction> ActionIn, FSlateBrush const*& BrushOut, FSlateColor& ColorOut, FText& ToolTipOut, FString& DocLinkOut, FString& DocExcerptOut, FSlateBrush const*& SecondaryBrushOut, FSlateColor& SecondaryColorOut)
{
	// Default to tooltip based on action supplied
	ToolTipOut = ActionIn->GetTooltipDescription().IsEmpty() ? ActionIn->GetMenuDescription() : ActionIn->GetTooltipDescription();

	if (ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph const* GraphAction = (FEdGraphSchemaAction_K2Graph const*)ActionIn.Get();
		FJointEdUtils::GetGraphIconForAction(GraphAction, BrushOut, ColorOut, ToolTipOut);
	}
}

/**
 * Takes the existing tooltip and concats a path id (for the specified action) 
 * to the end.
 * 
 * @param  ActionIn		The action you want to show the path for.
 * @param  OldToolTip	The tooltip that you're replacing (we fold it into the new one)/
 * @return The newly created tooltip (now with the action's path tacked on the bottom).
 */
static TSharedRef<IToolTip> ConstructToolTipWithActionPath(TSharedPtr<FEdGraphSchemaAction> ActionIn, TSharedPtr<IToolTip> OldToolTip)
{
	TSharedRef<IToolTip> NewToolTip = OldToolTip.ToSharedRef();
	if (!ActionIn)
	{
		return NewToolTip;
	}
	
	return NewToolTip;
}

/*******************************************************************************
* FJointEditorPaletteItemRenameUtils
*******************************************************************************/

/** A set of utilities to aid SJointGraphPaletteItem when the user attempts to rename one. */
struct FJointEditorPaletteItemRenameUtils
{
private:
	static bool VerifyNewAssetName(UObject* Object, const FText& InNewText, FText& OutErrorMessage)
	{
		if (!Object)
		{
			return false;
		}

		if (Object->GetName() == InNewText.ToString())
		{
			return true;
		}

		TArray<FAssetData> AssetData;
		FAssetRegistryModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetToolsModule.Get().GetAssetsByPath(FName(*FPaths::GetPath(Object->GetOutermost()->GetPathName())), AssetData);

		if(!FFileHelper::IsFilenameValidForSaving(InNewText.ToString(), OutErrorMessage) || !FName(*InNewText.ToString()).IsValidObjectName( OutErrorMessage ))
		{
			return false;
		}
		else if( InNewText.ToString().Len() >= NAME_SIZE )
		{
			OutErrorMessage = FText::Format(LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than {0} characters!"), NAME_SIZE);
		}
		else
		{
			// Check to see if the name conflicts
			for ( const FAssetData& AssetInfo : AssetData)
			{
				if(AssetInfo.AssetName.ToString() == InNewText.ToString())
				{
					OutErrorMessage = LOCTEXT("RenameFailed_AlreadyInUse", "Asset name already in use!");
					return false;
				}
			}
		}

		return true;
	}

	static void CommitNewAssetName(UObject* Object, FBlueprintEditor* BlueprintEditor, const FText& NewText)
	{
		if (Object && BlueprintEditor)
		{
			if(Object->GetName() != NewText.ToString())
			{
				TArray<FAssetRenameData> AssetsAndNames;
				const FString PackagePath = FPackageName::GetLongPackagePath(Object->GetOutermost()->GetName());
				new(AssetsAndNames) FAssetRenameData(Object, PackagePath, NewText.ToString());

				FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
				AssetToolsModule.Get().RenameAssetsWithDialog(AssetsAndNames);
			}

			TSharedPtr<SMyBlueprint> MyBlueprint = BlueprintEditor->GetMyBlueprintWidget();
			if (MyBlueprint.IsValid())
			{
				MyBlueprint->SelectItemByName(FName(*Object->GetPathName()));
			}	
		}
	}

public:
	
	/**
	 * Determines whether the target node, associated with the selected action, 
	 * can be renamed with the specified text.
	 * 
	 * @param  InNewText		The text you want to verify.
	 * @param  OutErrorMessage	Text explaining why the associated node couldn't be renamed (if the return value is false).
	 * @param  ActionPtr		The selected action that the calling palette item represents.
	 * @return True if it is ok to rename the associated node with the given string (false if not).
	 */
	static bool VerifyNewTargetNodeName(const FText& InNewText, FText& OutErrorMessage, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{

		bool bIsNameValid = false;
		OutErrorMessage = LOCTEXT("RenameFailed_NodeRename", "Cannot rename associated node!");

		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId());
		FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)ActionPtr.Pin().Get();

		UK2Node* AssociatedNode = TargetNodeAction->NodeTemplate;
		if (AssociatedNode && AssociatedNode->GetCanRenameNode())
		{
			TSharedPtr<INameValidatorInterface> NodeNameValidator = FNameValidatorFactory::MakeValidator(AssociatedNode);
			bIsNameValid = (NodeNameValidator->IsValid(InNewText.ToString(), true) == EValidatorResult::Ok);
		}
		return bIsNameValid;
	}

	/**
	 * Take the verified text and renames the target node associated with the 
	 * selected action.
	 * 
	 * @param  NewText		The new (verified) text to rename the node with.
	 * @param  InTextCommit	A value denoting how the text was entered.
	 * @param  ActionPtr	The selected action that the calling palette item represents.
	 */
	static void CommitNewTargetNodeName(const FText& NewText, ETextCommit::Type InTextCommit, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId());

		FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)ActionPtr.Pin().Get();
		if (TargetNodeAction->NodeTemplate)
		{
			TargetNodeAction->NodeTemplate->OnRenameNode(NewText.ToString());
		}
	}
};

/*******************************************************************************
* SJointGraphPaletteItem Public Interface
*******************************************************************************/

//------------------------------------------------------------------------------
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SJointGraphPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
{
	Construct(InArgs, InCreateData, JointEditorToolkitPtr);
}



bool IsPaletteActionReadOnly(TSharedPtr<FEdGraphSchemaAction> ActionIn, TSharedPtr<FJointEditorToolkit> const InJointEditorToolkit)
{
	check(InJointEditorToolkit.IsValid());
	
	bool bIsReadOnly = false;

	//InJointEditorToolkit->Edit()
	
	return bIsReadOnly;
}



void SJointGraphPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, TWeakPtr<FJointEditorToolkit> InJointEditorToolkit)
{
	check(InCreateData->Action.IsValid());
	
	bShowClassInTooltip = InArgs._ShowClassInTooltip;	

	TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;
	JointEditorToolkitPtr = InJointEditorToolkit;

	const bool bIsFullyReadOnly = !InJointEditorToolkit.IsValid() || InCreateData->bIsReadOnly;
	
	TWeakPtr<FEdGraphSchemaAction> WeakGraphAction = GraphAction;
	auto IsReadOnlyLambda = [WeakGraphAction, InJointEditorToolkit, bIsFullyReadOnly]()
	{ 
		if(WeakGraphAction.IsValid() && InJointEditorToolkit.IsValid())
		{
			return bIsFullyReadOnly || IsPaletteActionReadOnly(WeakGraphAction.Pin(), InJointEditorToolkit.Pin());
		}

		return bIsFullyReadOnly;
	};
	
	// We differentiate enabled/read-only state here to not dim icons out unnecessarily, which in some situations
	// (like the right-click palette menu) is confusing to users.
	auto IsEditingEnabledLambda = [InJointEditorToolkit]()
	{ 
		if(InJointEditorToolkit.IsValid())
		{
			//return InJointEditorToolkit.Pin()->InEditingMode();
		}

		return true;
	};

	TAttribute<bool> bIsReadOnly = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(IsReadOnlyLambda));
	TAttribute<bool> bIsEditingEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(IsEditingEnabledLambda));

	// construct the icon widget
	FSlateBrush const* IconBrush   = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("NoBrush"));
	FSlateBrush const* SecondaryBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("NoBrush"));
	FSlateColor        IconColor   = FSlateColor::UseForeground();
	FSlateColor        SecondaryIconColor   = FSlateColor::UseForeground();
	FText			   IconToolTip = GraphAction->GetTooltipDescription();
	FString			   IconDocLink, IconDocExcerpt;
	GetPaletteItemIcon(GraphAction, IconBrush, IconColor, IconToolTip, IconDocLink, IconDocExcerpt, SecondaryBrush, SecondaryIconColor);
	TSharedRef<SWidget> IconWidget = CreateIconWidget(IconToolTip, IconBrush, IconColor, IconDocLink, IconDocExcerpt, SecondaryBrush, SecondaryIconColor);
	IconWidget->SetEnabled(bIsEditingEnabled);

	UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();

	// Enum representing the access specifier of this function or variable
	enum class EAccessSpecifier : uint8
	{
		None		= 0,
		Private		= 1,
		Protected	= 2,
		Public		= 3
	};

	// We should only bother checking for access if the setting is on and this is not an animation graph
	const bool bShouldCheckForAccessSpec = Settings->bShowAccessSpecifier;

	EAccessSpecifier ActionAccessSpecifier = EAccessSpecifier::None;	

	// Setup a meta tag for this node
	FTutorialMetaData TagMeta("PaletteItem"); 
	if( ActionPtr.IsValid() )
	{
		TagMeta.Tag = *FString::Printf(TEXT("PaletteItem,%s,%d"), *GraphAction->GetMenuDescription().ToString(), GraphAction->GetSectionID());
		TagMeta.FriendlyName = GraphAction->GetMenuDescription().ToString();
	}
	// construct the text widget
	TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget(InCreateData, bIsReadOnly );

	if (bShouldCheckForAccessSpec && GraphAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		UFunction* FunctionToCheck = nullptr;

		if (FEdGraphSchemaAction_K2Graph* FuncGraphAction = (FEdGraphSchemaAction_K2Graph*)(GraphAction.Get()))
		{
			//TODO: make it work properly with Joint Editor
			//FunctionToCheck = FindUField<UFunction>(Blueprint->SkeletonGeneratedClass, FuncGraphAction->FuncName);

			// Handle override/interface functions
			if(!FunctionToCheck)
			{
				//FBlueprintEditorUtils::GetOverrideFunctionClass(Blueprint, FuncGraphAction->FuncName, &FunctionToCheck);			
			}
		}

		// If we have found a function that matches this action name, then grab it's access specifier
		if (FunctionToCheck)
		{
			if (FunctionToCheck->HasAnyFunctionFlags(FUNC_Protected))
			{
				ActionAccessSpecifier = EAccessSpecifier::Protected;
			}
			else if (FunctionToCheck->HasAnyFunctionFlags(FUNC_Private))
			{
				ActionAccessSpecifier = EAccessSpecifier::Private;
			}
			else
			{
				ActionAccessSpecifier = EAccessSpecifier::Public;
			}
		}
	}

	FText AccessModifierText = FText::GetEmpty();

	switch (ActionAccessSpecifier)
	{
		case EAccessSpecifier::Public:
		{
			AccessModifierText = LOCTEXT("AccessModifierPublic", "public");
		}
		break;
		case EAccessSpecifier::Protected:
		{
			AccessModifierText = LOCTEXT("AccessModifierProtected", "protected");
		}
		break;
		case EAccessSpecifier::Private:
		{
			AccessModifierText = LOCTEXT("AccessModifierPrivate", "private");
		}
		break;
	}

	// Calculate a color so that the text gets brighter the more accessible the action is
	const bool AccessSpecifierEnabled = (ActionAccessSpecifier != EAccessSpecifier::None) && bShouldCheckForAccessSpec;

	// Create the widget with an icon
	TSharedRef<SHorizontalBox> ActionBox = SNew(SHorizontalBox)		
		.AddMetaData<FTutorialMetaData>(TagMeta);


	auto CreateAccessSpecifierLambda = [&ActionBox, &AccessSpecifierEnabled, &AccessModifierText, &ActionAccessSpecifier]() {

		ActionBox.Get().AddSlot()
			.MaxWidth(50.f)
			.FillWidth(AccessSpecifierEnabled ? 0.4f : 0.0f)
			.Padding(FMargin(/* horizontal */ AccessSpecifierEnabled ? 6.0f : 0.0f, /* vertical */ 0.0f))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				// Will only display text if we have a modifier level
			.IsEnabled(AccessSpecifierEnabled)
			.Text(AccessModifierText)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			// Bold if public
			.TextStyle(FAppStyle::Get(), ActionAccessSpecifier == EAccessSpecifier::Public ? "BlueprintEditor.AccessModifier.Public" : "BlueprintEditor.AccessModifier.Default")
			];
	};

	
	ActionBox.Get().AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					IconWidget
				];

	// Only add an access specifier if we have one
	if (ActionAccessSpecifier != EAccessSpecifier::None)
	{
		CreateAccessSpecifierLambda();
	}

	ActionBox.Get().AddSlot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(/* horizontal */ 3.0f, /* vertical */ 3.0f)
		[
			NameSlotWidget
		];

	// Now, create the actual widget
	ChildSlot
	[
		ActionBox
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SJointGraphPaletteItem::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (JointEditorToolkitPtr.IsValid())
	{
		SGraphPaletteItem::OnDragEnter(MyGeometry, DragDropEvent);
	}
}

/*******************************************************************************
* SJointGraphPaletteItem Private Methods
*******************************************************************************/

//------------------------------------------------------------------------------
TSharedRef<SWidget> SJointGraphPaletteItem::CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnlyIn)
{
	FName const ActionTypeId = InCreateData->Action->GetTypeId();

	FOnVerifyTextChanged OnVerifyTextChanged;
	FOnTextCommitted     OnTextCommitted;
		
	// enums have different rules for renaming that exist outside the bounds of other items.
	if (ActionTypeId == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId())
	{
		OnVerifyTextChanged.BindStatic(&FJointEditorPaletteItemRenameUtils::VerifyNewTargetNodeName, ActionPtr);
		OnTextCommitted.BindStatic(&FJointEditorPaletteItemRenameUtils::CommitNewTargetNodeName, ActionPtr);
	}
	else
	{
		// default to our own rename methods
		OnVerifyTextChanged.BindSP(this, &SJointGraphPaletteItem::OnNameTextVerifyChanged);
		OnTextCommitted.BindSP(this, &SJointGraphPaletteItem::OnNameTextCommitted);
	}

	// Copy the mouse delegate binding if we want it
	if( InCreateData->bHandleMouseButtonDown )
	{
		MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	}

	TSharedPtr<SToolTip> ToolTipWidget = ConstructToolTipWidget();

	TSharedPtr<SOverlay> DisplayWidget;
	TSharedPtr<SInlineEditableTextBlock> EditableTextElement;
	SAssignNew(DisplayWidget, SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(EditableTextElement, SInlineEditableTextBlock)
				.Text(this, &SJointGraphPaletteItem::GetDisplayText)
				.HighlightText(InCreateData->HighlightText)
				.ToolTip(ToolTipWidget)
				.OnVerifyTextChanged(OnVerifyTextChanged)
				.OnTextCommitted(OnTextCommitted)
				.IsSelected(InCreateData->IsRowSelectedDelegate)
				.IsReadOnly(bIsReadOnlyIn)
		];
	InlineRenameWidget = EditableTextElement.ToSharedRef();

	InCreateData->OnRenameRequest->BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);

	if (GetDefault<UBlueprintEditorSettings>()->bShowActionMenuItemSignatures && ActionPtr.IsValid())
	{
		check(InlineRenameWidget.IsValid());
		TSharedPtr<IToolTip> ExistingToolTip = InlineRenameWidget->GetToolTip();

		DisplayWidget->AddSlot(0)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::Visible)
				.ToolTip(ConstructToolTipWithActionPath(ActionPtr.Pin(), ExistingToolTip))
			];
	}

	return DisplayWidget.ToSharedRef();
}

//------------------------------------------------------------------------------
FText SJointGraphPaletteItem::GetDisplayText() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if (MenuDescriptionCache.IsOutOfDate(K2Schema))
	{
		MenuDescriptionCache.SetCachedText(ActionPtr.Pin()->GetMenuDescription(), K2Schema);
	}

	return MenuDescriptionCache;
}

//------------------------------------------------------------------------------
bool SJointGraphPaletteItem::OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage)
{
	FString TextAsString = InNewText.ToString();

	FName OriginalName;

	UStruct* ValidationScope = nullptr;

	const UEdGraphSchema* Schema = nullptr;

	
	
	UEdGraph* Graph = nullptr;

	if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)ActionPtr.Pin().Get();
		Graph = GraphAction->EdGraph;
	}
	if (Graph)
	{
		OriginalName = Graph->GetFName();
		Schema = Graph->GetSchema();
	}

	

	if (OriginalName.IsNone() && ActionPtr.Pin()->IsA(FEdGraphSchemaAction_BlueprintVariableBase::StaticGetTypeId()))
	{
		FEdGraphSchemaAction_BlueprintVariableBase* BPVar = (FEdGraphSchemaAction_BlueprintVariableBase*)ActionPtr.Pin().Get();
		return BPVar->IsValidName(FName(TextAsString), OutErrorMessage);
	}
	else
	{
		//TODO: make it work properly with Joint Editor
		TSharedPtr<INameValidatorInterface> NameValidator = nullptr;
		if (Schema)
		{
			//NameValidator = Schema->GetNameValidator(BlueprintObj, OriginalName, ValidationScope, ActionPtr.Pin()->GetTypeId());	
		}
		
		if (NameValidator.IsValid())
		{
			EValidatorResult ValidatorResult = NameValidator->IsValid(TextAsString);
			switch (ValidatorResult)
			{
			case EValidatorResult::Ok:
			case EValidatorResult::ExistingName:
				// These are fine, don't need to surface to the user, the rename can 'proceed' even if the name is the existing one
				break;
			default:
				OutErrorMessage = INameValidatorInterface::GetErrorText(TextAsString, ValidatorResult);
				break;
			}
		}
	}
	
	return OutErrorMessage.IsEmpty();
}

//------------------------------------------------------------------------------
void SJointGraphPaletteItem::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	const FString NewNameString = NewText.ToString();
	const FName NewName = *NewNameString;

	if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)ActionPtr.Pin().Get();

		UEdGraph* Graph = GraphAction->EdGraph;
		
		if (Graph && (Graph->bAllowDeletion || Graph->bAllowRenaming)) return;
		if (!GraphAction->EdGraph) return;
			
		if (const UEdGraphSchema* GraphSchema = GraphAction->EdGraph->GetSchema())
		{
			FGraphDisplayInfo DisplayInfo;
			GraphSchema->GetGraphDisplayInformation(*GraphAction->EdGraph, DisplayInfo);

			// Check if the name is unchanged
			if (NewText.EqualTo(DisplayInfo.PlainName)) return;
			if (GraphSchema->TryRenameGraph(Graph, *NewText.ToString())) return;
		}
		// If we reach here, the rename failed
	}
	
	//JointEditorToolkitPtr.Pin()->GetMyBlueprintWidget()->SelectItemByName(NewName, ESelectInfo::OnMouseClick);
}

//------------------------------------------------------------------------------
FText SJointGraphPaletteItem::GetToolTipText() const
{
	TSharedPtr<FEdGraphSchemaAction> PaletteAction = ActionPtr.Pin();

	FText ToolTipText;
	FText ClassDisplayName;

	if (PaletteAction.IsValid())
	{
		// Default tooltip is taken from the action
		ToolTipText = PaletteAction->GetTooltipDescription().IsEmpty() ? PaletteAction->GetMenuDescription() : PaletteAction->GetTooltipDescription();

		if (UK2Node const* const NodeTemplate = FBlueprintActionMenuUtils::ExtractNodeTemplateFromAction(PaletteAction))
		{
			// If the node wants to create tooltip text, use that instead, because its probably more detailed
			FText NodeToolTipText = NodeTemplate->GetTooltipText();
			if (!NodeToolTipText.IsEmpty())
			{
				ToolTipText = NodeToolTipText;
			}

			if (UK2Node_CallFunction const* CallFuncNode = Cast<UK2Node_CallFunction const>(NodeTemplate))
			{			
				if(UClass* ParentClass = CallFuncNode->FunctionReference.GetMemberParentClass(CallFuncNode->GetBlueprintClassFromNode()))
				{
					UBlueprint* BlueprintObj = UBlueprint::GetBlueprintFromClass(ParentClass);
					if (BlueprintObj == nullptr)
					{
						ClassDisplayName = ParentClass->GetDisplayNameText();
					}
					else if (!BlueprintObj->HasAnyFlags(RF_Transient))
					{
						ClassDisplayName = FText::FromName(BlueprintObj->GetFName());
					}					
				}
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)PaletteAction.Get();
			if (GraphAction->EdGraph)
			{
				if (const UEdGraphSchema* GraphSchema = GraphAction->EdGraph->GetSchema())
				{
					FGraphDisplayInfo DisplayInfo;
					GraphSchema->GetGraphDisplayInformation(*(GraphAction->EdGraph), DisplayInfo);
					ToolTipText = DisplayInfo.Tooltip;
				}
			}
		}
	}

	if (bShowClassInTooltip && !ClassDisplayName.IsEmpty())
	{
		ToolTipText = FText::Format(LOCTEXT("BlueprintItemClassTooltip", "{0}\nClass: {1}"), ToolTipText, ClassDisplayName);
	}

	return ToolTipText;
}


TSharedPtr<SToolTip> SJointGraphPaletteItem::ConstructToolTipWidget() const
{
	TSharedPtr<FEdGraphSchemaAction> PaletteAction = ActionPtr.Pin();
	UEdGraphNode const* const NodeTemplate = FBlueprintActionMenuUtils::ExtractNodeTemplateFromAction(PaletteAction);

	FBlueprintActionMenuItem::FDocExcerptRef DocExcerptRef;

	if (PaletteAction.IsValid())
	{
		if (NodeTemplate != nullptr)
		{
			// Take rich tooltip from node
			DocExcerptRef.DocLink = NodeTemplate->GetDocumentationLink();
			DocExcerptRef.DocExcerptName = NodeTemplate->GetDocumentationExcerptName();

			// sometimes, with FBlueprintActionMenuItem's, the NodeTemplate 
			// doesn't always reflect the node that will be spawned (some things 
			// we don't want to be executed until spawn time, like adding of 
			// component templates)... in that case, the 
			// FBlueprintActionMenuItem's may have a more specific documentation 
			// link of its own (most of the time, it will reflect the NodeTemplate's)
			if ( !DocExcerptRef.IsValid() && (PaletteAction->GetTypeId() == FBlueprintActionMenuItem::StaticGetTypeId()) )
			{
				FBlueprintActionMenuItem* NodeSpawnerAction = (FBlueprintActionMenuItem*)PaletteAction.Get();
				DocExcerptRef = NodeSpawnerAction->GetDocumentationExcerpt();
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)PaletteAction.Get();
			if (GraphAction->EdGraph)
			{
				FGraphDisplayInfo DisplayInfo;
				if (const UEdGraphSchema* GraphSchema = GraphAction->EdGraph->GetSchema())
				{
					GraphSchema->GetGraphDisplayInformation(*(GraphAction->EdGraph), DisplayInfo);
				}

				DocExcerptRef.DocLink = DisplayInfo.DocLink;
				DocExcerptRef.DocExcerptName = DisplayInfo.DocExcerptName;
			}
		}
	}

	// Setup the attribute for dynamically pulling the tooltip
	TAttribute<FText> TextAttribute;
	TextAttribute.Bind(this, &SJointGraphPaletteItem::GetToolTipText);

	TSharedRef< SToolTip > TooltipWidget = IDocumentation::Get()->CreateToolTip(TextAttribute, nullptr, DocExcerptRef.DocLink, DocExcerptRef.DocExcerptName);

	// English speakers have no real need to know this exists.
	if ( (NodeTemplate != nullptr) && (FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName() != TEXT("en")) )
	{
		FText NativeNodeName = FText::FromString(NodeTemplate->GetNodeTitle(ENodeTitleType::ListView).BuildSourceString());
		const FTextBlockStyle& SubduedTextStyle = FJointEditorStyle::GetUEEditorSlateStyleSet().GetWidgetStyle<FTextBlockStyle>("Documentation.SDocumentationTooltipSubdued");

		TSharedPtr<SToolTip> InternationalTooltip;
		TSharedPtr<SVerticalBox> TooltipBody;

		SAssignNew(InternationalTooltip, SToolTip)
			// Emulate text-only tool-tip styling that SToolTip uses 
			// when no custom content is supplied.  We want node tool-
			// tips to be styled just like text-only tool-tips
			.BorderImage( FCoreStyle::Get().GetBrush("ToolTip.BrightBackground") )
			.TextMargin(FMargin(11.0f))
		[
			SAssignNew(TooltipBody, SVerticalBox)
		];

		if (!DocExcerptRef.IsValid())
		{
			auto GetNativeNamePromptVisibility = []()->EVisibility
			{
				FModifierKeysState KeyState = FSlateApplication::Get().GetModifierKeys();
				return KeyState.IsAltDown() ? EVisibility::Collapsed : EVisibility::Visible;
			};

			TooltipBody->AddSlot()
			[
				SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "Documentation.SDocumentationTooltip")
					.Text(NativeNodeName)
					.Visibility_Lambda([GetNativeNamePromptVisibility]()->EVisibility
					{
						return (GetNativeNamePromptVisibility() == EVisibility::Visible) ? EVisibility::Collapsed : EVisibility::Visible;
					})
			];

			TooltipBody->AddSlot()
			[
				SNew(SHorizontalBox)
					.Visibility_Lambda(GetNativeNamePromptVisibility)
				+SHorizontalBox::Slot()
				[
					TooltipWidget->GetContentWidget()
				]
			];

			TooltipBody->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 8.f, 0.f, 0.f)
			[

				SNew(STextBlock)
					.Text( LOCTEXT("NativeNodeName", "hold (Alt) for native node name") )
					.TextStyle(&SubduedTextStyle)
					.Visibility_Lambda(GetNativeNamePromptVisibility)
			];
		}
		else
		{
			auto GetNativeNodeNameVisibility = []()->EVisibility
			{
				FModifierKeysState KeyState = FSlateApplication::Get().GetModifierKeys();
				return KeyState.IsAltDown() && KeyState.IsControlDown() ? EVisibility::Visible : EVisibility::Collapsed;
			};

			// give the "advanced" tooltip a header
			TooltipBody->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(&SubduedTextStyle)
						.Text(LOCTEXT("NativeNodeNameLabel", "Native Node Name: "))
						.Visibility_Lambda(GetNativeNodeNameVisibility)
				]
				+SHorizontalBox::Slot()
					.AutoWidth()
				[
					SNew(STextBlock)
						.TextStyle(&SubduedTextStyle)
						.Text(NativeNodeName)
						.Visibility_Lambda(GetNativeNodeNameVisibility)
				]
			];

			TooltipBody->AddSlot()
			[
				TooltipWidget->GetContentWidget()
			];
		}

		return InternationalTooltip;
	}
	return TooltipWidget;
}









/*******************************************************************************
* SJointGraphPalette
*******************************************************************************/

//------------------------------------------------------------------------------
void SJointGraphPalette::Construct(const FArguments& InArgs, TWeakPtr<FJointEditorToolkit> InJointEditorToolkit)
{
	const float NumProgressFrames = 2.0f;
	const float SecondsToWaitBeforeShowingProgressDialog = 0.25f;


	
	JointEditorToolkitPtr = InJointEditorToolkit;


	

	FScopedSlowTask SlowTask(NumProgressFrames, LOCTEXT("ConstructingPaletteTabContent", "Initializing Palette..."));
	SlowTask.MakeDialogDelayed(SecondsToWaitBeforeShowingProgressDialog);

	//TODO: replace it with Joint Editor specific configs and settings.
	
	float FavoritesHeightRatio = 0.33f;
	GConfig->GetFloat(*JointEditorPalette::ConfigSection, *JointEditorPalette::FavoritesHeightConfigKey, FavoritesHeightRatio, GEditorPerProjectIni);
	float LibraryHeightRatio = 1.f - FavoritesHeightRatio;
	GConfig->GetFloat(*JointEditorPalette::ConfigSection, *JointEditorPalette::LibraryHeightConfigKey, LibraryHeightRatio, GEditorPerProjectIni);

	bool bUseLegacyLayout = false;
	GConfig->GetBool(*JointEditorPalette::ConfigSection, TEXT("bUseLegacyLayout"), bUseLegacyLayout, GEditorIni);
	
	this->ChildSlot
		[
			SAssignNew(PaletteSplitter, SSplitter)
				.Orientation(Orient_Vertical)
				.OnSplitterFinishedResizing(this, &SJointGraphPalette::OnSplitterResized)
				.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("FullJointEditorPalette")))
		];
}

//------------------------------------------------------------------------------
void SJointGraphPalette::OnSplitterResized() const
{
	FChildren const* const SplitterChildren = PaletteSplitter->GetChildren();
	for (int32 SlotIndex = 0; SlotIndex < SplitterChildren->Num(); ++SlotIndex)
	{
		SSplitter::FSlot const& SplitterSlot = PaletteSplitter->SlotAt(SlotIndex);

		if (SplitterSlot.GetWidget() == FavoritesWrapper)
		{
			GConfig->SetFloat(*JointEditorPalette::ConfigSection, *JointEditorPalette::FavoritesHeightConfigKey, SplitterSlot.GetSizeValue(), GEditorPerProjectIni);
		}
		else if (SplitterSlot.GetWidget() == LibraryWrapper)
		{
			GConfig->SetFloat(*JointEditorPalette::ConfigSection, *JointEditorPalette::LibraryHeightConfigKey, SplitterSlot.GetSizeValue(), GEditorPerProjectIni);
		}

	}
}

#undef LOCTEXT_NAMESPACE
