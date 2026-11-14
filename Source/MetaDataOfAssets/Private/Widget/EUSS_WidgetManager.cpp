// Copyright 2026 ljc. All rights reserved.

#include "Widget/EUSS_WidgetManager.h"
#include "Widget/WidgetInterface_MetaDataOfAssets.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"
#include "StatusBarSubsystem.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "Modules/ModuleManager.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "Widgets/SWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SNullWidget.h"


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

void UEUSS_WidgetManager::OnTabClosed(TSharedRef<SDockTab> InClosedTab, TWeakObjectPtr<UUserWidget> InWeakWidget, TWeakObjectPtr<UEditorUtilityWidgetBlueprint> InWeakBlueprint)
{
	UUserWidget* Widget = InWeakWidget.Get();
	// 关闭子控件对应的 Tab
	if (IsValid(Widget))
	{
		CloseDescendantTabs(Widget);
	}
	
	UEditorUtilityWidgetBlueprint* Blueprint = InWeakBlueprint.Get();
	if (IsValid(Blueprint))
	{
		// 因为覆盖了 SpawnEditorUITab 原本设置的回调1，手动补回原生清理逻辑
		Blueprint->UpdateRespawnListIfNeeded(InClosedTab);
	}

}

void UEUSS_WidgetManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 设置主控件资产的路径
	SetMainWidgetPath();

	// 是否启用调试
	GetEnableDebugSetting() ? EnableDebug() : DisableDebug();
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 初始化完成"), __FUNCTION__);
}

void UEUSS_WidgetManager::Deinitialize()
{
	// 关闭所有控件
	CloseAllWidgets();

	// 注销所有已注册 TabSpawner
	if (FLevelEditorModule* LevelEditor =
		FModuleManager::GetModulePtr<FLevelEditorModule>(
			TEXT("LevelEditor")))
	{
		TSharedPtr<FTabManager> Manager = LevelEditor->GetLevelEditorTabManager();

		if (Manager.IsValid())
		{
			for (FName TabID : RegisteredTabID)
			{
				if (Manager->HasTabSpawner(TabID))
				{
					Manager->UnregisterTabSpawner(TabID);
					RegisteredTabs.Remove(TabID);
				}
			}
		}
	}

	TabSpawnContexts.Empty();

	// 移除 Handle
	if (RetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RetryTickerHandle);
		RetryTickerHandle.Reset();
	}
	
	if (CompiledHandle.IsValid())
	{
		MainWidget->OnCompiled().Remove(CompiledHandle);
		CompiledHandle.Reset();
	}
	
	Super::Deinitialize();
}

void UEUSS_WidgetManager::RegisterMainWidgetTab()
{
	// 加载主控件资产
	MainWidget = LoadMainWidget();
	if (!MainWidget || !GEditor) return;

	const FName MainWidgetTabID = MakeMainWidgetTabID(MainWidget);
	const FName StatusBarName = MakeStatusBarName(MainWidget);

	// 构建生成参数
	FEUWTabSpawnContext SpawnContext(
		MainWidget,
		MainWidgetTabID,
		StatusBarName,
		true,
		FInstancedStruct());

	// 添加到 TabSpawnContexts 中，后续通过 TabID 来查找生成参数
	TabSpawnContexts.Add(SpawnContext.TabID, SpawnContext);
	
	if (!RegisterTab_Custom(SpawnContext))
	{
		// LevelEditor TabManager 尚未就绪时，下一帧重试
		RetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateWeakLambda(
				this,
				[this, SpawnContext](float)
				{
					const bool bSuccess = RegisterTab_Custom(SpawnContext);
					return !bSuccess;
				}
			));
	}
}

