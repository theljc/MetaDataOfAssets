// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilitySubsystem.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "UObject/SoftObjectPath.h"
#include "EUSS_MetaDataManager.generated.h"

class FAssetSyncTask;
struct FAssetData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetDeleted, FSoftObjectPath, AssetPath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetSynced, UObject*, Asset);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataAdded, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataDeleted, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaDataModified, UObject*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetaDataCopied, UObject*, SourceAsset, UObject*, TargetAsset);


struct FAssetMetaDataState
{
	// 保存此资产的元数据快照
	TMap<FName, FString> MetaDataSnapshot;

	// 此资产实际计入引用计数的 Tag
	TSet<FName> CountedTags;
};

struct FMetaDataUpdateResult
{
	// 资产自身的元数据相对上一次快照是否发生变化
	bool bAssetChanged = false;

	// AssetRegistry 中该资产的 Tag 是否需要刷新
	bool bRegistryTagsChanged = false;
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
	void SetAddedAssets(const TArray<UObject*>& InAssetRefs);

	// 获得上一次主控件关闭时已添加的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	TArray<UObject*> GetAddedAssets() const;
	
	// 扫描项目设置中配置路径的资产
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SyncAssetsInDirectory();

	// 获得规范化的路径，确保没有重复的路径
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	TArray<FScanDirectorySetting> GetNormalizedPathSettings(const TArray<FScanDirectorySetting>& InSettings);
	
	// 添加元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void AddMetaData(UObject* InAsset, const TArray<FName>& InTagToAdd, const TArray<FString>& InValueToAdd);

	// 修改元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void ModifyMetaData(UObject* InAsset, const TArray<FName>& InTagToModify, const TArray<FString>& InValueToModify);

	// 删除元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void DeleteMetaData(UObject* InAsset, const TArray<FName>& InTagToDelete);

	// 复制元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void CopyMetaData(const TArray<UObject*> InSourceAsset, const TArray<UObject*> InTargetAsset);
	
	// 同步资产元数据
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	void SyncAsset(UObject* InAsset);

	
	// 资产删除时的回调函数
	void OnAssetRemoved(const FAssetData& InAssetData);

	// 资产重命名或移动时清理旧路径状态，不迁移到新路径
	void OnAssetMoved(const FAssetData& InAssetData, const FString& InOldObjectPath);
	
	// 资产同步任务完成时的回调函数
	void OnTaskFinished(FAssetSyncTask* InFinishedTask);
	
private:
	
	// 添加标签到资产注册表
	void AddTagToRegisteredTags(const TArray<FName>& InTagToAdd, UObject* InAsset);

	// 从资产注册表移除标签
	void RemoveTagFromRegisteredTags(const TArray<FName>& InTagToRemove, UObject* InAsset);
	
	// 注册标签到全局资产注册表
	void AddTagToAssetRegistry(const TArray<FName>& InTagToAdd);

	// 从资产注册表中移除
	void RemoveTagFromAssetRegistry(const TArray<FName>& InTagToRemove);
	
	// 计算元数据修改前的快照，是后续计算元数据差异的基准
	void CalcAssetTagStateBaseline(UObject* InAsset);
	
	// 更新资产的标签状态
	FMetaDataUpdateResult UpdateAssetTagStates(UObject* InAsset);
	
	// 资产被删除时，从已注册的标签中移除
	void RemoveFromAssetTagStates(const FSoftObjectPath& InAssetPath);

	// 资产被删除时，从已添加资产列表中移除
	void RemoveAddedAsset(const FSoftObjectPath& InAssetPath);

	
	// 保存所有已添加的资产
	TArray<TSoftObjectPtr<UObject>> AddedAssets;
	
	// 以引用计数的方式保存已注册的标签
	TMap<FName, int32> RegisteredTags;

	// 此变量用于标记资产的元数据是否被同步过，外层 key 表示资产路径，内层结构体表示元数据的键值对
	TMap<FSoftObjectPath, FAssetMetaDataState> AssetTagStates;

	// 资产同步任务
	TSharedPtr<FAssetSyncTask> AssetSyncTask;
	
};
