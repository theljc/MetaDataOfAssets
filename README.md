<div align="center">
<h1>MetaData of Assets Plugin</h1>

![UE版本](https://img.shields.io/badge/UE-5.3%2B-blue)
[![许可证](https://img.shields.io/badge/License-MIT-green)](LICENSE)



</div>




## 目录

- [视频教程](#视频教程)
- [简介](#简介)
- [插件设置](#插件设置)
- [使用方法](#使用方法)
- [扩展](#扩展)
  - [目录结构](#目录结构)
  - [扩展建议](#扩展建议)
- [注意事项](#注意事项)






## 视频教程

**[链接待补充]**



## 简介

- **功能**
  - 用于在 UE 编辑器中批量管理资产的元数据，并自动注册到资产注册表

- **下载**
  - 方法一：FAB 商城直接下载：**[链接待补充]**
  - 方法二：**[链接待补充]** 下载对应的版本，解压到项目的 Plugins 文件中

- **启用**
  - 打开项目，在 [插件] 面板中搜索 **MetaDataOfAssets** 启用本插件



## 插件设置

> [!IMPORTANT]
>
> 由于内部使用引用计数机制，所以在启用 **是否允许管理资产注册表标签的删除** 后，需要同步现有的资产中的元数据（项目设置中配置好路径，并在主控件中点击"同步配置路径中的资产"按钮），否则会导致元数据被错误删除

- [项目设置->资产元数据]
  - **是否允许管理资产注册表标签的删除**：用于决定是否需要自动删除资产注册表中的标签（默认关闭），切换后需要重启编辑器
  - **扫描配置**：勾选 **是否允许管理资产注册表标签的删除** 后可用
    - **扫描路径**：指定要扫描的目录
    - **是否递归**：是否递归扫描此目录
- [编辑器偏好设置->快捷按键]
  - 资产元数据快捷键
    - **打开主控件**：打开主控件的快捷键，可自定义，默认快捷键是 **Alt + Z**
    - **关闭所有控件**：关闭所有由本插件创建的控件，可自定义，默认快捷键是 **Alt + Shift + Z**



## 使用方法

- 打开主控件

    - 点击关卡编辑器工具栏中的按钮打开主控件，或者使用快捷键打开（默认为 **Alt + Z**）

      ![](./Picture/打开主控件.png)


- 选择并添加资产

   - 方法一：直接在内容浏览器中选中资产，然后点击 **添加选中的资产** 按钮

   - 方法二：点击主控件中上方的箭头展开，在 Selected Folder 中配置要添加资产的目录，然后点击 **Add Assets Through Folder Config** 按钮

     ![](./Picture/选择并添加资产.png)

- 选择需要的操作

   ​	可以直接在主控件中对单个资产进行操作，也可点击批量操作按钮对已添加到控件中的资产进行批量操作

   - **添加**：为选择的资产添加元数据。批量添加时，会逐个为资产添加每条元数据。勾选 **是否允许管理资产注册表标签的删除** 时（在项目设置中），会使内部引用计数加一
   - **修改**：修改元数据的值。批量修改时，只会修改选择的资产共有的元数据
   - **删除**：删除资产的元数据。批量删除时，只会删除选择的资产共有的元数据。如果勾选 **是否允许管理资产注册表标签的删除**，会根据内部的引用计数判断是否需要删除注册到资产注册表的标签
   - **同步**：资产中如果有未添加到资产注册表的标签，则添加。如果勾选 **是否允许管理资产注册表标签的删除**，会根据引用计数判断是否删除注册到资产注册表的标签
   - **复制**：将一个资产的元数据复制给另一个资产（只复制另一个资产中没有的元数据），并自动注册到资产注册表中。批量复制时，会将选择的源资产逐个复制到所有目标资产

- 选择资产时，可按住 **Ctrl** 跳选或 **Shift** 多选以及 **Ctrl + A** 全选

- 快捷键（默认 **Alt + Shift + Z**） 关闭所有打开的窗口

- 右上角 **同步配置路径中的资产** 按钮，可同步项目设置中配置的路径下的资产，在项目设置中勾选 **是否允许管理资产注册表标签的删除** 后可用

- 控制台命令

   - **MetaDataOfAssets.EnableDebug**：开启本插件相关的 UE_LOG 和 PrintString 输出（默认为开启状态）
   - **MetaDataOfAssets.DisableDebug**：关闭本插件相关的 UE_LOG 和 PrintString 输出



## 扩展

### 目录结构

```
MetaDataOfAssets/
	Resources/					# 工具栏图标
	Config/						# 本插件相关的配置
	Content/
		EUW_MainWidget.uasset	# 主控件资产
		Widget/					# 包含所有子控件
		Data/					# 包含所有数据
		Localization/			# 本插件相关的本地化数据
	Source/MetaDataOfAssets/
		Command/				# 定义插件命令
		Style/					# 定义插件样式
		Log/					# 自定义 Log 分类
		Global/					# 全局属性
		MetaData/				# 包含 MetaDataManager，处理元数据相关
		Settings/				# 项目设置中的可配置属性
		Task/					# 资产同步任务
		Widget/					# 包含控件管理类与控件基类
```

### 扩展建议

- **规范**
  - 创建 UserWidget 时，必须使用 UEUSS_WidgetManager::CreateUserWidget，使用 InstancedStruct 构建参数
  - 创建 EditorUtilityWidget 时，必须使用 UEUSS_WidgetManager::CreateEditorUtilityWidget，使用 InstancedStruct 构建参数
  - 新增的 UserWidget 必须继承 UUW_BaseDataWidget
  - 新增的 EditorUtilityWidget 必须继承 UEUW_BaseUtilityWidget
  - 新增的控件可重写 OnOpen 接口用于接收参数并初始化，OnClose 接口用于控件销毁时的清理
  - 如果想让控件在父类被关闭时，自己也一起被关闭，则可以在控件的 OnOpen 中使用 UEUSS_WidgetManager::AddToChildNode 节点将自身添加到节点树中。ParentWidget 传入父类控件，ChildWidget 传入自身
  - 建议不要修改现有的父子节点关系



## 注意事项

- 本插件为编辑器插件
- 本插件做了中文和英语本地化，在 [编辑器偏好设置] 中切换语言时会自动更改
- 本插件涉及的项目配置都保存在本插件目录的 [Config/DefaultMetaDataOfAssets.ini] 文件中
  - [/Script/MetaDataOfAssets.MetaDataPluginSettings] 分段
    - bAllowManageAssetRegistryTagsDelete：是否允许自动删除 AssetRegistry 的标签
    - ScanSettings：同步资产时使用的扫描路径
    - bEnableDebug：是否开启本插件相关的 UE_LOG 和 PrintString 输出
    - MainWidgetPath：主控件资产的路径。禁止手动修改，否则无法使用编辑器工具栏中的按钮打开主控件
- 将资产添加到主控件中，以及点击 **同步配置路径中的资产** 按钮时，都会加载这个资产
- 点击 **同步配置路径中的资产** 按钮时，会在编辑器中分帧加载资产，右下角会显示加载进度

