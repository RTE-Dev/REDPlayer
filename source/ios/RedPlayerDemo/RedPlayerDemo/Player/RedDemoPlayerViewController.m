//
//  RedDemoPlayerViewController.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/1.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import "RedDemoPlayerViewController.h"
#import "RedMediaControl.h"
#import "RedPlayerSettings.h"
#import <RedPlayer/RedPlayerController.h>
#import <AVFoundation/AVFoundation.h>
#import <AVFAudio/AVFAudio.h>
#import "RedLoadingView.h"
#import "pthread.h"

@import Masonry;
@import MediaPlayer;

#define kMediaControlMargin (UIDeviceOrientationIsLandscape(UIDevice.currentDevice.orientation)? 100 : 20)
#define kMediaControlHeight 140
#define kHUDMargin 20
#define kHUDWidth 350
#define kHUDHeight 300

typedef NS_ENUM(NSUInteger, RedTipsPosition) {
    RedTipsPositionNone,
    RedTipsPositionCenter,
    RedTipsPositionLeft,
    RedTipsPositionRight
};

@interface RedDemoPlayerViewController () <RedMediaControlDelegate>
@property (nonatomic, strong) NSString *url;
@property (nonatomic, assign) BOOL isJson;
@property (nonatomic, assign) BOOL isLive;
@property (nonatomic, strong) NSArray *playList;
@property (nonatomic, assign) NSInteger playListIndex;
@property (nonatomic, assign) RedScalingMode scaleMode;
@property (nonatomic, strong) id<RedMediaPlayback> player;

@property (nonatomic, strong) RedExtendedButton *closeButton;
@property (nonatomic, strong) RedExtendedButton *scaleModeButton;
@property (nonatomic, strong) RedMediaControl *mediaControl;
@property (nonatomic, strong) UITextView *hudTextView;
@property (nonatomic, strong) RedLoadingView *loadingView;
@property (nonatomic, strong) UILabel *tipsView;
@property (nonatomic, strong) UIView *overlayView;
@property (nonatomic, strong) UIImageView *animationView;
@property (nonatomic, assign) CGFloat recordPlaybackRate;
@property (nonatomic, assign) BOOL isMute;
@property (nonatomic, copy) DemoCloseBlock closeBlock;
@property (nonatomic, assign) BOOL isLoop;
@property (nonatomic, strong) MPVolumeView *mpVolumeView;
@property (nonatomic, strong) UISlider *volumeViewSlider;
@property (nonatomic, strong) UIProgressView *customVolumeIndicator;

@end

@implementation RedDemoPlayerViewController
{
    BOOL _isPlayingBeforeInactive;
}

- (void)dealloc {
    [AVAudioSession.sharedInstance removeObserver:self forKeyPath:@"outputVolume"];
    [self shutdown];
}

- (void)shutdown {
    [RedMediaUtil shared].reuseBox = nil;
    if (self.player) {
        [self.player shutdown];
        self.player = nil;
    }
    [self removeMovieNotificationObservers];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

+ (void)setupLogCallback {
    [RedPlayerController setLogCallbackLevel:k_RED_LOG_DEBUG];
    [RedPlayerController setLogCallback:^(RedLogLevel loglevel, NSString *tagInfo, NSString *logContent) {
        NSLog(@"%@", logContent);
    }];
}

+ (void)presentFromViewController:(UIViewController *)viewController
                        withTitle:(NSString *)title
                              URL:(NSString *)url
                           isJson:(BOOL)isJson
                           isLive:(BOOL)isLive
                         playList:(NSArray *)playList
                    playListIndex:(NSInteger)playListIndex
                       completion:(void (^)(void))completion
                       closeBlock:(void (^)(void))closeBlock {
    
    [RedDemoPlayerViewController setupLogCallback];
    RedDemoPlayerViewController *demoPlayerVC = [[RedDemoPlayerViewController alloc] initWithURL:url isJson:isJson isLive:isLive];
    demoPlayerVC.playList = [playList copy];
    demoPlayerVC.playListIndex = playListIndex;
    demoPlayerVC.modalPresentationStyle = UIModalPresentationFullScreen;
    demoPlayerVC.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
    [viewController presentViewController:demoPlayerVC animated:YES completion:completion];
    demoPlayerVC.closeBlock = closeBlock;
}

- (instancetype)initWithURL:(NSString *)url isJson:(BOOL)isJson isLive:(BOOL)isLive {
    self = [super init];
    if (self) {
        self.url = url;
        self.isJson = isJson;
        self.isLive = isLive;
    }
    return self;
}

- (void)registerApplicationObservers {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillEnterForeground)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationDidBecomeActive)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillResignActive)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationDidEnterBackground)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillTerminate)
                                                 name:UIApplicationWillTerminateNotification
                                               object:nil];
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}

