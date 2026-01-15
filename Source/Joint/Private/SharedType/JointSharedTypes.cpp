//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "SharedType/JointSharedTypes.h"

#include "JointManager.h"
#include "Node/JointNodeBase.h"
#include "Misc/MessageDialog.h"
#include "Misc/UObjectToken.h"

#define LOCTEXT_NAMESPACE "FJointNodePointer"


FJointActorExecutionElement::FJointActorExecutionElement() : ExecutionType(EJointActorExecutionType::None)
{
}

FJointActorExecutionElement::FJointActorExecutionElement(const EJointActorExecutionType InExecutionType, UJointNodeBase* InTargetNode) : ExecutionType(InExecutionType), TargetNode(InTargetNode)
{
}

FJointGraphNodePropertyData::FJointGraphNodePropertyData() : PropertyName(NAME_None)
{
}

FJointGraphNodePropertyData::FJointGraphNodePropertyData(const FName& InPropertyName) : PropertyName(InPropertyName)
{
}

FJointNodePointer::FJointNodePointer()
#if WITH_EDITORONLY_DATA
	:
	Node(nullptr),
	EditorNode(nullptr)
#endif
{
}

FJointNodePointer::~FJointNodePointer()
{
}

bool FJointNodePointer::operator==(const FJointNodePointer& Other) const
{
	return Node.ToSoftObjectPath() == Other.Node.ToSoftObjectPath() && HasSameRestrictionsAs(Other);
}


bool FJointNodePointer::HasSameRestrictionsAs(const FJointNodePointer& Other) const
{
	//check if the allowed types and disallowed types are the same.
	return AllowedType.Num() == Other.AllowedType.Num() && DisallowedType.Num() == Other.DisallowedType.Num() &&
		[&]() {
			for (const TSubclassOf<UJointNodeBase>& Type : AllowedType)
			{
				if (!Other.AllowedType.Contains(Type)) return false;
			}
			return true;
	}() &&
	[&]() {
		for (const TSubclassOf<UJointNodeBase>& Type : DisallowedType)
		{
			if (!Other.DisallowedType.Contains(Type)) return false;
		}
		return true;
	}();
}

bool FJointNodePointer::CheckMatchRestrictions(const TSet<TSubclassOf<UJointNodeBase>>& AllowedClass, const TSet<TSubclassOf<UJointNodeBase>>& DisallowedClasses) const
{
	//check if the allowed types and disallowed types are the same.
	return AllowedType.Num() == AllowedClass.Num() && DisallowedType.Num() == DisallowedClasses.Num() &&
		[&]() {
			for (const TSubclassOf<UJointNodeBase>& Type : AllowedType)
			{
				if (!AllowedClass.Contains(Type)) return false;
			}
			return true;
		}() &&
		[&]() {
			for (const TSubclassOf<UJointNodeBase>& Type : DisallowedType)
			{
				if (!DisallowedClasses.Contains(Type)) return false;
			}
			return true;
		}();
}


bool FJointNodePointer::IsValid() const
{
	return Node.IsValid();
}

FJointNodes::FJointNodes()
{
}

FJointNodes::FJointNodes(const TArray<UJointNodeBase*>& InNodes): Nodes(InNodes)
{
}

bool FJointEdPinData::HasSameSignature(const FJointEdPinData& Other) const
{
	return PinName == Other.PinName && Direction == Other.Direction && Type == Other.Type;
}

void FJointEdPinData::CopyPropertiesFrom(const FJointEdPinData& Other)
{
	PinName = Other.PinName;
	Direction = Other.Direction;
	Type = Other.Type;
	Settings = Other.Settings;
}

bool FJointEdPinData::operator==(const FJointEdPinData& Other) const
{
	return HasSameSignature(Other)
		&& ImplementedPinId == Other.ImplementedPinId;
}

bool operator!=(const FJointEdPinData& A, const FJointEdPinData& B)
{
	return !(A == B);
}


IJointEdNodeInterface::IJointEdNodeInterface()
{
}

#if WITH_EDITOR


bool FJointNodePointer::CanSetNodeOnProvidedJointNodePointer(FJointNodePointer Structure,
                                                             UJointNodeBase* Node)
{
	if (!Structure.AllowedType.IsEmpty() && !Structure.AllowedType.Contains(Node->GetClass())) return false;

	if (!Structure.DisallowedType.IsEmpty() && Structure.DisallowedType.Contains(Node->GetClass())) return false;

	return true;
}

