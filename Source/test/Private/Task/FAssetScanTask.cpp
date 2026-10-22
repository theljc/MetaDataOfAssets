#include "test/Public/Task/FAssetScanTask.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Editor.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "MetaData/EUSS_MetaDataManager.h"
#include "Task/SProgressBarNotification_ScanAsset.h"
#include "Widget/EUSS_WidgetManager.h"

void FAssetSyncTask::Start()
{
    UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
    
    if (!GEditor
        || Assets.Num() == 0
        || !WidgetManager)
    {
        Cancel();
        return;
    }

    WidgetManager->OnTaskStart();
    
    // 进度条控件
    ProgressWidget = SNew(SProgressBarNotification_ScanAsset)
        .ProgressText(NSLOCTEXT("AssetScanTask", "LoadingAssetProgress", "Loading Assets"));
    
    // 创建通知信息
    FNotificationInfo Info(ProgressWidget);
    Info.bFireAndForget = false;
    Info.bAllowThrottleWhenFrameRateIsLow = false;
    Info.ExpireDuration = 0.0f;

    // 添加通知
    ProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);

    TArray<FSoftObjectPath> LoadedItems;
    for (FAssetData& Asset : Assets)
    {
        LoadedItems.AddUnique(Asset.GetSoftObjectPath());
    }

    // 加载资产
    FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();;
    StreamableHandle = Streamable.RequestAsyncLoad(
        LoadedItems,
        FStreamableDelegate::CreateLambda([this]()
        {
            // 加载完成后的回调
            OnLoadComplete();
        })
    );
    
    // 每帧执行 Tick
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(AsShared(), &FAssetSyncTask::Tick),
        0.0f
    );
    
}

void FAssetSyncTask::Cancel()
{
    if (StreamableHandle.IsValid())
    {
        StreamableHandle->CancelHandle();
        StreamableHandle.Reset();
    }
    
    Finish(false);

    UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
    if (!MetaDataManager) return;

    MetaDataManager->OnTaskFinished(this);
    
}

bool FAssetSyncTask::Tick(float DeltaTime)
{
    // 每帧更新进度条
    if (ProgressWidget.IsValid() && StreamableHandle.IsValid())
    {
        ProgressWidget->SetProgressPercent(StreamableHandle->GetProgress());
    }
    
    return true; // 继续 Tick
}

void FAssetSyncTask::Finish(const bool bSuccess)
{
    // 停止 Tick
    if (TickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
        TickHandle.Reset();
    }

    // 关闭通知
    if (ProgressNotification.IsValid())
    {
        ProgressNotification->ExpireAndFadeout();
        ProgressNotification.Reset();
    }

    ProgressWidget.Reset();

    if (!bSuccess) return;
    
    /** 同步资产 */
    UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
    UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
    if (!MetaDataManager || !WidgetManager) return;
    
    for (FAssetData& LoadedItem : Assets)
    {
        // 已经加载过，GetAsset 不会再次加载
        UObject* Asset = LoadedItem.GetAsset();
        if (IsValid(Asset))
        {
            MetaDataManager->SyncAsset(Asset);
            UE_LOG(LogTemp, Log, TEXT("加载资产: %s"), *Asset->GetName())
        }
    }
    
    WidgetManager->OnTaskComplete();
    MetaDataManager->OnTaskFinished(this);
    
    UE_LOG(LogTemp, Log, TEXT("%s"), TEXT("加载完成"))
}

void FAssetSyncTask::OnLoadComplete()
{
    ProgressWidget->SetProgressPercent(StreamableHandle->GetProgress());
    Finish(true);
}