- (void)applicationWillEnterForeground {
}

- (void)applicationDidBecomeActive {
    if (![RedPlayerSettings sharedInstance].bgdNotPause) { // become active - resume
        if (_isPlayingBeforeInactive) {
            [self.player play];
        }
    }
}

- (void)applicationWillResignActive {
    if (![RedPlayerSettings sharedInstance].bgdNotPause) { // resign active - pause
        _isPlayingBeforeInactive = self.player.isPlaying;
        [self.player pause];
    }
}

- (void)applicationDidEnterBackground {
}

- (void)applicationWillTerminate {
    [self.player pause];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
        
    // active audiosession
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
    [[AVAudioSession sharedInstance] setActive:YES error:nil];

    [self setupPlayer];
    // HUD Info
    [self createHudTextView];
    // Back Button
    [self createCloseView];
    // gestures
    [self addGestureControls];

    [self.view addSubview:[self mpVolumeView]];
}

- (void)setupPlayer {
    RenderType renderType = [RedPlayerSettings sharedInstance].renderType;
    BOOL isReusePlayer = NO;
    if ([_url isEqualToString:[RedMediaUtil shared].reuseBox.reuseUrl] && [RedMediaUtil shared].reuseBox.reusePlayer) {
        self.player = [RedMediaUtil shared].reuseBox.reusePlayer;
        isReusePlayer = YES;
    } else {
        self.player = [[RedPlayerController alloc] initWithContentURLString:@"" withRenderType:renderType == RenderTypeSampleBuffer ? RedRenderTypeSampleBufferDisplayLayer :(renderType == RenderTypeOpenGLES ? RedRenderTypeOpenGL : RedRenderTypeMetal)];

        if (self.isJson) { // json type url
            [self.player setContentString:self.url];
        } else {
            [self.player setContentURL:[NSURL URLWithString:self.url]];
        }
        
        if ([RedPlayerSettings sharedInstance].hdrSwitch) {
            [self.player setEnableHDR:YES];
        }
    }
    
    self.player.view.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.player.view.frame = self.view.bounds;
    self.player.scalingMode = RedScalingModeAspectFit;
    self.player.shouldAutoplay = YES;
    self.player.notPauseGlviewBackground = [RedPlayerSettings sharedInstance].bgdNotPause;
    [self.player setLoop:self.isLoop ? 0 : 1];
    [self.player setMute:NO];
    [self.player setEnableVTB:![RedPlayerSettings sharedInstance].softDecoder];
    [self.player setVideoCacheDir:[RedMediaUtil cachePath]];
    if (self.isLive) {
        [self.player setLiveMode:YES]; // config live mode
    }
    [self.view addSubview:self.player.view];
    [self.view sendSubviewToBack:self.player.view];
    [self installMovieNotificationObservers];
    if (!isReusePlayer) {
        [self.player prepareToPlay];
    }
    self.recordPlaybackRate = 1.0;
    // Register Application Observers
    [self registerApplicationObservers];
    // Media Control UI
    [self createMediaControlView];
    // Loading UI
    if (!self.player.isPlaying) {
        [self startLoading];
    }
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    [self removeMovieNotificationObservers];
    
    [UIApplication sharedApplication].idleTimerDisabled = NO;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)loadStateDidChange:(NSNotification*)notification {

    RedLoadState loadState = _player.loadState;
    
    if ((loadState & RedLoadStatePlaythroughOK) != 0) {
        [self stopLoading];
        NSLog(@"loadStateDidChange: RedLoadStatePlaythroughOK: %d\n", (int)loadState);
    } else if ((loadState & RedLoadStateStalled) != 0) {
        [self startLoading];
        NSLog(@"loadStateDidChange: RedLoadStateStalled: %d\n", (int)loadState);
    } else {
        NSLog(@"loadStateDidChange: ???: %d\n", (int)loadState);
    }
}

- (void)moviePlayUrlChangeMsg:(NSNotification*)notification {
    NSLog(@"movie player url change msg");
}

