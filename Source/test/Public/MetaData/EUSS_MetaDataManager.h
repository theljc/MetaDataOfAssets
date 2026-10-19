#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Public/EditorUtilitySubsystem.h"
#include "EUSS_MetaDataManager.generated.h"


// 由于动态委托不能传入 TMap 之类的复杂类型，所以用结构体替代
// USTRUCT(BlueprintType)
// struct FIntObjectPair
// {
// 	GENERATED_BODY()
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	int32 Key;
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	UObject* Value;
// };

class FAssetSyncTask;
class UMetaDataPluginSettings;
struct FScanDirectorySetting;
class UWidget;

// 创建 Widget 后，显示模态框前，将数据进行广播（因为模态框会暂停后续逻辑的执行）
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWidgetCreated, UObject*, AssetRef, const TArray<FIntObjectPair>&, AssetMap, UEditorUtilityWidget*, Widget);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBatchAddWidgetCreated, UEditorUtilityWidget*, EdtiorUtilityWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetDeleted, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetSynced, UObject*, Asset);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataAdded, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataDeleted, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataModified, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetaDataCopied, UObject*, SourceAsset, UObject*, TargetAsset);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetaDataAdded, UObject*, AddedObject, , MetaData);

// DeleteMode
USTRUCT()
struct FMetaDataMap
{
	GENERATED_BODY()
	
	TMap<FName, uint32> Map;
};

/**
 * 负责管理元数据
 */
UCLASS()
class TEST_API UEUSS_MetaDataManager : public UEditorUtilitySubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 元数据被添加时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnMetaDataAdded OnMetaDataAdded;

	// 元数据被删除时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnMetaDataDeleted OnMetaDataDeleted;

	// 元数据被修改时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnMetaDataModified OnMetaDataModified;

	// 元数据被复制时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnMetaDataCopied OnMetaDataCopied;

	// 资产被同步时的回调（未使用）
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnAssetSynced OnAssetSynced;

	// 资产删除回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataPlugin")
	FOnAssetDeleted OnAssetDeleted;
	
	// 添加元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void AddMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd);

	// 修改元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void ModifyMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd);

	// 删除元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void DeleteMetaData(UObject* Asset, FName TagToAdd);

	// 复制元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void CopyMetaData(TArray<UObject*> SourceAsset, TArray<UObject*> TargetAsset);
	
	// 添加标签到资产注册表
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void AddTagToRegisteredTags(const FName TagToAdd, UObject* Asset);

	// DeleteMode
	// 从资产注册表移除标签
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void RemoveTagFromRegisteredTags(const FName TagToRemove, UObject* Asset);
	
	// 注册标签到全局资产注册表
	void AddTagToAssetRegistry(const FName TagToAdd);

	// DeleteMode
	// 从资产注册表中移除
	void RemoveTagFromAssetRegistry(const FName TagToRemove, UObject* Asset);
	// 从全局移除时，用于移除配置文件中的对应标签
	// FString RemoveTagFromMetaDataString(const FString& InputString, const FName& TagToRemove);

	// 同步资产元数据
	void SyncAsset(UObject* Asset);
	
	// 打开模态框
	// UFUNCTION(BlueprintCallable)
	// UUserWidget* OpenModalWindow(TSubclassOf<UUserWidget> BatchAddWidget, UObject* AssetRef, TArray<FIntObjectPair> AssetInfoMap, UEditorUtilityWidget* UtilityWidget, FVector2D WindowSize);
	
	// UFUNCTION(BlueprintCallable)
	// UEditorUtilityWidget* OpenModalWindow_EUW(UEditorUtilityWidgetBlueprint* InBlueprint, FVector2D WindowSize);
	
	
	// 创建主窗口
	// void CreateWidgetBP();
	
	// UFUNCTION(BlueprintCallable)
	// FVector2D GetAbsolutePosition(UWidget* Widget);
	
	// 保存主窗口的蓝图资产
	// UPROPERTY(BlueprintReadWrite, Category = "MetaData Plugin")
	// TSoftObjectPtr<UEditorUtilityWidgetBlueprint> My_EUBP;
	
	// 保存模态框的蓝图资产
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My Plugin Settings")
	// TSubclassOf<UUserWidget> BatchAddWidget;

	// UPROPERTY(BlueprintAssignable)
	// FOnWidgetCreated OnWidgetCreated;

	// UPROPERTY(BlueprintAssignable)
	// FOnBatchAddWidgetCreated OnBatchAddWidgetCreated;
	

	// UFUNCTION(BlueprintCallable)
	// void OnAssetSaved();

	
	// 主控件关闭时，设置已添加的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void SetAddedAssets(const TArray<UObject*> NewAssetRefs) { AddedAssets = NewAssetRefs; }

	// 获得上一次主控件关闭时已添加的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	TArray<UObject*> GetAddedAssets() { return AddedAssets; }

	// DeleteMode
	// 扫描项目设置中配置路径的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	void SyncAssetsInDirectory();

	// 获得项目设置中的配置
	const UMetaDataPluginSettings* GetMetaDataPluginSettings();

	// 获得规范化的路径，确保没有重复的路径
	UFUNCTION(BlueprintCallable, Category = "MetaDataPlugin")
	TArray<FScanDirectorySetting> GetNormalizedPathSettings(const TArray<FScanDirectorySetting>& Settings);
	
	// 扫描已添加到主控件中的资产
	// UFUNCTION(BlueprintCallable)
	// void SyncAssetsInMainWidget(TArray<UObject*> Assets);

	void UpdateAssetTagStates(UObject* Asset);

	// DeleteMode
	// 资产被删除时，从已注册的标签中移除
	void RemoveFromAssetTagStates(UObject* Asset);
	// 资产删除回调
	void OnAssetRemoved(const FAssetData& AssetData);
	
	// UFUNCTION(BlueprintCallable)
	// TArray<FName> GetCommonMetadataKeys(const TArray<UObject*>& Assets);

	// 在 Initialize 时从项目设置中同步值，用于确定是否要自动删除资产注册表中的 MetaData Tag
	UPROPERTY(BlueprintReadOnly, Category = "MetaDataPlugin")
	bool bAllowManageAssetRegistryTagsDelete;

	void OnTaskFinished(FAssetSyncTask* FinishedTask);
	
private:
	
	// DeleteMode
	void RemoveSavedMetaData(UObject* Asset, TMap<FName, uint32>* TagMapToAdd);

	void AddActualMetaData(UObject* Asset, const TMap<FName, FString>& ObjectMetaDataMap, TMap<FName, uint32>* TagMapToAdd);
	
	// TMap<FName, FString>* GetObjectMetaDataMap(UObject* Asset);

	// DeleteMode
	// 以引用计数的方式保存已注册的标签
	// UPROPERTY(Config)
	TMap<FName, int32> RegisteredTags;
	
	// 保存所有已添加的资产
	UPROPERTY()
	TArray<TObjectPtr<UObject>> AddedAssets;

	// DeleteMode
	// 此变量用于标记资产的元数据，外层 key 表示资产路径，内层 key 表示元数据的键，内层 value 表示元数据的值的哈希值
	// UPROPERTY(Config)
	TMap<FSoftObjectPath, FMetaDataMap> AssetTagStates;
	
	// 委托句柄，用于解绑
	// FDelegateHandle AssetRemovedHandle;
	

	// 资产同步任务
	TSharedPtr<FAssetSyncTask> AssetSyncTask;
	// 每帧扫描的资产数量
	// int32 BatchSize;
	
};