bool UEUSS_WidgetManager::RegisterTab_Custom(const FEUWTabSpawnContext& InSpawnContext)
{
	// 参照 UEditorUtilitySubsystem::RegisterTabAndGetID
	if (!IsRunningCommandlet())
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
		if (!LevelEditorTabManager.IsValid()) return false;

		// 是否已注册
		if (!LevelEditorTabManager->HasTabSpawner(InSpawnContext.TabID))
		{
			UEditorUtilityWidgetBlueprint* Blueprint = InSpawnContext.Blueprint.Get();
			if (!Blueprint) return false;

			// 注册 TabSpawner，绑定自定义生成 Tab 的回调函数
			LevelEditorTabManager->RegisterTabSpawner(
				InSpawnContext.TabID,
				FOnSpawnTab::CreateUObject(
					this,
					&UEUSS_WidgetManager::OnSpawnEUWTab,
					InSpawnContext.TabID
					)
				)
			.SetDisplayName(Blueprint->GetTabDisplayName())
			.SetMenuType(ETabSpawnerMenuType::Hidden);	// 在 Tools 分类中隐藏此控件

			Blueprint->SetRegistrationName(InSpawnContext.TabID);
			RegisteredTabs.Add(InSpawnContext.TabID, Blueprint);

			// 用于保存由本插件注册的 TabID
			RegisteredTabID.Add(InSpawnContext.TabID);
		}

		return true;
	}
	
	return false;
}

UEditorUtilityWidget* UEUSS_WidgetManager::SpawnAndRegisterTabWithId_Custom(UEditorUtilityWidgetBlueprint* InBlueprint, const FName InTabID, FInstancedStruct InParams)
{
	// 参照 UEditorUtilitySubsystem::SpawnAndRegisterTabWithId
	FEUWTabSpawnContext SpawnContext(
		InBlueprint,
		InTabID,
		NAME_None,
		false,
		MoveTemp(InParams));
	
	TabSpawnContexts.Add(SpawnContext.TabID, SpawnContext);
	
	RegisterTab_Custom(SpawnContext);
	SpawnRegisteredTabByID_Custom(SpawnContext);
	return FindUtilityWidgetFromBlueprint(InBlueprint);
}

void UEUSS_WidgetManager::OnBlueprintCompiled(UBlueprint* InCompiledBlueprint, const FName InTabID)
{
	if (!GEditor || PendingTabContentRestore.Contains(InTabID)) return;

	// 防止重复添加
	PendingTabContentRestore.Add(InTabID);
	
	GEditor->GetTimerManager()->SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(
			this,
			[this, InTabID]()
			{
				RestoreTabContent(InTabID);
			}));
}

void UEUSS_WidgetManager::RestoreTabContent(const FName InTabID)
{
	if (!GEditor) return;

	// 查找生成参数
	FEUWTabSpawnContext* SpawnContext = TabSpawnContexts.Find(InTabID);
	if (!SpawnContext) return;

	// 查找已存在的 Tab
	TSharedPtr<SDockTab> Tab = FindExistingTab(SpawnContext->TabID);
	if (!Tab.IsValid()) return;

	// 获得已创建的 Widget 实例
	UEditorUtilityWidget* NewWidget = SpawnContext->Blueprint.Get()->GetCreatedWidget();
	if (!IsValid(NewWidget)) return;

	// 重新绑定 OnTabClosed
	Tab->SetOnTabClosed(
		SDockTab::FOnTabClosedCallback::CreateUObject(
			this,
			&UEUSS_WidgetManager::OnTabClosed,
			TWeakObjectPtr<UUserWidget>(NewWidget),
			SpawnContext->Blueprint));
	
	// 新的 Widget 实例需要重新建立 TabID 映射
	AddTo_WidgetToTabName(NewWidget, SpawnContext->TabID);

	// 重新初始化新 Widget
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(NewWidget);
	if (!Interface) return;

	Interface->OnInitialize(SpawnContext->Params);

	// 是否重新附加 ContentBrowserDrawer
	if (SpawnContext->bAddContentBrowserDrawer && !SpawnContext->StatusBarName.IsNone())
	{
		TSharedRef<SWidget> EUWContent = Tab->GetContent();
		
		TSharedRef<SWidget> StatusBar =
			GEditor->GetEditorSubsystem<UStatusBarSubsystem>()
				->MakeStatusBarWidget(SpawnContext->StatusBarName, Tab.ToSharedRef());

		Tab->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				EUWContent
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				StatusBar
			]
		);
	}
	
	PendingTabContentRestore.Remove(InTabID);
}

