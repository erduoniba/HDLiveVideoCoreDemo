//
//  ViewController.m
//  LiveVideoCoreDemo
//
//  Created by Harry on 16/10/17.
//  Copyright © 2016年 HarryDeng. All rights reserved.
//

#import "ViewController.h"

#import "LiveVideoCoreSDK.h"

@interface ViewController () <LIVEVCSessionDelegate>

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    //rtmp://live.hkstv.hk.lxdns.com:1935/live/hks (流媒体数据)
    //rtmp://10.0.44.72:1935/denglibinglive/room (本地局域网的服务地址)
    //rtmp://115.28.135.68:1935/yuzhouheike/room (别人搭建好的云主机服务地址)
    [[LiveVideoCoreSDK sharedinstance] LiveInit:[NSURL URLWithString:@"rtmp://115.28.135.68:1935/yuzhouheike/room"]
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

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
