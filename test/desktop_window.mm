//
//  desktop_window.mm
//  test
//
//  Created by Albert Yang on 2026-04-08.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#define GLFW_EXPOSE_NATIVE_COCOA   // ← add this before the native include
#import <GLFW/glfw3.h>            // ← glfw3.h must come before glfw3native.h
#import <GLFW/glfw3native.h>

#include "desktop_window.hpp"


void makeDesktopWindow(GLFWwindow* glfwWindow) {
    NSWindow* win = glfwGetCocoaWindow(glfwWindow);

    // kCGDesktopWindowLevel = 0 (wallpaper layer)
    // +1 puts us above wallpaper, below everything else
    [win setLevel: kCGDesktopWindowLevel + 1];

    // remove titlebar so it's edge-to-edge
    [win setStyleMask: NSWindowStyleMaskBorderless];

    // cover the full display using the screen's own frame
    NSScreen* screen = [NSScreen mainScreen];
    [win setFrame: screen.frame display: YES];

    // don't block mouse events for other apps
    [win setIgnoresMouseEvents: YES];
    
    // visible on every Space (most important flag)
    [win setCollectionBehavior:
        NSWindowCollectionBehaviorCanJoinAllSpaces
        | NSWindowCollectionBehaviorStationary
        | NSWindowCollectionBehaviorIgnoresCycle];

    // don't hide when another app becomes active
    [win setCanHide: NO];
    [win setHidesOnDeactivate: NO];
}

// MenuHandler gets a method to show/hide the editor
@interface MenuHandler : NSObject
@property (nonatomic, copy) void (^callback)(int);
@property (nonatomic, strong) NSPanel* editorPanel;
- (void)itemClicked:(NSMenuItem*)item;
- (void)showEditor:(id)sender;
@end


@implementation MenuHandler
- (void)itemClicked:(NSMenuItem*)item {
    if (self.callback) self.callback((int)item.tag);
}
- (void)showEditor:(id)sender {
    if (!self.editorPanel) {
        self.editorPanel = [self buildEditorPanel];
    }
    [self.editorPanel makeKeyAndOrderFront:nil];
}
- (NSPanel*)buildEditorPanel {
    NSPanel* panel = [[NSPanel alloc]
        initWithContentRect: NSMakeRect(200, 200, 720, 480)
                  styleMask: (NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskResizable |
                              NSWindowStyleMaskUtilityWindow)
                    backing: NSBackingStoreBuffered
                      defer: NO];
    panel.title = @"L-System Editor";
    panel.floatingPanel = YES;  // stays above other windows
    panel.hidesOnDeactivate = NO;

    // tab bar across the top
    NSSegmentedControl* tabs = [[NSSegmentedControl alloc]
        initWithFrame: NSMakeRect(8, 440, 500, 28)];
    tabs.segmentCount = 1;
    [tabs setLabel:@"untitled" forSegment:0];
    tabs.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
    [panel.contentView addSubview:tabs];

    // + button to add a new tab
    NSButton* addTab = [[NSButton alloc]
        initWithFrame: NSMakeRect(510, 440, 28, 28)];
    addTab.title = @"+";
    addTab.bezelStyle = NSBezelStyleRounded;
    [panel.contentView addSubview:addTab];

    // monospaced text editor
    NSScrollView* scroll = [[NSScrollView alloc]
        initWithFrame: NSMakeRect(0, 0, 720, 434)];
    scroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.hasVerticalScroller = YES;

    NSTextView* tv = [[NSTextView alloc]
        initWithFrame: scroll.bounds];
    tv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    tv.font = [NSFont fontWithName:@"Menlo" size:13];
    tv.backgroundColor = [NSColor colorWithWhite:0.1 alpha:1.0];
    tv.textColor = [NSColor colorWithRed:1 green:1 blue:1 alpha:1];
    tv.automaticQuoteSubstitutionEnabled = NO;
    tv.automaticSpellingCorrectionEnabled = NO;
    tv.richText = NO;  // plain text only

    scroll.documentView = tv;
    [panel.contentView addSubview:scroll];

    return panel;
}
@end

static NSStatusItem* gStatusItem = nil;
static NSMenu*       gMenu       = nil;
static MenuHandler*  gHandler    = nil;

void createStatusBarItem(std::function<void(int)> onChange) {
    gHandler = [MenuHandler new];
    gHandler.callback = ^(int idx) { onChange(idx); };

    gStatusItem = [[NSStatusBar systemStatusBar]
        statusItemWithLength: NSVariableStatusItemLength];
    gStatusItem.button.title = @"🌿 plantground";

    gMenu = [[NSMenu alloc] init];

    NSMenuItem* editorItem = [[NSMenuItem alloc]
        initWithTitle: @"Open Editor..."
               action: @selector(showEditor:)
        keyEquivalent: @"e"];
    editorItem.target = gHandler;
    [gMenu addItem: editorItem];

    [gMenu addItem: [NSMenuItem separatorItem]];

    // program switcher items
    NSArray* names = @[@"Fractal Tree", @"Koch Curve", @"Dragon Curve"];
    for (int i = 0; i < (int)names.count; i++) {
        NSMenuItem* item = [[NSMenuItem alloc]
            initWithTitle: names[i]
                   action: @selector(itemClicked:)
            keyEquivalent: @""];
        item.tag = i;
        item.target = gHandler;
        [gMenu addItem: item];
    }

    [gMenu addItem: [NSMenuItem separatorItem]];

    NSMenuItem* q = [[NSMenuItem alloc]
        initWithTitle: @"Quit"
               action: @selector(terminate:)
        keyEquivalent: @"q"];
    q.target = NSApp;
    [gMenu addItem: q];

    gStatusItem.menu = gMenu;
}
