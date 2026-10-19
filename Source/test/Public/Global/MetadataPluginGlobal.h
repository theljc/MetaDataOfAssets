#pragma once

namespace MetadataOfAssetsConstant
{
	// 字符串表 ID
	inline const FName& GetTableID()
	{
		static const FName ID("MetadataPlugin");
		return ID;
	}

	// 字符串表命名空间
	inline const FTextKey& GetNamespace()
	{
		static const FTextKey Namespace("MetadataOfAssets");
		return Namespace;
	}

	// 字符串表文件路径
	inline const FString& GetLocalizationFilePath()
	{
		static const FString LocalizationFilePath("MetaDataPlugin/Content/Localization/MetadataPlugin.csv");
		return LocalizationFilePath;
	}
	
}
