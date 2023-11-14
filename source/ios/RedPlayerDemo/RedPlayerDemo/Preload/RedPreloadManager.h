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
+ (void)preloadVideoURL:(NSURL *)URL;
+ (void)preloadVideoJson:(NSString *)json;
@end

NS_ASSUME_NONNULL_END
