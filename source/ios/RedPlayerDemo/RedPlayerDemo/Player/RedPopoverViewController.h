//
//  RedPopoverViewController.h
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/4.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef void(^PopoverOptionBlock)(NSInteger selectedIndex);

@interface RedPopoverViewController : UIViewController

- (instancetype)initWithOptions:(NSArray<NSString *> *)options
                          icons:(nullable NSArray<NSString *> *)icons
                  selectedIndex:(NSInteger)selectedIndex
              completionHandler:(PopoverOptionBlock)completionHandler;

@end


NS_ASSUME_NONNULL_END
