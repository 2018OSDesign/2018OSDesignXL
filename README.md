# 咸鱼操作系统设计及功能说明文档
![SystemOpen](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/SystemOpen.png)
## 目录
<!-- MarkdownTOC -->

- [队员组成](#队员组成)
- [开发环境说明](#开发环境说明)
    - [开发系统环境](#开发系统环境)
    - [相关软件](#相关软件)
    - [开发语言](#开发语言)
    - [项目管理平台](#项目管理平台)
- [咸鱼操作系统设计说明](#咸鱼操作系统设计说明)
    - [系统概要](#系统概要)
        - [操作系统设计思路](#操作系统设计思路)
        - [操作系统组成](#操作系统组成)
        - [操作系统功能](#操作系统功能)
        - [操作系统各部分简述](#操作系统各部分简述)
            - [控制台](#控制台)
            - [文件系统](#文件系统)
            - [应用](#应用)
- [咸鱼操作系统基本功能说明](#咸鱼操作系统基本功能说明)
    - [功能汇总](#功能汇总)
    - [app](#app)
    - [clear](#clear)
    - [welcome](#welcome)
    - [help](#help)
    - [chat](#chat)
    - [mkfile](#mkfile)
    - [mkdir](#mkdir)
    - [read](#read)
    - [delete](#delete)
    - [deletedir](#deletedir)
    - [ls](#ls)
    - [cd](#cd)
    - [print](#print)
    - [edit](#edit)
    - [edit+](#edit+)
    - [information](#information)
    - [proc](#proc)
- [咸鱼操作系统主要功能说明](#咸鱼操作系统主要功能说明)
    - [二级文件系统](#二级文件系统)
- [参考文献](#参考文献)
- [鸣谢](#鸣谢)




<!-- MarkdownTOC -->
<h2 id="队员组成"> 队员组成</h2>
<p>|  学号   |  姓名  | 备注 |            分工            | 分数比例 |</p>
<p>| 1652666 | 徐仁和 | 队长 |   环境搭建、文档、应用制作  |    50%   |</p>
<p>| 1652664 | 吕慕创 | 队员 |  环境搭建、文件系统编写、PPT|    50%   |</p>

<h2 id="开发环境说明"> 开发环境说明</h2>
<h3 id="开发系统环境"> 开发系统环境</h3>
Ubuntu-16.04 64位 
<h3 id="相关软件"> 相关软件</h3>
<p>- VMware</p>
<p>- bochs</p>
<h3 id="开发语言"> 开发语言</h3>
<p>- C语言</p>
<p>- 汇编</p>
<h3 id="项目管理平台"> 项目管理平台</h3>

[github](https://github.com/2018OSDesign/2018OSDesignXL)



<h2 id="咸鱼操作系统设计说明"> 咸鱼操作系统设计说明</h2>
<h3 id="系统概要"> 系统概要</h3>
<h4 id="操作系统设计思路"> 操作系统设计思路</h4>
因为我们同组的两人代码实力都偏弱，所以我们开始做的时候目的是很明确的，优先完成用户级的应用以求完成最低难度的项目
要求。所以我们先加了5个小程序，装上了简易计算器、扫雷、推箱子、猜数字和拿火柴，除了计算器都是些比较小的游戏。因为
我们找不到把应用打包写进软盘的方法，就只能把游戏都写进内核的main文件中直接由内核中转，也算是应用不足的地方。操作
系统的GUI我们是没能力做成图形界面的就保持控制台。Orange的源码关于文件时简单的一级文件系统，我们在此基础上尝试修改
、添加，使其实现二级文件系统。
<h4 id="操作系统组成"> 操作系统组成</h4>
<p>- boot（引导）</p>
<p>- kernel（内核）</p>
<p>- fs（文件系统）</p>
<p>- lib（可用代码库）</p>
<p>- include（头文件集）</p>
<h4 id="操作系统功能"> 操作系统功能</h4>
<p>-实现控制台操作</p>
<p>-实现一套易用的命令行命令</p>
<p>-简易计算器（应用）</p>
<p>-扫雷(应用)</p>
<p>-推箱子(应用)</p>
<p>-猜数字(应用)</p>
<p>-拿火柴(应用)</p>
<p>**改写文件系统实现文件隔离的二级文件系统</p>

<h4 id="操作系统各部分简述"> 操作系统各部分简述</h4>
<h5 id="控制台"> 控制台</h5>
>与文件系统关联的控制台

    可以通过在特殊文件中读写实现控制台的输入和输出
<h5 id="文件系统"> 文件系统</h5>
>二级文件系统 

    设定系统的根目录为root ,可以在root目录下新建文件或者文件夹，同时可以进入到文件夹中，在文件夹中新建文件 
    支持对文件的新建、覆盖性修改文本、链接性修改文本、读取与删除； 
    支持对文件夹的新建、进入、删除、读取文件夹中内容； 
    总体而言还是有很多的不足，不过对我们而言已经比较满意了
<h5 id="应用"> 应用</h5>
>五种控制台应用

    虽然没有将应用打包自动写入操作系统软盘区，但也通过努力在操作系统上实现了5款小应用，扫雷手动输入位置、推箱子
    手动输入移动方向来操作，计算器直接输入一道算式，同时都支持随时退出，体验良好。
<h2 id="咸鱼操作系统基本功能说明"> 咸鱼操作系统基本功能说明</h2>
<h3 id="功能汇总"> 功能汇总</h3>
<p>|   命令   |    参数    |        概述        |</p>
<p>|   app    |     -      |    进入应用选择    |</p>
<p>|  clear   |     -      |清屏并打印欢迎语句  |</p>
<p>| welcome  |     -      |不清屏打印欢迎语句  |</p>
<p>|  help    |     -      | 打印指令列表       |</p>
<p>|  chat    |     -      |  "人工智能"        |</p>
<p>|  mkfile  | [str][str] | 前者文件名后者内容 |</p>
<p>|  mkdir   | [str]      | 文件夹名称         |</p>
<p>|  read    | [str]      | 文件名             |</p>
<p>| delete   | [str]      | 文件名             |</p>
<p>|deletedir | [str]      |  文件夹名称        |</p>
<p>|    ls    |     -      | 展示当前目录下的文件、文件夹|</p>
<p>|    cd    |  [str]     |  进入文件夹        |</p>
<p>|    edit  | [str][str] |文件名，覆盖的内容  |</p>
<p>|  edit+   | [str][str] |文件名，追加的内容  |</p>
<p>|information|   -       |输出制作学生        |</p>
<p>|   print  |    [str]   | 打印该字符串       |</p>
<p>|  proc    |     -      | 打印进程           |</p>

<h3 id="app"> app</h3>

![app](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/app.png)
<h4> GuessNumber</h4>

![GuessNumber](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/GuessNumber.png)

    在控制台进入app后选择GuessNumber即可开始游戏
<h4> Caculator</h4>

![Caculator](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/Calculator.png)

    在控制台进入app后选择Caculator即可打开计算器，输入一道四则算式如51*49回车即可看到结果。算式输入一次只能两个数运算
<h4> PushBox</h4>

![PushBox](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/PushBox.png)

    在控制台进入app后选择PushBox即可进入游戏，操作方式为输入| w |向上|| a |向左|| s |向下|| d |向右|| q |退出游戏|
<h4> PickSticks</h4>

![PickSticks](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/PickSticks.png)

    在控制台进入app后选择PickSticks即可进入游戏，按提示操作即可
<h4> MinesWeeper</h4>

![MinesWeeper](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/MineWeeper.png)

    在控制台进入app后选择MineWeeper即可进入游戏，输入(x,y)的坐标点进行操作，左上角为原点
<h3 id="clear"> clear</h3>

    在控制台输入clear后，系统会清屏并打印欢迎界面，回到刚开机的显示状态
<h3 id="welcome"> welcome</h3>

![welcome](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/welcom.png)

    在控制台输入welcome,系统会打印欢迎界面，但不清屏，之前的操作输入都还在
<h3 id="help"> help</h3>

![help](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/help.png)

    在控制台输入help,系统会打印所有参数信息
<h3 id="chat"> chat</h3>

![chat](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/chat.png)

    在控制台输入chat,系统会进入内置的几句对话选项，蒙中就回答，Byebye退出对话
<h3 id="mkfile"> mkfile</h3>

![mkfile](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/makefile.png)

    在控制台输入mkfile+文件名+文件内容可在当前文件夹创建该文件
<h3 id="mkdir"> mkdir</h3>

![mkdir](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/mkdir.png)

    在控制台输入mkdir+文件夹名则会在当前文件夹创建一个子文件夹
<h3 id="read"> read</h3>

![read](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/read.png)

    在控制台输入read+文件名则会读取这个文件内容
<h3 id="delete"> delete</h3>

    在控制台输入delete+文件名则会删除该路径下的存在文件，若不存在则报错
<h3 id="deletedir"> deletedir</h3>

    在控制台输入deletedir+文件夹名则会删除该路径下存在的文件夹，不存在则返回错误信息
<h3 id="ls"> ls</h3>

![ls](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/ls.png)

    在控制台输入ls后，系统会打印该路径下所有文件和文件夹
<h3 id="cd"> cd</h3>

![cd](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/cd.png)

![cd..](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/cd%20...png)

    在控制台输入cd+文件夹名，若存在则会进入该文件夹，不存在则报错
<h3 id="edit"> edit</h3>

![edit](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/edit.png)

    在控制台输入edit+文件名+内容，则会修改文件内容，方式为覆盖
<h3 id="edit+"> edit+</h3>

![edit+](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/edit%2B.png)

    在控制台输入edit+ 和文件名 和内容，则会修改改文件内容，方式为追加
<h3 id="information"> information</h3>

![information](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/information.png)

    在控制台输入information，系统会打印制作人员，也就是我们的信息
<h3 id="print"> print</h3>

    在控制台输入print+内容后，系统会将该内容再打印一遍
<h3 id="proc"> proc</h3>

![proc](https://github.com/2018OSDesign/2018OSDesignXL/blob/master/image/proc.png)

    在控制台输入proc后，系统会打印当前的进程信息


<h2 id="咸鱼操作系统主要功能说明"> 咸鱼操作系统主要功能说明</h2>
<h3 id="二级文件系统"> 二级文件系统</h3>


    1.在控制台中输入mkdir [directoryname],可以在当前root目录下新建名为[directoryname]的文件夹，同时控制台反馈信息 
    2.在控制台中输入deletedir [directoryname],可以删除名为[directoryname]的文件夹 
    3.在控制台中输入ls ,可以显示当前目录中所有的文件夹以及文件的命名 
    4.在控制台中输入cd [directoryname],可以进入到root目录下的名为[directoryname]的文件夹中 
    注：输入 cd [..] 可以返回上一级目录，在该系统中即返回到root目录下 


    1.在控制台中输入mkfile [filename][str],可以在当前目录下新建名为[filename]的文件，同时将[str]写入新建的文件中 
    2.在控制台中输入read [filename],可以读取到当前目录下存在的名为[filename]的文件中的文本内容 
    3.在控制台中输入delete [filename],可以删除名为[filename]的文件 
    4.在控制台中输入edit [filename][str],可以对名为[filename]的文件进行修改，以[str]进行覆盖性修改 
    5.在控制台中输入edit+ [filename][str],可以对名为[filename]的文件进行修改，将[str]写到原文件的末尾，不覆盖 
<h2 id="参考文献"> 参考文献</h2>
<p>-Orange's 一个操作系统的实现  | 于渊 著  | 电子工业出版社</p>
<h2 id="鸣谢"> 鸣谢</h2>
-在此鸣谢为此次课程设计付出努力辛苦的老师助教。同时感谢为此项目努力的队友
