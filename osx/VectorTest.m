#import "VectorTest.h"

#include "../test/VectorTestImpl.h"

@implementation VectorTest

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (id)init {
    self = [super init];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    VectorTestImpl_Draw();
    [self setNeedsDisplay:YES];
}

- (void)prepareOpenGL {
    // Synchronize buffer swaps with vertical refresh rate
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

    VectorTestImpl_Init();
}

- (void)reshape {
    NSRect rect = [self bounds];
    VectorTestImpl_Resize(rect.size.width, rect.size.height);
}

@end