- (void)moviePlayBackDidFinish:(NSNotification*)notification {
    int reason = [[[notification userInfo] valueForKey:RedPlayerPlaybackDidFinishReasonUserInfoKey] intValue];
    
    switch (reason)
    {
        case RedFinishReasonPlaybackEnded: {
            NSLog(@"playbackStateDidChange: RedFinishReasonPlaybackEnded: %d\n", reason);
            if (!self.isLoop && self.playListIndex + 1 < self.playList.count) {
                [self chooseVideoIndex: self.playListIndex + 1];
            }
        }
            break;
            
        case RedFinishReasonUserExited:
            NSLog(@"playbackStateDidChange: RedFinishReasonUserExited: %d\n", reason);
            break;
            
        case RedFinishReasonPlaybackError:
            NSLog(@"playbackStateDidChange: RedFinishReasonPlaybackError: %d\n", reason);
            break;
            
        default:
            NSLog(@"playbackPlayBackDidFinish: ???: %d\n", reason);
            break;
    }
}

- (void)mediaIsPreparedToPlayDidChange:(NSNotification*)notification {
    NSLog(@"mediaIsPreparedToPlayDidChange\n");
}

- (void)moviePlayBackStateDidChange:(NSNotification*)notification {
    switch (_player.playbackState)
    {
        case RedPlaybackStateStopped: {
            [self pauseAction];
            NSLog(@"RedPlayBackStateDidChange %d: stoped", (int)_player.playbackState);
            break;
        }
        case RedPlaybackStatePlaying: {
            [self playAction];
            NSLog(@"RedPlayBackStateDidChange %d: playing", (int)_player.playbackState);
            break;
        }
        case RedPlaybackStatePaused: {
            [self pauseAction];
            NSLog(@"RedPlayBackStateDidChange %d: paused", (int)_player.playbackState);
            break;
        }
        case RedPlaybackStateInterrupted: {
            NSLog(@"RedPlayBackStateDidChange %d: interrupted", (int)_player.playbackState);
            break;
        }
        case RedPlaybackStateSeekingForward:
        case RedPlaybackStateSeekingBackward: {
            NSLog(@"RedPlayBackStateDidChange %d: seeking", (int)_player.playbackState);
            break;
        }
        default: {
            NSLog(@"RedPlayBackStateDidChange %d: unknown", (int)_player.playbackState);
            break;
        }
    }
}

- (void)moviePlayCacheDidFinish:(NSNotification*)notification {
    NSDictionary *userInfo = notification.userInfo;
    if (!userInfo) {
        return;
    }
    
    NSURL *URL = userInfo[RedPlayerCacheURLUserInfoKey];
    if (!URL) {
        return;
    }
    NSLog(@"******* moviePlayCacheDidFinish URL:%@",URL);
}

#pragma mark Install Movie Notifications

/* Register observers for the various movie object notifications. */
- (void)installMovieNotificationObservers {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(loadStateDidChange:)
                                                 name:RedPlayerLoadStateDidChangeNotification
                                               object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackDidFinish:)
                                                 name:RedPlayerPlaybackDidFinishNotification
                                               object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(mediaIsPreparedToPlayDidChange:)
                                                 name:RedMediaPlaybackIsPreparedToPlayDidChangeNotification
                                               object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayBackStateDidChange:)
                                                 name:RedPlayerPlaybackStateDidChangeNotification
                                               object:_player];
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayCacheDidFinish:)
                                                 name:RedPlayerCacheDidFinishNotification
                                               object:_player];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(moviePlayUrlChangeMsg:)
                                                 name:RedPlayerUrlChangeMsgNotification
                                               object:_player];
    
}

#pragma mark Remove Movie Notification Handlers

/* Remove the movie notification observers from the movie object. */
- (void)removeMovieNotificationObservers {
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedPlayerLoadStateDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedPlayerPlaybackDidFinishNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedMediaPlaybackIsPreparedToPlayDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedPlayerPlaybackStateDidChangeNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedPlayerUrlChangeMsgNotification object:_player];
    [[NSNotificationCenter defaultCenter]removeObserver:self name:RedPlayerNaturalSizeAvailableNotification object:_player];
}

