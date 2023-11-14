//
//  RedPlayerSettings.h
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/20.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#ifndef RedPlayerSettings_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, RenderType) {
    RenderTypeMetal,
    RenderTypeOpenGLES,
    RenderTypeSampleBuffer
};

@interface RedPlayerSettings : NSObject
@property (nonatomic, assign) RenderType renderType;
@property (nonatomic, assign) BOOL hdrSwitch;
@property (nonatomic, assign) BOOL softDecoder;
@property (nonatomic, assign) BOOL bgdNotPause;
@property (nonatomic, assign) BOOL autoPreload;

+ (instancetype)sharedInstance;
@end

NS_ASSUME_NONNULL_END


#endif /* RedPlayerSettings_h */

