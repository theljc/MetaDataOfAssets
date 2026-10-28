// Copyright 2026 ljc

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Public/EditorUtilitySubsystem.h"
#include "InstancedStruct.h"
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
	UEditorUtilityWidget* CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, const FInstancedStruct& Params);

	// 创建树的根节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SetToRootNode(UUserWidget* InRootWidget);

	// 往树中添加子节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void AddToChildNode(UUserWidget* ParentWidget, UUserWidget* ChildWidget);

	// 查找某节点下的所有子节点
	TArray<UUserWidget*> GetAllDescendants(UUserWidget* InWidget);

	// 在 EUW 的 NativeDestruct 中调用：关闭当前及所有子节点 Widget，并清理树节点
	void RemoveNodeAndDescendants(UUserWidget* InWidget);
	
	// 创建主控件
	void CreateMainWidget();

	// 聚焦到指定的 EUW 控件
	bool FocusEditorUtilityTab(FName TabID);

	// 主控件资产移动或重命名时触发，确保主窗口的资产移动后，下次启动编辑器依然能打开
	void OnMainWidgetMoved(const FAssetData& AssetData, const FString& OldObjectPath);
	
	// 通过 TabID 关闭对应的 EUW
	bool CloseEditorUtilityTab(FName TabID);

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

	bool GetInCreatingSubWidget() const { return bInSubWidgetCreating; }
	
	/** Setter */
	void SetMainWidgetSoftPtr(const TSoftObjectPtr<UEditorUtilityWidgetBlueprint>& InMainWidgetSoftPtr) { MainWidgetSoftPtr = InMainWidgetSoftPtr; }

	void SetInCreatingSubWidget(const bool bInCreating) { bInSubWidgetCreating = bInCreating; }

private:

	// 用于判断是否正在创建 EUW
	bool bInSubWidgetCreating = false;
	
	// 创建或查找树节点
	UEuwTreeNode* CreateOrFindTreeNode(UUserWidget* Widget);

	// 从树中移除单个节点
	void RemoveNodeFromTree(UUserWidget* Widget);
	
	// 关闭一个 Widget 对应的 Tab
	void CloseWidgetTab(UUserWidget* Widget);
	
	// 设置主控件资产的路径
	void SetMainWidgetPath();

	// 主控件资产的路径
	FString MainWidgetPath;
	
	// 存储已创建的控件列表
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> ActiveWidgets;

	// 主控件蓝图资产
	UPROPERTY()
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> MainWidgetSoftPtr;

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
	
};
