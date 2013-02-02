//
//  MainViewController.m
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#import "MainViewController.h"
#include "vector_font_simplex.h"
#include "../test/VectorTestImpl.h"

@implementation MainViewController

- (void)loadView
{
    [super loadView];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = [[GLKView alloc] init];
    self.view = view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    self.preferredFramesPerSecond = 30;

    [EAGLContext setCurrentContext:self.context];

    CGRect frame = [[UIScreen mainScreen] applicationFrame];
    
    //check if you should rotate the view, e.g. change width and height of the frame
    BOOL rotate = NO;
    if ( UIInterfaceOrientationIsLandscape( [UIApplication sharedApplication].statusBarOrientation ) ) {
        if (frame.size.width < frame.size.height) {
            rotate = YES;
        }
    }
    
    if ( UIInterfaceOrientationIsPortrait( [UIApplication sharedApplication].statusBarOrientation ) ) {
        if (frame.size.width > frame.size.height) {
            rotate = YES;
        }
    }
    
    if (rotate) {
        CGFloat tmp = frame.size.height;
        frame.size.height = frame.size.width;
        frame.size.width = tmp;
    }

    CGFloat scale = [[UIScreen mainScreen] scale];
    VectorTestImpl_Init(frame.size.width * scale,
                        frame.size.height * scale);

}

- (void)update {
    VectorTestImpl_Draw();
}

- (void)dealloc
{
    [EAGLContext setCurrentContext:self.context];
    
    VectorTestImpl_Destroy();
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

@end
