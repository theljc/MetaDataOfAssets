// Copyright 2026 ljc

#include "Widget/UW_BaseDataWidget.h"
#include "Widget/EUSS_WidgetManager.h"
#include "Editor.h"
#include "Log/MetaDataOfAssetsLogChannel.h"


void UUW_BaseDataWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UUW_BaseDataWidget::NativeDestruct()
{
	if (GEditor)
	{
		TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(this);
		if (Interface)
		{
			Interface->OnDeinitialize();
		}
	}
	
	Super::NativeDestruct();
}

void UUW_BaseDataWidget::OnOpen_Implementation(const FInstancedStruct& Params)
{
	// 默认空实现 — 蓝图可重写此函数以接收初始化参数
	UE_LOG(LogMetaDataOfAssets, Verbose, TEXT("[%hs] : 控件 [%s] 被创建"), __FUNCTION__, *GetName());
}

void UUW_BaseDataWidget::OnClose_Implementation()
{
	// 默认空实现 — 蓝图可重写此函数以进行清理
	UE_LOG(LogMetaDataOfAssets, Verbose, TEXT("[%hs] : 控件 [%s] 正在关闭"), __FUNCTION__, *GetName());
}

void UUW_BaseDataWidget::OnInitialize(const FInstancedStruct& Params)
{
	// 获得 Subsystem
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (!IsValid(WidgetManager))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : WidgetManager 无效"), __FUNCTION__);
		return;
	}

	// 确保已创建的控件中没有该控件
	if (WidgetManager->GetActiveWidgets().Contains(this))
	{
		UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 控件 [%s] 已存在"), __FUNCTION__, *GetName());
		return;
	}
	
	// 添加到 ActiveWidgets 中，并触发广播
	WidgetManager->AddToActiveWidgets(this);
	WidgetManager->OnWidgetInstanceCreated.Broadcast(this);
	
	// 调用接口
	IWidgetInterface_MetaDataOfAssets::Execute_OnOpen(this, Params);

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 已初始化控件 [%s]"), __FUNCTION__, *GetName());
	
}

void UUW_BaseDataWidget::OnDeinitialize()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (!IsValid(WidgetManager)) return;

	// 关闭时，确保控件存在
	if (!WidgetManager->GetActiveWidgets().Contains(this)) return;

	// 调用接口
	IWidgetInterface_MetaDataOfAssets::Execute_OnClose(this);

	// 从 ActiveWidgets 中移除
	WidgetManager->RemoveFromActiveWidgets(this);

	// 通知本控件被移除
	WidgetManager->OnWidgetInstanceRemoved.Broadcast(this);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 已移除控件 [%s]"), __FUNCTION__, *GetName());

}
