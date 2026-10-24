#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Public/EditorUtilitySubsystem.h"
#include "EUSS_MetaDataManager.generated.h"

class FAssetSyncTask;
class UMetaDataOfAssetsSettings;
struct FScanDirectorySetting;
class UWidget;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetDeleted, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetSynced, UObject*, Asset);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataAdded, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataDeleted, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataModified, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetaDataCopied, UObject*, SourceAsset, UObject*, TargetAsset);


USTRUCT()
struct FMetaDataMap
{
	GENERATED_BODY()
	
	TMap<FName, FString> Map;
};


/**
 * 负责管理元数据
 */
UCLASS()
class METADATAOFASSETS_API UEUSS_MetaDataManager : public UEditorUtilitySubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// 在 Initialize 时从项目设置中同步值，用于确定是否要自动删除资产注册表中的 MetaData Tag
	UPROPERTY(BlueprintReadOnly, Category = "MetaDataOfAssets")
	bool bAllowManageAssetRegistryTagsDelete;
	
	// 元数据被添加时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnMetaDataAdded OnMetaDataAdded;

	// 元数据被删除时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnMetaDataDeleted OnMetaDataDeleted;

	// 元数据被修改时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnMetaDataModified OnMetaDataModified;

	// 元数据被复制时的回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnMetaDataCopied OnMetaDataCopied;

	// 资产被同步时的回调（未使用）
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnAssetSynced OnAssetSynced;

	// 资产删除回调
	UPROPERTY(BlueprintAssignable, Category = "MetaDataOfAssets")
	FOnAssetDeleted OnAssetDeleted;


	// 主控件关闭时，设置已添加的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SetAddedAssets(const TArray<UObject*> NewAssetRefs) { AddedAssets = NewAssetRefs; }

	// 获得上一次主控件关闭时已添加的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	TArray<UObject*> GetAddedAssets() { return AddedAssets; }
	
	// 扫描项目设置中配置路径的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SyncAssetsInDirectory();

	// 获得规范化的路径，确保没有重复的路径
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	TArray<FScanDirectorySetting> GetNormalizedPathSettings(const TArray<FScanDirectorySetting>& Settings);
	
	// 添加元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void AddMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd);

	// 修改元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void ModifyMetaData(UObject* Asset, FName TagToAdd, FString ValueToAdd);

	// 删除元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void DeleteMetaData(UObject* Asset, FName TagToAdd);

	// 复制元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void CopyMetaData(TArray<UObject*> SourceAsset, TArray<UObject*> TargetAsset);
	
	// 添加标签到资产注册表
	void AddTagToRegisteredTags(const FName TagToAdd, UObject* Asset);

	// 从资产注册表移除标签
	void RemoveTagFromRegisteredTags(const FName TagToRemove, UObject* Asset);
	
	// 注册标签到全局资产注册表
	void AddTagToAssetRegistry(const FName TagToAdd);

	// 从资产注册表中移除
	void RemoveTagFromAssetRegistry(const FName TagToRemove, UObject* Asset);
	
	// 在删除模式下，更新资产的标签状态
	void UpdateAssetTagStates(UObject* Asset);

	// 资产被删除时，从已注册的标签中移除
	void RemoveFromAssetTagStates(UObject* Asset);

	// 同步资产元数据
	void SyncAsset(UObject* Asset);

	// 资产删除回调
	void OnAssetRemoved(const FAssetData& AssetData);
	
	// 资产同步任务完成时的回调
	void OnTaskFinished(FAssetSyncTask* FinishedTask);
	
private:
	
	// 移除已保存的旧的元数据，并更新 AssetRegistry
	void RemoveSavedMetaData(UObject* Asset, TMap<FName, FString>* TagMapToAdd);

	// 添加实际的新的元数据，并更新 AssetRegistry
	void AddActualMetaData(UObject* Asset, const TMap<FName, FString>& ObjectMetaDataMap, TMap<FName, FString>* TagMapToAdd);
	
	
	// 保存所有已添加的资产
	UPROPERTY()
	TArray<TObjectPtr<UObject>> AddedAssets;
	
	// 以引用计数的方式保存已注册的标签
	TMap<FName, int32> RegisteredTags;

	// 此变量用于标记资产的元数据是否被同步过，外层 key 表示资产路径，内层结构体表示元数据的键值对
	TMap<FSoftObjectPath, FMetaDataMap> AssetTagStates;
	
	// 资产同步任务
	TSharedPtr<FAssetSyncTask> AssetSyncTask;
	
};
