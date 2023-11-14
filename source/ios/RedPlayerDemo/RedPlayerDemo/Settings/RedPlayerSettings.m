//
//  RedPlayerSettings.m
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/20.
//  Copyright © 2024 Xiaohongshu. All rights reserved.


#import "RedPlayerSettings.h"
#import <objc/runtime.h>

@interface RedPlayerSettings ()
@end

@implementation RedPlayerSettings

+ (instancetype)sharedInstance {
    static RedPlayerSettings *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
        [sharedInstance loadSettings];
    });
    return sharedInstance;
}

- (void)loadSettings {
    unsigned int count;
    objc_property_t *properties = class_copyPropertyList([self class], &count);
    for (int i = 0; i < count; i++) {
        objc_property_t property = properties[i];
        const char *propertyName = property_getName(property);
        NSString *key = [NSString stringWithUTF8String:propertyName];
        id value = [[NSUserDefaults standardUserDefaults] objectForKey:key];
        if (value) {
            [self setValue:value forKey:key];
        }
        [self setObserverForProperty:key];
    }
    free(properties);
}

- (void)setObserverForProperty:(NSString *)property {
    [self addObserver:self forKeyPath:property options:NSKeyValueObservingOptionNew context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if (object == self) {
        id newValue = change[NSKeyValueChangeNewKey];
        if (newValue) {
            [[NSUserDefaults standardUserDefaults] setObject:newValue forKey:keyPath];
            [[NSUserDefaults standardUserDefaults] synchronize];
        }
    }
}

@end

