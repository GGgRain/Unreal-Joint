<div align="center">
  <img src="https://github.com/user-attachments/assets/bc821943-7b90-46c7-b215-856214b775e9" height="200" alt="Joint Logo">
</div>

<div align="center">
  <h1>Joint</h1>
  <h3>Visual Modular Dialogue & Gameplay Scripting Framework For Unreal 4 & 5</h3>
</div>

<p align="center">
  <a href="https://github.com/GGgRain/Unreal-Joint/fork" target="_blank">
    <img src="https://img.shields.io/github/forks/GGgRain/Unreal-Joint?" alt="Badge showing the total of project forks">
  </a>
  <a href="https://github.com/GGgRain/Unreal-Joint/stargazers" target="_blank">
    <img src="https://img.shields.io/github/stars/GGgRain/Unreal-Joint?" alt="Badge showing the total of project stars">
  </a>
  <a href="https://discord.gg/DzNFax2aBS">
    <img src="https://img.shields.io/discord/977755047557496882?logo=discord&logoColor=white" alt="Chat on Discord">
  </a>
  <a href="https://gggrain.github.io/Unreal-Joint-Documentation/docs/category/joint-1">
    <img src="https://img.shields.io/badge/release%20notes-00B2EE.svg" alt="Release Notes">
  </a>
</p>

<p align="center">
  <a href="#mag-about">About</a> &#xa0; | &#xa0;
  <a href="#inbox_tray-related-projects--derivative-projects">Related & Derivative Projects </a> &#xa0; | &#xa0;
  <a href="#memo-joint-plugin-license--changes-after-it-became-open-source">License & Changed Things On Open Source</a> &#xa0; | &#xa0;
  <a href="#clipboard-main-features">Main Features</a> &#xa0; | &#xa0;
  <a href="#inbox_tray-installation">Installation</a> &#xa0; | &#xa0;
  <a href="#loudspeaker-supports">Supports</a> &#xa0;
</p>