TSharedPtr<SDockTab> UEUSS_WidgetManager::FindExistingTab(const FName InTabID)
{
	// 从 LevelEditorTabManager 查找
	if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		if (TSharedPtr<FTabManager> Manager = LevelEditor->GetLevelEditorTabManager())
		{
			if (TSharedPtr<SDockTab> Tab = Manager->FindExistingLiveTab(FTabId(InTabID)))
			{
				return Tab;
			}
		}
	}

	// 从 GlobalTabmanager 查找 Nomad Tab
	return FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(InTabID));
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

void UEUSS_WidgetManager::AddToChildNode(UUserWidget* InParentWidget, UUserWidget* InChildWidget)
{
	if (!IsValid(InParentWidget) || !IsValid(InChildWidget)) return;
	
	// 找到或创建父节点和子节点
	UEuwTreeNode* ParentNode = CreateOrFindTreeNode(InParentWidget);
	UEuwTreeNode* ChildNode = CreateOrFindTreeNode(InChildWidget);
	
	// 防止把自己加为自己子节点
	if (ParentNode == ChildNode) return;
	
	// 建立父子关系
	ChildNode->Parent = ParentNode;
	ParentNode->Children.AddUnique(ChildNode);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : %s 被添加为 %s 的子节点"),
		__FUNCTION__, *InChildWidget->GetName(), *InParentWidget->GetName());
}

