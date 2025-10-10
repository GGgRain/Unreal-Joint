//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEditorStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Brushes/SlateImageBrush.h"

#include "Engine/Texture2D.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"
#include "UObject/Package.h"

TSharedPtr<ISlateStyle> FJointEditorStyle::Instance = nullptr;


void FJointEditorStyle::ResetToDefault() { SetStyleSet(FJointEditorStyle::Create()); }

const ISlateStyle& FJointEditorStyle::GetUEEditorSlateStyleSet()
{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	return FEditorStyle::Get();
#else
	return FAppStyle::Get();
#endif
}

const FName FJointEditorStyle::GetUEEditorSlateStyleSetName()
{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	return FEditorStyle::GetStyleSetName();
#else
	return FAppStyle::GetAppStyleSetName();
#endif
}

void FJointEditorStyle::SetStyleSet(const TSharedRef<ISlateStyle>& NewStyle)
{
	if (Instance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Instance.Get());
	}
	else if (const ISlateStyle* StyleSet = FSlateStyleRegistry::FindSlateStyle(GetStyleSetName()))
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	}

	Instance = NewStyle;

	if (Instance.IsValid()) { FSlateStyleRegistry::RegisterSlateStyle(*Instance.Get()); }
	else { ResetToDefault(); }
}


FName FJointEditorStyle::GetStyleSetName()
{
	return TEXT("JointStyle");
}


const FMargin FJointEditorStyle::Margin_Large(FMargin(6));
const FMargin FJointEditorStyle::Margin_Normal(FMargin(4));
const FMargin FJointEditorStyle::Margin_Small(FMargin(2));
const FMargin FJointEditorStyle::Margin_Tiny(FMargin(1));

const FMargin FJointEditorStyle::Margin_Shadow(FMargin(4.5));
const FMargin FJointEditorStyle::Margin_SubNode(FMargin(1.5));


const FLinearColor FJointEditorStyle::Color_Normal(0.006, 0.006, 0.006);
const FLinearColor FJointEditorStyle::Color_Hover(0.1, 0.1, 0.15);
const FLinearColor FJointEditorStyle::Color_Selected(0.2, 0.2, 0.25);
const FLinearColor FJointEditorStyle::Color_Disabled(0, 0, 0, 0);

const FLinearColor FJointEditorStyle::Color_SolidNormal(1, 1, 1);
const FLinearColor FJointEditorStyle::Color_SolidHover(1, 1, 1, 0.7);
const FLinearColor FJointEditorStyle::Color_SolidSelected(1, 1, 1, 0.8);


const FLinearColor FJointEditorStyle::Color_Node_TabBackground(0.009, 0.009, 0.012);
const FLinearColor FJointEditorStyle::Color_Node_Inactive(0.08f, 0.08f, 0.08f);
const FLinearColor FJointEditorStyle::Color_Node_Selected(0.75f, 0.75f, 1.00f);
const FLinearColor FJointEditorStyle::Color_Node_Invalid(1.f, 0.f, 0.f);
const FLinearColor FJointEditorStyle::Color_Node_Shadow(FColor(0, 0, 0, 90));

const FLinearColor FJointEditorStyle::Color_Node_DragMarker(1.0f, 1.0f, 0.2f);

