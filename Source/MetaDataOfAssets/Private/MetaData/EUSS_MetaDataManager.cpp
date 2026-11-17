// Copyright 2026 ljc. All rights reserved.

#include "MetaData/EUSS_MetaDataManager.h"
#include "EditorAssetLibrary.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "Engine/AssetManagerSettings.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "Task/FAssetScanTask.h"


void UEUSS_MetaDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!IsValid(Settings)) return;

	// 只有编辑器重启后才重新赋值
	bAllowManageAssetRegistryTagsDelete = Settings->bAllowManageAssetRegistryTagsDelete;
	
}

void UEUSS_MetaDataManager::Deinitialize()
{
	if (AssetSyncTask.IsValid())
	{
		// 取消资产同步任务
		AssetSyncTask->Cancel();
		AssetSyncTask.Reset();
	}
	
	Super::Deinitialize();
}

void UEUSS_MetaDataManager::SetAddedAssets(const TArray<UObject*>& InAssetRefs)
{
	AddedAssets.Reset();
	AddedAssets.Reserve(InAssetRefs.Num());

	TSet<FSoftObjectPath> AddedPaths;
	for (UObject* Asset : InAssetRefs)
	{
		if (!IsValid(Asset)) continue;

		const FSoftObjectPath AssetPath(Asset);
		bool bAlreadyExists = false;
		// 是否有重复资产
		AddedPaths.Add(AssetPath, &bAlreadyExists);
		
		// 不添加重复资产
		if (!bAlreadyExists)
		{
			AddedAssets.Add(TSoftObjectPtr<UObject>(AssetPath));
		}
	}
}

TArray<UObject*> UEUSS_MetaDataManager::GetAddedAssets() const
{
	TArray<UObject*> Result;
	Result.Reserve(AddedAssets.Num());

	// 遍历已添加的资产
	for (const TSoftObjectPtr<UObject>& SoftAsset : AddedAssets)
	{
		// 判断是否已加载
		UObject* Asset = SoftAsset.Get();
		if (!IsValid(Asset))
		{
			// 未加载则加载
			Asset = SoftAsset.LoadSynchronous();
		}

		if (IsValid(Asset))
		{
			Result.Add(Asset);
		}
	}

	return Result;
}

TArray<FScanDirectorySetting> UEUSS_MetaDataManager::GetNormalizedPathSettings(const TArray<FScanDirectorySetting>& InSettings)
{
	TArray<FScanDirectorySetting> Result;

	if (InSettings.IsEmpty()) return Result;

	// 保存相同路径的多个配置
	TMap<FString, FScanDirectorySetting> UniqueSettingsByPath;

	// 规范化路径，并按路径合并重复配置
	for (const FScanDirectorySetting& PathSetting : InSettings)
	{
		FScanDirectorySetting Normalized = PathSetting;
		FPaths::NormalizeDirectoryName(Normalized.Directory.Path);

		const FString PathKey = Normalized.Directory.Path.ToLower();
		if (FScanDirectorySetting* ExistingSetting = UniqueSettingsByPath.Find(PathKey))
		{
			// 同一路径存在递归和非递归配置时，合并为递归配置
			ExistingSetting->bRecursive |= Normalized.bRecursive;
		}
		else
		{
			// 确保相同路径的多个配置只保留一条
			UniqueSettingsByPath.Add(PathKey, MoveTemp(Normalized));
		}
	}

	// 获得合并后的路径配置
	TArray<FScanDirectorySetting> NormalizedSettings;
	NormalizedSettings.Reserve(UniqueSettingsByPath.Num());
	for (TPair<FString, FScanDirectorySetting>& Pair : UniqueSettingsByPath)
	{
		NormalizedSettings.Add(MoveTemp(Pair.Value));
	}

	// 按路径长度排序，短路径优先
	NormalizedSettings.Sort([](const FScanDirectorySetting& A, const FScanDirectorySetting& B)
	{
		if (A.Directory.Path.Len() != B.Directory.Path.Len())
		{
			return A.Directory.Path.Len() < B.Directory.Path.Len();
		}

		return A.Directory.Path < B.Directory.Path;
	});

	// 过滤掉被父目录递归扫描覆盖的子目录
	for (const FScanDirectorySetting& Current : NormalizedSettings)
	{
		bool bIsCovered = false;
		for (const FScanDirectorySetting& Effective : Result)
		{
			// IsUnderDirectory 判断 Current 路径是否在 Effective 路径下
			if (Effective.bRecursive &&
				FPaths::IsUnderDirectory(Current.Directory.Path, Effective.Directory.Path))
			{
				bIsCovered = true;
				break;
			}
		}

		if (!bIsCovered)
		{
			Result.Add(Current);
		}
	}

	return Result;
}