TArray<TSharedPtr<FTokenizedMessage>> FJointNodePointer::GetCompilerMessage(
	FJointNodePointer& Structure, UJointNodeBase* Node, const FString& OptionalStructurePropertyName = "")
{
	TArray<TSharedPtr<FTokenizedMessage>> Messages;

	UJointManager* JointManager = Node->GetJointManager();

	//If it doesn't have any path, return nothing.
	if (Structure.Node.ToSoftObjectPath().IsNull()) return Messages;

	if (Structure.Node.IsValid())
	{
		if (UJointNodeBase* NodeInstance = Structure.Node.Get())
		{
			const UJointManager* NodeInstanceJointManager = NodeInstance->GetJointManager();

			if (!NodeInstanceJointManager)
			{
				FText Message = LOCTEXT("NoJointManager",
				                        "The node that has been referenced doesn't have valid Joint manager outer. This might be caused because the node it refers has been removed.");

				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
				TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
				TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
				TokenizedMessage->AddToken(FUObjectToken::Create(Node));
				if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
					FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
				TokenizedMessage->AddToken(FTextToken::Create(Message));
				TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

				Messages.Add(TokenizedMessage);
			}
			else if (NodeInstanceJointManager != JointManager)
			{
				FText Message1 = LOCTEXT("NotOnSameJointManager1",
				                         "The node that has been referenced is not originated in the same Joint manager asset. (");
				FText Message2 = LOCTEXT("NotOnSameJointManager2",
				                         ") We do not allow such action due to the security concern and memory allocation.");


				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
				TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
				TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
				TokenizedMessage->AddToken(FUObjectToken::Create(Node));
				if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
					FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
				TokenizedMessage->AddToken(FTextToken::Create(Message1));
				TokenizedMessage->AddToken(
					FAssetNameToken::Create(NodeInstanceJointManager ? NodeInstanceJointManager->GetName() : "NONE"));
				TokenizedMessage->AddToken(
					FTextToken::Create(FText::FromString(" : " + NodeInstanceJointManager->GetPathName())));
				TokenizedMessage->AddToken(FTextToken::Create(Message2));

				TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

				Messages.Add(TokenizedMessage);
			}
			else if (!CanSetNodeOnProvidedJointNodePointer(Structure, NodeInstance))
			{
				FText Message = LOCTEXT("CannotSetThatNode",
				                        "This type of node instance is not allowed to be referenced in this node pointer. Check AllowedType, DisallowedType of the node pointer structure.");

				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
				TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
				TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
				TokenizedMessage->AddToken(FUObjectToken::Create(Node));
				if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
					FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
				TokenizedMessage->AddToken(FTextToken::Create(Message));
				TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

				Messages.Add(TokenizedMessage);
			}else
			{

				//If target node's build target is provided, check if
				if(!Structure.Node->GetBuildPreset().IsNull())
				{
					UJointBuildPreset* TargetNodeBuildTargetPreset = Structure.Node->GetBuildPreset().LoadSynchronous();
					
					if(!Node->GetBuildPreset().IsNull())
					{
						UJointBuildPreset* ParentBuildTargetPreset = Node->GetBuildPreset().LoadSynchronous();

						if(TargetNodeBuildTargetPreset == ParentBuildTargetPreset) return Messages;
	
					}

					FText Message = LOCTEXT("DifferentBuildTarget",
											"This node pointer refers to a node that has different build preset. Be especially careful with the usage.");

					TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Warning);
					TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
					TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
					TokenizedMessage->AddToken(FUObjectToken::Create(Node));
					if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
						FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
					TokenizedMessage->AddToken(FTextToken::Create(Message));
					TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

					Messages.Add(TokenizedMessage);
					
				}
		
				return Messages;
			}
		}
		else
		{
			FText Message = LOCTEXT("NotJointNode", "This node pointer has an object that is not inherited UJointNodeBase.");

			TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
			TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
			TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
			TokenizedMessage->AddToken(FUObjectToken::Create(Node));
			if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
				FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
			TokenizedMessage->AddToken(FTextToken::Create(Message));
			TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

			Messages.Add(TokenizedMessage);
		}
	}
	else if (Structure.EditorNode.Get())
	{

		IJointEdNodeInterface* Interface = Cast<IJointEdNodeInterface>(Structure.EditorNode.Get());

		if(!Interface)
		{
			FText Message = LOCTEXT("NotValid_HasEditorNode_NoJointEdNodeInterface", "The object this node pointer is pointing isn't a valid object, and It seems like the graph node isn't inherited from UJointEdGraphNode either. This is not allowed.");

			TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
			TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
			TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
			TokenizedMessage->AddToken(FUObjectToken::Create(Node));
			if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
				FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
			TokenizedMessage->AddToken(FTextToken::Create(Message));
			TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

			Messages.Add(TokenizedMessage);
		}else
		{
			FText Message = LOCTEXT("NotValid_HasEditorNode_JointEdNodeInterface", "The object this node pointer is pointing isn't a valid object, But has an editor node reference for the target node. Please read and check out the popup.");

			TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
			TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
			TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
			TokenizedMessage->AddToken(FUObjectToken::Create(Node));
			if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
				FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
			TokenizedMessage->AddToken(FTextToken::Create(Message));
			TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

			Messages.Add(TokenizedMessage);


			//open a dialog warning message.
			
			FText DialogMessage = LOCTEXT("NotValid_HasEditorNode_JointEdNodeInterface_Dialog",
				"The object that this node pointer is pointing isn't a valid object, But has an editor node reference for the target node."
				"\nWould you try to retrieve the node instance reference from the editor node?");
			
			FText DialogMessageNo = LOCTEXT("NotValid_HasEditorNode_JointEdNodeInterface_Dialog_No", "Please update the property manually (either clear out, or feed a valid one instead of the corrupted data) \nCheck out the log for the further details.");

			FText DialogMessageYes_Succeeded = LOCTEXT("NotValid_HasEditorNode_JointEdNodeInterface_Dialog_Yes_Succeed", "Retrieval Succeeded. Please save the asset, and if you were packaging or cooking the project, please try it again.");
			FText DialogMessageYes_Failed = LOCTEXT("NotValid_HasEditorNode_JointEdNodeInterface_Dialog_Yes_Fail", "Retrieval Failed. Please update the property manually.");

			
			switch (FMessageDialog::Open(EAppMsgType::YesNo,DialogMessage))
			{
			case EAppReturnType::No:
				
				FMessageDialog::Open(EAppMsgType::Ok, DialogMessageNo);
				
				break;
			case EAppReturnType::Yes:

				if(UObject* InNode = Interface->JointEdNodeInterface_GetNodeInstance())
				{
					if(UJointNodeBase* CastedNode = Cast<UJointNodeBase>(InNode))
					{
						Structure.Node = CastedNode;

						FMessageDialog::Open(EAppMsgType::Ok, DialogMessageYes_Succeeded);

						break;
					}
				}

				FMessageDialog::Open(EAppMsgType::Ok, DialogMessageYes_Failed);
				
				break;
			default: break;
			}
		}
	}
	else
	{
		FText Message = LOCTEXT("NotValid", "The object this node pointer is pointing isn't a valid object. This might be caused because the node it refers has been removed.");

		TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
		TokenizedMessage->AddToken(FAssetNameToken::Create(JointManager ? JointManager->GetName() : "NONE"));
		TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
		TokenizedMessage->AddToken(FUObjectToken::Create(Node));
		if (!OptionalStructurePropertyName.IsEmpty()) TokenizedMessage->AddToken(
			FTextToken::Create(FText::FromString(", " + OptionalStructurePropertyName + " :")));
		TokenizedMessage->AddToken(FTextToken::Create(Message));
		TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(Node));

		Messages.Add(TokenizedMessage);
	}

	return Messages;
}

