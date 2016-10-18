### 直播技术汇总

[做一款仿映客的直播App？看我就够了](http://www.jianshu.com/p/5b1341e97757)

[直播时代--IOS直播客户端SDK，美颜直播](http://www.jianshu.com/p/53c393098ba3) 

#### 一、直播一些技术名词

1、什么是 [FFmpeg](http://www.ffmpeg.org/) ，[FFmpeg](http://www.ffmpeg.org/) 是一个开源免费跨平台的视频和音频流方案，属于自由软件，它提供了**录制、转换以及流化音视频**的完整解决方案。直播系统多使用该方案。

2、什么是 [RTMP](http://www.adobe.com/cn/devnet/rtmp.html)（Real Time Messaging Protocol：实时消息传输协议）Adobe公司开发。一个专门为**高效传输视频，音频和数据**而设计的协议。它通过建立一个二进制TCP连接或者连接HTTP隧道实现实时的视频和声音传输。



#### 二、直播整体技术实现

技术相对都比较成熟，设备也都支持硬编码。iOS还提供现成的 Video ToolBox框架，可以对摄像头和流媒体数据结构进行处理，但Video ToolBox框架只兼容8.0以上版本，8.0以下就需要用x264的库软编了。

github上有现成的开源实现，推流、美颜、水印、弹幕、点赞动画、滤镜、播放都有。技术其实不是很难，而且现在很多云厂商都提供SDK，七牛云、金山云、乐视云、腾讯云、百度云、斗鱼直播伴侣推流端，功能几乎都是一样的，没啥亮点，不同的是整个直播平台服务差异和接入的简易性。后端现在 RTMP/HTTP-FLV 清一色，App挂个源站直接接入云厂商或CDN就OK。



**1、直播项目传输方式介绍** 

![](http://upload-images.jianshu.io/upload_images/2004362-77c68e1fbd99c638.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240) 



**2、直播项目采集端（推送端）介绍** 

推流端可以选择很多GitHub上的开源项目，比如 [VideoCore](https://github.com/jgh-/VideoCore) 、[LiveVideoCoreSDK](https://github.com/runner365/LiveVideoCoreSDK) 、[LFLiveKit](https://github.com/LaiFengiOS/LFLiveKit)  。商用的话可以选择各大厂商的SDK，网易直播云、七牛、腾讯、百度、新浪。使用直播云的好处就是能快速上线App，功能十分齐全，可以播放器和推流端，服务器一套下来，有专业客服人员帮助集成到工程中，缺点就是流量费太贵了，具体可以了解下各大厂商的收费标准。



2.1.目前比较知名的有 [VideoCore](https://github.com/jgh-/VideoCore)

目前国内很多知名的推流框架都是对VideoCore的二次开发。这个框架主要使用C++写的，支持RTMP推流，但对于iOS开发者来说有点晦涩难懂（精通C++的除外）。想开源和免费的可以选择现在的几个知名项目VideoCore + GPUImage+基于GPU的美颜滤镜 ,播放用IJKPlayer自己修改。



2.2.国内比较火的 [LiveVideoCoreSDK](https://github.com/runner365/LiveVideoCoreSDK)

框架提供iOS苹果手机的RTMP推流填写RTMP服务地址，直接就可以进行推流，SDK下载后简单的工程配置后能直接运行，实现了美颜直播和滤镜功能，基于OpenGL，前后摄像头随时切换，提供RTMP连接状态的回调。

这个框架是国内比较早的一款推流框架有不少在使用这个SDK，功能非常齐全，作者也比较牛，用来学习推流采集相关内容非常好，但是集成到工程中有些困难（对于我来说）。总的来说这是一款非常厉害的推流SDK，几乎全部使用C++写的，编译效率非常好，如果有实力的话推荐使用这个框架来做自己项目的推流端。



2.3.可读性比较好的推流  [LFLiveKit](https://github.com/LaiFengiOS/LFLiveKit) 

框架支持RTMP、HlS (HTTP Live Streaming：苹果自家的动态码率自适应技术) 。主要用于PC和Apple终端的音视频服务。包括一个m3u(8)的索引文件，TS媒体分片文件和key加密串文件。

推荐这个框架第一是因为它主要使用OC写的，剩下的用C语言写的，框架文件十分清晰，这对不精通C++的初学者提供了很大的便利，并且拓展性非常强，支持动态切换码率功能，支持美颜功能。



2.4.美颜功能

美颜的话一般都是使用 [GPUImage](https://github.com/BradLarson/GPUImage) 的基于OpenGl开发，纯OC语言，这个框架十分强大，可以做出各种不同滤镜，可拓展性高。如果对美颜没有具体思路可以直接用  [BeautifyFace](https://github.com/Guikunzhi/BeautifyFaceDemo) ，可以加入到项目中，很方便的实现美颜效果。 



**3、直播项目播放端介绍**

播放端用的针对RTMP优化过的 [ijkplayer](https://github.com/Bilibili/ijkplayer) ，[ijkplayer](https://github.com/Bilibili/ijkplayer) 是基于FFmpeg的跨平台播放器，这个开源项目已经被多个 App 使用，其中映客、美拍和斗鱼使用了 ijkplayer（8000+⭐️） 。可以参考这篇博客获取framework：[iOS中集成ijkplayer视频直播框架](http://www.jianshu.com/p/1f06b27b3ac0) 。也可以直接下载别人已经集成好的framework：[IJKMediaFramework.framework](https://pan.baidu.com/s/1eSLRmme) 

播放方法：

```objective-c
- (void)goPlaying {

    //获取url
    self.url = [NSURL URLWithString:_liveUrl];
    _player = [[IJKFFMoviePlayerController alloc] initWithContentURL:self.url withOptions:nil];

    UIView *playerview = [self.player view];
    UIView *displayView = [[UIView alloc] initWithFrame:self.view.bounds];

    self.PlayerView = displayView;
    [self.view addSubview:self.PlayerView];

    // 自动调整自己的宽度和高度
    playerview.frame = self.PlayerView.bounds;
    playerview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    [self.PlayerView insertSubview:playerview atIndex:1];
    [_player setScalingMode:IJKMPMovieScalingModeAspectFill];

}	
```



#### 三、直播实践 (直播和观看都不支持模拟器)

[**基于LiveVideoCoreSDK完整项目源码**](https://github.com/runner365/LiveVideoCoreSDK) 

1、搭建 **nginx+RTMP** 服务器:  [Mac搭建nginx+rtmp服务器](http://www.tuicool.com/articles/muya6rz) 

i. 安装 [nginx](https://nginx.org/en/) (网页服务器，它能反向代理HTTP, HTTPS, SMTP, POP3, IMAP的协议链接，以及一个负载均衡器和一个HTTP缓存) :

```shell
## clone nginx项目到本地
$ brew tap homebrew/nginx		
## 执行安装
$ brew install nginx-full --with-rtmp-module
## 成功之后
$ nginx
```

在浏览器里打开 [http://localhost:8080](http://localhost:8080/) 如果出现下图, 则表示安装成功

![](http://upload-images.jianshu.io/upload_images/1038348-a7171240f3eae6c9.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240) 

ii. 配置nginx和RTMP:

```shell
## 查看nginx安装到哪了
$ brew info nginx-full
...
...
==> Caveats
Docroot is: /usr/local/var/www

The default port has been set in /usr/local/etc/nginx/nginx.conf to 8080 so that
nginx can run without sudo.

nginx will load all files in /usr/local/etc/nginx/servers/.
```

找到 `nginx.conf ` 文件所在位置 ` (/usr/local/etc/nginx/nginx.conf)` ， 点击`Finder` -> `前往` -> `前往文件夹` -> `输入/usr/local/etc/nginx/nginx.conf` ->  用记事本工具(推荐`Sublime Text`) 打开 `nginx.conf`。直接滚到最后一行, 在最后一个 `}` (即最后的空白处, 没有任何`{}`)后面添加：

```shell
# 在http节点后面加上rtmp配置：
rtmp {
    server {
# 端口一般使用4位，本人开始使用三位，一直有问题，折腾了半天！
        listen 7160;
        application denglibinglive {
            live on;
            record off;
        }
    }
}
```

然后重启nginx(其中的1.10.1要换成你自己安装的`nginx`版本号, 查看版本号用`nginx -v`命令即可)：

```shell
$ /usr/local/Cellar/nginx-full/1.10.1/bin/nginx -s reload
```

iii. 安装FFmpeg:

```shell
$ brew install ffmpeg
```

iv. 下载能播放流媒体的播放器 [VLC](http://www.videolan.org/vlc/) 

下载后安装，打开 VLC，然后 File->open network（`command+N`）输入：

```http
rtmp://localhost:7160/denglibinglive/room
```

![](http://7xqhx8.com1.z0.glb.clouddn.com/%E7%9B%B4%E6%92%AD1.png) 

v.开始FFMpeg推流：

[mp4视频下载地址](http://7xqhx8.com1.z0.glb.clouddn.com/loginmovie.mp4)

下载视频到桌面后，执行推流命令：

```shell
$ ffmpeg -re -i /Users/denglibing/Desktop/loginmovie.mp4 -vcodec libx264 -acodec aac -strict -2 -f flv rtmp://localhost:7160/denglibinglive/room
```

VLC便开始播放流媒体：

![](http://upload-images.jianshu.io/upload_images/1038348-10fd8e48e415eed3.gif?imageMogr2/auto-orient/strip) 



2、下载 [基于LiveVideoCoreSDK完整项目源码](https://github.com/runner365/LiveVideoCoreSDK)   

将项目中 `ViewController.m`  的 `viewDidLoad` 中改为如下：

```objective-c
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    [self UIInit];
    
    _RtmpUrlTextField.text = @"rtmp://ossrs.net/live/123456";
  	// 注意 192.168.1.20 是你电脑的ip地址即nginx服务器地址，7160/denglibinglive/room 是博客上面对应的参数，切勿搞错了。
    _RtmpUrlTextField.text = @"rtmp://192.168.1.20:7160/denglibinglive/room";
}
```

`iOS10` 以上的手机需要在 `info.plist` 中加入直播所需要的权限：

```xml
	<key>NSCameraUsageDescription</key>
	<string>运行项目使用相机</string>
	<key>NSAppleMusicUsageDescription</key>
	<string>运行项目使用多媒体</string>
	<key>NSMicrophoneUsageDescription</key>
	<string>运行项目使用麦克风</string>	
```

运行即可直播！



3、直播项目采集端：从0到1开始集成LiveVideoCoreSDK

i. 在 `Desktop`  新建文件夹 `HDLiveVideoCoreDemo`  使用Xcode建立项目组（Workspace），命名为 `HDLiveVideoCoreDemo.xcworkspace`  并保存在 `HDLiveVideoCoreDemo` 中：

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo1.png) 

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo2.png) 



ii、将 [基于LiveVideoCoreSDK完整项目源码](https://github.com/runner365/LiveVideoCoreSDK)  源码的 `LiveVideoCoreSDK` 、`LiveVideoCoreCoreSDK.xcodeproj` 、`RtmpLivePushSDK` 复制到文件夹 `HDLiveVideoCoreDemo`  中：

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo3.png) 



iii、打开 `HDLiveVideoCoreDemo.xcworkspace` ，新建 `LiveVideoCoreDemo.xcodeproj` 项目：

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo4.png) 

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo5.png) 

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo6.png) 



iv、分别将 `LiveVideoCoreSDK.xcodeproj`  和 `RtmpLivePushSDK.xcodeproj` 拖入到项目组中：

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo7.png) 

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo8.png) 



v、将 `libLiveVideoCoreSDK` 加入到  `LiveVideoCoreDemo` 项目中，并添加对应所需要的系统框架和tdb文件：

```shell
AVFoundation.framework
AudioToolbox.framework
CFNetwork.framework
CoreMedia.framework
OpenGLES.framework
VideoToolbox.framework
CoreVideo.framework
libz.tbd
libstdc++.tbd
```

![](http://7xqhx8.com1.z0.glb.clouddn.com/zhibo_demo9.png) 



vi、到此为止，`LiveVideoCoreSDK` 依赖的环境系统文件已经集成到新项目中，接下来对项目进行必要的配置：

因为 `LiveVideoCoreSDK`  不支持 `Bitcode` , 所以需要设置 `Enable Bitcode` 为 `NO`

因为 `LiveVideoCoreSDK.h` 不在 `LiveVideoCoreDemo` 项目中，所以需要 设置

 `Header Search Paths` :  `../LiveVideoCoreSDK/` 

 `../LiveVideoCoreSDK/`  这个是 `LiveVideoCoreSDK.h`  相对于  `LiveVideoCoreDemo.xcodeproj` 的路径。



vii、添加代码运行。

```objective-c
- (void)viewDidLoad {
    [super viewDidLoad];
    
    
    [[LiveVideoCoreSDK sharedinstance] LiveInit:[NSURL URLWithString:@"rtmp://192.168.1.20:7160/denglibinglive/room"]
                                        Preview:self.view
                                       VideSize:(CGSizeMake(540, 960))
                                        BitRate:LIVE_BITRATE_800Kbps
                                      FrameRate:LIVE_VIDEO_DEF_FRAMERATE
                                    highQuality:true];
    [LiveVideoCoreSDK sharedinstance].delegate = self;
    [[LiveVideoCoreSDK sharedinstance] connect];
    
    [LiveVideoCoreSDK sharedinstance].micGain = 5;
}

//rtmp status delegate:
- (void) LiveConnectionStatusChanged: (LIVE_VCSessionState) sessionState{
    NSLog(@"RTMP状态 : %d", (int)sessionState);
}
```

`LiveVideoCoreSDK` 集成完毕！

Demo下载地址：[HDLiveVideoCoreDemo](https://github.com/erduoniba/HDLiveVideoCoreDemo.git) 



4、直播播放端：

播放端用的针对RTMP优化过的 [ijkplayer](https://github.com/Bilibili/ijkplayer) ，[ijkplayer](https://github.com/Bilibili/ijkplayer) 是基于FFmpeg的跨平台播放器，这个开源项目已经被多个 App 使用，其中映客、美拍和斗鱼使用了 ijkplayer（8000+⭐️） 。可以参考这篇博客获取framework：[iOS中集成ijkplayer视频直播框架](http://www.jianshu.com/p/1f06b27b3ac0) 。也可以直接下载别人已经集成好的framework：[IJKMediaFramework.framework](https://pan.baidu.com/s/1eSLRmme) 

集成也很简单，新建项目，将下载的 [IJKMediaFramework.framework](https://pan.baidu.com/s/1eSLRmme)  加入到项目即可：

```objective-c
- (void)viewDidLoad {
    [super viewDidLoad];
    
    //rtmp://live.hkstv.hk.lxdns.com:1935/live/hks (流媒体数据)
    //rtmp://10.0.44.72:1935/denglibinglive/room (本地局域网的服务地址,局域网内测试直播和播放)
    //rtmp://115.28.135.68:1935/yuzhouheike/room (别人搭建好的云主机服务地址，支持不在局域网的直播和播放)
    _moviePlayer = [[IJKFFMoviePlayerController alloc] initWithContentURLString:@"rtmp://115.28.135.68:1935/yuzhouheike/room" withOptions:nil];
    _moviePlayer.view.frame = self.view.bounds;
    // 填充fill
    _moviePlayer.scalingMode = IJKMPMovieScalingModeAspectFill;
    // 设置自动播放(必须设置为NO, 防止自动播放, 才能更好的控制直播的状态)
    _moviePlayer.shouldAutoplay = NO;
    [self.view insertSubview:_moviePlayer.view atIndex:0];
    [_moviePlayer prepareToPlay];
    
    // 设置监听
    [self initObserver];
}


- (void)initObserver{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(stateDidChange) name:IJKMPMoviePlayerLoadStateDidChangeNotification object:self.moviePlayer];
}

- (void)stateDidChange{
    if ((self.moviePlayer.loadState & IJKMPMovieLoadStatePlaythroughOK) != 0) {
        if (!self.moviePlayer.isPlaying) {
            [self.moviePlayer play];
        }
    }
}
```

 Demo下载地址： [HDIJKMediaSDKDemo](https://github.com/erduoniba/HDIJKMediaSDKDemo.git) 