TArray<UUserWidget*> UEUSS_WidgetManager::GetAllDescendants(UUserWidget* InWidget)
{
	TArray<UUserWidget*> Result;
	if (!IsValid(InWidget)) return Result;
	
	// 查找当前节点
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(InWidget);
	if (!NodePtr || !IsValid(*NodePtr)) return Result;
	
	UEuwTreeNode* Node = *NodePtr;
	
	/** 深度优先遍历（迭代法）*/
	TArray<UEuwTreeNode*> Stack;
	Stack.Add(Node);
	
	while (Stack.Num() > 0)
	{
		// 取出当前节点
		UEuwTreeNode* Current = Stack.Pop();
		if (!Current) continue;

		// 遍历子节点
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

bool UEUSS_WidgetManager::TryFocusEditorUtilityTab(const FName InTabID)
{
	// 查找已存在的 Tab
	TSharedPtr<SDockTab> Tab = FindExistingTab(InTabID);
	if (!Tab.IsValid()) return false;

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

		// 激活 Tab
		Tab->ActivateInParent(ETabActivationCause::SetDirectly);
		
		return true;
	}
	
	return false;
}

TSharedRef<SDockTab> UEUSS_WidgetManager::OnSpawnEUWTab(const FSpawnTabArgs& InArgs, const FName InTabID)
{
	// 查找生成参数
	FEUWTabSpawnContext* SpawnContext = TabSpawnContexts.Find(InTabID);
	if (!SpawnContext) return SNew(SDockTab).TabRole(ETabRole::NomadTab);
	
	UEditorUtilityWidgetBlueprint* WidgetBlueprint = SpawnContext->Blueprint.Get();
	if (!WidgetBlueprint || !GEditor)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建失败"), __FUNCTION__);
		return SNew(SDockTab).TabRole(ETabRole::NomadTab);
	}
	
	// 先按原生方式创建 Tab 和 EUW
	TSharedRef<SDockTab> Tab = WidgetBlueprint->SpawnEditorUITab(InArgs);

	// 防止重复绑定
	if (CompiledHandle.IsValid())
	{
		MainWidget->OnCompiled().Remove(CompiledHandle);
	}
	
	// 绑定蓝图编译完成的回调
	CompiledHandle =
		SpawnContext->Blueprint->OnCompiled().AddUObject(
			this,
			&UEUSS_WidgetManager::OnBlueprintCompiled,
			SpawnContext->TabID
		);
	
	UEditorUtilityWidget* Widget = WidgetBlueprint->GetCreatedWidget();
	if (!IsValid(Widget))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建失败"), __FUNCTION__);
		return SNew(SDockTab).TabRole(ETabRole::NomadTab);
	}

	// 绑定标签页关闭的回调
	Tab->SetOnTabClosed(
	SDockTab::FOnTabClosedCallback::CreateUObject(
		this,
		&UEUSS_WidgetManager::OnTabClosed,
		TWeakObjectPtr<UUserWidget>(Widget),
		SpawnContext->Blueprint));
	
	// 添加到 WidgetToTabName
	AddTo_WidgetToTabName(Widget, SpawnContext->TabID);

	// 初始化控件
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
	if (!Interface) return SNew(SDockTab).TabRole(ETabRole::NomadTab);

	Interface->OnInitialize(SpawnContext->Params);
	
	// 只有 bAddContentBrowserDrawer 为 true 且 StatusBarName 不为空时才添加 ContentBrowserDrawer
	if (SpawnContext->bAddContentBrowserDrawer && !SpawnContext->StatusBarName.IsNone())
	{
		// 取出原生 EUW 的内容
		TSharedRef<SWidget> EUWContent = Tab->GetContent();
		
		// 创建 ContentBrowserDrawer
		TSharedRef<SWidget> StatusBar =
			GEditor->GetEditorSubsystem<UStatusBarSubsystem>()
				->MakeStatusBarWidget(
					SpawnContext->StatusBarName,
					Tab
				);
	
		// 重新设置 Tab 的内容
		// 两个槽位，对应 EUW 控件的内容和 ContentBrowserDrawer
		Tab->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				EUWContent
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				StatusBar
			]
		);
	}

	return Tab;
}

UEuwTreeNode* UEUSS_WidgetManager::CreateOrFindTreeNode(UUserWidget* InWidget)
{
	if (!IsValid(InWidget)) return nullptr;
	
	// 已存在则返回
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(InWidget);
	if (NodePtr && IsValid(*NodePtr))
	{	
		return *NodePtr;
	}
	
	// 创建新节点
	UEuwTreeNode* NewNode = NewObject<UEuwTreeNode>(this);
	NewNode->Widget = InWidget;
	WidgetToNodeMap.Add(InWidget, NewNode);
	
	
	return NewNode;
}

void UEUSS_WidgetManager::OnAssetMoved(const FAssetData& InAssetData, const FString& InOldObjectPath)
{
	UMetaDataOfAssetsSettings* Settings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	
	// 判断移动的是否是主控件的资产
	if (Settings->MainWidgetPath == InOldObjectPath)
	{
		MainWidgetPath = InAssetData.GetSoftObjectPath().ToString();
		// 保存主控件资产的路径
		Settings->SetMainWidgetPath(MainWidgetPath);
	}
}