#endif

FJointEdNodeSetting::FJointEdNodeSetting()
{
	
#if WITH_EDITORONLY_DATA

	IconicNodeImageBrush = FSlateBrush();
	IconicNodeImageBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	IconicNodeImageBrush.ImageSize = FVector2D(2,2);

#endif

}


void FJointEdNodeSetting::UpdateFromNode(const UJointNodeBase* Node)
{
	if(!Node) return;

#if WITH_EDITORONLY_DATA
	
	IconicNodeImageBrush = Node->IconicNodeImageBrush;
	bUseSpecifiedGraphNodeBodyColor = Node->bUseSpecifiedGraphNodeBodyColor;
	NodeBodyColor = Node->NodeBodyColor;
	bAllowDisplayClassFriendlyNameText = Node->bAllowDisplayClassFriendlyNameText;
	bUseSimplifiedDisplayClassFriendlyNameText = Node->bUseSimplifiedDisplayClassFriendlyNameText;
	SimplifiedClassFriendlyNameText = Node->SimplifiedClassFriendlyNameText;
	DefaultEdSlateDetailLevel = Node->DefaultEdSlateDetailLevel;
	PropertyDataForSimpleDisplayOnGraphNode = Node->PropertyDataForSimpleDisplayOnGraphNode;
	bAllowEditingOfPinDataOnDetailsPanel = Node->bAllowNodeInstancePinControl;
	
#endif
}