// MARK: Close Button
- (void)createCloseView {
    if (!self.closeButton) {
        self.closeButton = [[RedExtendedButton alloc] initWithExtendedTouchArea:30];
        [self.closeButton setImage:[UIImage imageNamed:@"back_arrow"] forState:UIControlStateNormal];
        [self.closeButton addTarget:self action:@selector(closeClick:) forControlEvents:UIControlEventTouchUpInside];
        [self.view addSubview:self.closeButton];
    }
    [self.closeButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(self.view.mas_safeAreaLayoutGuideLeft).offset(20);
        make.top.mas_equalTo(self.view.mas_safeAreaLayoutGuideTop).offset(40);
        make.width.mas_equalTo(25);
        make.height.mas_equalTo(25);
    }];
}

- (void)closeClick:(id)sender {
    [self shutdown];
    [self stopLoading];
    [self.mediaControl removeFromSuperview];
    [self.mediaControl stopRefresh];
    self.mediaControl = nil;
    [self dismissViewControllerAnimated:NO completion:^{
        if (self.closeBlock) {
            self.closeBlock();
        }
    }];
}

// MARK: ScaleMode Button
- (void)createScaleModeButton {
    if (!self.scaleModeButton) {
        self.scaleModeButton = [[RedExtendedButton alloc] initWithExtendedTouchArea:30];
        [self.scaleModeButton setImage:[UIImage imageNamed:@"icon_fit"] forState:UIControlStateNormal];
        [self.scaleModeButton addTarget:self action:@selector(scaleModeClick:) forControlEvents:UIControlEventTouchUpInside];
        [self.view addSubview:self.scaleModeButton];
    }
    [self.scaleModeButton mas_makeConstraints:^(MASConstraintMaker *make) {
        make.right.mas_equalTo(self.view.mas_safeAreaLayoutGuideRight).offset(-20);
        make.top.mas_equalTo(self.view.mas_safeAreaLayoutGuideTop).offset(40);
        make.width.mas_equalTo(30);
        make.height.mas_equalTo(30);
    }];
}

- (void)scaleModeClick:(id)sender {
    [self switchScaleMode];
}

- (void)switchScaleMode {
    self.player.scalingMode = (self.player.scalingMode + 1) > RedScalingModeFill ? RedScalingModeAspectFit : (self.player.scalingMode + 1);
    [self.scaleModeButton setImage:[UIImage imageNamed:[self scalingModeIcon:self.player.scalingMode]] forState:UIControlStateNormal];
}


- (NSString *)scalingModeIcon:(RedScalingMode)scalingMode {
    switch (scalingMode) {
        case RedScalingModeAspectFit:
            return @"icon_fit";
        case RedScalingModeAspectFill:
            return @"icon_full";
        case RedScalingModeFill:
            return @"icon_fill";
        default:
            return @"icon_fit";
    }
}

#pragma mark - HUD
- (void)createHudTextView {
    self.hudTextView = [[UITextView alloc] initWithFrame:CGRectMake(kHUDMargin, 44, kHUDWidth, kHUDHeight)];
    self.hudTextView.editable = NO;
    self.hudTextView.userInteractionEnabled = NO;
    self.hudTextView.textColor = UIColor.greenColor;
    self.hudTextView.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.5];
    [self.view addSubview:self.hudTextView];
    self.hudTextView.hidden = YES;
    [self.hudTextView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(self.view.mas_safeAreaLayoutGuideLeft).offset(kHUDMargin);
        make.top.mas_equalTo(self.view.mas_safeAreaLayoutGuideTop).offset(44);
        make.width.mas_equalTo(kHUDWidth);
        make.height.mas_equalTo(kHUDHeight);
    }];
}


- (void)showHideHud {
    self.hudTextView.hidden = !self.hudTextView.isHidden;
}
#pragma mark - mediaControl

- (void)createMediaControlView {
    if (self.scaleModeButton) {
        [self.scaleModeButton removeFromSuperview];
        self.scaleModeButton = nil;
    }
    [self createScaleModeButton];
    
    if (self.mediaControl) {
        [self.mediaControl removeFromSuperview];
        self.mediaControl.delegatePlayer = nil;
        self.mediaControl = nil;
    }
    self.mediaControl = [[RedMediaControl alloc] initWithFrame:CGRectZero viewController:self playList:[self.playList copy] isLoop:self.isLoop isLive:self.isLive];
    self.mediaControl.playListIndex = _playListIndex;
    self.mediaControl.layer.cornerRadius = 10;
    self.mediaControl.layer.masksToBounds = YES;
    [self.view addSubview:self.mediaControl];
    self.mediaControl.delegatePlayer = self;
    [self.mediaControl mas_makeConstraints:^(MASConstraintMaker *make) {
        make.left.mas_equalTo(self.view.mas_safeAreaLayoutGuideLeft).offset(20);
        make.right.mas_equalTo(self.view.mas_safeAreaLayoutGuideRight).offset(-20);
        make.bottom.mas_equalTo(self.view.mas_safeAreaLayoutGuideBottom);
        make.height.mas_equalTo(kMediaControlHeight);
    }];
}

