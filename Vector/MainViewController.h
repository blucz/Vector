//
//  MainViewController.h
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

#import "vector_display.h"

@interface MainViewController : GLKViewController {
    vector_display_t *display;

    GLuint depthBuff;
    int frame;

    float pos;
    float angle;

    float pos_s[25];
    float angle_s[25];
}

@property (strong, nonatomic) EAGLContext *context;

- (void)loadView;

@end
