//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

/**
 * Implements an action for Joint manager assets.
 */
class JOINTEDITOR_API FJointManagerActions : public FAssetTypeActions_Base
{
public:

	/**
	 * Creates and initializes a new instance. Get the category from the toolkit and use it at the same time.
	 */
	FJointManagerActions(EAssetTypeCategories::Type InAssetCategory);

public:
	
	virtual bool CanFilter() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	
public:
	
	static void StoreEditorModuleClassCache();

public:

	//Category of the asset that this asset will be displayed at on the content browser and elsewhere.
	EAssetTypeCategories::Type Category;
	
};
