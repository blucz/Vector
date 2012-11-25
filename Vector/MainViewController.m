//
//  MainViewController.m
//  Vector
//
//  Created by Brian Luczkiewicz on 11/22/12.
//  Copyright (c) 2012 Brian Luczkiewicz. All rights reserved.
//

#import "MainViewController.h"

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
    
    int rc;
    rc = vector_display_new(&display, 2048, 1536);
    if (rc != 0) {
        NSLog(@"Failed to create vector display: rc=%d", rc);
        exit(EXIT_FAILURE);
    }
    
    rc = vector_display_setup(display);
    if (rc != 0) {
        NSLog(@"Failed to setup vector display: rc=%d", rc);
        exit(EXIT_FAILURE);
    }
}

- (void)update {
    pos += 40.0f;
    angle += 8.0f / 360.0f * 2.0f * M_PI;
    if (pos > 2048) pos -= 2048;
    vector_display_clear(display);
    vector_display_set_color(display, 0.9f, 0.9f, 0.95f);

    // draw spokes
    vector_display_draw(display, 
                        pos + 200.0f * sin(angle), 
                        400.0f - 200.0f * cos(angle), 
                        pos - 200.0f * sin(angle), 
                        400.0f + 200.0f * cos(angle));
    vector_display_draw(display, 
                        pos + 200.0f * sin(angle + M_PI / 4.0f), 
                        400.0f - 200.0f * cos(angle + M_PI / 4.0f), 
                        pos - 200.0f * sin(angle + M_PI / 4.0f), 
                        400.0f + 200.0f * cos(angle + M_PI / 4.0f));
    vector_display_draw(display, 
                        pos + 200.0f * sin(angle + M_PI / 2.0f), 
                        400.0f - 200.0f * cos(angle + M_PI / 2.0f), 
                        pos - 200.0f * sin(angle + M_PI / 2.0f), 
                        400.0f + 200.0f * cos(angle + M_PI / 2.0f));
    vector_display_draw(display, 
                        pos + 200.0f * sin(angle + 3.0f * M_PI / 4.0f), 
                        400.0f - 200.0f * cos(angle + 3.0f * M_PI / 4.0f), 
                        pos - 200.0f * sin(angle + 3.0f * M_PI / 4.0f), 
                        400.0f + 200.0f * cos(angle + 3.0f * M_PI / 4.0f));

    float edgeangle;
    for (edgeangle = 0; edgeangle < 2 * M_PI; edgeangle += M_PI/4.0f) {
        vector_display_draw(display,
                            pos + 200.0f * sin(angle + edgeangle),
                            400.0f - 200.0f * cos(angle + edgeangle),
                            pos + 200.0f * sin(angle + edgeangle + M_PI / 4.0f),
                            400.0f - 200.0f * cos(angle + edgeangle + M_PI / 4.0f));
    }

    int rc;
    rc = vector_display_update(display);
    if (rc != 0) {
        NSLog(@"Failed to update vector display: rc=%d", rc);
        exit(EXIT_FAILURE);
    }
    frame++;
}

- (void)dealloc
{
    [EAGLContext setCurrentContext:self.context];
    
    int rc;
    rc = vector_display_teardown(display);
    if (rc != 0) {
        NSLog(@"Failed to tear down vector display: rc=%d", rc);
        exit(EXIT_FAILURE);
    }
    vector_display_delete(display);
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

@end
