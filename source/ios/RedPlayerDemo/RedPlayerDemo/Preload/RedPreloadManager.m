//
//  RedPreloadManager.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import "RedPreloadManager.h"

@import RedPlayer;
@implementation RedPreloadManager

+ (void)initPreloadConfig {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [RedPreLoad initPreLoad:[RedMediaUtil cachePath] maxSize:50*1024*1024]; // 50MB cache storage 
        [RedPreLoad setPreLoadMsgCallback:^(const RedPreLoadTask task, RedPreLoadControllerMsgType msgType, void * _Nonnull userData) {
            NSString *url = task.url;
            if (msgType == RedPreLoadControllerMsgTypeError) {
                NSLog(@"RedPreLoad-[ERROR]: %@", [NSString stringWithFormat:@"url:%@, error: %d", url, task.error]);
            } else if (msgType == RedPreLoadControllerMsgTypeCompleted) {
                NSLog(@"RedPreLoad-[Completed]: %@", [NSString stringWithFormat:@"url:%@", url]);
            } else if (msgType == RedPreLoadControllerMsgTypeSpeed) {
                NSLog(@"RedPreLoad-[Speed]: %lld", task.tcpSpeed);
            }
        }];
    });
}

#pragma mark preload url
+ (void)preloadVideoURL:(NSURL *)URL {
    // Initialize preload during the first launch.
    [self initPreloadConfig];
    RedPreloadParam param = {
        [RedMediaUtil cachePath],
        512 * 1024,
        @"",
        3000,
        0,
        @"",
        @""
    };
    
    [RedPreLoad open:URL param:param userData:NULL];
}

#pragma mark preload Json
+ (void)preloadVideoJson:(NSString *)json {
    // Initialize preload during the first launch.
    [self initPreloadConfig];
    RedPreloadParam param = {
        [RedMediaUtil cachePath],
        512 * 1024,
        @"",
        3000,
        0,
        @"",
        @""
    };
    [RedPreLoad openJson:json param:param userData:NULL];
}
@end
