// Copyright 2026 ljc

#include "Widget/EUSS_WidgetManager.h"
#include "Widget/WidgetInterface_MetaDataOfAssets.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"
#include "StatusBarSubsystem.h"
#include "Engine/World.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "Modules/ModuleManager.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "Widgets/SWindow.h"
#include "Widgets/Docking/SDockTab.h"


void UEUSS_WidgetManager::SetMainWidgetPath()
{
	// 从配置文件读取主控件路径
	UMetaDataOfAssetsSettings* Settings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	if (!Settings) return;
	
	const FString& ConfigWidgetPath = Settings->MainWidgetPath;
	
	// 没有值时，表示首次启动插件
	if (ConfigWidgetPath.IsEmpty())
	{
		// 设置默认路径并保存到配置文件
		MainWidgetPath = TEXT("/MetaDataOfAssets/EUW_MainWidget.EUW_MainWidget");
		Settings->SetMainWidgetPath(MainWidgetPath);
	}
	// 有值则使用配置文件中的路径
	else
	{
		MainWidgetPath = ConfigWidgetPath;
	}
	
}

void UEUSS_WidgetManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SetMainWidgetPath();
	
	GetEnableDebugSetting() ? EnableDebug() : DisableDebug();
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 初始化完成"), __FUNCTION__);
}

void UEUSS_WidgetManager::Deinitialize()
{
	Super::Deinitialize();
}

void UEUSS_WidgetManager::TestFun()
{
	// GEditor->GetEditorSubsystem<UStatusBarSubsystem>()
	// ->ToggleContentBrowserDrawer();
}

void UEUSS_WidgetManager::SetToRootNode(UUserWidget* InRootWidget)
{
	if (!IsValid(InRootWidget)) return;

	// 找到或创建该 Widget 对应的节点
	UEuwTreeNode* NewRoot = CreateOrFindTreeNode(InRootWidget);

	// 设置为新的根节点
	RootNode = NewRoot;
	// 根节点没有父节点
	NewRoot->Parent = nullptr;

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 根节点被设置为: %s"), __FUNCTION__, *InRootWidget->GetName());

}

void UEUSS_WidgetManager::AddToChildNode(UUserWidget* ParentWidget, UUserWidget* ChildWidget)
{
	if (!IsValid(ParentWidget) || !IsValid(ChildWidget)) return;

	// 找到或创建父节点和子节点
	UEuwTreeNode* ParentNode = CreateOrFindTreeNode(ParentWidget);
	UEuwTreeNode* ChildNode = CreateOrFindTreeNode(ChildWidget);

	// 防止把自己加为自己子节点
	if (ParentNode == ChildNode) return;
	
	// 建立父子关系
	ChildNode->Parent = ParentNode;
	ParentNode->Children.AddUnique(ChildNode);

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : %s 被添加为 %s 的子节点"),
		__FUNCTION__, *ChildWidget->GetName(), *ParentWidget->GetName());
}

TArray<UUserWidget*> UEUSS_WidgetManager::GetAllDescendants(UUserWidget* InWidget)
{
	TArray<UUserWidget*> Result;
	if (!IsValid(InWidget)) return Result;

	// 查找当前节点
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(InWidget);
	if (!NodePtr || !IsValid(*NodePtr)) return Result;

	UEuwTreeNode* Node = *NodePtr;

	// 深度优先遍历（迭代法）
	TArray<UEuwTreeNode*> Stack;
	Stack.Add(Node);

	while (Stack.Num() > 0)
	{
		UEuwTreeNode* Current = Stack.Pop();
		if (!Current) continue;

		for (UEuwTreeNode* ChildNode : Current->Children)
		{
			if (!IsValid(ChildNode)) continue;

			if (UUserWidget* ChildWidget = ChildNode->Widget.Get())
			{
				Result.Add(ChildWidget);
			}
			Stack.Add(ChildNode);
		}
	}

	return Result;
}

bool UEUSS_WidgetManager::FocusEditorUtilityTab(FName TabID)
{
	TSharedPtr<SDockTab> Tab;

	// 根据 TabID 查找 Tab
	if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		if (TSharedPtr<FTabManager> Manager = LevelEditor->GetLevelEditorTabManager())
		{
			Tab = Manager->FindExistingLiveTab(TabID);
		}
	}

	// Nomad Tab 由 GlobalTabManager 管理
	if (!Tab)
	{
		Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabID);
	}

	if (!Tab)
	{
		return false;
	}

	if (TSharedPtr<SWindow> Window = Tab->GetParentWindow())
	{
		// 窗口是否最小化
		if (Window->IsWindowMinimized())
		{
			// 最小化则恢复
			Window->Restore();
		}

		// 将窗口置于最上层
		Window->BringToFront();
	}

	// 激活 Tab
	Tab->ActivateInParent(ETabActivationCause::SetDirectly);
	
	return true;
}

