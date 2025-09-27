#pragma once

#include "CoreMinimal.h"
#include "Kismet2/Kismet2NameValidators.h"

class UJointManager;

class JOINTEDITOR_API FJointEditorNameValidator : public INameValidatorInterface
{
public:
	
	FJointEditorNameValidator(UObject* InOuter, FName InExistingName = NAME_None);
	virtual ~FJointEditorNameValidator() override;

	/** Return the name validator maximum string length */
	static int32 GetMaximumNameLength();

	// Begin FNameValidatorInterface
	virtual EValidatorResult IsValid( const FString& Name, bool bOriginal = false) override;
	virtual EValidatorResult IsValid( const FName& Name, bool bOriginal = false) override;
	// End FNameValidatorInterface

public:

	EValidatorResult Validate(const FString& InName, FText& OutErrorMessage);
	EValidatorResult Validate(const FName& InName, FText& OutErrorMessage);

public:

	/**
	 * Try to find a valid string by appending a number suffix to the base name.
	 * if the given name has a suffix already, it will be removed first.
	 * e.g. "MyName_2" will be treated as "MyName"
	 * @param InOutName 
	 * @return 
	 */
	EValidatorResult FindValidStringPruningSuffixes(FString& InOutName);
	
private:
	
	/** Name set to validate */
	TSet<FName> Names;
	
	/** The blueprint to check for validity within */
	const UObject* Outer;
	
	/** The current name of the object being validated */
	FName ExistingName;


public:

	static TSharedPtr<INameValidatorInterface> MakeValidator(UJointManager* InJointManager, FName InExistingName);

	static bool SplitNameIntoBaseAndNumericSuffix(const FString& InName, FString& OutBaseName, int32& OutNumericSuffix);

	
};
