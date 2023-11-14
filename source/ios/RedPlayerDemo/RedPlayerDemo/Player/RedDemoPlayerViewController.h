//
//  RedDemoPlayerViewController.h
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RedPlayer/RedMediaPlayBack.h>
@class RedMediaControl;

typedef void(^DemoCloseBlock)(void);

@interface RedDemoPlayerViewController : UIViewController
+ (void)presentFromViewController:(UIViewController *)viewController
                        withTitle:(NSString *)title
                              URL:(NSString *)url
                           isJson:(BOOL)isJson
                         playList:(NSArray *)playList
                    playListIndex:(NSInteger)playListIndex
                       completion:(void(^)(void))completion
                       closeBlock:(void(^)(void))closeBlock;
@end
