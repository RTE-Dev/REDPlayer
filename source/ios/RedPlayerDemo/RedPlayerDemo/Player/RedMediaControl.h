//
//  RedMediaControl.h
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import <UIKit/UIKit.h>
@import RedPlayer;
@protocol RedMediaControlDelegate <NSObject>

- (void)onClickPlayPause:(id)sender;
- (void)switchPrevious:(id)sender;
- (void)switchNext:(id)sender;
- (void)onClickMute:(id)sender;
- (void)didSliderTouchUpInside;
- (void)slideCancelMediaControlHide;
- (void)slideContinueMediaControlHide;
- (void)speedChange:(CGFloat)speed;
- (void)showHideHud;
- (void)updatePlayInfo;
- (NSTimeInterval)duration;
- (NSTimeInterval)currentPlaybackTime;
- (void)chooseVideoIndex:(NSInteger)index;
- (void)mediaControlSetLoop:(BOOL)loop;

@end


@interface RedMediaControl : UIControl
@property (nonatomic, assign) NSInteger speedIndex;
@property (nonatomic, assign) NSInteger playListIndex;
@property (nonatomic, weak) id<RedMediaControlDelegate> delegatePlayer;
- (id)initWithFrame:(CGRect)frame 
     viewController:(UIViewController *)viewController
           playList:(NSArray *)playList
             isLoop:(BOOL)isLoop;
- (float)progressValue;
- (void)updatePlayPauseBtn:(BOOL)isPlaying;
- (void)updateMuteBtn:(BOOL)isMute;
- (void)startRefresh;
- (void)stopRefresh;
@end