bool UEUSS_WidgetManager::CloseTabByID_Custom(const FName InTabID)
{
	// 参照 UEditorUtilitySubsystem::CloseTabByID
	if (!IsRunningCommandlet())
	{
		if (TSharedPtr<SDockTab> Tab = FindExistingTab(InTabID))
		{
			Tab->RequestCloseTab();
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

void UEUSS_WidgetManager::OnTaskFinished()
{
	// 任务完成后，启用所有已创建的 Widget
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		Widget->SetIsEnabled(true);
	}
}

void UEUSS_WidgetManager::CloseWidgetTab(UUserWidget* InWidget)
{
	if (!IsValid(InWidget)) return;

	// 从 WidgetToTabName 中查找 TabID
	FName* TabIdPtr = WidgetToTabName.Find(InWidget);
	
	if (!TabIdPtr || TabIdPtr->IsNone()) return;

	// 关闭 Tab
	CloseTabByID_Custom(*TabIdPtr);
}

void UEUSS_WidgetManager::RemoveNodeFromTree(UUserWidget* InWidget)
{
	if (!IsValid(InWidget)) return;
	
	// 查找节点
	TObjectPtr<UEuwTreeNode>* NodePtr = WidgetToNodeMap.Find(InWidget);
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
	WidgetToNodeMap.Remove(InWidget);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 移除节点: %s"), __FUNCTION__, *InWidget->GetName());
}

void UEUSS_WidgetManager::CloseDescendantTabs(UUserWidget* InRootWidget)
{
	if (!IsValid(InRootWidget)) return;

	// 获得所有子节点的控件
	TArray<UUserWidget*> Descendants = GetAllDescendants(InRootWidget);

	// 关闭所有子节点的控件，清理树节点
	for (UUserWidget* Descendant : Descendants)
	{
		if (!IsValid(Descendant)) continue;
	
		CloseWidgetTab(Descendant);
		RemoveNodeFromTree(Descendant);
	}
}

UEditorUtilityWidgetBlueprint* UEUSS_WidgetManager::LoadMainWidget()
{
	UEditorUtilityWidgetBlueprint* MainWidgetPtr = GetMainWidgetSoftPtr().Get();

	// 已加载则返回，未加载则加载
	if (!MainWidgetPtr)
	{
		FSoftObjectPath WidgetSoftPath(MainWidgetPath);
		SetMainWidgetSoftPtr(TSoftObjectPtr<UEditorUtilityWidgetBlueprint>(WidgetSoftPath));
		
		MainWidgetPtr = GetMainWidgetSoftPtr().LoadSynchronous();
	}
	
	return MainWidgetPtr;
}

FName UEUSS_WidgetManager::MakeMainWidgetTabID(const UEditorUtilityWidgetBlueprint* InBlueprint)
{
	// 这个 TabID 是根据源代码中的 TabID 来构建的
	// 详见 UEditorUtilitySubsystem::RegisterTabAndGetID 的 RegistrationName
	return FName(*FString::Printf(TEXT("%s_ActiveTab"), *InBlueprint->GetPathName()));
}

FName UEUSS_WidgetManager::MakeStatusBarName(const UEditorUtilityWidgetBlueprint* InBlueprint)
{
	return FName(*FString::Printf(TEXT("%s_StatusBar"), *InBlueprint->GetPathName()));
}

void UEUSS_WidgetManager::SpawnRegisteredTabByID_Custom(const FEUWTabSpawnContext& InSpawnContext)
{
	// 参照 UEditorUtilitySubsystem::SpawnRegisteredTabByID
	if (!IsRunningCommandlet())
	{
		// 尝试聚焦已经创建的主控件
		if (TryFocusEditorUtilityTab(InSpawnContext.TabID)) return;
		
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager())
		{
			if (LevelEditorTabManager->HasTabSpawner(InSpawnContext.TabID))
			{
				// TryInvokeTab 打开主控件
				if (TSharedPtr<SDockTab> NewDockTab = LevelEditorTabManager->TryInvokeTab(InSpawnContext.TabID))
				{
					// 启用状态与"是否处于调试模式"相关
					NewDockTab->SetEnabled(FSlateApplication::Get().GetNormalExecutionAttribute());
				}
			}
		}
	}

}

void UEUSS_WidgetManager::CreateMainWidget()
{
	// 构建主控件的 TabID
	FName MainWidgetTabID = MakeMainWidgetTabID(MainWidget);
	const FName StatusBarName = MakeStatusBarName(MainWidget);

	// 构建生成参数
	FEUWTabSpawnContext SpawnContext(
		MainWidget,
		MainWidgetTabID,
		StatusBarName,
		true,
		FInstancedStruct());
	
	SpawnRegisteredTabByID_Custom(SpawnContext);
}

bool UEUSS_WidgetManager::GetEnableDebugSetting()
{
	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!Settings) return false;
	
	return Settings->bEnableDebug;
}