UEuwTreeNode* UEUSS_WidgetManager::CreateOrFindTreeNode(UUserWidget* Widget)
{
	if (!IsValid(Widget)) return nullptr;

	// 已存在则返回
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(Widget);
	if (NodePtr && IsValid(*NodePtr))
	{	
		return *NodePtr;
	}

	// 创建新节点
	UEuwTreeNode* NewNode = NewObject<UEuwTreeNode>(this);
	NewNode->Widget = Widget;
	WidgetToNodeMap.Add(Widget, NewNode);
	

	return NewNode;
}

void UEUSS_WidgetManager::RemoveNodeAndDescendants(UUserWidget* InWidget)
{
	if (!IsValid(InWidget)) return;

	// 已经在移除中，直接返回，避免递归重复处理
	if (PendingRemoval.Contains(InWidget)) return;
	PendingRemoval.Add(InWidget);

	// 收集所有子控件
	TArray<UUserWidget*> Descendants = GetAllDescendants(InWidget);

	// 关闭所有子控件
	// 关闭子控件会触发它们自己的 NativeDestruct -> OnDeinitialize -> RemoveNodeAndDescendants
	// 用 PendingRemoval 确保不会重复处理当前节点
	for (UUserWidget* Descendant : Descendants)
	{
		if (IsValid(Descendant))
		{
			CloseWidgetTab(Descendant);
		}
	}

	// 关闭当前 Widget 自己的 Tab
	CloseWidgetTab(InWidget);

	// 清理树节点
	RemoveNodeFromTree(InWidget);

	// 移除标记
	PendingRemoval.Remove(InWidget);
}

void UEUSS_WidgetManager::OnMainWidgetMoved(const FAssetData& AssetData, const FString& OldObjectPath)
{
	UMetaDataOfAssetsSettings* Settings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	
	// 判断移动的是否是主控件的资产
	if (Settings->MainWidgetPath == OldObjectPath)
	{
		MainWidgetPath = AssetData.GetSoftObjectPath().ToString();
		// 保存主控件资产的路径
		Settings->SetMainWidgetPath(MainWidgetPath);
	}
}

bool UEUSS_WidgetManager::CloseEditorUtilityTab(FName TabID)
{
	if (!IsRunningCommandlet())
	{
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<FTabManager> Manager = LevelEditor.GetLevelEditorTabManager())
		{
			if (TSharedPtr<SDockTab> Tab = Manager->FindExistingLiveTab(TabID))
			{
				Tab->RequestCloseTab();
				return true;
			}
		}

		/**
		 * 由于 Nomad Tab 由 FGlobalTabmanager 管理，
		 * 而 UEditorUtilitySubsystem::CloseTabByID 在 5.6 版本之前没有处理，所以手动调用关闭
		 */
		if (TSharedPtr<SDockTab> NomadTab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabID))
		{
			NomadTab->RequestCloseTab();
			return true;
		}
	}
	
	return false;
}

void UEUSS_WidgetManager::OnTaskStart()
{
	// 任务开始时，禁用所有已创建的 Widget
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		Widget->SetIsEnabled(false);
	}
}

void UEUSS_WidgetManager::OnTaskComplete()
{
	// 任务完成后，启用所有已创建的 Widget
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		Widget->SetIsEnabled(true);
	}
}

void UEUSS_WidgetManager::CloseWidgetTab(UUserWidget* Widget)
{
	if (!IsValid(Widget)) return;

	// 从 WidgetToTabName 中查找 TabID
	FName* TabIdPtr = WidgetToTabName.Find(Widget);
	
	if (!TabIdPtr || TabIdPtr->IsNone()) return;

	// 关闭 Tab
	CloseEditorUtilityTab(*TabIdPtr);
	
}

void UEUSS_WidgetManager::RemoveNodeFromTree(UUserWidget* Widget)
{
	if (!IsValid(Widget)) return;

	// 查找节点
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(Widget);
	if (!NodePtr || !IsValid(*NodePtr)) return;
	
	UEuwTreeNode* Node = *NodePtr;
	
	// 从父节点的 Children 中移除自己
	if (UEuwTreeNode* Parent = Node->Parent.Get())
	{
		Parent->Children.Remove(Node);
	}

	// 如果自己是根节点，清空根
	if (RootNode == Node)
	{
		RootNode = nullptr;
	}

	// 从映射中移除
	WidgetToNodeMap.Remove(Widget);

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 移除节点: %s"), __FUNCTION__, *Widget->GetName());
}

