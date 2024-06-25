//
//  XYBaseMediaPlayback.m
//  XYAbstractPlayer
//
//  Created by 张继东 on 2023/11/9.
//  Copyright © 2023 XingIn. All rights reserved.
//

#import "RedMediaPlayback.h"

NSString *const RedMediaPlaybackIsPreparedToPlayDidChangeNotification =
    @"RedMediaPlaybackIsPreparedToPlayDidChangeNotification";

NSString *const RedPlayerPlaybackDidFinishNotification =
    @"RedPlayerPlaybackDidFinishNotification";
NSString *const RedPlayerPlaybackDidFinishReasonUserInfoKey =
    @"RedPlayerPlaybackDidFinishReasonUserInfoKey";
NSString *const RedPlayerPlaybackDidFinishErrorUserInfoKey =
    @"RedPlayerPlaybackDidFinishErrorUserInfoKey";
NSString *const RedPlayerPlaybackDidFinishDetailedErrorUserInfoKey =
    @"RedPlayerPlaybackDidFinishDetailedErrorUserInfoKey";

NSString *const RedPlayerPlaybackStateDidChangeNotification =
    @"RedPlayerPlaybackStateDidChangeNotification";
NSString *const RedPlayerLoadStateDidChangeNotification =
    @"RedPlayerLoadStateDidChangeNotification";

NSString *const RedPlayerNaturalSizeAvailableNotification =
    @"RedPlayerNaturalSizeAvailableNotification";

NSString *const RedPlayerVideoDecoderOpenNotification =
    @"RedPlayerVideoDecoderOpenNotification";

NSString *const RedPlayerFirstVideoFrameRenderedNotification =
    @"RedPlayerFirstVideoFrameRenderedNotification";
NSString *const RedPlayerFirstVideoFrameRenderedNotificationUserInfo =
    @"RedPlayerFirstVideoFrameRenderNotificationUserInfo";
NSString *const RedPlayerFirstAudioFrameRenderedNotification =
    @"RedPlayerFirstAudioFrameRenderedNotification";
NSString *const RedPlayerFirstAudioFrameDecodedNotification =
    @"RedPlayerFirstAudioFrameDecodedNotification";
NSString *const RedPlayerFirstVideoFrameDecodedNotification =
    @"RedPlayerFirstVideoFrameDecodedNotification";
NSString *const RedPlayerOpenInputNotification =
    @"RedPlayerOpenInputNotification";
NSString *const RedPlayerFindStreamInfoNotification =
    @"RedPlayerFindStreamInfoNotification";
NSString *const RedPlayerComponentOpenNotification =
    @"RedPlayerComponentOpenNotification";

NSString *const RedPlayerAccurateSeekCompleteNotification =
    @"RedPlayerAccurateSeekCompleteNotification";

NSString *const RedPlayerDidSeekCompleteNotification =
    @"RedPlayerDidSeekCompleteNotification";
NSString *const RedPlayerDidSeekCompleteTargetKey =
    @"RedPlayerDidSeekCompleteTargetKey";
NSString *const RedPlayerDidSeekCompleteErrorKey =
    @"RedPlayerDidSeekCompleteErrorKey";

NSString *const RedPlayerDidAccurateSeekCompleteCurPos =
    @"RedPlayerDidAccurateSeekCompleteCurPos";

NSString *const RedPlayerSeekAudioStartNotification =
    @"RedPlayerSeekAudioStartNotification";
NSString *const RedPlayerSeekVideoStartNotification =
    @"RedPlayerSeekVideoStartNotification";

NSString *const RedPlayerVideoFirstPacketInDecoderNotification =
    @"RedPlayerVideoFirstPacketInDecoderNotification";
NSString *const RedPlayerVideoStartOnPlayingNotification =
    @"RedPlayerVideoStartOnPlayingNotification";

NSString *const RedPlayerCacheDidFinishNotification =
    @"RedPlayerCacheDidFinishNotification";
NSString *const RedPlayerCacheURLUserInfoKey = @"RedPlayerCacheURLUserInfoKey";

NSString *const RedPlayerUrlChangeMsgNotification =
    @"RedPlayerUrlChangeMsgNotification";
NSString *const RedPlayerUrlChangeCurUrlKey = @"RedPlayerUrlChangeCurUrlKey";
NSString *const RedPlayerUrlChangeCurUrlHttpCodeKey =
    @"RedPlayerUrlChangeCurUrlHttpCodeKey";
NSString *const RedPlayerUrlChangeNextUrlKey = @"RedPlayerUrlChangeNextUrlKey";

NSString *const RedPlayerMessageTimeUserInfoKey =
    @"RedPlayerMessageTimeUserInfoKey";
