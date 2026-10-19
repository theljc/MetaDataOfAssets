// EUW_WidgetManager.cpp — 控件管理子系统实现

#include "Widget/EUSS_WidgetManager.h"
#include "Widget/WidgetInterface_MetaDataPlugin.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
// #include "AssetRegistry/IAssetRegistry.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
// #include "Settings/MetaDataPluginSettings.h"
#include "Log/MetaDataPluginLogChannel.h"
#include "Settings/MetaDataPluginSettings.h"
#include "StructUtils/InstancedStruct.h"
// #include "../Private/Global/MetadataPluginGlobal.cpp"
// #include "Internationalization/StringTableRegistry.h"
// #include "Kismet/KismetStringTableLibrary.h"
// #include "Task/FAssetScanTask.h"
#include "Global/MetadataPluginGlobal.h"
#include "Widgets/SWindow.h"

void UEUSS_WidgetManager::SetMainWidgetPath()
{
	// 从配置文件读取主控件路径
	UMetaDataPluginSettings* Settings = GetMutableDefault<UMetaDataPluginSettings>();
	if (!Settings) return;
	
	// FString WidgetPath = MetadataOfAssetsUtils::GetWidgetPathFromConfigFile();
	const FString& ConfigWidgetPath = Settings->MainWidgetPath;
	
	// 没有值时，表示首次启动插件
	if (ConfigWidgetPath.IsEmpty())
	{
		// TODO: 插件名未修改
		// 设置默认路径并保存到配置文件
		MainWidgetPath = TEXT("/test/EUW_MainWidget.EUW_MainWidget");
		Settings->SaveMainWidgetPath(MainWidgetPath);
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
	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 初始化完成"), __FUNCTION__);
}

void UEUSS_WidgetManager::Deinitialize()
{
	// 清理所有仍处于活跃状态的控件
	// UE_LOG(LogMetaDataPlugin, Log, TEXT("[UEUSS_WidgetManager] 子系统即将清空 %d 个活跃控件"), ActiveWidgets.Num());
	
	// ActiveWidgets.Empty();

	Super::Deinitialize();
}

// ==================== 聚焦辅助函数 ====================

// bool UEUSS_WidgetManager::FocusWidgetTab(UEditorUtilityWidget* UtilityWidget)
// {
// 	if (!UtilityWidget)
// 	{
// 		return false;
// 	}
//
// 	TSharedPtr<SWidget> CachedWidget = UtilityWidget->GetCachedWidget();
// 	if (!CachedWidget.IsValid())
// 	{
// 		UE_LOG(LogMetaDataPlugin, Warning, TEXT("[UEUW_WidgetManager] FocusWidgetTab 失败：CachedWidget 无效"));
// 		return false;
// 	}
//
// 	// 获取 Widget 的父窗口（即 Slate Tab 所在的 SWindow）
// 	TSharedPtr<SWidget> ParentWidget = CachedWidget->GetParentWidget();
// 	if (!ParentWidget.IsValid())
// 	{
// 		UE_LOG(LogMetaDataPlugin, Warning, TEXT("[UEUW_WidgetManager] FocusWidgetTab 失败：ParentWidget 无效"));
// 		return false;
// 	}
//
// 	TSharedRef<SWidget> ParentWidgetRef = ParentWidget.ToSharedRef();
// 	TSharedPtr<SWindow> WidgetWindow = FSlateApplication::Get().FindWidgetWindow(ParentWidgetRef);
// 	if (!WidgetWindow.IsValid())
// 	{
// 		UE_LOG(LogMetaDataPlugin, Warning, TEXT("[UEUW_WidgetManager] FocusWidgetTab 失败：找不到 SWindow"));
// 		return false;
// 	}
//
// 	// 将窗口换入前方
// 	WidgetWindow->BringToFront();
// 	// 聚焦窗口内容
// 	FSlateApplication::Get().SetUserFocus(
// 		FSlateApplication::Get().GetUserIndexForKeyboard(),
// 		WidgetWindow->GetContent());
//
// 	UE_LOG(LogMetaDataPlugin, Log, TEXT("[UEUW_WidgetManager] FocusWidgetTab — 已聚焦窗口 [%s]"),
// 		*UtilityWidget->GetName());
//
// 	return true;
// }

void UEUSS_WidgetManager::SetToRootNode(UUserWidget* InRootWidget)
{
	if (!IsValid(InRootWidget)) return;

	// 找到或创建该 Widget 对应的节点
	UEuwTreeNode* NewRoot = CreateOrFindTreeNode(InRootWidget);

	// 2. 如果已有旧的根节点，把旧根挂到新根下（避免丢失原有树）
	// if (RootNode && RootNode != NewRoot)
	// {
	// 	// 如果旧根本身就是新根，跳过
	// 	NewRoot->Children.AddUnique(RootNode);
	// 	RootNode->Parent = NewRoot;
	// }

	// 设置为新的根节点
	RootNode = NewRoot;
	// 根节点没有父节点
	NewRoot->Parent = nullptr;

	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 根节点被设置为: %s"), __FUNCTION__, *InRootWidget->GetName());

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

	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : %s 被添加为 %s 的子节点"),
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
	UMetaDataPluginSettings* Settings = GetMutableDefault<UMetaDataPluginSettings>();
	
	// 判断移动的是否是主控件的资产
	if (Settings->MainWidgetPath == OldObjectPath)
	{
		MainWidgetPath = AssetData.GetSoftObjectPath().ToString();
		// 保存主控件资产的路径
		Settings->SaveMainWidgetPath(MainWidgetPath);
	}
}

void UEUSS_WidgetManager::OnTaskStart()
{
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		Widget->SetIsEnabled(false);
	}
}

