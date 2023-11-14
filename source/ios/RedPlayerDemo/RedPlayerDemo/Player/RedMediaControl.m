//
//  RedMediaControl.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import "RedMediaControl.h"
#import "RedPopoverViewController.h"
#import <MobileCoreServices/MobileCoreServices.h>
@import Masonry;
@interface RedMediaControl () <UIPopoverPresentationControllerDelegate, UIDocumentPickerDelegate>
@property (nonatomic, weak) UIViewController *viewController;

@property (nonatomic, strong) UIButton *playButton;
@property (nonatomic, strong) UIButton *preButton;
@property (nonatomic, strong) UIButton *nextButton;

@property (nonatomic, strong) UILabel *currentTimeLabel;
@property (nonatomic, strong) UILabel *totalDurationLabel;
@property (nonatomic, strong) UISlider *mediaProgressSlider;

@property (nonatomic, strong) UIButton *speedButton;
@property (nonatomic, strong) UIButton *hudButton;
@property (nonatomic, strong) UIButton *muteButton;

@property (nonatomic, strong) UIButton *loopButton;
@property (nonatomic, assign) BOOL isLoop;
@property (nonatomic, strong) UIButton *listButton;
@property (nonatomic, strong) NSArray *playList;

@end

@implementation RedMediaControl {
    BOOL _isMediaSliderBeingDragged;
    float _playSpeed;
    dispatch_source_t _timer;
}

- (void)dealloc {
    [self stopRefresh];
}

- (id)initWithFrame:(CGRect)frame
     viewController:(UIViewController *)viewController
           playList:(NSArray *)playList
             isLoop:(BOOL)isLoop {
    self = [super initWithFrame:frame];
    if (self) {
        _viewController = viewController;
        self.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.4];
        self.speedIndex = 1;
        self.playListIndex = 0;
        self.playList = playList;
        self.isLoop = isLoop;
        [self setupSubviews];
        [self startRefresh];

    }
    return self;
}

- (float)progressValue {
    return self.mediaProgressSlider.value;
}

