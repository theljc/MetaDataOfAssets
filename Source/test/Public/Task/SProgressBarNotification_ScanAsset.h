#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/INotificationWidget.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

class SProgressBar;


class SProgressBarNotification_ScanAsset : public SCompoundWidget, public INotificationWidget
{
public:
	SLATE_BEGIN_ARGS(SProgressBarNotification_ScanAsset)
			: _ProgressText(FText::FromString("Sync Assets..."))
	{}
		SLATE_ARGUMENT(FText, ProgressText);
	SLATE_END_ARGS()

	// 构造通知窗口
	void Construct(const FArguments& InArgs)
	{
		ChildSlot[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ProgressTextBlock, STextBlock)
				.Text(InArgs._ProgressText)
				.AutoWrapText(true)
				.Font(FAppStyle::Get().GetFontStyle("NotificationList.FontBold"))
			]
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(9)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					[
						SAssignNew(ProgressBar, SProgressBar)
						.Percent(0.0f)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(PercentText, STextBlock)
					]
				]
			]
		];
	}

	// 设置进度条百分比
	void SetProgressPercent(const float Percent)
	{
		ProgressBar->SetPercent(Percent);
		PercentText->SetText(FText::AsPercent(Percent));
		PercentText->SetColorAndOpacity(FLinearColor::White);
	}

	// 设置进度条文本
	void SetProgressText(const FText& InText)
	{
		if (ProgressTextBlock.IsValid())
		{
			ProgressTextBlock->SetText(InText);
		}
	}

// INotificationWidget interface
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState) override
	{}

	virtual TSharedRef<SWidget> AsWidget() override
	{
		return AsShared();
	}
// End INotificationWidget interface

private:
	
	TSharedPtr<STextBlock> ProgressTextBlock;
	TSharedPtr<SProgressBar> ProgressBar;
	TSharedPtr<STextBlock> PercentText;
	
};