void UEUSS_WidgetManager::OnTaskComplete()
{
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		Widget->SetIsEnabled(true);
	}
}

// void UEUSS_WidgetManager::test()
// {
// 	UMetaDataPluginSettings* Settings = MetadataOfAssetsUtils::GetMetaDataPluginSettings();
// 	Settings->SaveMainWidgetPath(TEXT("/test/元数据Widget/EUWBP_MetaData.EUWBP_MetaData"));
// 	
// }

void UEUSS_WidgetManager::CloseWidgetTab(UUserWidget* Widget)
{
	if (!IsValid(Widget)) return;

	// 从 WidgetToTabName 中查找 TabID
	FName* TabIdPtr = WidgetToTabName.Find(Widget);
	
	if (!TabIdPtr || TabIdPtr->IsNone()) return;

	// 关闭 Tab
	CloseTabByID(*TabIdPtr);
	
	// if (DoesTabExist(*TabIdPtr))
	// {
	// }

}

void UEUSS_WidgetManager::RemoveNodeFromTree(UUserWidget* Widget)
{
	if (!IsValid(Widget)) return;

	// 查找节点
	UEuwTreeNode* Node = *WidgetToNodeMap.Find(Widget);
	if (!IsValid(Node)) return;
	
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

	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 移除节点: %s"), __FUNCTION__, *Widget->GetName());
}


// void UEUSS_WidgetManager::OnAssetRemoved(const FAssetData& AssetData)
// {
// 	// 获取被删除资产的路径（例如 /Game/MyAsset.MyAsset）
// 	FSoftObjectPath DeletedPath = AssetData.ToSoftObjectPath();
// 	FString DeletedPathStr = DeletedPath.ToString();
//
// 	// 从后向前遍历，安全删除
// 	for (int32 i = AssetRefs.Num() - 1; i >= 0; --i)
// 	{
// 		UObject* Obj = AssetRefs[i];
// 		if (Obj)
// 		{
// 			// 比较对象路径是否与删除的资产路径一致
// 			FSoftObjectPath ObjPath(Obj);
// 			if (ObjPath == DeletedPath)
// 			{
// 				AssetRefs.RemoveAt(i);
// 				// 触发广播，通知删除
// 				OnAssetDeleted.Broadcast(DeletedPathStr);
// 				// 通常一个资产只会对应一个对象，所以找到后即可跳出
// 				break;
// 			}
// 		}
// 		else
// 		{
// 			// 如果对象已被GC置空，顺便清理
// 			AssetRefs.RemoveAt(i);
// 		}
// 	}
// }

// FName UEUSS_WidgetManager::GetWidgetTabName(UUserWidget* Widget)
// {
	// // 获取真实的 TabID — 从 Slate 层级中找到 SDockTab
	// FName RealTabID;
	// if (TSharedPtr<SWidget> CachedSlate = Widget->GetCachedWidget())
	// {
	// 	TSharedPtr<SWidget> Parent = CachedSlate->GetParentWidget();
	// 	while (Parent.IsValid())
	// 	{
	// 		// SDockTab 的类型标识是 "SDockTab"
	// 		if (Parent->GetTypeAsString() == TEXT("SDockingTabStack"))
	// 		{
	// 			TSharedPtr<SDockTab> DockTab = StaticCastSharedPtr<SDockTab>(Parent);
	// 			RealTabID = DockTab->GetLayoutIdentifier().TabType;
	// 			break;
	// 		}
	// 		Parent = Parent->GetParentWidget();
	// 	}
	// }
	//
	// // UE_LOG(LogMetaDataPlugin, Log, TEXT("[EUW_MetaData] 真实 TabID = %hhd"), RealTabID);
	// return RealTabID;