- (void)showHideMediaControl {
    if (self.mediaControl.isHidden) {
        self.closeButton.hidden = NO;
        self.mediaControl.hidden = NO;
        self.scaleModeButton.hidden = NO;
        [self cancelDelayedHide];
        [self performSelector:@selector(hideMediaControl) withObject:nil afterDelay:5];
        [self.mediaControl startRefresh];
    } else {
        [self hideMediaControl];
    }
}

- (void)hideMediaControl {
    self.mediaControl.hidden = YES;
    self.closeButton.hidden = YES;
    self.scaleModeButton.hidden = YES;
    [self cancelDelayedHide];
}

- (void)cancelDelayedHide {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(hideMediaControl) object:nil];
}

- (void)playAction {
    [self.mediaControl updatePlayPauseBtn:YES];
    if (self.mediaControl.isHidden == NO) {
        [self.mediaControl startRefresh];
    }
}

- (void)pauseAction {
    [self.mediaControl updatePlayPauseBtn:NO];
    [self.mediaControl stopRefresh];
}

- (void)chooseVideoIndex:(NSInteger)index {
    _playListIndex = index;
    [self shutdown];
    NSDictionary *playInfo = self.playList[index];
    self.url = playInfo[@"videoUrl"];
    self.isJson = [playInfo[@"isJson"] boolValue];
    [self setupPlayer];
}

- (void)mediaControlSetLoop:(BOOL)loop {
    self.isLoop = loop;
    [self.player setLoop:loop ? 0 : 1];
}

- (void)switchNext:(id)sender {
    if (_playListIndex + 1 < self.playList.count) {
        [self chooseVideoIndex:_playListIndex + 1];
    }
}

- (void)switchPrevious:(id)sender {
    if (_playListIndex - 1 >= 0) {
        [self chooseVideoIndex:_playListIndex - 1];
    }
}

// MARK: Loading
- (void)startLoading {
    if (!self.loadingView) {
        self.loadingView = [[RedLoadingView alloc]initWithFrame:CGRectMake(0, 0, 100, 100)];
    }
    [self.view addSubview:self.loadingView];
    [self.view bringSubviewToFront:self.loadingView];
    [self.loadingView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.centerX.mas_equalTo(self.view);
        make.centerY.equalTo(self.view.mas_bottom).multipliedBy(1.0/3.0);
        make.width.height.mas_equalTo(100);
    }];
    [self.loadingView startLoading];
}
- (void)stopLoading {
    [self.loadingView stopLoading];
    [self.loadingView removeFromSuperview];
    self.loadingView = nil;
}

// MARK: Gestures
- (void)addGestureControls {
    UITapGestureRecognizer *doubleTap = nil;
    if (!self.isLive) {
        // Long Press - 2x Playback Speed
        UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
        [self.view addGestureRecognizer:longPress];
        
        // Double Tap
        doubleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTap:)];
        doubleTap.numberOfTapsRequired = 2;
        [self.view addGestureRecognizer:doubleTap];
    }
    
    // Single Tap - Show/Hide tips
    UITapGestureRecognizer *singleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap:)];
    singleTap.numberOfTapsRequired = 1;
    [self.view addGestureRecognizer:singleTap];
    if (doubleTap) {
        [singleTap requireGestureRecognizerToFail:doubleTap];
    }
    
    // Pan - Volume/Brightness
    UIPanGestureRecognizer *panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
    [self.view addGestureRecognizer:panGesture];
}