UUserWidget* UEUSS_WidgetManager::CreateUserWidget(TSubclassOf<UUserWidget> InWidgetClass, const FInstancedStruct& InParams)
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

	if (!InWidgetClass)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : WidgetClass 无效"), __FUNCTION__);
		return nullptr;
	}

	// 创建 UserWidget
	UUserWidget* Widget = CreateWidget<UUserWidget>(EditorWorld, InWidgetClass);
	if (!Widget)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
	if (!Interface) return nullptr;

	Interface->OnInitialize(InParams);

	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());

	return Widget;
}

UEditorUtilityWidget* UEUSS_WidgetManager::CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, FInstancedStruct InParams)
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

	// 每次创建 EUW 时都使用不同的 TabID
	FName TabID = FName(*FGuid::NewGuid().ToString());
	UEditorUtilityWidget* Widget = SpawnAndRegisterTabWithId_Custom(InBlueprint, TabID, MoveTemp(InParams));
	if (!Widget)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}
	
	// 添加 Widget 和 TabID 的映射
	AddTo_WidgetToTabName(Widget, TabID);

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
	if (!Interface) return nullptr;

	Interface->OnInitialize(InParams);
	
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());
	
	return Widget;
}

void UEUSS_WidgetManager::AddToActiveWidgets(UUserWidget* InWidget)
{
	ActiveWidgets.Add(InWidget);
}

void UEUSS_WidgetManager::RemoveFromActiveWidgets(UUserWidget* InWidget)
{
	ActiveWidgets.Remove(InWidget);
}

void UEUSS_WidgetManager::AddTo_WidgetToTabName(UUserWidget* InWidget, FName InTabName)
{
	WidgetToTabName.Add(InWidget, InTabName);
}

void UEUSS_WidgetManager::RemoveFrom_WidgetToTabName(UUserWidget* InWidget)
{
	WidgetToTabName.Remove(InWidget);
}

void UEUSS_WidgetManager::CloseOldWidget(UEditorUtilityWidgetBlueprint* InBlueprint)
{
	UEditorUtilityWidget* OldWidget = FindUtilityWidgetFromBlueprint(InBlueprint);
	if (OldWidget)
	{
		CloseWidgetTab(OldWidget);
	}
}

void UEUSS_WidgetManager::CloseAllWidgets()
{
	TArray<UUserWidget*> WidgetsToClose = GetActiveWidgets();
	TArray<FName> TabIDsToClose;

	// 先保存 TabID，因为之后 OnDeinitialize 会进行清理
	for (UUserWidget* Widget : WidgetsToClose)
	{
		if (FName* TabID = WidgetToTabName.Find(Widget))
		{
			TabIDsToClose.AddUnique(*TabID);
		}
	}
	
	// 切换关卡触发关闭控件时，需要清理控件
	for (UUserWidget* Widget : WidgetsToClose)
	{
		if (!IsValid(Widget)) continue;

		TScriptInterface<IWidgetInterface_MetaDataOfAssets> Interface(Widget);
		if (!Interface) continue;

		Interface->OnDeinitialize();
	}

	// 通过 TabID 查找并关闭 Tab
	for (const FName TabID : TabIDsToClose)
	{
		if (TSharedPtr<SDockTab> Tab = FindExistingTab(TabID))
		{
			Tab->SetContent(SNullWidget::NullWidget);
			Tab->RequestCloseTab();
		}
	}

	ActiveWidgets.Empty();
	WidgetToTabName.Empty();
	WidgetToNodeMap.Empty();
	PendingRemoval.Empty();
	RootNode = nullptr;
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