#define kPlayButtonWidth 30
#define kSliderMargin 20
#define kSliderHeight 30
#define kTimeLabelWidth 50
#define kTimeLabelHeight 30
- (void)setupSubviews {
    if (!self.playButton) {
        self.playButton = [[UIButton alloc] initWithFrame:CGRectZero];
        [self.playButton setImage:[UIImage imageNamed:@"btn_player_pause"] forState:UIControlStateNormal];
        [self.playButton addTarget:self action:@selector(playAction:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.playButton];
    }
    [self.playButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.centerX.centerY.mas_equalTo(self);
        make.width.height.mas_equalTo(kPlayButtonWidth);
    }];
    
    if (self.playList.count > 0) {
        if (!self.preButton) {
            self.preButton = [[UIButton alloc] initWithFrame:CGRectZero];
            [self.preButton setImage:[UIImage imageNamed:@"icon_pre"] forState:UIControlStateNormal];
            [self.preButton addTarget:self action:@selector(preAction:) forControlEvents:UIControlEventTouchUpInside];
            [self addSubview:self.preButton];
        }
        self.preButton.enabled = (self.playListIndex > 0);
        [self.preButton mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerY.mas_equalTo(self);
            make.right.mas_equalTo(self.playButton.mas_left).offset(-20);
            make.width.height.mas_equalTo(kPlayButtonWidth);
        }];
        if (!self.nextButton) {
            self.nextButton = [[UIButton alloc] initWithFrame:CGRectZero];
            [self.nextButton setImage:[UIImage imageNamed:@"icon_next"] forState:UIControlStateNormal];
            [self.nextButton addTarget:self action:@selector(nextAction:) forControlEvents:UIControlEventTouchUpInside];
            [self addSubview:self.nextButton];
        }
        self.nextButton.enabled = (self.playListIndex < self.playList.count - 1);
        [self.nextButton mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerY.mas_equalTo(self);
            make.left.mas_equalTo(self.playButton.mas_right).offset(20);
            make.width.height.mas_equalTo(kPlayButtonWidth);
        }];
    }
    
    if (!self.mediaProgressSlider) {
        self.mediaProgressSlider = [[UISlider alloc] initWithFrame:CGRectZero];
        self.mediaProgressSlider.maximumTrackTintColor = UIColor.grayColor;
        self.mediaProgressSlider.tintColor = UIColor.redColor;
        [self.mediaProgressSlider setThumbImage:[UIImage imageNamed:@"slider"] forState:UIControlStateNormal];
        [self addSubview:self.mediaProgressSlider];
        
        [self.mediaProgressSlider addTarget:self action:@selector(slideTouchDown) forControlEvents:UIControlEventTouchDown];
        [self.mediaProgressSlider addTarget:self action:@selector(slideTouchUpInside) forControlEvents:UIControlEventTouchUpInside];
        [self.mediaProgressSlider addTarget:self action:@selector(slideValueChanged) forControlEvents:UIControlEventValueChanged];
        [self.mediaProgressSlider addTarget:self action:@selector(slideTouchUpOutside) forControlEvents:UIControlEventTouchUpOutside];
        [self.mediaProgressSlider addTarget:self action:@selector(slideTouchCancel) forControlEvents:UIControlEventTouchCancel];
    }
    [self.mediaProgressSlider mas_makeConstraints:^(MASConstraintMaker *make) {
        make.centerX.mas_equalTo(self);
        make.bottom.mas_equalTo(self).offset(-10);
        make.left.mas_equalTo(kSliderMargin);
        make.right.mas_equalTo(-kSliderMargin);
    }];
            
    if (!self.currentTimeLabel) {
        self.currentTimeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        self.currentTimeLabel.textColor = UIColor.whiteColor;
        self.currentTimeLabel.font = [UIFont boldSystemFontOfSize:15];
        [self addSubview:self.currentTimeLabel];
    }
    
    [self.currentTimeLabel mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(self.mediaProgressSlider);
        make.centerY.mas_equalTo(self);
        make.width.mas_equalTo(kTimeLabelWidth);
        make.height.mas_equalTo(kTimeLabelHeight);
    }];
    
    if (!self.totalDurationLabel) {
        self.totalDurationLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        self.totalDurationLabel.textColor = UIColor.whiteColor;
        self.totalDurationLabel.font = [UIFont boldSystemFontOfSize:15];
        [self addSubview:self.totalDurationLabel];
    }
    [self.totalDurationLabel mas_makeConstraints:^(MASConstraintMaker *make) {
        make.right.mas_equalTo(self.mediaProgressSlider);
        make.centerY.mas_equalTo(self);
        make.width.mas_equalTo(kTimeLabelWidth);
        make.height.mas_equalTo(kTimeLabelHeight);
    }];

    if(!self.hudButton) {
        self.hudButton = [[UIButton alloc] initWithFrame:CGRectZero];
        [self.hudButton setTitle:@"HUD" forState:UIControlStateNormal];
        self.hudButton.titleLabel.textAlignment = NSTextAlignmentLeft;
        self.hudButton.titleLabel.font = [UIFont boldSystemFontOfSize:20];
        [self.hudButton addTarget:self action:@selector(hudClick:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.hudButton];
    }
    [self.hudButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(kSliderMargin);
        make.top.mas_equalTo(10);
        make.width.mas_equalTo(50);
        make.height.mas_equalTo(30);
    }];
    
    if (!self.muteButton) {
        self.muteButton = [[UIButton alloc] init];
        [self.muteButton setImage:[UIImage imageNamed:@"volume"] forState:UIControlStateNormal];
        [self.muteButton addTarget:self action:@selector(muteChange:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.muteButton];
    }
    [self.muteButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(self.hudButton.mas_right).offset(10);
        make.centerY.mas_equalTo(self.hudButton);
        make.width.mas_equalTo(30);
        make.height.mas_equalTo(30);
    }];
    
    
    if (!self.listButton) {
        self.listButton = [[UIButton alloc] initWithFrame:CGRectZero];
        self.listButton.titleLabel.textAlignment = NSTextAlignmentCenter;
        [self.listButton setImage:[UIImage imageNamed:@"list"] forState:UIControlStateNormal];
        [self.listButton setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        if (self.playList.count == 0) {
            self.listButton.enabled = NO;
        }
        [self.listButton addTarget:self action:@selector(listButtonClick:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.listButton];
    }
    [self.listButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.right.mas_equalTo(self).offset(-kSliderMargin);
        make.centerY.mas_equalTo(self.hudButton);
        make.width.mas_equalTo(20);
        make.height.mas_equalTo(20);
    }];
        
    if (!self.loopButton) {
        self.loopButton = [[UIButton alloc] initWithFrame:CGRectZero];
        self.loopButton.titleLabel.textAlignment = NSTextAlignmentCenter;
        [self.loopButton setImage:[UIImage imageNamed:self.isLoop ? @"loop" : @"sequence"] forState:UIControlStateNormal];
        [self.loopButton setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        [self.loopButton addTarget:self action:@selector(loopButtonClick:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.loopButton];
    }
    [self.loopButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.right.mas_equalTo(self.listButton.mas_left).offset(-20);
        make.centerY.mas_equalTo(self.hudButton);
        make.width.mas_equalTo(20);
        make.height.mas_equalTo(20);
    }];
    
    if (!self.speedButton) {
        self.speedButton = [[UIButton alloc] initWithFrame:CGRectZero];
        self.speedButton.titleLabel.textAlignment = NSTextAlignmentCenter;
        [self.speedButton setTitle:@"1.0x" forState:UIControlStateNormal];
        [self.speedButton setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        self.speedButton.titleLabel.font = [UIFont boldSystemFontOfSize:18];
        [self.speedButton addTarget:self action:@selector(speedButtonClick:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.speedButton];
    }
    [self.speedButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.right.mas_equalTo(self.loopButton.mas_left).offset(-10);
        make.centerY.mas_equalTo(self.hudButton);
        make.width.mas_equalTo(50);
        make.height.mas_equalTo(30);
    }];

}

- (void)loopButtonClick:(UIButton *)sender {
    self.isLoop = !self.isLoop;
    [self.loopButton setImage:[UIImage imageNamed:self.isLoop? @"loop": @"sequence"] forState:UIControlStateNormal];
    if ([self.delegatePlayer respondsToSelector:@selector(mediaControlSetLoop:)]) {
        [self.delegatePlayer mediaControlSetLoop:self.isLoop];
    }
}
    
- (void)listButtonClick:(UIButton *)sender {
    NSMutableArray *fileNameList = [@[] mutableCopy];
    NSMutableArray *iconList = [@[] mutableCopy];
    [self.playList enumerateObjectsUsingBlock:^(id  _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        if ([obj isKindOfClass:[NSDictionary class]]) {
            NSDictionary *playInfo = obj;
            [fileNameList addObject:playInfo[@"fileName"] ?: [NSString stringWithFormat:@"Local File %ld", idx]];
            [iconList addObject:playInfo[@"coverImage"]];
        }
    }];
    __weak typeof(self) weakSelf = self;
    RedPopoverViewController *contentViewController = [[RedPopoverViewController alloc] initWithOptions:fileNameList
                                                                                                  icons:iconList
                                                                                          selectedIndex:self.playListIndex
                                                                                      completionHandler:^(NSInteger selectedIndex) {
        __strong typeof(self) strongSelf = weakSelf;
        if ([strongSelf.delegatePlayer respondsToSelector:@selector(chooseVideoIndex:)]) {
            [strongSelf.delegatePlayer chooseVideoIndex:selectedIndex];
        }
        strongSelf.playListIndex = selectedIndex;
    }];
    // 设置Popover
    contentViewController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *popoverPresentationController = contentViewController.popoverPresentationController;
    popoverPresentationController.delegate = self;
    popoverPresentationController.sourceView = _listButton;
    popoverPresentationController.sourceRect = _listButton.bounds;
    popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
        
    // 显示Popover
    [self.viewController presentViewController:contentViewController animated:YES completion:nil];
}

- (void)speedButtonClick:(UIButton *)sender {
    // 创建要显示的内容视图控制器
    NSArray *options = @[@"0.5x", @"1.0x", @"1.25x", @"1.5x", @"2.0x"];
    __weak typeof(self) weakSelf = self;
    RedPopoverViewController *contentViewController = [[RedPopoverViewController alloc] initWithOptions:options
                                                                                                  icons:nil
                                                                                          selectedIndex:_speedIndex
                                                                                      completionHandler:^(NSInteger selectedIndex) {
        __strong typeof(self) strongSelf = weakSelf;
        NSString *speedStr = options[selectedIndex];
        if ([strongSelf.delegatePlayer respondsToSelector:@selector(speedChange:)]) {
            CGFloat speed = [[speedStr substringToIndex:speedStr.length - 1] floatValue];
            [strongSelf.delegatePlayer speedChange:speed];
        }
        strongSelf.speedIndex = selectedIndex;
        [strongSelf.speedButton setTitle:speedStr forState:UIControlStateNormal];
    }];
    // 设置Popover
    contentViewController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *popoverPresentationController = contentViewController.popoverPresentationController;
    popoverPresentationController.delegate = self;
    popoverPresentationController.sourceView = sender;
    popoverPresentationController.sourceRect = sender.bounds;
    popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionDown;
        
    // 显示Popover
    [self.viewController presentViewController:contentViewController animated:YES completion:nil];
}

- (void)hudClick:(id)sender {
    if ([self.delegatePlayer respondsToSelector:@selector(showHideHud)]) {
        [self.delegatePlayer showHideHud];
    }
}

#pragma mark - UIPopoverPresentationControllerDelegate

- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController *)controller {
    return UIModalPresentationNone;
}

- (void)beginDragMediaSlider {
    _isMediaSliderBeingDragged = YES;
}

- (void)endDragMediaSlider {
    _isMediaSliderBeingDragged = NO;
}

- (void)continueDragMediaSlider {
    [self refreshMediaControl];
}

- (void)refreshMediaControl {
    NSTimeInterval duration = self.delegatePlayer.duration;

    // duration
    NSInteger intDuration = duration + 0.5;
    if (intDuration > 0) {
        self.mediaProgressSlider.maximumValue = duration;
        self.totalDurationLabel.text = [NSString stringWithFormat:@"%02d:%02d", (int)(intDuration / 60), (int)(intDuration % 60)];
    } else {
        self.totalDurationLabel.text = @"--:--";
        self.mediaProgressSlider.maximumValue = 1.0f;
    }

    // position
    NSTimeInterval position = 0.0;
    if (_isMediaSliderBeingDragged) {
        position = self.mediaProgressSlider.value;
    } else {
        position = self.delegatePlayer.currentPlaybackTime;
    }
    NSInteger intPosition = position + 0.5;
    if (intDuration > 0) {
        self.mediaProgressSlider.value = position;
    } else {
        self.mediaProgressSlider.value = 0.0f;
    }
    self.currentTimeLabel.text = [NSString stringWithFormat:@"%02d:%02d", (int)(intPosition / 60), (int)(intPosition % 60)];

    if ([self.delegatePlayer respondsToSelector:@selector(updatePlayInfo)]) {
        [self.delegatePlayer updatePlayInfo];
    }
}

- (void)startRefresh {
    // 创建GCD定时器
    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(_timer, DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC, 0 * NSEC_PER_SEC);
    dispatch_source_set_event_handler(_timer, ^{
        [self refreshMediaControl];
    });
    dispatch_resume(_timer);
}

- (void)stopRefresh {
    // 取消定时器
    if (_timer) {
        dispatch_source_cancel(_timer);
        _timer = nil;
    }
}


- (void)playAction:(id)sender {
    if ([self.delegatePlayer respondsToSelector:@selector(onClickPlayPause:)]) {
        [self.delegatePlayer onClickPlayPause:sender];
    }
}

- (void)preAction:(id)sender {
    if ([self.delegatePlayer respondsToSelector:@selector(switchPrevious:)]) {
        [self.delegatePlayer switchPrevious:sender];
    }
}

- (void)nextAction:(id)sender {
    if ([self.delegatePlayer respondsToSelector:@selector(switchNext:)]) {
        [self.delegatePlayer switchNext:sender];
    }
}

- (void)slideTouchDown {
    [self beginDragMediaSlider];
    [self slideCancelMediaControlHide];
}

- (void)slideTouchCancel {
    [self endDragMediaSlider];
    [self slideContinueMediaControlHide];
}

- (void)slideTouchUpOutside {
    [self endDragMediaSlider];
    [self slideContinueMediaControlHide];
}

- (void)slideTouchUpInside {
    if ([self.delegatePlayer respondsToSelector:@selector(didSliderTouchUpInside)]) {
        [self.delegatePlayer didSliderTouchUpInside];
    }
    [self endDragMediaSlider];
    [self slideContinueMediaControlHide];
}

- (void)slideValueChanged {
    [self continueDragMediaSlider];
}

- (void)slideCancelMediaControlHide {
    if ([self.delegatePlayer respondsToSelector:@selector(slideCancelMediaControlHide)]) {
        [self.delegatePlayer slideCancelMediaControlHide];
    }
}

- (void)slideContinueMediaControlHide {
    if ([self.delegatePlayer respondsToSelector:@selector(slideContinueMediaControlHide)]) {
        [self.delegatePlayer slideContinueMediaControlHide];
    }
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self setupSubviews];
}

- (void)updatePlayPauseBtn:(BOOL)isPlaying {
    [self.playButton setImage:[UIImage imageNamed: isPlaying ? @"btn_player_pause" : @"btn_player_play"] forState:UIControlStateNormal];
}

- (void)updateMuteBtn:(BOOL)isMute {
    [self.muteButton setImage:[UIImage imageNamed:isMute ? @"mute" : @"volume"] forState:UIControlStateNormal];
}

- (void)muteChange:(UIButton *)muteButton {
    if ([self.delegatePlayer respondsToSelector:@selector(onClickMute:)]) {
        [self.delegatePlayer onClickMute:muteButton];
    }
}
@end