const FVector2D Icon128x128(128.f, 128.f);
const FVector2D Icon64x64(64.f, 64.f);
const FVector2D Icon48x48(48.0f, 48.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon24x24(24.0f, 24.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon12x12(12.0f, 12.0f);
const FVector2D Icon8x8(8.f, 8.f);


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ISlateStyle> FJointEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Joint"));
	check(Plugin.IsValid());
	if (Plugin.IsValid())
	{
		Style->SetContentRoot(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));
	}

	// Set the Images of the properties to be equal of our new images, finding the property names can be a bit tricky however.
	Style->Set("ClassThumbnail.JointManager", new Joint_IMAGE_BRUSH("Icon/Ico_Manager", Icon128x128));
	Style->Set("ClassIcon.JointManager", new Joint_IMAGE_BRUSH("Icon/Ico_Manager", Icon16x16));
	Style->Set("ClassThumbnail.JointFragment", new Joint_IMAGE_BRUSH("Icon/Ico_Fragment", Icon128x128));
	Style->Set("ClassIcon.JointFragment", new Joint_IMAGE_BRUSH("Icon/Ico_Fragment", Icon16x16));


	//Pin Images
	Style->Set("Pin.InOut", new Joint_IMAGE_BRUSH("Graph/Pin/InOut_24x", Icon24x24));
	Style->Set("Pin.SelectAnswerOut", new Joint_IMAGE_BRUSH("Graph/Pin/SelectAnswerOut_24x", Icon24x24));
	Style->Set("Pin.RandomOut", new Joint_IMAGE_BRUSH("Graph/Pin/RandomOut_24x", Icon24x24));

	//UX Border 

	Style->Set("JointUI.Border.Empty", new FSlateRoundedBoxBrush(FLinearColor::Transparent, 0.f));
	Style->Set("JointUI.Border.Solid", new FSlateRoundedBoxBrush(FLinearColor::White, 0.f));
	Style->Set("JointUI.Border.Round", new FSlateRoundedBoxBrush(FLinearColor::White, 4.f));
	Style->Set("JointUI.Border.Sphere", new FSlateRoundedBoxBrush(FLinearColor::White, 12.f));

	Style->Set("JointUI.Border.NodeShadow", new Joint_BOX_BRUSH("Graph/Node/NodeShadowX48", Icon64x64, FMargin(0.12f)));
	Style->Set("JointUI.Border.NodeShadowSphere",
	           new Joint_BOX_BRUSH("Graph/Node/NodeShadowSphereX48", Icon64x64, FMargin(0.12f)));

	//Image

	Style->Set("JointUI.Image.JointManager", new Joint_IMAGE_BRUSH("Icon/Ico_Manager", Icon48x48));
	Style->Set("JointUI.Image.JointFragment", new Joint_IMAGE_BRUSH("Icon/Ico_Fragment", Icon48x48));
	Style->Set("JointUI.Image.JointFragment.Small", new Joint_IMAGE_BRUSH("Icon/Ico_Fragment", Icon16x16));

	Style->Set("JointUI.Image.Joint", new Joint_IMAGE_BRUSH("Joint", FVector2D(156,74)));
	Style->Set("JointUI.Image.Volt", new Joint_IMAGE_BRUSH("Volt", FVector2D(166,66)));

	if (UTexture2D* DynamicTextureLoad = LoadObject<UTexture2D>(GetTransientPackage(),
	                                                            TEXT(
		                                                            "Texture2D'/Engine/ArtTools/RenderToTexture/Textures/T_EV_BlankWhite_01.T_EV_BlankWhite_01'")))
	{
		Style->Set("JointUI.Image.GraphBackground",
		           new FSlateImageBrush(DynamicTextureLoad, Icon8x8, FLinearColor(1, 1, 1, 1)));
	}

	//TextBlock In editor
	Joint_AssignTextBlockStyle(Regular_h1, "JointUI.TextBlock.Regular.h1", "Regular", 13, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Regular_h2, "JointUI.TextBlock.Regular.h2", "Regular", 11, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Regular_h3, "JointUI.TextBlock.Regular.h3", "Regular", 9, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Regular_h4, "JointUI.TextBlock.Regular.h4", "Regular", 8, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Regular_h5, "JointUI.TextBlock.Regular.h5", "Regular", 7, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Regular_h6, "JointUI.TextBlock.Regular.h6", "Regular", 5, FLinearColor(0.9,0.9,0.9,1))

	Joint_AssignTextBlockStyle(Black_h1, "JointUI.TextBlock.Black.h1", "Black", 13, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Black_h2, "JointUI.TextBlock.Black.h2", "Black", 11, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Black_h3, "JointUI.TextBlock.Black.h3", "Black", 9, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Black_h4, "JointUI.TextBlock.Black.h4", "Black", 8, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Black_h5, "JointUI.TextBlock.Black.h5", "Black", 7, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Black_h6, "JointUI.TextBlock.Black.h6", "Black", 5, FLinearColor(0.9,0.9,0.9,1))

	Joint_AssignTextBlockStyle(Italic_h1, "JointUI.TextBlock.Italic.h1", "Italic", 13, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Italic_h2, "JointUI.TextBlock.Italic.h2", "Italic", 11, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Italic_h3, "JointUI.TextBlock.Italic.h3", "Italic", 9, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Italic_h4, "JointUI.TextBlock.Italic.h4", "Italic", 8, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Italic_h5, "JointUI.TextBlock.Italic.h5", "Italic", 7, FLinearColor(0.9,0.9,0.9,1))
	Joint_AssignTextBlockStyle(Italic_h6, "JointUI.TextBlock.Italic.h6", "Italic", 5, FLinearColor(0.9,0.9,0.9,1))


	FTextBlockStyle NodeRenameTextBlockStyle = Regular_h4;
	NodeRenameTextBlockStyle.ColorAndOpacity = FLinearColor(0.9, 0.9, 0.9, 0.7);
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_NodeTitleInlineEditableText, "JointUI.InlineEditableTextBlock.NodeTitleInlineEditableText", NodeRenameTextBlockStyle)

	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h1, "JointUI.InlineEditableTextBlock.Regular.h1", Regular_h1)
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h2, "JointUI.InlineEditableTextBlock.Regular.h2", Regular_h2)
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h3, "JointUI.InlineEditableTextBlock.Regular.h3", Regular_h3)
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h4, "JointUI.InlineEditableTextBlock.Regular.h4", Regular_h4)
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h5, "JointUI.InlineEditableTextBlock.Regular.h5", Regular_h5)
	Joint_AssignInlineEditableTextBlockStyle(InlineEditableTextBlock_Regular_h6, "JointUI.InlineEditableTextBlock.Regular.h6", Regular_h6)

	

	const FEditableTextStyle EditableTextStyle = FEditableTextStyle().SetFont(Black_h5.Font);

	Style->Set("JointUI.EditableText.Tag", EditableTextStyle);

	// FEditableTextBoxStyle

	FEditableTextBoxStyle EditableTextBoxStyle = FEditableTextBoxStyle()
		.SetPadding(0);

	Style->Set("JointUI.EditableTextBoxStyle.ZeroPadding", EditableTextStyle);

	//Buttons
	{
		const FButtonStyle JointEmptyButton = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(FLinearColor::Transparent, 0))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor::Transparent, 0))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor::Transparent, 0))
			.SetNormalPadding(FMargin(0))
			.SetPressedPadding(FMargin(0));

		Style->Set("JointUI.Button.Empty", JointEmptyButton);


		const FButtonStyle JointRoundButton = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Color_Normal, 4))
			.SetHovered(FSlateRoundedBoxBrush(Color_Hover, 4))
			.SetPressed(FSlateRoundedBoxBrush(Color_Selected, 4))
			.SetNormalPadding(FMargin(0))
			.SetPressedPadding(FMargin(0));

		Style->Set("JointUI.Button.Round", JointRoundButton);

		const FButtonStyle JointRoundSolidButton = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Color_SolidNormal, 4))
			.SetHovered(FSlateRoundedBoxBrush(Color_SolidHover, 4))
			.SetPressed(FSlateRoundedBoxBrush(Color_SolidSelected, 4))
			.SetNormalPadding(FMargin(0))
			.SetPressedPadding(FMargin(0));

		Style->Set("JointUI.Button.RoundSolid", JointRoundSolidButton);


		const FButtonStyle JointRoundButton_White = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White, 4))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 4))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor::White, 4))
			.SetNormalPadding(FMargin(0))
			.SetPressedPadding(FMargin(0));

		Style->Set("JointUI.Button.Round.White", JointRoundButton_White);

		const FButtonStyle JointSphereButton_White = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White, 12.f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 12.f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor::White, 12.f))
		.SetNormalPadding(FMargin(0))
		.SetPressedPadding(FMargin(0));
		Style->Set("JointUI.Button.Sphere.White", JointSphereButton_White);

		
		const FButtonStyle SimpleButton = FButtonStyle()
			.SetNormal(FSlateNoResource())
			.SetHovered(FSlateRoundedBoxBrush(Color_Hover, 4.0f))
			.SetPressed(FSlateRoundedBoxBrush(Color_Hover, 4.0f))
			.SetDisabled(FSlateNoResource())
			.SetNormalForeground(Color_Normal)
			.SetHoveredForeground(Color_Hover)
			.SetPressedForeground(Color_Selected)
			.SetDisabledForeground(Color_Disabled)
			.SetNormalPadding(FMargin(0))
			.SetPressedPadding(FMargin(0));

		FComboButtonStyle SimpleComboButton = FComboButtonStyle()
			.SetButtonStyle(SimpleButton)
			.SetDownArrowImage(*GetUEEditorSlateStyleSet().GetBrush("Icons.ChevronDown"))
			.SetShadowOffset(FVector2D(0.0f, 0.0f))
			.SetMenuBorderBrush(FSlateRoundedBoxBrush(Color_Hover, 0.0f, Color_Hover, 1.0f))
			.SetContentPadding(0.f)
			.SetMenuBorderPadding(0.0f)
			.SetDownArrowPadding(0.f);
		Style->Set("JointUI.ComboButton.Round", SimpleComboButton);

		FComboBoxStyle SimpleComboBox = FComboBoxStyle()
			.SetContentPadding(0.f)
			.SetMenuRowPadding(0.f)
			.SetComboButtonStyle(SimpleComboButton);
		Style->Set("JointUI.ComboBox.Round", SimpleComboBox);
	}

	return Style;
}

const ISlateStyle& FJointEditorStyle::Get()
{
	return *(Instance.Get());
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef Joint_IMAGE_BRUSH
#undef Joint_BOX_BRUSH
#undef Joint_BORDER_BRUSH
#undef Joint_DEFAULT_FONT