void UEUSS_MetaDataManager::SyncAssetsInDirectory()
{
	// 是否允许管理元数据删除，不允许则不需要扫描资产
	if (!bAllowManageAssetRegistryTagsDelete) return;
	
	const UMetaDataOfAssetsSettings* Settings = GetDefault<UMetaDataOfAssetsSettings>();
	if (!IsValid(Settings)) return;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	// 获得规范化的路径
	TArray<FScanDirectorySetting> NormalizedPathSettings = GetNormalizedPathSettings(Settings->ScanSettings);

	TArray<FAssetData> AssetDataList;

	// 获得路径下所有的资产数据
	for (FScanDirectorySetting& Setting : NormalizedPathSettings)
	{
		if (Setting.Directory.Path.IsEmpty()) continue;
		
		TArray<FAssetData> AssetDataList_Sub;
		AssetRegistry.GetAssetsByPath(FName(*Setting.Directory.Path), AssetDataList_Sub, Setting.bRecursive);

		AssetDataList.Append(AssetDataList_Sub);
	}
	
	// 防止重复启动任务
	if (AssetSyncTask.IsValid() || AssetDataList.IsEmpty()) return;

	// 创建并启动资产同步任务
	AssetSyncTask = MakeShared<FAssetSyncTask>(AssetDataList);
	AssetSyncTask->Start();
	
}

void UEUSS_MetaDataManager::OnTaskFinished(FAssetSyncTask* InFinishedTask)
{
	// 任务结束后，重置指针
	if (AssetSyncTask.IsValid() && AssetSyncTask.Get() == InFinishedTask)
	{
		AssetSyncTask.Reset();
	}
}

void UEUSS_MetaDataManager::CalcAssetTagStateBaseline(UObject* InAsset)
{
	if (!IsValid(InAsset))
	{
		return;
	}

	// 已存在表示资产已经被同步过
	const FSoftObjectPath AssetPath(InAsset);
	if (AssetTagStates.Contains(AssetPath))
	{
		return;
	}

	RemovedAssetPaths.Remove(AssetPath);

	// 资产首次同步时，添加元数据快照
	FAssetMetaDataState& Baseline = AssetTagStates.Add(AssetPath);
	Baseline.MetaDataSnapshot = UEditorAssetLibrary::GetMetadataTagValues(InAsset);
}

FMetaDataUpdateResult UEUSS_MetaDataManager::UpdateAssetTagStates(UObject* InAsset)
{
	FMetaDataUpdateResult Result;
	if (!IsValid(InAsset)) return Result;

	TMap<FName, FString> ObjectMetaDataMap = UEditorAssetLibrary::GetMetadataTagValues(InAsset);
	
	// 构建 FSoftObjectPath
	FSoftObjectPath AssetPath(InAsset);

	// 查找资产已保存的元数据快照
	FAssetMetaDataState* ExistingTagMap = AssetTagStates.Find(AssetPath);
	const bool bHadExistingState = ExistingTagMap != nullptr;

	TMap<FName, FString> PreviousMetaData;
	TSet<FName> PreviousCountedTags;
	if (ExistingTagMap)
	{
		PreviousMetaData = ExistingTagMap->MetaDataSnapshot;
		PreviousCountedTags = ExistingTagMap->CountedTags;
	}

	// 只移除真实删除的 Tag
	for (const TPair<FName, FString>& Previous : PreviousMetaData)
	{
		if (!ObjectMetaDataMap.Contains(Previous.Key))
		{
			Result.bRegistryTagsChanged = true;
			Result.bAssetChanged |= bHadExistingState;

			if (PreviousCountedTags.Contains(Previous.Key))
			{
				RemoveTagFromRegisteredTags(Previous.Key, InAsset);
			}
		}
	}

	// 只添加真实新增的 Tag
	bool bValueChanged = false;
	TSet<FName> CurrentCountedTags;
	for (const TPair<FName, FString>& Current : ObjectMetaDataMap)
	{
		const FString* PreviousValue = PreviousMetaData.Find(Current.Key);
		if (!PreviousValue)
		{
			Result.bRegistryTagsChanged = true;
			Result.bAssetChanged |= bHadExistingState;

			AddTagToRegisteredTags(Current.Key, InAsset);
			if (bAllowManageAssetRegistryTagsDelete)
			{
				CurrentCountedTags.Add(Current.Key);
			}
		}
		else
		{
			if (PreviousCountedTags.Contains(Current.Key))
			{
				CurrentCountedTags.Add(Current.Key);
			}

			if (*PreviousValue != Current.Value)
			{
				bValueChanged = true;
				Result.bAssetChanged = true;
				Result.bRegistryTagsChanged = true;
			}
		}
	}

	// 最后使用真实元数据覆盖快照
	FAssetMetaDataState& NewTagMap = AssetTagStates.FindOrAdd(AssetPath);
	NewTagMap.MetaDataSnapshot = MoveTemp(ObjectMetaDataMap);
	NewTagMap.CountedTags = MoveTemp(CurrentCountedTags);

	if (bValueChanged)
	{
		OnMetaDataModified.Broadcast(InAsset);
	}

	return Result;
}