- (MPVolumeView *)mpVolumeView {
    if (!_mpVolumeView) {
        _mpVolumeView = [[MPVolumeView alloc] init];
        _mpVolumeView.hidden = NO;
        [_mpVolumeView setFrame:CGRectMake(-100, -100, 40, 40)];
        [_mpVolumeView setShowsVolumeSlider:YES];

        for (UIView *view in [_mpVolumeView subviews]){
            if ([view.class.description isEqualToString:@"MPVolumeSlider"]){
                self.volumeViewSlider = (UISlider*)view;
                [AVAudioSession.sharedInstance addObserver:self forKeyPath:@"outputVolume" options:NSKeyValueObservingOptionNew context:nil];
                [self createVolumeIndicator];
                break;
            }
        }
    }
    return _mpVolumeView;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if ([keyPath isEqualToString:@"outputVolume"]) {
        [self showHideVolumeIndicator:YES];
        CGFloat volume = [change[@"new"] floatValue];
        self.customVolumeIndicator.progress = volume;
    }
}

- (void)createVolumeIndicator {
    self.customVolumeIndicator = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    self.customVolumeIndicator.backgroundColor = UIColor.lightGrayColor;
    self.customVolumeIndicator.tintColor = UIColor.whiteColor;
    self.customVolumeIndicator.progress = self.volumeViewSlider.value;
    self.customVolumeIndicator.hidden = YES;
    [self.view addSubview:self.customVolumeIndicator];
    [self.customVolumeIndicator mas_makeConstraints:^(MASConstraintMaker *make) {
        make.centerX.mas_equalTo(self.view);
        make.centerY.equalTo(self.view.mas_bottom).multipliedBy(1.0/8.0);
        make.width.mas_equalTo(200);
        make.height.mas_equalTo(5);
    }];
}

- (void)showHideVolumeIndicator:(BOOL)show {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(delayHideVolumeIndicator) object:nil];
    if (show) {
        self.customVolumeIndicator.hidden = NO;
        [self performSelector:@selector(delayHideVolumeIndicator) withObject:nil afterDelay:1];
    } else {
        self.customVolumeIndicator.hidden = YES;
    }
}

- (void)delayHideVolumeIndicator {
    [self showHideVolumeIndicator:NO];
}

- (void)handlePanGesture:(UIPanGestureRecognizer *)gesture {
    static CGFloat startBrightness; // record brightness
    static CGFloat startVolume;     // record volume
    static BOOL isBrightnessChange = NO; // allow brightness change
    static BOOL isVolumeChange = NO; // allow volume change
    static BOOL brightNessCanChange = NO;     // allow brightness gesture
    static BOOL volumeCanChange = NO;     // allow volume gesture
    CGPoint location = [gesture locationInView:self.view];
    CGPoint translation = [gesture translationInView:self.view];
    
    CGFloat width = self.view.bounds.size.width;
    CGFloat height;
    if (UIDeviceOrientationIsLandscape([UIDevice currentDevice].orientation)) {
        // landscape
        height = MIN(self.view.bounds.size.width, self.view.bounds.size.height);
    } else {
        // portrait
        height = MAX(self.view.bounds.size.width, self.view.bounds.size.height);
    }
    
    CGFloat locationRatio = location.x / width;
    CGFloat translationRatio = translation.y / height * 5;
    if (gesture.state == UIGestureRecognizerStateBegan) {
        if (locationRatio < 0.125 || locationRatio > 0.875) {
            if (locationRatio < 0.125) {
                startBrightness = [UIScreen mainScreen].brightness;
                brightNessCanChange = YES;
            } else {
                startVolume = self.volumeViewSlider.value;
                volumeCanChange = YES;
            }
        }
    }
    if (!brightNessCanChange && !volumeCanChange) {
        return;
    }
    if (gesture.state == UIGestureRecognizerStateEnded || gesture.state == UIGestureRecognizerStateCancelled) {
        isBrightnessChange = NO;
        isVolumeChange = NO;
        brightNessCanChange = NO;
        volumeCanChange = NO;
    }
    if (fabs(translation.y) >= 1) {
        if (brightNessCanChange) {
            isBrightnessChange = YES;
        }
        if (volumeCanChange) {
            isVolumeChange = YES;
        }
    }
    if (isBrightnessChange) {
        CGFloat newBrightness = startBrightness - translationRatio;
        newBrightness = MAX(0, MIN(1, newBrightness));
        [UIScreen mainScreen].brightness = newBrightness;
        startBrightness = newBrightness;
        [self showTips:[NSString stringWithFormat:@"Brightness\n %.f", newBrightness * 100] position:RedTipsPositionLeft autoHide:YES hasFeedback:NO animationView:nil];
    } else if (isVolumeChange) {
        CGFloat newVolume = startVolume - translationRatio;
        newVolume = MAX(0, MIN(1, newVolume));
        self.volumeViewSlider.value = newVolume;
        startVolume = newVolume;
        [self showTips:[NSString stringWithFormat:@"Volume\n %.f", newVolume * 100] position:RedTipsPositionRight autoHide:YES hasFeedback:NO animationView:nil];
    }
    [gesture setTranslation:CGPointZero inView:self.view];
}


