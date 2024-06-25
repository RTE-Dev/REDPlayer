//
//  NSString+REDMedia.m
//  RedPlayer
//
//  Created by 王统华 on 2023/5/31.
//

#import "NSString+REDMedia.h"

@implementation NSString (REDMedia)
+ (NSString *)red_stringBeEmptyIfNil:(NSString *)src {
  if (src == nil)
    return @"";

  return src;
}

@end