// 	if (!Widget)
// 	{
// 		return FName();
// 	}
//
// 	// 1. 获取 Widget 的 Slate 表示
// 	TSharedPtr<SWidget> CachedSlate = Widget->GetCachedWidget();
// 	if (!CachedSlate.IsValid())
// 	{
// 		return FName();
// 	}
//
// 	// 2. 通过 FSlateApplication 查找所属的 SWindow
// 	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(CachedSlate.ToSharedRef());
// 	if (!ParentWindow.IsValid())
// 	{
// 		return FName();
// 	}
//
// 	// 3. 获取窗口的内容
// 	TSharedRef<SWidget> WindowContent = ParentWindow->GetContent();
//
// 	// 4. 尝试将内容转换为 SDockTab
// 	//    注意：这里可能是 SDockTab 本身，也可能是其他容器，
// 	//    需要根据实际情况调整
// 	TSharedPtr<SDockTab> DockTab = StaticCastSharedPtr<SDockTab>(WindowContent.ToSharedPtr());
// 	if (DockTab.IsValid())
// 	{
// 		return DockTab->GetLayoutIdentifier().TabType;
// 	}
//
// 	// 5. 备选：如果内容不是 SDockTab，可能需要进一步遍历其子控件
// 	//    但大多数情况下，EUW 直接作为 SDockTab 的内容
// 	return FName();
// }

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
			UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : 加载失败"), __FUNCTION__);
			return;
		}
		
		CreateEditorUtilityWidget(MainWidget_Ins, FInstancedStruct());
		
		return;
	}
	
	CreateEditorUtilityWidget(GetMainWidgetSoftPtr().Get(), FInstancedStruct());

	
	

	// 查找 UtilityWidget 是否已创建
	// UEditorUtilityWidget* UtilityWidget = FindUtilityWidgetFromBlueprint(EUWBP_MetaData.Get());
	// if (UtilityWidget)
	// {
	// 	// 已创建 → 聚焦
	// 	FocusWidgetTab(UtilityWidget);
	// }
	// else
	// {
	// 	// 未创建 → 通过 SpawnAndRegisterTab 创建（也会走到上面的 FocusWidgetTab 逻辑）
	// 	SpawnAndRegisterTab(Get_EUWBP().Get());
	// }
	
}

// FString UEUSS_WidgetManager::GetWidgetPathFromConfigFile()
// {
// 	const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini");
// 	FString CurrentPath;
// 	
// 	GConfig->GetString(TEXT("MetaDataPluginConfig"),
// 		TEXT("CurrentAssetPath"),
// 		CurrentPath,
// 		FConfigCacheIni::NormalizeConfigIniPath(Path)	// 指向项目的 DefaultGame.ini
// 	);
// 	
// 	return CurrentPath;
// }

UUserWidget* UEUSS_WidgetManager::CreateUserWidget(TSubclassOf<UUserWidget> WidgetClass, const FInstancedStruct& Params)
{
	if (!GEditor)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : GEditor 无效"), __FUNCTION__);
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : EditorWorld 无效"), __FUNCTION__);
		return nullptr;
	}

	if (!WidgetClass)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : WidgetClass 无效"), __FUNCTION__);
		return nullptr;
	}

	// 创建 UserWidget
	UUserWidget* Widget = CreateWidget<UUserWidget>(EditorWorld, WidgetClass);
	if (!Widget)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataPlugin> Interface(Widget);
	if (Interface)
	{
		Interface->OnInitialize(Params);
	}

	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());

	return Widget;
}

UEditorUtilityWidget* UEUSS_WidgetManager::CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, const FInstancedStruct& Params)
{
	if (!GEditor)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : GEditor 无效"), __FUNCTION__);
		return nullptr;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : EditorWorld 无效"), __FUNCTION__);
		return nullptr;
	}

	if (!InBlueprint)
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : InBlueprint 无效"), __FUNCTION__);
		return nullptr;
	}

	// 为了防止 NativeConstruct 中重复创建
	SetInCreatingSubWidget(true);

	FName TabID;
	UEditorUtilityWidget* Widget = SpawnAndRegisterTabAndGetID(InBlueprint, TabID);
	if (!Widget)
	{
		SetInCreatingSubWidget(false);

		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : 创建 Widget 失败"), __FUNCTION__);
		return nullptr;
	}

	// 添加 Widget 和 TabID 的映射
	AddTo_WidgetToTabName(Widget, TabID);
	

	// Widget 只有继承了 IWidgetInterface_MetaDataPlugin，Interface 才有效
	TScriptInterface<IWidgetInterface_MetaDataPlugin> Interface(Widget);
	if (Interface)
	{
		Interface->OnInitialize(Params);
	}
	
	SetInCreatingSubWidget(false);
	
	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 成功创建控件: [%s]"),
		__FUNCTION__, *Widget->GetName());
	
	// if (bOpenAsModal)
	// {
		// OpenAsModalWindow(Widget, ModalWindowSize);
	// }
	
	// 添加 Widget 和 TabID 的关联
	// AddTo_WidgetToTabName(Widget, TabID);

	// 2. 确保 Widget 已在 ActiveWidgets 中（NativeConstruct 应该已添加，这里做防重复检查）
	// if (!ActiveWidgets.Contains(Widget))
	// {
	// 	AddToActiveWidgets(Widget);
	// 	OnWidgetInstanceCreated.Broadcast(Widget);
	// }

	// 3. 调用 IWidgetInterface::OnOpen 进行初始化（如果 Widget 实现了该接口）
	// if (Widget->Implements<UWidgetInterface>())
	// {
	// 	IWidgetInterface::Execute_OnOpen(Widget, FInstancedStruct());
	// }
	
	return Widget;
}

