// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Public/EditorUtilitySubsystem.h"
#include "InstancedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "EUSS_WidgetManager.generated.h"

class FAssetSyncTask;
struct FInstancedStruct;
class UUserWidget;
class UEditorUtilityWidget;
class UEditorUtilityWidgetBlueprint;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWidgetInstanceCreated, UUserWidget*, Widget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWidgetInstanceRemoved, UUserWidget*, Widget);


// 树节点
UCLASS()
class METADATAOFASSETS_API UEuwTreeNode : public UObject
{
	GENERATED_BODY()

public:
	
	// 对应的 EUW 实例
	TWeakObjectPtr<UUserWidget> Widget;

	// 父节点
	TWeakObjectPtr<UEuwTreeNode> Parent;

	// 子节点
	UPROPERTY()
	TArray<TObjectPtr<UEuwTreeNode>> Children;
	
};

// EUW 控件生成参数
USTRUCT()
struct FEUWTabSpawnContext
{
	GENERATED_BODY()

	// 默认构造函数
	FEUWTabSpawnContext()
		: Blueprint(nullptr)
		, TabID(NAME_None)
		, StatusBarName(NAME_None)
		, bAddContentBrowserDrawer(false)
		, Params(FInstancedStruct())
	{
	}

	/**
	 * 必须传入 Blueprint 和 TabID
	 *
	 * @param InBlueprint					EUW 蓝图资产
	 * @param InTabID						EUW 的 TabID
	 * @param InStatusBarName				底部 ContentBrowserDrawer 的名称
	 * @param bInAddContentBrowserDrawer	是否添加 ContentBrowserDrawer
	 * @param InParams						初始化时传入的参数
	 */
	FEUWTabSpawnContext(
		UEditorUtilityWidgetBlueprint* InBlueprint,
		const FName InTabID,
		const FName InStatusBarName = NAME_None,
		const bool bInAddContentBrowserDrawer = false,
		FInstancedStruct InParams = FInstancedStruct())
		: Blueprint(InBlueprint)
		, TabID(InTabID)
		, StatusBarName(InStatusBarName)
		, bAddContentBrowserDrawer(bInAddContentBrowserDrawer)
		, Params(MoveTemp(InParams))
	{
	}
	
	TWeakObjectPtr<UEditorUtilityWidgetBlueprint> Blueprint;

	FName TabID;

	FName StatusBarName;
	
	bool bAddContentBrowserDrawer;

	UPROPERTY()
	FInstancedStruct Params;

};


/**
 * 负责统一管理所有通过本插件创建的控件
 */
UCLASS()
class METADATAOFASSETS_API UEUSS_WidgetManager : public UEditorUtilitySubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	
	// 控件被创建时触发，提供委托，但未使用
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets", meta = (DisplayName = "OnWidgetCreated"))
	FOnWidgetInstanceCreated OnWidgetInstanceCreated;

	// 控件被移除时触发，提供委托，但未使用
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets", meta = (DisplayName = "OnWidgetRemoved"))
	FOnWidgetInstanceRemoved OnWidgetInstanceRemoved;


	// 从配置中获取是否启用打印字符串设置
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MetaDataOfAssets")
	bool GetEnableDebugSetting();

	/**
	 * 用于创建 UserWidget
	 *
	 * @param WidgetClass	要创建的 UserWidget 类
	 * @param Params		创建时可以通过 FInstancedStruct 传递参数
	 * @return				创建完成的实例
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	UUserWidget* CreateUserWidget(TSubclassOf<UUserWidget> WidgetClass, const FInstancedStruct& Params);

	/**
	 * 用于创建 EditorUtilityWidget
	 * 
	 * @param InBlueprint   要创建的 EUW 类
	 * @param Params		创建时可以通过 FInstancedStruct 传递参数
	 * @return				创建完成的实例
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	UEditorUtilityWidget* CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, FInstancedStruct Params);

	// 创建树的根节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SetToRootNode(UUserWidget* InRootWidget);

	// 往树中添加子节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void AddToChildNode(UUserWidget* ParentWidget, UUserWidget* ChildWidget);

	// 查找某节点下的所有子节点
	TArray<UUserWidget*> GetAllDescendants(UUserWidget* InWidget);

	// 在 EUW 的 NativeDestruct 中调用：关闭当前及所有子节点 Widget，并清理树节点
	// void RemoveNodeAndDescendants(UUserWidget* InWidget);

	// 注册主控件
	void RegisterMainWidgetTab();
	
	// 创建主控件
	void CreateMainWidget();

	// 关闭旧的控件
	void CloseOldWidget(UEditorUtilityWidgetBlueprint* InBlueprint);

	// 尝试聚焦到指定的 EUW 控件，返回是否聚焦成功
	bool TryFocusEditorUtilityTab(FName InTabID);

	// 主控件资产移动或重命名时触发，确保主窗口的资产移动后，下次启动编辑器依然能打开
	void OnMainWidgetMoved(const FAssetData& AssetData, const FString& OldObjectPath);
	
	// 通过 TabID 关闭对应的 EUW
	bool CloseEditorUtilityTab(FName InTabID);

	// 关闭所有已创建的控件
	void CloseAllWidgets();

	// 开启和关闭调试模式
	void DisableDebug();
	void EnableDebug();
	
	// 添加和移除 ActiveWidgets
	void AddToActiveWidgets(UUserWidget* Widget);
	void RemoveFromActiveWidgets(UUserWidget* Widget);

	// 添加和移除 WidgetToTabName
	void AddTo_WidgetToTabName(UUserWidget* Widget, FName TabName);
	void RemoveFrom_WidgetToTabName(UUserWidget* Widget);

	// 资产同步任务开始和完成时
	void OnTaskStart();
	void OnTaskComplete();
	
	
	/** Getter */
	TArray<UUserWidget*> GetActiveWidgets() const { return ActiveWidgets; }

	TMap<TObjectPtr<UUserWidget>, FName> GetWidgetToTabName() const { return WidgetToTabName; }

	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> GetMainWidgetSoftPtr() const { return MainWidgetSoftPtr; }

	// bool GetInCreatingSubWidget() const { return bInSubWidgetCreating; }
	
	/** Setter */
	void SetMainWidgetSoftPtr(const TSoftObjectPtr<UEditorUtilityWidgetBlueprint>& InMainWidgetSoftPtr) { MainWidgetSoftPtr = InMainWidgetSoftPtr; }

	// void SetInCreatingSubWidget(const bool bInCreating) { bInSubWidgetCreating = bInCreating; }

