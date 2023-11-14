//
//  RedLoadingView.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/4.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//
#import "RedLoadingView.h"
#import "net/if.h"
#import "ifaddrs.h"
@interface RedLoadingView ()<NSURLSessionDelegate>
@property (nonatomic, strong) UIImageView *loadingImageView;
@property (nonatomic, strong) UILabel *networkSpeedLabel;
@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, strong) NSURLSession *session;
@property (nonatomic, strong) NSURLSessionDownloadTask *downloadTask;
@property (nonatomic, assign) long long lastBytes;
@property (nonatomic, assign) NSTimeInterval lastTime;
@end

@implementation RedLoadingView

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self showLoadingView];
    }
    return self;
}

- (void)showLoadingView {
    // 添加loading图
    self.loadingImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
    self.loadingImageView.center = self.center;
    self.loadingImageView.image = [UIImage imageNamed:@"loading"];
    [self addSubview:self.loadingImageView];
    
    // 添加网速label
    self.networkSpeedLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, CGRectGetMaxY(self.loadingImageView.frame) + 10, CGRectGetWidth(self.frame), 20)];
    self.networkSpeedLabel.textAlignment = NSTextAlignmentCenter;
    self.networkSpeedLabel.textColor = [UIColor whiteColor];
    self.networkSpeedLabel.font = [UIFont systemFontOfSize:12];
    [self addSubview:self.networkSpeedLabel];
}

- (void)updateSpeedLabel {
    long long bytes = self.downloadTask.countOfBytesReceived - self.lastBytes;
    NSTimeInterval time = [NSDate timeIntervalSinceReferenceDate] - self.lastTime;
    self.lastBytes = self.downloadTask.countOfBytesReceived;
    self.lastTime = [NSDate timeIntervalSinceReferenceDate];
    double speed = bytes / time;
    
    self.networkSpeedLabel.text = [NSString stringWithFormat:@"%@/s", [self formatNetWorkSpeed:speed]];
}

- (NSString *)formatNetWorkSpeed:(double)speed {
    if (speed < 1024) {
        return [NSString stringWithFormat:@"%.0fB", speed];
    } else if (speed >= 1024 && speed < 1024 * 1024) {
        return [NSString stringWithFormat:@"%.1fKB", speed / 1024.0];
    } else if (speed >= 1024 * 1024 && speed < 1024 * 1024 * 1024) {
        return [NSString stringWithFormat:@"%.1fMB", speed / (1024.0 * 1024.0)];
    } else {
        return [NSString stringWithFormat:@"%.1fGB", speed / (1024.0 * 1024.0 * 1024.0)];
    }
}

- (void)startLoading {
    CABasicAnimation *rotationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    rotationAnimation.fromValue = @(0);
    rotationAnimation.toValue = [NSNumber numberWithFloat:M_PI * 2.0];
    rotationAnimation.duration = 1;
    rotationAnimation.cumulative = YES;
    rotationAnimation.repeatCount = HUGE_VALF;
    rotationAnimation.removedOnCompletion = NO;
    [self.loadingImageView.layer addAnimation:rotationAnimation forKey:@"rotationAnimation"];
    [self startMonitoring];
}

- (void)startMonitoring {
    if (!self.session) {
        NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
        self.session = [NSURLSession sessionWithConfiguration:config delegate:self delegateQueue:nil];
    }
    if (self.downloadTask) {
        [self.downloadTask cancel];
        self.downloadTask = nil;
    }
    NSURL *url = [NSURL URLWithString:@"http://speedtest.tele2.net/100MB.zip"];
    self.downloadTask = [self.session downloadTaskWithURL:url];

    [self.downloadTask resume];
    
    if (!self.timer) {
        self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(updateSpeedLabel) userInfo:nil repeats:YES];
    }
}

- (void)stopLoading {
    [self.loadingImageView stopAnimating];
    [self.loadingImageView removeFromSuperview];
    self.loadingImageView = nil;
    [self.timer invalidate];
    self.timer = nil;
    
    [self.downloadTask cancel];
    self.downloadTask = nil;
}

#pragma mark - NSURLSessionDelegate

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
    NSLog(@"Download finished: %@", location);
    [[NSFileManager defaultManager] removeItemAtURL:location error:nil];
    [self startMonitoring];
}
@end