FJointEdPinDataSetting::FJointEdPinDataSetting()
{

}

UObject* IJointEdNodeInterface::JointEdNodeInterface_GetNodeInstance()
{
	return nullptr;
}

TArray<FJointEdPinData> IJointEdNodeInterface::JointEdNodeInterface_GetPinData()
{
	TArray<FJointEdPinData> Array;

#if WITH_EDITOR

	//Bubbled in the default implementation.

#endif

	return Array;
}


//Keep this "Dialogue", otherwise it will mess up the system.

const FEdGraphPinType FJointEdPinData::PinType_Joint_Normal(
	FEdGraphPinType(
		FName("PC_Joint_Normal"),
		NAME_None,
		nullptr,
		EPinContainerType::None,
		false,
		FEdGraphTerminalType()
	)
);

//Keep this "Dialogue", otherwise it will mess up the system.

const FEdGraphPinType FJointEdPinData::PinType_Joint_SubNodeParent(
	FEdGraphPinType(
		FName("PC_Joint_SubNodeParent"),
		NAME_None,
		nullptr,
		EPinContainerType::None,
		false,
		FEdGraphTerminalType()
	)
);


const FJointEdPinData FJointEdPinData::PinData_Null = FJointEdPinData();

FJointEdPinData::FJointEdPinData():
	PinName(NAME_None),
	Direction(EEdGraphPinDirection::EGPD_Input),
	Type(PinType_Joint_Normal)
{
}

FJointEdPinData::FJointEdPinData(
	const FName& InPinName,
	const EEdGraphPinDirection& InDirection):
		PinName(InPinName),
		Direction(InDirection),
		Type(PinType_Joint_Normal)
{
}

FJointEdPinData::FJointEdPinData(
	const FName& InPinName,
	const EEdGraphPinDirection& InDirection,
	const FJointEdPinDataSetting& InSettings) :
		PinName(InPinName),
		Direction(InDirection),
		Type(PinType_Joint_Normal),
		Settings(InSettings)
{
}

FJointEdPinData::FJointEdPinData(
	const FName& InPinName,
	const EEdGraphPinDirection& InDirection,
	const FEdGraphPinType& InType):
		PinName(InPinName),
		Direction(InDirection),
		Type(InType)
{
}

FJointEdPinData::FJointEdPinData(
	const FName& InPinName,
	const EEdGraphPinDirection& InDirection,
	const FEdGraphPinType& InType,
	const FJointEdPinDataSetting& InSettings):
		PinName(InPinName),
		Direction(InDirection),
		Type(InType),
		Settings(InSettings)
{
}

FJointEdPinData::FJointEdPinData(
	const FName& InPinName,
	const EEdGraphPinDirection& InDirection,
	const FEdGraphPinType& InType,
	const FJointEdPinDataSetting& InSettings,
	const FGuid& InImplementedPinGuid):
		PinName(InPinName),
		Direction(InDirection),
		Type(InType),
		Settings(InSettings),
		ImplementedPinId(InImplementedPinGuid)
{
}


FJointEdPinData::FJointEdPinData(const FJointEdPinData& Other) :
	PinName(Other.PinName),
	Direction(Other.Direction),
	Type(Other.Type),
	Settings(Other.Settings),
	ImplementedPinId(Other.ImplementedPinId)
{
}


FJointEdLogMessage::FJointEdLogMessage(): Severity(EJointEdMessageSeverity::Type::Info)
{
}

FJointEdLogMessage::FJointEdLogMessage(const EJointEdMessageSeverity::Type& InVerbosity, const FText& InMessage):
	Severity(InVerbosity), Message(InMessage)
{
}


#undef LOCTEXT_NAMESPACE
