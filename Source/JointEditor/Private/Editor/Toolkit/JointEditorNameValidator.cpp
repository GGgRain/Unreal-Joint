#include "JointEditorNameValidator.h"

#include "JointEdGraph.h"
#include "JointManager.h"

#include "Kismet2/Kismet2NameValidators.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"

#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "KismetNameValidators"

//////////////////////////////////////////////////
// FKismetNameValidator

namespace JointNameConstants
{
	int32 NameMaxLength = 100;
}

FJointEditorNameValidator::FJointEditorNameValidator(UObject* InOuter, FName InExistingName) :
	Outer(InOuter), ExistingName(InExistingName)
{
}

FJointEditorNameValidator::~FJointEditorNameValidator()
{
	
}

int32 FJointEditorNameValidator::GetMaximumNameLength()
{
	return JointNameConstants::NameMaxLength;
}


/*
 
void SanitizeName(FString& InOutName)
{
	// Sanitize the name
	for (int32 i = 0; i < InOutName.Len(); ++i)
	{
		TCHAR& C = InOutName[i];

		const bool bGoodChar = FChar::IsAlpha(C) || // Any letter
			(C == '_') || (C == '-') || (C == '.') || // _  - . anytime
			((i > 0) && (FChar::IsDigit(C))) || // 0-9 after the first character
			((i > 0) && (C == ' ')); // Space after the first character to support virtual bones

		if (!bGoodChar)
		{
			C = '_';
		}
	}

	if (InOutName.Len() > FKismetNameValidator::GetMaximumNameLength())
	{
		InOutName.LeftChopInline(InOutName.Len() - FKismetNameValidator::GetMaximumNameLength());
	}
}

FName UJointEdGraphNode::GetSafeNewName(const FString& InPotentialNewName, UObject* Outer) const
{
	FString SanitizedName = InPotentialNewName;
	SanitizeName(SanitizedName);
	FString Name = SanitizedName;

	int32 Suffix = 1;
	while (StaticFindObject(nullptr, Outer, *Name, true))
	{
		FString BaseString = SanitizedName;


		FString LString, RString;

		//if we found it as numeric, then trim it.
		if (BaseString.Split("_", &LString, &RString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && RString.
			IsNumeric())
		{
			BaseString = LString;
		}

		if (BaseString.Len() > FKismetNameValidator::GetMaximumNameLength() - 4)
		{
			BaseString.LeftChopInline(BaseString.Len() - (FKismetNameValidator::GetMaximumNameLength() - 4));
		}
		Name = *FString::Printf(TEXT("%s_%d"), *BaseString, ++Suffix);
	}

	return *Name;
}
*/

EValidatorResult FJointEditorNameValidator::FindValidStringPruningSuffixes(FString& InOutName)
{
	/**
	 * Try to find a valid string by appending a number suffix to the base name.
	 * if the given name has a suffix already, it will be removed first.
	 * e.g. "MyName_2" will be treated as "MyName"
	 * @param InOutName 
	 * @return 
	 */
	FString DesiredName = InOutName;
	int32 NameIndex = INDEX_NONE;

	SplitNameIntoBaseAndNumericSuffix(DesiredName, DesiredName, NameIndex);
	
	FString NewName = InOutName;
	
	while (true)
	{
		EValidatorResult VResult = IsValid(NewName, true);

		if (VResult == EValidatorResult::Ok)
		{
			InOutName = NewName;
			return EValidatorResult::Ok;
		}else if (VResult != EValidatorResult::AlreadyInUse) 
		{
			// If the name is not valid for any other reason then pre-occupication, just return that result - no point trying to append numbers
			return VResult;
		}

		NewName = FString::Printf(TEXT("%s_%d"), *DesiredName, NameIndex++);
	}
}



EValidatorResult FJointEditorNameValidator::IsValid(const FString& Name, bool /*bOriginal*/)
{
	FText Dummy;
	return Validate(Name, Dummy);
}

EValidatorResult FJointEditorNameValidator::IsValid(const FName& Name, bool /* bOriginal */)
{
	FText Dummy;
	return Validate(Name, Dummy);
}