- (void)handleSingleTap:(UITapGestureRecognizer *)gesture {
    if (gesture.state == UIGestureRecognizerStateRecognized) {
        [self showHideMediaControl];
    }
}

- (void)handleLongPress:(UILongPressGestureRecognizer *)gesture {
    if (gesture.state == UIGestureRecognizerStateBegan) {
        self.recordPlaybackRate = self.player.playbackRate;
        self.player.playbackRate = 2.0;
        [self showTips:@"2.0X" 
              position:RedTipsPositionNone
              autoHide:NO
           hasFeedback:NO
         animationView:[self createForwardAnimationView]];
    } else if (gesture.state == UIGestureRecognizerStateEnded || gesture.state == UIGestureRecognizerStateCancelled) {
        self.player.playbackRate = self.recordPlaybackRate;
        [self showTips:[NSString stringWithFormat:@"%.2fX", self.recordPlaybackRate] 
              position:RedTipsPositionNone
              autoHide:YES
           hasFeedback:NO
         animationView:nil];
    }
}

- (UIImageView *)createForwardAnimationView {
    UIImageView *forwardAnimationView = [[UIImageView alloc] initWithFrame:CGRectZero];
    NSArray *images = @[[UIImage imageNamed:@"forward1"],
                        [UIImage imageNamed:@"forward2"]
    ];
    forwardAnimationView.animationImages = images;
    forwardAnimationView.animationDuration = 0.5;
    forwardAnimationView.animationRepeatCount = 0;
    return forwardAnimationView;
}

- (void)handleDoubleTap:(UITapGestureRecognizer *)gesture {
    if (gesture.state == UIGestureRecognizerStateRecognized) {
        CGPoint location = [gesture locationInView:self.view];
        CGFloat width = self.view.bounds.size.width;

        if (location.x < width / 3) {
            self.player.currentPlaybackTime -= 5;
            [self showTips:@"backward\n5s"
                  position:RedTipsPositionLeft
                  autoHide:YES
               hasFeedback:NO
             animationView:nil];
        } else if (location.x > 2 * width / 3) {
            self.player.currentPlaybackTime += 5;
            [self showTips:@"Forward\n5s"
                  position:RedTipsPositionRight
                  autoHide:YES
               hasFeedback:NO
             animationView:nil];
        } else {
            if (self.player.isPlaying) {
                [self.player pause];
                [self showTips:@"PAUSE" 
                      position:RedTipsPositionCenter
                      autoHide:YES
                   hasFeedback:NO
                 animationView:nil];
            } else {
                [self.player play];
                [self showTips:@"PLAY" 
                      position:RedTipsPositionCenter
                      autoHide:YES
                   hasFeedback:NO
                 animationView:nil];
            }
        }
    }
}