private:

	// 注册标签页
	bool RegisterTab_Custom(const FEUWTabSpawnContext& InSpawnContext);

	// 注册并生成子控件
	UEditorUtilityWidget* SpawnAndRegisterTabWithId_Custom(UEditorUtilityWidgetBlueprint* InBlueprint, const FName InTabID, FInstancedStruct Params);

	void OnBlueprintCompiled(UBlueprint* InCompiledBlueprint, const FName InTabID);
	void RestoreTabContent(const FName InTabID);

	TSharedPtr<SDockTab> FindExistingTab(FName TabID);
	
	// 用于判断是否正在创建 EUW
	// bool bInSubWidgetCreating = false;
	
	// 创建或查找树节点
	UEuwTreeNode* CreateOrFindTreeNode(UUserWidget* Widget);

	// 从树中移除单个节点
	void RemoveNodeFromTree(UUserWidget* Widget);
	
	void CloseDescendantTabs(UUserWidget* RootWidget);
	
	// 关闭一个 Widget 对应的 Tab
	void CloseWidgetTab(UUserWidget* Widget);
	
	// 加载主控件的资产
	UEditorUtilityWidgetBlueprint* LoadMainWidget();

	// 构建主控件的 TabID
	FName MakeMainWidgetTabID(const UEditorUtilityWidgetBlueprint* Blueprint);

	// 构建 StatusBar 的名称
	FName MakeStatusBarName(const UEditorUtilityWidgetBlueprint* Blueprint);

	// 生成控件
	void SpawnRegisteredTabByID_Custom(const FEUWTabSpawnContext& SpawnContext);

	// 设置主控件资产的路径
	void SetMainWidgetPath();
	
	// 标签页被关闭时
	void OnTabClosed(TSharedRef<SDockTab> ClosedTab, TWeakObjectPtr<UUserWidget> WeakWidget, TWeakObjectPtr<UEditorUtilityWidgetBlueprint> WeakBlueprint);
	
	// 标签页被创建时
	TSharedRef<SDockTab> OnSpawnEUWTab(const FSpawnTabArgs& Args, const FName InTabID);

	// 创建子控件的标签页
	// TSharedRef<SDockTab> SpawnSubWidgetTab(const FSpawnTabArgs& Args, UEditorUtilityWidgetBlueprint* InBlueprint);
	
	// 主控件资产的路径
	FString MainWidgetPath;
	
	// 存储已创建的控件列表
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> ActiveWidgets;

	// 主控件蓝图资产软引用对象
	UPROPERTY()
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> MainWidgetSoftPtr;

	// 主控件蓝图资产
	UPROPERTY()
	TObjectPtr<UEditorUtilityWidgetBlueprint> MainWidget;
	
	// EUW 和 TabID 的映射
	UPROPERTY()
	TMap<TObjectPtr<UUserWidget>, FName> WidgetToTabName;
	
	// 存储根节点
	UPROPERTY()
	TObjectPtr<UEuwTreeNode> RootNode;
	
	// 用于通过 Widget 找到树节点
	UPROPERTY()
	TMap<TWeakObjectPtr<UUserWidget>, TObjectPtr<UEuwTreeNode>> WidgetToNodeMap;
	
	// 标记待移除的 Widget 列表
	TSet<TWeakObjectPtr<UUserWidget>> PendingRemoval;

	// 保存 EUW 创建的 TabID 和 生成参数
	UPROPERTY()
	TMap<FName, FEUWTabSpawnContext> TabSpawnContexts;

	// 防止快速编译多次从而生成多个 ContentBrowserDrawer
	TSet<FName> PendingTabContentRestore;

	// 保存已注册的 Spawner 的 TabID
	TSet<FName> RegisteredTabID;
	
	FTSTicker::FDelegateHandle RetryTickerHandle;
	FDelegateHandle CompiledHandle;
};