<div align="center">
  <h3>🎉 Joint is now available on GitHub! (25.04.14)</h3>
  <h3>(This readme page is still WIP, I'm having really busy days recently..)</h3>
</div>


## :mag: About ##

Joint is Unreal Engine 4 & 5's strongest modular dialogue & gameplay scripting framework for every type of projects.

Joint is a powerful tool for game developers, designers, and writers alike. You can create complex dialogue productions with whatever gameplay mechanics you want to use with Joint, using very intuitive, the state-of-the-art visual scripting interface.

Do you like the dialogue production of Animal Crossing, or the dialogue system of The Witcher 3? Or do you want to create your own unique dialogue system? Joint is the perfect tool for you.

<p align="center">
  <i>Please consider leaving a star if you loved this project! </i>⭐
</p>

## :inbox_tray: Related Projects & Derivative Projects ##

There are some derivative projects that can be worked with Joint!

Currently We provide **Joint Native**, an completely-free-open-source project (apache-2.0) that provides built-in fragment and asset sets for basic and common dialogue productions.

<p align="center">
  <a href="https://github.com/GGgRain/Unreal-Joint-Native" target="_blank">
    <img src="https://github.com/user-attachments/assets/30601e49-3fe1-48d9-ac61-f5b3818c0eff" height="150" alt="Joint-Native Logo">
  </a>
</p>

Also, We also provide **Volt** plugin (MIT License), Unreal Slate Architecture Animation Framework, that was originally developed as a part of Joint to provide cool editor animation and interactions. If you have interest, please take a look into it.

<p align="center">
  <a href="https://github.com/GGgRain/Unreal-Volt" target="_blank">
    <img src="https://github.com/user-attachments/assets/6e5eecb0-836b-4c47-a80f-970acdc6c828" height="150" alt="Volt Logo">
  </a>
</p>

## :memo: Joint Plugin License & Changes After It Became Open Source ##

### 🎉 Now Joint is an Open-Source Project!

All content is available on our official GitHub repository, and you can download it and start using Joint in a second!

This project is licensed under our **Joint Plugin License**, Here are the important parts of the whole license:

<hr/>

<h3 align="center">💸 Don't Pay Anything Until Your Project Settles Down!</h3>
<p>
We aim to empower indie and small-scale developers to use Joint in a practical and proactive way, without financial pressure,<br/>
As of April 5, 2025, <b>if your project using Joint earns more than $15,000 USD in annual gross revenue, a 1% royalty fee will apply only to the portion exceeding that amount.</b><br/>
<b>Have you purchased Joint via Fab or Unreal Marketplace before May 20, 2025? You are exempt from this royalty program!</b>
</p>

<h3 align="center">🤝 Contribute and Support Our Projects – To Earn Rewards!</h3>
<p>
We plan to offer substantial royalty discounts to users who provide meaningful support – <b>whether technical, financial, business, or promotional.</b><br/>
Contributions, especially in the form of promotion or business partnerships, are extremely valuable to our team. In return, we plan to provide reasonable rewards for those who support us.
</p>

<h3 align="center">🎁 Make Your Derivative Software With Joint</h3>
<p>
Got a brilliant idea for a plugin or tool based on Joint – like <b>Joint Native</b> or our secret upcoming project <b>Joint Yggdrasil?</b><br/>
You are welcome to build and monetize your own Derivative Software based on Joint. You will retain full ownership and revenue rights for your derivative product.<br/>
You can also receive active technical support via our official GitHub and Discord channels to bring your ideas to life.
</p>

<hr/>

> [!NOTE]
> Please refer to the [LICENSE](LICENSE.md) file for further details.

## :clipboard: Main Features ##

### 1. Implement, and Stack Dialogue Features To Build Incredible Dialogue

Joint allows you to bring any gameplay mechanics or data you need on the script graph in the form of a module called **Fragment**.

Playing sounds, triggering animations, spawning actors, granting items or quests... Whatever it is, if Unreal Engine Can do it, you can make it a fragment and attach it to your dialogue.


(wait 스킵, 시퀀서, 캐릭터 애니메이션, 캐릭터 파티클 재생, 오디오 재생, 미니게임 재생 장면 하나하나 빌드)


Joint also provides built-in fragment and asset sets for basic and common dialogue productions in Joint Native, apache 2.0 open-source Sub Plugin for Joint, to let you starts off right away!

> [!NOTE]
> In Joint Native 1.12.2, you can find 39 fragments and assets for:
> * Basic Text-Based Dialogue (Text, Text Style)
> * Audio and Dubbing Based Dialogue (Dialogue Voice, Dialogue Wave)
> * Level Sequencer Controlled Dialogue
> * Dialogue Widget Related (Custom Rendering, render size, alpha, location interpolation control, Portrait system with participants) & Participant Control
> * Player Controller Control & Player Interactions Based Dialogue Control (Wait Skip until certain players are ready, Vote between players to select next branch to move)
> * Conditional Branching & Flow Control & Conditional Playback of Fragments
> * Sub Dialogue Control
> * Networking & Localization Based Control (On Server, On Client, On Standalone, etc, Culture Specific Playback Control)
> 
> \+ There are many more than these! Check out the full list [here](https://github.com/GGgRain/Unreal-Joint-Native), and more will be added in the future update

### 2. Powerful & Intuitive, Thus Easy Scripting With Joint Graph Editor

Joint is deeply considered to be easy and intuitive to use, even for those who are not familiar with programming or scripting.

https://github.com/user-attachments/assets/e836a7af-840b-4466-ad5b-3c277b21e8e3

### 3. COMPLETE Multiplayer Support Via RPC & Replication for ALL FRAGMENTS

Joint is designed to be fully multiplayer compatible. Fragments in Joint can be replicated and called RPC; via them, you can easily make your fragments work in multiplayer games.

For example, watch how Vote fragment from Joint Native make between players select the next branch to move in a dialogue in the networked game!

https://github.com/user-attachments/assets/3d595634-1e4b-4953-8919-c82d172082e8

> [!NOTE]
> You can check out the full blueprint logic for each of the nodes in [Joint Native](https://github.com/GGgRain/Unreal-Joint-Native). It's also free, So take a look!

### 4. Reactive & Dynamic Dialogue - Even in Multiplayer, Even After Localization, Even Between Clients with Different Languages

모듈러 시스템의 특징 설명

### 5. PERFECT QOL Features

검색, 변경, 벌크 검색, 컴파일러, 매니지먼트, 디버거 등

## :inbox_tray: Installation ##

### Steps:
1. Download this repository as a zip file or clone it to your workstation, then unzip the folder and rename it to `Joint`.
2. Place the unzipped `Joint` folder into the `Plugins` directory of your Unreal project. If this directory doesn’t exist, create it.
3. Open `Joint.uplugin` with notepad, and change "EngineVersion" to your Unreal Engine version. For example, if you are using Unreal Engine 5.5.3, change it to `"EngineVersion": "5.5"`.
4. Launch your project and confirm it runs successfully. If you see a prompt asking about building modules, click **Yes**.

> [!TIP] 
> Check this out, especially if you've never compiled or used C++ code on your Unreal Engine before:
> If the plugin doesn’t launch properly, you may check your machine's Visual Studio configuration for the C++ code build to be valid. Please make sure to follow these steps:
> 
> In your Visual Studio configuration (you can modify it in Visual Studio Installer) :
> * Click Modify on the 2022 version
> * Go to Individual components
> * Type “MSVC” in the search bar
> * Check “MSVC v143 - VS 2022 C++ x64/86 build tools (v14.38-17.8)”  (**IMPORTANT, YOU HAVE TO UNINSTALL ALL OTHER VERSIONS OF THIS COMPONENT TO MAKE IT RUN!!!!**)
> * Then click modify in the right bottom corner
>
> Second, You have to install **Game development with C++** workload as well.
> 
> ![vs2022_workloads_game_development](https://github.com/user-attachments/assets/6465a82e-ffc9-4dbd-bda4-86c30b0a83c1)
>
> Big thanks to "Kieran" - who posted the fix for the issue on the forum. 
> 
> Original Post: https://forums.unrealengine.com/t/help-visual-studio-preferred-version-in-5-4/2001249

If you have an issue on installing our plugin, please join [our official support Discord channel](https://discord.gg/DzNFax2aBS) for further assistance.


## Official Webpage ##

Joint provides [official documentation page](https://gggrain.github.io/Unreal-Joint-Documentation/) that contains tutorial & release notes & news for the updates!

Come along to find further details for the lastest updates and how to use the system or to see how the system has evolved!

<div align="center">
    <a href="https://github.com/GGgRain/Unreal-Joint/stargazers" target="_blank">
      <img src="https://github.com/user-attachments/assets/8e96ec7c-11dd-4f67-96ec-f1dbd105b144" alt="Badge showing the total of project stars">
      <img src="https://github.com/user-attachments/assets/1676b25d-d9d5-4a5b-b061-89718e4f1442" alt="Badge showing the total of project stars">
    </a>
</div>

## Contributions ##

사용자 권리 명시

## :loudspeaker: Supports ##

<div align="center">
  <a href="https://discord.gg/DzNFax2aBS" target="_blank">
    <img src="https://github.com/user-attachments/assets/02a06438-3198-4b43-9c07-17eaa7075b82" height="100px" alt="Chat on Discord">
  </a>
</div>


If you need help, feel free to join our [official Discord support channel](https://discord.gg/DzNFax2aBS). Our community is happy to assist you with any plugin-related queries.
