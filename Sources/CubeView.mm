#import "CubeView.h"

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#import "VulkanRenderer.h"

@implementation CubeView {
    VulkanRenderer _renderer;
    CADisplayLink* _displayLink;
    BOOL _initialized;
    BOOL _failed;
    CGSize _drawableSize;
}

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor blackColor];
        self.opaque = YES;

        CAMetalLayer* metalLayer = (CAMetalLayer*)self.layer;
        metalLayer.device = MTLCreateSystemDefaultDevice();
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = YES;
        metalLayer.opaque = YES;
        metalLayer.contentsScale = [UIScreen mainScreen].scale;
    }
    return self;
}

- (void)layoutSubviews {
    [super layoutSubviews];

    CAMetalLayer* metalLayer = (CAMetalLayer*)self.layer;
    CGFloat scale = metalLayer.contentsScale > 0 ? metalLayer.contentsScale
                                                 : [UIScreen mainScreen].scale;
    CGSize size = CGSizeMake(self.bounds.size.width * scale,
                             self.bounds.size.height * scale);
    if (size.width < 1.0 || size.height < 1.0) {
        return;
    }

    metalLayer.drawableSize = size;
    _drawableSize = size;

    if (_initialized) {
        _renderer.resize((uint32_t)size.width, (uint32_t)size.height);
    }
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window) {
        [self startRendering];
    } else {
        [self stopRendering];
    }
}

- (void)startRendering {
    if (_failed) {
        return;
    }

    if (!_initialized) {
        NSString* vert = [[NSBundle mainBundle] pathForResource:@"cube.vert" ofType:@"spv"];
        NSString* frag = [[NSBundle mainBundle] pathForResource:@"cube.frag" ofType:@"spv"];
        if (vert == nil || frag == nil) {
            NSLog(@"VulkanCube: shaders not found in bundle");
            _failed = YES;
            return;
        }

        CGSize size = _drawableSize;
        if (size.width < 1.0 || size.height < 1.0) {
            size = CGSizeMake(1.0, 1.0);
        }

        if (!_renderer.init((__bridge void*)self.layer,
                            (uint32_t)size.width,
                            (uint32_t)size.height,
                            vert.fileSystemRepresentation,
                            frag.fileSystemRepresentation)) {
            NSLog(@"VulkanCube: renderer init failed");
            _failed = YES;
            return;
        }
        _initialized = YES;
    }

    if (_displayLink == nil) {
        _displayLink = [CADisplayLink displayLinkWithTarget:self
                                                   selector:@selector(renderFrame)];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                           forMode:NSRunLoopCommonModes];
    }
}

- (void)stopRendering {
    [_displayLink invalidate];
    _displayLink = nil;
}

- (void)renderFrame {
    _renderer.draw();
}

- (void)dealloc {
    [self stopRendering];
    _renderer.shutdown();
}

@end
