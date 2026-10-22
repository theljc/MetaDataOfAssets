#include "Widget/EUW_BaseUtilityWidget.h"
#include "Widget/EUSS_WidgetManager.h"
#include "Editor.h"
#include "Log/MetaDataPluginLogChannel.h"
#include "StructUtils/InstancedStruct.h"


void UEUW_BaseUtilityWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GEditor)
	{
		UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();

		// 用 UEUSS_WidgetManager 创建的控件，此处会直接 return
		if (WidgetManager->GetInCreatingSubWidget()) return;

		// 只有主控件创建时，并且不是通过插件提供的按钮创建时，才需要在这里执行初始化
		// 等待下一帧执行，确保控件已注册到 TabManager，DoesTabExist 才能正常工作
		GEditor->GetTimerManager()->SetTimerForNextTick([this, WidgetManager]()
		{
			// 为了确保即使不通过插件提供的按钮创建时，创建的控件也能被
			// 构造两种 TabID（这两个是源代码中创建 EUW 时使用的 TabID）
			FName TabID_1 = FName(*(GetClass()->ClassGeneratedBy->GetPathName() + NSLOCTEXT("MetaDataPlugin", "ActiveTabSuffix", "_ActiveTab").ToString()));
			FName TabID_2 = FName(*(GetClass()->ClassGeneratedBy->GetPathName() + GetClass()->ClassGeneratedBy->GetFName().ToString()));

			// 鼠标右键创建时，使用 TabID_1
			if (WidgetManager->DoesTabExist(TabID_1))
			{
				WidgetManager->AddTo_WidgetToTabName(this, TabID_1);
			}
			// 蓝图中的按钮创建时，使用 TabID_2
			else if (WidgetManager->DoesTabExist(TabID_2))
			{
				WidgetManager->AddTo_WidgetToTabName(this, TabID_2);
			}
			
			// 主控件继承了 IWidgetInterface_MetaDataPlugin，所以 Interface 有效
			TScriptInterface<IWidgetInterface_MetaDataPlugin> Interface(this);
			if (Interface)
			{
				Interface->OnInitialize(FInstancedStruct());
			}
			
		});
		
	}
}

void UEUW_BaseUtilityWidget::NativeDestruct()
{
	if (GEditor)
	{
		TScriptInterface<IWidgetInterface_MetaDataPlugin> Interface(this);
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
	UE_LOG(LogMetaDataPlugin, Verbose, TEXT("[%hs] : 控件 [%s] 被创建"), __FUNCTION__, *GetName());
}

void UEUW_BaseUtilityWidget::OnClose_Implementation()
{
	// 默认空实现，蓝图可重写
	UE_LOG(LogMetaDataPlugin, Verbose, TEXT("[%hs] : 控件 [%s] 正在关闭"), __FUNCTION__, *GetName());
}

void UEUW_BaseUtilityWidget::OnInitialize(const FInstancedStruct& Params)
{
	// 获得 Subsystem
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (!IsValid(WidgetManager))
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : WidgetManager 无效"), __FUNCTION__);
		return;
	}
	
	// 确保已创建的控件中没有该控件
	if (WidgetManager->GetActiveWidgets().Contains(this))
	{
		UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 控件 [%s] 已存在"), __FUNCTION__, *GetName());
		return;
	}
	
	// 添加到 ActiveWidgets 中，并触发广播
	WidgetManager->AddToActiveWidgets(this);
	WidgetManager->OnWidgetInstanceCreated.Broadcast(this);
	
	// 调用接口
	IWidgetInterface_MetaDataPlugin::Execute_OnOpen(this, Params);

	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 已初始化控件 [%s]"), __FUNCTION__, *GetName());
	
}

void UEUW_BaseUtilityWidget::OnDeinitialize()
{
	// 获得 Subsystem
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (!IsValid(WidgetManager)) return;

	// 关闭时，确保控件存在
	if (!WidgetManager->GetActiveWidgets().Contains(this)) return;

	// 调用接口
	IWidgetInterface_MetaDataPlugin::Execute_OnClose(this);

	// 递归移除自身和子节点
	WidgetManager->RemoveNodeAndDescendants(this);

	// 从 ActiveWidgets 中移除
	WidgetManager->RemoveFromActiveWidgets(this);
	
	// 从 WidgetToTabName 中移除
	WidgetManager->RemoveFrom_WidgetToTabName(this);

	// 通知本控件被移除
	WidgetManager->OnWidgetInstanceRemoved.Broadcast(this);
	
	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 已移除控件 [%s]"), __FUNCTION__, *GetName());

}
