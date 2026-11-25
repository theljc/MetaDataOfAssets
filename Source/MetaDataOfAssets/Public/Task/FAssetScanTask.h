// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Ticker.h"

class SNotificationItem;
struct FStreamableHandle;
class SProgressBarNotification_ScanAsset;


class FAssetSyncTask : public TSharedFromThis<FAssetSyncTask>
{
	
public:
	
	explicit FAssetSyncTask(const TArray<FAssetData>& InAssets, const int32 InSyncBatchSize)
		: Assets(InAssets)
		, SyncBatchSize(InSyncBatchSize)
	{
	}

	// 开始任务
	void Start();
	
	// 结束任务
	void Finish(const bool bSuccess);

private:

	enum class EPhase : uint8
	{
		Loading,
		Syncing,
		Finished
	};

	// 加载完成时
	void OnLoadComplete();
	// 分批同步资产
	bool SyncTick(float DeltaTime);

	// 资产列表
	TArray<FAssetData> Assets;

	// 进度条控件
	TSharedPtr<SProgressBarNotification_ScanAsset> ProgressWidget;
	// 进度条通知
	TSharedPtr<SNotificationItem> ProgressNotification;
	
	FTSTicker::FDelegateHandle SyncTickHandle;
	TSharedPtr<FStreamableHandle> StreamableHandle;

	// 当前同步到的资产索引
	int32 NextSyncIndex = 0;
	// 每帧最多同步的资产数量
	int32 SyncBatchSize = 32;
	// 每帧同步的时间预算，避免单个批次耗时过长
	double SyncTimeBudgetSeconds = 0.01;

	// 同步成功的资产数量
	int32 SyncedAssetCount = 0;
	// 同步失败的资产数量
	int32 FailedAssetCount = 0;

	EPhase Phase = EPhase::Loading;

	// 防止重复执行
	bool bFinished = false;
	
};
