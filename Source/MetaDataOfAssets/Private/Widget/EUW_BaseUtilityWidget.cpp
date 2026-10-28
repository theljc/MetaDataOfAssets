// Copyright 2026 ljc

#include "Widget/EUW_BaseUtilityWidget.h"
#include "Widget/EUSS_WidgetManager.h"
#include "Editor.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "StructUtils/InstancedStruct.h"


void UEUW_BaseUtilityWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GEditor)
	{
		UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();

		// 用 UEUSS_WidgetManager 创建的控件，此处会直接 return
		if (WidgetManager->GetInCreatingSubWidget()) return;

		UEditorUtilityWidgetBlueprint* Blueprint =
				Cast<UEditorUtilityWidgetBlueprint>(GetClass()->ClassGeneratedBy);

		// 查找 TabID
		const FName TabID = Blueprint ? Blueprint->GetRegistrationName() : NAME_None;

		// 添加到 WidgetToTabName 中
		WidgetManager->AddTo_WidgetToTabName(this, TabID);

		// 主控件继承了 IWidgetInterface_MetaDataPlugin，所以 Interface 有效
		TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(this);
		if (Interface)
		{
			Interface->OnInitialize(FInstancedStruct());
		}
	}
}

void UEUW_BaseUtilityWidget::NativeDestruct()
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

void UEUW_BaseUtilityWidget::OnOpen_Implementation(const FInstancedStruct& Params)
{
	// 默认空实现，蓝图可重写
	UE_LOG(LogMetaDataOfAssets, Verbose, TEXT("[%hs] : 控件 [%s] 被创建"), __FUNCTION__, *GetName());
}

void UEUW_BaseUtilityWidget::OnClose_Implementation()
{
	// 默认空实现，蓝图可重写
	UE_LOG(LogMetaDataOfAssets, Verbose, TEXT("[%hs] : 控件 [%s] 正在关闭"), __FUNCTION__, *GetName());
}

void UEUW_BaseUtilityWidget::OnInitialize(const FInstancedStruct& Params)
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
		WidgetManager->FocusEditorUtilityTab(WidgetManager->GetWidgetToTabName()[this]);

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

void UEUW_BaseUtilityWidget::OnDeinitialize()
{
	// 获得 Subsystem
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (!IsValid(WidgetManager)) return;

	// 关闭时，确保控件存在
	if (!WidgetManager->GetActiveWidgets().Contains(this)) return;

	// 调用接口
	IWidgetInterface_MetaDataOfAssets::Execute_OnClose(this);

	// 递归移除自身和子节点
	WidgetManager->RemoveNodeAndDescendants(this);

	// 从 ActiveWidgets 中移除
	WidgetManager->RemoveFromActiveWidgets(this);
	
	// 从 WidgetToTabName 中移除
	WidgetManager->RemoveFrom_WidgetToTabName(this);

	// 通知本控件被移除
	WidgetManager->OnWidgetInstanceRemoved.Broadcast(this);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 已移除控件 [%s]"), __FUNCTION__, *GetName());

}