void UEUSS_MetaDataManager::RemoveFromAssetTagStates(const FSoftObjectPath& InAssetPath)
{
	FAssetMetaDataState* TagMapToRemove = AssetTagStates.Find(InAssetPath);
	// 判断资产是否已同步过
	if (!TagMapToRemove) return;

	if (bAllowManageAssetRegistryTagsDelete)
	{
		for (const FName& CountedTag : TagMapToRemove->CountedTags)
		{
			// 查找 Tag 的引用计数
			int32* CountPtr = RegisteredTags.Find(CountedTag);
			if (!CountPtr) continue;

			int32& Count = *CountPtr;
			// 引用计数减 1
			--Count;

			// 引用计数为 0 时，从资产注册表移除
			if (Count <= 0)
			{
				RegisteredTags.Remove(CountedTag);
				RemoveTagFromAssetRegistry(CountedTag);
			}
		}
	}

	AssetTagStates.Remove(InAssetPath);
}

void UEUSS_MetaDataManager::RemoveAddedAsset(const FSoftObjectPath& InAssetPath)
{
	// 从 AddedAssets 中移除 InAssetPath 对应的资产
	AddedAssets.RemoveAll(
		[&InAssetPath](const TSoftObjectPtr<UObject>& SoftAsset)
		{
			return SoftAsset.ToSoftObjectPath() == InAssetPath;
		});
}

void UEUSS_MetaDataManager::OnAssetRemoved(const FAssetData& InAssetData)
{
	const FSoftObjectPath AssetPath(InAssetData.GetSoftObjectPath());

	// 防止重复触发
	bool bAlreadyHandled = false;
	RemovedAssetPaths.Add(AssetPath, &bAlreadyHandled);
	if (bAlreadyHandled) return;

	RemoveAddedAsset(AssetPath);

	// 无论是否自动管理 AssetRegistry 标签的删除，删除资产都必须清理缓存状态
	RemoveFromAssetTagStates(AssetPath);

	// 资产删除通知
	OnAssetDeleted.Broadcast(AssetPath);
}

void UEUSS_MetaDataManager::OnAssetMoved(
	const FAssetData& InAssetData,
	const FString& InOldObjectPath)
{
	const FSoftObjectPath OldPath(InOldObjectPath);
	const FSoftObjectPath NewPath(InAssetData.GetSoftObjectPath());
	if (OldPath == NewPath || !OldPath.IsValid()) return;

	// 不迁移旧状态：移动后由用户重新添加并同步新路径
	RemovedAssetPaths.Add(OldPath);
	RemoveAddedAsset(OldPath);
	RemoveFromAssetTagStates(OldPath);
}

void UEUSS_MetaDataManager::AddMetaData(UObject* InAsset, const FName InTagToAdd, const FString InValueToAdd)
{
	if (!IsValid(InAsset) || InTagToAdd.IsNone())
	{
		UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] : Asset 无效或 Tag 为空"), __FUNCTION__);
		return;
	}

	// 计算元数据快照
	CalcAssetTagStateBaseline(InAsset);

	// 添加元数据并同步资产
	UEditorAssetLibrary::SetMetadataTag(InAsset, InTagToAdd, InValueToAdd);
	SyncAsset(InAsset);
}