EValidatorResult FJointEditorNameValidator::Validate(const FString& InName, FText& OutErrorMessage)
{
	// Converting a string that is too large for an FName will cause an assert, so verify the length
	if (InName.Len() >= NAME_SIZE)
	{
		OutErrorMessage = LOCTEXT("NameTooLongError", "Name is too long.");
		return EValidatorResult::TooLong;
	}

	// If not defined in name table, not current graph name
	return Validate(FName(*InName), OutErrorMessage);
}

EValidatorResult FJointEditorNameValidator::Validate(const FName& InName, FText& OutErrorMessage)
{
	EValidatorResult ValidatorResult = EValidatorResult::AlreadyInUse;

	if (InName == NAME_None)
	{
		OutErrorMessage = LOCTEXT("EmptyNameError", "Name cannot be empty.");
		ValidatorResult = EValidatorResult::EmptyName;
	}
	else if (InName == ExistingName)
	{
		ValidatorResult = EValidatorResult::Ok;
	}else if ( !InName.IsValidObjectName(OutErrorMessage))
	{
		ValidatorResult = EValidatorResult::ContainsInvalidCharacters;
	}
	else if (InName.ToString().Len() > JointNameConstants::NameMaxLength)
	{
		OutErrorMessage = LOCTEXT("NameTooLongError", "Name is too long.");
		ValidatorResult = EValidatorResult::TooLong;
	}
	else
	{
		if (!Names.Contains(InName))
		{
			// If it is in the names list then it is already in use.
			ValidatorResult = EValidatorResult::Ok;

			// Check for collision with an existing object.
#if UE_VERSION_OLDER_THAN(5, 7, 0)
			if (UObject* ExistingObject = StaticFindObject(NULL, const_cast<UObject*>(Outer), *InName.ToString(), false))
#else
			if (UObject* ExistingObject = StaticFindObject(NULL, const_cast<UObject*>(Outer), *InName.ToString(), EFindObjectFlags::None))
#endif
			{
				// To allow the linker to resolve imports when dependent Blueprints are loaded, macro libraries
				// will leave behind a redirector whenever one of its macro graph objects are renamed. These get
				// moved aside to allow their names to be recycled, so we consider a collision with an existing
				// redirector object in that case to be a false positive.
				// 
				// Note that while the linker will resolve references to either a graph or redirector by name on load,
				if (!ExistingObject->IsA<UObjectRedirector>())
				{
					OutErrorMessage = LOCTEXT("NameAlreadyInUseError", "Name is already in use by another object.");
					ValidatorResult = EValidatorResult::AlreadyInUse;
				}
			}	
		}

		if (ValidatorResult == EValidatorResult::Ok)
		{
			if (Cast<UJointManager>(Outer) == nullptr)
			{
				return ValidatorResult;
			}
			
			if (Cast<UJointManager>(Outer)->GetJointGraphAs())
			{

				TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Cast<UJointManager>(Outer));

				for (UJointEdGraph* JointEdGraph : Graphs) {

					if (!JointEdGraph) continue;
					
					TSet<TWeakObjectPtr<UJointEdGraphNode>> Nodes = JointEdGraph->GetCachedJointGraphNodes(true);

					for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : Nodes)
					{
						if (JointEdGraphNode.IsValid())
						{
							if (JointEdGraphNode->GetFName() == InName)
							{
								OutErrorMessage = LOCTEXT("NameAlreadyInUseError", "Name is already in use by another object.");
								ValidatorResult = EValidatorResult::AlreadyInUse;
								break;
							}
						}
						
					}
				}
			}
		}
	}

	return ValidatorResult;
}

TSharedPtr<INameValidatorInterface> FJointEditorNameValidator::MakeValidator(UJointManager* InJointManager, FName InExistingName)
{
	return MakeShareable(new FJointEditorNameValidator(InJointManager, InExistingName));
}

bool FJointEditorNameValidator::SplitNameIntoBaseAndNumericSuffix(const FString& InName, FString& OutBaseName, int32& OutNumericSuffix)
{
	FString LString, RString;
	//if we found it as numeric, then trim it.
	if (InName.Split("_", &LString, &RString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && RString.IsNumeric())
	{
		OutBaseName = LString;

		OutNumericSuffix = FCString::Atoi(*RString);

		return true;
	}

	return false;

}

#undef LOCTEXT_NAMESPACE
