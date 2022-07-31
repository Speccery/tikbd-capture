//
//  macos_paste.m
//  tikbd-capture
//
//  Created by Erik Piehl on 31.7.2022.
//

#import <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include "paste.h"

int get_paste_data(char *buf, size_t len) {
  NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
  NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
  if(string == nil) {
    // Couldn't get a string.
    return -2;
  }
  int reqsize = (int)[string lengthOfBytesUsingEncoding:NSUTF8StringEncoding]+1;
  if (reqsize > len) {
    return -1;      // Buffer is too small
  }
  memcpy(buf, [string UTF8String], reqsize);
  return reqsize;
}