// ==================== RemoveWidget ====================

// void UEUSS_WidgetManager::RemoveWidget(UUserWidget* Widget)
// {
// 	if (!Widget)
// 	{
// 		UE_LOG(LogMetaDataPlugin, Warning, TEXT("[UEUW_WidgetManager] RemoveWidget 失败：Widget 为空"));
// 		return;
// 	}
//
// 	// 1. 调用 IWidgetInterface::OnClose 进行清理
// 	if (Widget->Implements<UWidgetInterface_MetaDataPlugin>())
// 	{
// 		IWidgetInterface_MetaDataPlugin::Execute_OnClose(Widget);
// 	}
//
// 	// 2. 从 ActiveWidgets 中移除
// 	if (ActiveWidgets.Contains(Widget))
// 	{
// 		RemoveFromActiveWidgets(Widget);
// 		OnWidgetInstanceRemoved.Broadcast(Widget);
// 	}
//
// 	// 3. 从父容器中移除（会触发 NativeDestruct，其中会做防重复检查）
// 	Widget->RemoveFromParent();
//
// 	UE_LOG(LogMetaDataPlugin, Log, TEXT("[UEUW_WidgetManager] RemoveWidget 已移除控件，当前活跃控件数：%d"), ActiveWidgets.Num());
// }

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

// void UEUSS_WidgetManager::OpenAsModalWindow(UUserWidget* Widget, FVector2D WindowSize)
// {
// 	// 获取父窗口
// 	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
//     
// 	// 创建窗口
// 	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
// 		.Title(FText::FromString(TEXT("模态窗口")))
// 		.ClientSize(WindowSize);
//
// 	// EUW 转换为 Slate 框架能使用的 SWidget
// 	TSharedRef<SWidget> SlateContent = Widget->TakeWidget();
//
// 	// 设置窗口的内容
// 	ModalWindow->SetContent(SlateContent);
//
// 	// 打开为模态窗口
// 	FSlateApplication::Get().AddModalWindow(ModalWindow, ParentWindow);
//
// }

void UEUSS_WidgetManager::CloseAllWidgets()
{
	if (!IsValid(RootNode) || !RootNode->Widget.IsValid()) return;

	// 关闭根节点对应的控件，其下所有子节点控件会被一起关闭
	// RemoveNodeAndDescendants(RootNode->Widget.Get());
	TArray<UUserWidget*> Loc_ActiveWidgets = GetActiveWidgets();
	for (UUserWidget* Widget : Loc_ActiveWidgets)
	{
		FName* TabID = WidgetToTabName.Find(Widget);
		if (TabID)
		{
			CloseTabByID(*TabID);
		}
	}
	
	// UUserWidget* RootWidget = RootNode->Widget.Get();
	// if (WidgetToTabName.Find(RootWidget))
	// {
	// 	CloseTabByID(WidgetToTabName[RootWidget]);
	// }

	// 复制数组避免迭代时修改 ActiveWidgets（RemoveWidget 会从中移除元素）
	// TArray<UUserWidget*> WidgetsCopy = ActiveWidgets;
	// for (UUserWidget* Widget : WidgetsCopy)
	// {
	// 	if (!Widget) continue;
	// 	CloseWidgetTab(Widget);
		
		// UEditorUtilityWidget* EU_Widget = Cast<UEditorUtilityWidget>(Widget);
		// if (EU_Widget)
		// {
		
		// if (WidgetToTabName.Find(Widget))
		// {
		// 	CloseTabByID(WidgetToTabName[Widget]);
		// }
		
			// CloseTabByID(GetWidgetTabName(Widget));
		// }
		// else
		// {
		// 只关闭 EUW
		// 	Widget->RemoveFromParent();
		// }
	// }

	
}
