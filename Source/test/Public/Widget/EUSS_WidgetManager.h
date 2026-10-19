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
// DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskComplete);


// 树节点
UCLASS()
class TEST_API UEuwTreeNode : public UObject
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
class TEST_API UEUSS_WidgetManager : public UEditorUtilitySubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Widget 被创建时触发，提供委托，但未使用
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin", meta = (DisplayName = "OnWidgetCreated"))
	FOnWidgetInstanceCreated OnWidgetInstanceCreated;

	// Widget 被移除时触发，提供委托，但未使用
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin", meta = (DisplayName = "OnWidgetRemoved"))
	FOnWidgetInstanceRemoved OnWidgetInstanceRemoved;

	// 资产的同步任务完成时触发
	// UPROPERTY(BlueprintAssignable, Category = "元数据插件", meta = (DisplayName = "任务完成时"))
	// FOnTaskComplete OnTaskComplete;
	
	/** 有资产被删除时触发 */
	// UPROPERTY(BlueprintAssignable, Category = "Widget Manager")
	// FOnAssetDeleted OnAssetDeleted;

	/**
	 * 用于创建 UserWidget
	 *
	 * @param WidgetClass	要创建的 UserWidget 类
	 * @param Params		创建时可以通过 FInstancedStruct 传递参数
	 * @return				创建完成的实例
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	UUserWidget* CreateUserWidget(TSubclassOf<UUserWidget> WidgetClass, const FInstancedStruct& Params);

	/**
	 * 用于创建 EUW
	 * 
	 * @param InBlueprint   要创建的 EUW 类
	 * @param Params		创建时可以通过 FInstancedStruct 传递参数
	 * @return				创建完成的实例
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	UEditorUtilityWidget* CreateEditorUtilityWidget(UEditorUtilityWidgetBlueprint* InBlueprint, const FInstancedStruct& Params);

	/**
	 * RemoveWidget — 移除 Widget，蓝图可调用，默认由 Widget 的 NativeDestruct 函数触发
	 *
	 * @param Widget 要移除的 Widget 实例
	 */
	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")
	// void RemoveWidget(UUserWidget* Widget);

	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")
	// 关闭所有已创建的控件
	void CloseAllWidgets();
	
	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")

	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")

	// void OpenAsModalWindow(UUserWidget* Widget, FVector2D WindowSize);
	
	void AddToActiveWidgets(UUserWidget* Widget);
	void RemoveFromActiveWidgets(UUserWidget* Widget);

	void AddTo_WidgetToTabName(UUserWidget* Widget, FName TabName);
	void RemoveFrom_WidgetToTabName(UUserWidget* Widget);

	// 创建主控件
	void CreateMainWidget();

	// 从配置文件获得主控件的路径
	// FString GetWidgetPathFromConfigFile();
	
	/**
	 * FocusWidgetTab — 将已存在的 EditorUtilityWidget 窗口聚焦到前台
	 *
	 * @param UtilityWidget 已存在的 EditorUtilityWidget 实例
	 * @return 聚焦成功返回 true
	 */
	// bool FocusWidgetTab(UEditorUtilityWidget* UtilityWidget);

	// 有资产被删除时触发
	// void OnAssetRemoved(const FAssetData& AssetData);
	
	// UFUNCTION(BlueprintCallable)
	// TArray<UObject *> GetAssetRefs() { return AssetRefs; }
	
	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")
	
	/** Getter */
	TArray<UUserWidget*> GetActiveWidgets() const { return ActiveWidgets; }

	TMap<TObjectPtr<UUserWidget>, FName> GetWidgetToTabName() const { return WidgetToTabName; }

	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> GetMainWidgetSoftPtr() const { return MainWidgetSoftPtr; }

	bool GetInCreatingSubWidget() const { return bInSubWidgetCreating; }


	/** Setter */
	// UFUNCTION(BlueprintCallable, Category = "Widget Manager")
	void SetMainWidgetSoftPtr(const TSoftObjectPtr<UEditorUtilityWidgetBlueprint>& InMainWidgetSoftPtr) { MainWidgetSoftPtr = InMainWidgetSoftPtr; }

	// UFUNCTION(BlueprintCallable)
	// void SetAssetRefs(const TArray<UObject *> NewAssetRefs) { AssetRefs = NewAssetRefs; }

	void SetInCreatingSubWidget(const bool bInCreating) { bInSubWidgetCreating = bInCreating; }

	
	// 创建树的根节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void SetToRootNode(UUserWidget* InRootWidget);

	// 往树中添加子节点
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void AddToChildNode(UUserWidget* ParentWidget, UUserWidget* ChildWidget);

	// UFUNCTION(BlueprintCallable)
	// 查找某节点下的所有子节点
	TArray<UUserWidget*> GetAllDescendants(UUserWidget* InWidget);

	// 当某个 Widget 关闭时，从树中移除，同时移除所有子节点
	// void RemoveNode(UEditorUtilityWidget* InWidget);
	
	// 在 EUW 的 NativeDestruct 中调用：关闭当前及所有子节点 Widget，并清理树节点
	void RemoveNodeAndDescendants(UUserWidget* InWidget);

	// UFUNCTION(BlueprintCallable)
	// void test();

	// 主控件资产移动或重命名时触发
	void OnMainWidgetMoved(const FAssetData& AssetData, const FString& OldObjectPath);

	void OnTaskStart();
	void OnTaskComplete();
	
private:

	// 创建或查找树节点
	UEuwTreeNode* CreateOrFindTreeNode(UUserWidget* Widget);

	// 关闭一个 Widget 对应的 Tab
	void CloseWidgetTab(UUserWidget* Widget);

	// 从树中移除单个节点
	void RemoveNodeFromTree(UUserWidget* Widget);
	
	// FName GetWidgetTabName(UUserWidget* Widget);

	// 存储已创建的控件列表
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> ActiveWidgets;

	// 主控件蓝图资产
	UPROPERTY()
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> MainWidgetSoftPtr;

	// UEditorUtilityWidget 和 TabID 的映射
	UPROPERTY()
	TMap<TObjectPtr<UUserWidget>, FName> WidgetToTabName;

	// 用于判断是否正在创建 EUW
	bool bInSubWidgetCreating = false;

	/** 资产相关 */
	// 保存所有已添加的资产
	// TArray<TObjectPtr<UObject>> AssetRefs;

	// 存储根节点
	UPROPERTY()
	TObjectPtr<UEuwTreeNode> RootNode;

	// 用于通过 Widget 找到树节点
	UPROPERTY()
	TMap<TWeakObjectPtr<UUserWidget>, TObjectPtr<UEuwTreeNode>> WidgetToNodeMap;

	// 存储待移除的 Widget 列表
	TSet<TWeakObjectPtr<UUserWidget>> PendingRemoval;

	// 设置主控件资产的路径
	void SetMainWidgetPath();

	// 主控件资产的路径
	FString MainWidgetPath;
	
};