- (void)showTips:(NSString *)tips 
        position:(RedTipsPosition)position
        autoHide:(BOOL)autoHide 
     hasFeedback:(BOOL)hasFeedback
   animationView:(UIImageView *)animationView {
    if (hasFeedback) {
        UIImpactFeedbackGenerator *feedbackGenerator = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
        [feedbackGenerator prepare];
        [feedbackGenerator impactOccurred];
    }
    [self hideTips];
    CGRect overlayFrame = CGRectZero;
    BOOL overlayArc = NO;
    CGFloat screenWidth = self.view.bounds.size.width;
    CGFloat screenHeight = self.view.bounds.size.height;
    CGFloat overlayWidth = screenWidth / 3;
    switch (position) {
        case RedTipsPositionCenter:
            overlayFrame = CGRectMake((screenWidth - overlayWidth)/2.0, 0, overlayWidth, screenHeight);
            break;
        case RedTipsPositionLeft:
            overlayFrame = CGRectMake(-overlayWidth, - screenHeight, overlayWidth * 2, screenHeight * 3);
            overlayArc = YES;
            break;
        case RedTipsPositionRight:
            overlayFrame = CGRectMake(screenWidth - overlayWidth, - screenHeight, overlayWidth * 2, screenHeight * 3);
            overlayArc = YES;
            break;
        default:
            break;
    }
    self.overlayView = [self createOverlayViewWithFrame:overlayFrame isArc:overlayArc];
    [self.view addSubview:self.overlayView];
    self.tipsView = [[UILabel alloc] initWithFrame:CGRectZero];
    self.tipsView.text = tips;
    self.tipsView.textAlignment = NSTextAlignmentCenter;
    self.tipsView.textColor = UIColor.whiteColor;
    self.tipsView.font = [UIFont boldSystemFontOfSize:20];
    self.tipsView.numberOfLines = 0;
    [self.view addSubview:self.tipsView];
    [self.tipsView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.centerY.equalTo(self.view.mas_bottom).multipliedBy(1.0/3.0);
        make.centerX.mas_equalTo(self.view).offset(position == RedTipsPositionCenter || position == RedTipsPositionNone ? 0 : (position == RedTipsPositionLeft ? (-screenWidth/3) : (screenWidth/3)));
        make.width.mas_equalTo(100);
        make.height.mas_equalTo(50);
    }];
    
    if (animationView) {
        self.animationView = animationView;
        [self.view addSubview:self.animationView];
        [self.animationView startAnimating];
        [self.animationView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerX.mas_equalTo(self.tipsView);
            make.top.mas_equalTo(self.tipsView.mas_bottom);
            make.width.mas_equalTo(102);
            make.height.mas_equalTo(32);
        }];
    }
    if (autoHide) {
        [UIView animateWithDuration:0.8 animations:^{
            self.tipsView.alpha = 0;
            self.overlayView.alpha = 0;
            self.animationView.alpha = 0;
        } completion:^(BOOL finished) {
            if (finished) {
                [self hideTips];
            }
        }];
    }
}
- (UIView *)createOverlayViewWithFrame:(CGRect)frame isArc:(BOOL)isArc {
    UIView *overlayView = [[UIView alloc] initWithFrame:frame];
    overlayView.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.2];
    overlayView.userInteractionEnabled = NO;
    if (isArc) {
        CAShapeLayer *maskLayer = [CAShapeLayer layer];
        maskLayer.path = [UIBezierPath bezierPathWithOvalInRect:overlayView.bounds].CGPath;
        overlayView.layer.mask = maskLayer;
    }

    return overlayView;
}

- (void)hideTips {
    [self.overlayView removeFromSuperview];
    self.overlayView = nil;
    [self.tipsView removeFromSuperview];
    self.tipsView = nil;
    [self.animationView stopAnimating];
    [self.animationView removeFromSuperview];
    self.animationView = nil;
}

// MARK: RedMediaControlDelegate
- (void)onClickPlayPause:(id)sender {
    BOOL isPlaying = self.player.isPlaying;
    if (isPlaying) {
        [self.player pause];
        [self pauseAction];
    } else {
        [self.player play];
        [self playAction];
    }
}

- (void)onClickMute:(id)sender {
    if (self.isMute) {
        [self.player setMute:NO];
        [self.mediaControl updateMuteBtn:NO];
    } else {
        [self.player setMute:YES];
        [self.mediaControl updateMuteBtn:YES];
    }
    self.isMute = !self.isMute;
}

- (NSTimeInterval)duration {
    return self.player.duration;
}

- (NSTimeInterval)currentPlaybackTime {
    return self.player.currentPlaybackTime;
}

- (void)speedChange:(CGFloat)speed {
    self.player.playbackRate = speed;
}

- (void)didSliderTouchUpInside {
    self.player.currentPlaybackTime = self.mediaControl.progressValue;
}

- (void)updatePlayInfo {
    NSString * debugInfo = self.player.getVideoDebugInfo.description;
    debugInfo = [debugInfo stringByAppendingFormat:@"\nurl = %@", [self.player getPlayUrl]];
    self.hudTextView.text = debugInfo;
}

- (void)slideCancelMediaControlHide {
    [self cancelDelayedHide];
}

- (void)slideContinueMediaControlHide {
    [self performSelector:@selector(hideMediaControl) withObject:nil afterDelay:5];
}

- (void)mediaControlLiveRefresh {
    [self shutdown];
    [self setupPlayer];
}
@end
    
