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

// 静态任务管理字典，字典key是场景
static NSMutableDictionary *sceneTasks;

+ (void)initPreloadConfig {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        // 初始化场景任务管理字典
        if (!sceneTasks) {
            sceneTasks = [NSMutableDictionary dictionary];
        }
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


+ (void)addTask:(RedPreLoad *)task forScene:(NSString *)scene {
    NSMutableArray *tasks = sceneTasks[scene];
    if (!tasks) {
        tasks = [NSMutableArray array];
        sceneTasks[scene] = tasks;
    }
    // 添加任务到对应场景的数组中
    [tasks addObject:task];
}

+ (void)closePreloadForScene:(NSString *)scene {
    NSArray *tasks = sceneTasks[scene];
    if (tasks) {
        for (RedPreLoad *task in tasks) {
            // 关闭预加载任务
            [task close];
        }
        // 移除对应场景下的所有任务
        [sceneTasks removeObjectForKey:scene];
    }
}

#pragma mark preload url
+ (void)preloadVideoURL:(NSURL *)URL forScene:(NSString *)scene {
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
    RedPreLoad *task = [RedPreLoad new];
    [task open:URL param:param userData:NULL];
    [self addTask:task forScene:scene];
}

#pragma mark preload Json
+ (void)preloadVideoJson:(NSString *)json forScene:(NSString *)scene {
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
    RedPreLoad *task = [RedPreLoad new];
    [task openJson:json param:param userData:NULL];
    [self addTask:task forScene:scene];
}
@end
