//
//  RedPreloadManager.h
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface RedPreloadManager : NSObject
+ (void)preloadVideoURL:(NSURL *)URL forScene:(NSString *)scene;
+ (void)preloadVideoJson:(NSString *)json forScene:(NSString *)scene;
+ (void)closePreloadForScene:(NSString *)scene;
@end

NS_ASSUME_NONNULL_END