void UEUSS_MetaDataManager::ModifyMetaData(UObject* InAsset, const FName InTagToAdd, const FString InValueToAdd)
{
	if (!IsValid(InAsset) || InTagToAdd.IsNone())
	{
		UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] : Asset 无效或 Tag 为空"), __FUNCTION__);
		return;
	}
	
	// 计算元数据快照
	CalcAssetTagStateBaseline(InAsset);

	// 修改元数据并同步资产
	UEditorAssetLibrary::SetMetadataTag(InAsset, InTagToAdd, InValueToAdd);
	SyncAsset(InAsset);
}

void UEUSS_MetaDataManager::DeleteMetaData(UObject* InAsset, const FName InTagToAdd)
{
	if (!IsValid(InAsset) || InTagToAdd.IsNone())
	{
		UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] : Asset 无效或 Tag 为空"), __FUNCTION__);
		return;
	}

	// 计算元数据快照
	CalcAssetTagStateBaseline(InAsset);

	// 移除元数据并同步资产
	UEditorAssetLibrary::RemoveMetadataTag(InAsset, InTagToAdd);
	SyncAsset(InAsset);
}

void UEUSS_MetaDataManager::CopyMetaData(const TArray<UObject*> InSourceAsset, const TArray<UObject*> InTargetAsset)
{
	if (InSourceAsset.Num() == 0 || InTargetAsset.Num() == 0) return;

	// 数组中键表示资产对象，值表示该资产的元数据
	TArray<TPair<UObject*, TMap<FName, FString>>> ValidSources;
	ValidSources.Reserve(InSourceAsset.Num());

	// 获得源资产和其对应的元数据
	for (UObject* SourceAssetObj : InSourceAsset)
	{
		if (!IsValid(SourceAssetObj))
		{
			continue;
		}

		ValidSources.Emplace(
			SourceAssetObj,
			UEditorAssetLibrary::GetMetadataTagValues(SourceAssetObj));
	}

	// 遍历目标资产
	for (UObject* TargetAssetObj : InTargetAsset)
	{
		if (!IsValid(TargetAssetObj))
		{
			continue;
		}

		// 计算元数据快照
		CalcAssetTagStateBaseline(TargetAssetObj);

		// 获得目标资产的元数据
		TMap<FName, FString> TargetAssetMetaDatas =
			UEditorAssetLibrary::GetMetadataTagValues(TargetAssetObj);

		bool bTargetChanged = false;

		// 遍历源资产数组
		for (const TPair<UObject*, TMap<FName, FString>>& Source : ValidSources)
		{
			// 遍历每个源资产的元数据
			for (const TPair<FName, FString>& SourceAssetMetaData : Source.Value)
			{
				// 判断目标资产是否不包含此元数据
				if (!TargetAssetMetaDatas.Contains(SourceAssetMetaData.Key))
				{
					// 不包含则添加
					UEditorAssetLibrary::SetMetadataTag(
						TargetAssetObj,
						SourceAssetMetaData.Key,
						SourceAssetMetaData.Value);

					TargetAssetMetaDatas.Add(
						SourceAssetMetaData.Key,
						SourceAssetMetaData.Value);

					bTargetChanged = true;
				}
			}
		}

		// 如果目标资产的元数据有变化则同步资产
		if (bTargetChanged)
		{
			SyncAsset(TargetAssetObj);
		}

		// 每对源资产和目标资产都触发广播
		for (const TPair<UObject*, TMap<FName, FString>>& Source : ValidSources)
		{
			OnMetaDataCopied.Broadcast(Source.Key, TargetAssetObj);
		}
	}
}

void UEUSS_MetaDataManager::SyncAsset(UObject* InAsset)
{
	if (!IsValid(InAsset))
	{
		UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] : Asset 无效"), __FUNCTION__);
		return;
	}
	
	RemovedAssetPaths.Remove(FSoftObjectPath(InAsset));

	// 根据保存的快照，更新 AssetTagStates
	const FMetaDataUpdateResult UpdateResult = UpdateAssetTagStates(InAsset);

	if (UpdateResult.bAssetChanged)
	{
		// 资产元数据对比快照发生了变化，必须进行保存
		if (!UEditorAssetLibrary::SaveLoadedAsset(InAsset, false))
		{
			UE_LOG(
				LogMetaDataOfAssets,
				Warning,
				TEXT("[%hs] : 资产保存失败: %s"),
				__FUNCTION__,
				*InAsset->GetPathName());

			// 保存失败时至少刷新资产注册表的搜索数据
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
				TEXT("AssetRegistry")).Get().AssetUpdateTags(
					InAsset,
					EAssetRegistryTagsCaller::Fast);
		}
	}
	else if (UpdateResult.bRegistryTagsChanged)
	{
		// 首次扫描资产时，不保存资产
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			TEXT("AssetRegistry")).Get().AssetUpdateTags(
				InAsset,
				EAssetRegistryTagsCaller::Fast);
	}

	OnAssetSynced.Broadcast(InAsset);
}

