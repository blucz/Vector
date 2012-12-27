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

static void draw_wheel(vector_display_t *display, float angle, float x, float y, float radius) {
    float spokeradius = radius - 4.0f;
    // draw spokes
    vector_display_draw(display, 
                        x + spokeradius * sin(angle), 
                        y - spokeradius * cos(angle), 
                        x - spokeradius * sin(angle), 
                        y + spokeradius * cos(angle));
    vector_display_draw(display, 
                        x + spokeradius * sin(angle + M_PI / 4.0f), 
                        y - spokeradius * cos(angle + M_PI / 4.0f), 
                        x - spokeradius * sin(angle + M_PI / 4.0f), 
                        y + spokeradius * cos(angle + M_PI / 4.0f));
    vector_display_draw(display, 
                        x + spokeradius * sin(angle + M_PI / 2.0f), 
                        y - spokeradius * cos(angle + M_PI / 2.0f), 
                        x - spokeradius * sin(angle + M_PI / 2.0f), 
                        y + spokeradius * cos(angle + M_PI / 2.0f));
    vector_display_draw(display, 
                        x + spokeradius * sin(angle + 3.0f * M_PI / 4.0f), 
                        y - spokeradius * cos(angle + 3.0f * M_PI / 4.0f), 
                        x - spokeradius * sin(angle + 3.0f * M_PI / 4.0f), 
                        y + spokeradius * cos(angle + 3.0f * M_PI / 4.0f));

    float edgeangle;
    float angadjust = 2.0f / 360.0f * 2 * M_PI;
    for (edgeangle = 0; edgeangle < 2 * M_PI - 0.001; edgeangle += M_PI/4.0f) {
        vector_display_draw(display,
                            x + radius * sin(angle + edgeangle + angadjust),
                            y - radius * cos(angle + edgeangle + angadjust),
                            x + radius * sin(angle + edgeangle + M_PI / 4.0f - angadjust),
                            y - radius * cos(angle + edgeangle + M_PI / 4.0f - angadjust));
    }
}

static void draw_circle(vector_display_t *display, float angle, float x, float y, float radius) {
    float edgeangle;
    float angadjust = 0.0f;

    float step = M_PI / 8.0f;

    for (edgeangle = 0; edgeangle < 2 * M_PI - 0.001; edgeangle += step) {
        vector_display_draw(display,
                            x + radius * sin(angle + edgeangle + angadjust),
                            y - radius * cos(angle + edgeangle + angadjust),
                            x + radius * sin(angle + edgeangle + step - angadjust),
                            y - radius * cos(angle + edgeangle + step - angadjust));
    }
}

static void draw_box(vector_display_t *display, float x, float y, float w, float h) {
    float edgeangle;
    float step = M_PI / 8.0f;


    vector_display_draw(display, x, y, x + w, y);
    vector_display_draw(display, x + w, y, x + w, y + h);
    vector_display_draw(display, x + w, y + h, x, y + h);
    vector_display_draw(display, x, y + h, x, y);
}

- (void)update {
    int i,j;

    vector_display_clear(display);

    for (i = 0; i < 15; i++) {
        pos_s[i] += i/2.0f;
        if (pos_s[i] > 2048) pos_s[i] -= 2048;
        angle_s[i] += (15.0f/i) / 360.0f * 2.0f * M_PI;

        if (i % 3 == 0) {
            vector_display_set_color(display, 0.6f, 0.6f, 1.0f);  
            draw_circle(display, angle_s[i], pos_s[i], i * 90, i * 10);
        } else if (i % 3 == 1) {
            vector_display_set_color(display, 1.0f, 0.6f, 1.0f);  
            draw_wheel(display, angle_s[i], pos_s[i], i * 90, i * 10);
        } else {
            vector_display_set_color(display, 0.6f, 1.0f, 0.6f);  
            draw_box(display, pos_s[i] - i * 5, i * 90 - i * 5, i * 10, i * 10);
        }
    }

    //
    // test pattern for lines
    //
    vector_display_set_color( display, 1.0f, 0.6f, 0.6f);  
    for (i = 0; i < 10; i++) {
        for (j = 0; j < i; j++) {
            vector_display_draw(display, 100, 100 + 100 * i, 400, 100 + 100 * i);
            vector_display_draw(display, 450, 100 + 100 * i, 450, 100 + 100 * i);
            vector_display_draw(display, 500, 100 + 100 * i, 500, 100 + 100 * i);
            vector_display_draw(display, 550, 100 + 100 * i, 550, 100 + 100 * i);
        }
    }

    /*
    pos += 2.0f;
    angle += 0.8f / 360.0f * 2.0f * M_PI;
    if (pos > 2048) pos -= 2048;
    draw_wheel(display, angle, pos, 800.0f, 400.0f);
    */

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