void UEUSS_WidgetManager::CreateMainWidget()
{
	// 已加载则直接创建，未加载则加载后再创建
	if (!GetMainWidgetSoftPtr().Get())
	{
		FSoftObjectPath WidgetSoftPath(MainWidgetPath);
		SetMainWidgetSoftPtr(TSoftObjectPtr<UEditorUtilityWidgetBlueprint>(WidgetSoftPath));
		
		UEditorUtilityWidgetBlueprint* MainWidget_Ins = GetMainWidgetSoftPtr().LoadSynchronous();
		// 验证加载是否成功
		if (!MainWidget_Ins)
		{
			UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 加载失败"), __FUNCTION__);
			return;
		}

		SpawnAndRegisterTab(MainWidget_Ins);
		
		return;
	}
	
	SpawnAndRegisterTab(GetMainWidgetSoftPtr().Get());
	
}

bool UEUSS_WidgetManager::GetEnableDebugSetting()
{
	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!Settings) return false;
	
	return Settings->bEnableDebug;
}

UUserWidget* UEUSS_WidgetManager::CreateUserWidget(TSubclassOf<UUserWidget> WidgetClass, const FInstancedStruct& Params)
{
	if (!GEditor)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : GEditor 无效"), __FUNCTION__);
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : EditorWorld 无效"), __FUNCTION__);
		return nullptr;
	}

	if (!WidgetClass)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : WidgetClass 无效"), __FUNCTION__);
		return nullptr;
	}

	// 创建 UserWidget
	UUserWidget* Widget = CreateWidget<UUserWidget>(EditorWorld, WidgetClass);
	if (!Widget)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
	if (Interface)
	{
		Interface->OnInitialize(Params);
	}

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());

	return Widget;
}

UEditorUtilityWidget* UEUSS_WidgetManager::CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, const FInstancedStruct& Params)
{
	if (!GEditor)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : GEditor 无效"), __FUNCTION__);
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : EditorWorld 无效"), __FUNCTION__);
		return nullptr;
	}

	if (!InBlueprint)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : InBlueprint 无效"), __FUNCTION__);
		return nullptr;
	}

	// 每次创建新 EUW 时都关闭旧的
	CloseOldWidget(InBlueprint);
	
	// 为了防止 NativeConstruct 中重复创建
	SetInCreatingSubWidget(true);
	
	FName TabID = FName(*FGuid::NewGuid().ToString());
	UEditorUtilityWidget* Widget = SpawnAndRegisterTabAndGetID(InBlueprint, TabID);
	if (!Widget)
	{
		SetInCreatingSubWidget(false);

		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}
	
	// 添加 Widget 和 TabID 的映射
	AddTo_WidgetToTabName(Widget, TabID);
	

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
	if (Interface)
	{
		Interface->OnInitialize(Params);
	}
	
	SetInCreatingSubWidget(false);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());
	
	return Widget;
}

void UEUSS_WidgetManager::AddToActiveWidgets(UUserWidget* Widget)
{
	ActiveWidgets.Add(Widget);
}

void UEUSS_WidgetManager::RemoveFromActiveWidgets(UUserWidget* Widget)
{
	ActiveWidgets.Remove(Widget);
}

void UEUSS_WidgetManager::AddTo_WidgetToTabName(UUserWidget* Widget, FName TabName)
{
	WidgetToTabName.Add(Widget, TabName);
}

void UEUSS_WidgetManager::RemoveFrom_WidgetToTabName(UUserWidget* Widget)
{
	WidgetToTabName.Remove(Widget);
}

void UEUSS_WidgetManager::CloseOldWidget(UEditorUtilityWidgetBlueprint* InBlueprint)
{
	UEditorUtilityWidget* OldWidget = FindUtilityWidgetFromBlueprint(InBlueprint);
	FName OldTabID = NAME_None;
	if (OldWidget)
	{
		if (FName* TabIdPtr = WidgetToTabName.Find(OldWidget))
		{
			OldTabID = *TabIdPtr;
			CloseEditorUtilityTab(OldTabID);
		}
	}
}

void UEUSS_WidgetManager::CloseAllWidgets()
{
	// 通过 WidgetToTabName 查找 TabID，通过 TabID 关闭控件
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		FName* TabID = WidgetToTabName.Find(Widget);
		if (TabID)
		{
			CloseEditorUtilityTab(*TabID);
		}
	}
}

void UEUSS_WidgetManager::DisableDebug()
{
	UMetaDataOfAssetsSettings* PluginSettings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	if (GEngine && GEditor && PluginSettings)
	{
		// 关闭 LogMetaDataOfAssets 分类的日志
		GEngine->Exec(GEditor->GetWorld(), TEXT("log LogMetaDataOfAssets off"));

		// 关闭 PrintString
		PluginSettings->SetEnableDebug(false);
	}
}

void UEUSS_WidgetManager::EnableDebug()
{
	UMetaDataOfAssetsSettings* PluginSettings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	if (GEngine && GEditor && PluginSettings)
	{
		// 开启 LogMetaDataOfAssets 分类的日志
		GEngine->Exec(GEditor->GetWorld(), TEXT("log LogMetaDataOfAssets all"));

		// 开启 PrintString
		PluginSettings->SetEnableDebug(true);
	}
}