void UEUSS_MetaDataManager::AddTagToRegisteredTags(const FName InTagToAdd, UObject* InAsset)
{
	if (InTagToAdd.IsNone() || !IsValid(InAsset))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : TagToAdd 为空或 Asset 无效"), __FUNCTION__);
		return;
	}

	// 仅添加模式，不增加引用计数
	if (!bAllowManageAssetRegistryTagsDelete)
	{
		AddTagToAssetRegistry(InTagToAdd);
		
		OnMetaDataAdded.Broadcast(InAsset);
		
		return;
	}

	int32& Count = RegisteredTags.FindOrAdd(InTagToAdd);
	// 引用计数加 1
	Count++;
	
	// 注册 Tag 到 AssetRegistry
	AddTagToAssetRegistry(InTagToAdd);

	OnMetaDataAdded.Broadcast(InAsset);

}

void UEUSS_MetaDataManager::RemoveTagFromRegisteredTags(const FName InTagToRemove, UObject* InAsset)
{
	if (InTagToRemove.IsNone() || !IsValid(InAsset))
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : TagToRemove 为空或 Asset 无效"), __FUNCTION__);
		return;
	}
	
	// 仅添加模式，不从 AssetRegistry 中移除
	if (!bAllowManageAssetRegistryTagsDelete)
	{
		OnMetaDataDeleted.Broadcast(InAsset);
		return;
	}

	// 获得当前 Tag 的引用计数
	int32* CountPtr = RegisteredTags.Find(InTagToRemove);
	if (!CountPtr) return;

	int32& Count = *CountPtr;
	if (Count <= 0)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : RegisteredTags 中 %s 的引用计数为 %d"), __FUNCTION__, *InTagToRemove.ToString(), Count);
		RegisteredTags.Remove(InTagToRemove);
		return;
	}

	// 引用计数减 1
	--Count;
	if (Count == 0)
	{
		RegisteredTags.Remove(InTagToRemove);
		
		// 引用计数归零，从 AssetRegistry 中删除
		RemoveTagFromAssetRegistry(InTagToRemove);
	}

	OnMetaDataDeleted.Broadcast(InAsset);

}

void UEUSS_MetaDataManager::AddTagToAssetRegistry(const FName InTagToAdd)
{
	// 获得 UAssetManagerSettings
	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();
	if (!Settings)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : UAssetManagerSettings 无效"), __FUNCTION__);
		return;
	}

	// 标签不存在则添加
	if (!Settings->MetaDataTagsForAssetRegistry.Contains(InTagToAdd))
	{
		// 添加到配置源
		Settings->MetaDataTagsForAssetRegistry.Add(InTagToAdd);
		
		// 添加到全局缓存
		TSet<FName>& GlobalTagsForAssetRegistry = UObject::GetMetaDataTagsForAssetRegistry();
		if (!GlobalTagsForAssetRegistry.Contains(InTagToAdd))
		{
			GlobalTagsForAssetRegistry.Add(InTagToAdd);
		}
		
		// 更新配置文件
		Settings->TryUpdateDefaultConfigFile();
	}
	
}

void UEUSS_MetaDataManager::RemoveTagFromAssetRegistry(const FName InTagToRemove)
{
	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();
	if (!Settings)
	{
		UE_LOG(LogMetaDataOfAssets, Error, TEXT("[%hs] : UAssetManagerSettings 无效"), __FUNCTION__);
		return;
	}

	// 标签存在则移除
	if (Settings->MetaDataTagsForAssetRegistry.Contains(InTagToRemove))
	{
		// 从配置源移除
		Settings->MetaDataTagsForAssetRegistry.Remove(InTagToRemove);

		// 从全局缓存移除
		TSet<FName>& GlobalTagsForAssetRegistry = UObject::GetMetaDataTagsForAssetRegistry();
		GlobalTagsForAssetRegistry.Remove(InTagToRemove);

		// 更新配置文件
		Settings->TryUpdateDefaultConfigFile();
	}

}
