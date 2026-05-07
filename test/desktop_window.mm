// desktop_window.mm

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#import <GLFW/glfw3.h>
#import <GLFW/glfw3native.h>
#include "desktop_window.hpp"

static NSString* default_code = (
    @"global float angle   = 28.0;\n"
    @"global float pitch   = 22.0;\n"
    @"global float r       = 0.70;\n"
    @"global float leaf_r  = 0.72;\n"
    @"global float stop    = 0.08;\n"
    @"global float scale   = 18.0;\n"
    @"\n"
    @"symbol stem(float x);\n"
    @"symbol side(float x);\n"
    @"symbol spray(float x);\n"
    @"symbol leaf(float x);\n"
    @"symbol flower(float x);\n"
    @"\n"
    @"rule: stem(float x) when x > stop * scale ->\n"
    @"    stem(x * r)\n"
    @"    [ side(x * 0.78) ]\n"
    @"    [ leaf(x * 0.65) ];\n"
    @"\n"
    @"rule: stem(float x) when rand() < 0.6 ->\n"
    @"    spray(x);\n"
    @"\n"
    @"rule: stem(float x) ->\n"
    @"    flower(x);\n"
    @"\n"
    @"rule: side(float x) when x > stop * scale * 0.7 ->\n"
    @"    side(x * r)\n"
    @"    [ leaf(x * 0.60) ];\n"
    @"\n"
    @"rule: side(float x) ->\n"
    @"    flower(x);\n"
    @"\n"
    @"rule: spray(float x) when x > stop * scale * 0.5 ->\n"
    @"    spray(x * r)\n"
    @"    [ flower(x * 0.85) ];\n"
    @"\n"
    @"rule: spray(float x) ->\n"
    @"    flower(x);\n"
    @"\n"
    @"rule: leaf(float x) when x > stop * scale * 0.4 ->\n"
    @"    leaf(x * leaf_r);\n"
    @"\n"
    @"rule: leaf(float x) -> leaf(x);\n"
    @"\n"
    @"rule: flower(float x) -> flower(x);\n"
    @"\n"
    @"draw_rule: stem {\n"
    @"    turn_yaw angle * 0.12;\n"
    @"    turn_pitch pitch * 0.08;\n"
    @"    line x;\n"
    @"};\n"
    @"\n"
    @"draw_rule: side {\n"
    @"    turn_yaw angle * 1.1;\n"
    @"    turn_pitch pitch * 1.2;\n"
    @"    turn_roll 35.0;\n"
    @"    line x;\n"
    @"};\n"
    @"\n"
    @"draw_rule: spray {\n"
    @"    turn_yaw angle * 0.7;\n"
    @"    turn_pitch pitch * 0.9;\n"
    @"    line x;\n"
    @"};\n"
    @"\n"
    @"draw_rule: leaf {\n"
    @"    turn_pitch pitch * 1.4;\n"
    @"    turn_roll 40.0;\n"
    @"    fill(0.14, 0.55, 0.18, 0.95) {\n"
    @"        line x * 0.50;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 70.0;\n"
    @"        line x * 0.30;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.50;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.20;\n"
    @"    };\n"
    @"};\n"
    @"\n"
    @"draw_rule: flower {\n"
    @"    turn_pitch pitch * 0.4;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.92, 0.32, 0.58, 1.0) {\n"
    @"        line x * 0.28;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 60.0;\n"
    @"        line x * 0.18;\n"
    @"        turn_yaw 55.0;\n"
    @"        line x * 0.22;\n"
    @"        turn_yaw 45.0;\n"
    @"        line x * 0.28;\n"
    @"    };\n"
    @"    turn_yaw 60.0;\n"
    @"    fill(0.98, 0.88, 0.20, 1.0) {\n"
    @"        line x * 0.10;\n"
    @"        turn_yaw 90.0;\n"
    @"        line x * 0.10;\n"
    @"        turn_yaw 90.0;\n"
    @"        line x * 0.10;\n"
    @"        turn_yaw 90.0;\n"
    @"        line x * 0.10;\n"
    @"    };\n"
    @"};\n"
    @"\n"
);


@interface SyntaxStorage : NSTextStorage
@property (nonatomic, strong) NSMutableAttributedString* backing;
@end

@implementation SyntaxStorage

static NSDictionary* sBase    = nil;
static NSDictionary* sKw      = nil;
static NSDictionary* sDrawCmd = nil;
static NSDictionary* sNum     = nil;
static NSDictionary* sComment = nil;

+ (void)initialize {
    if (self != [SyntaxStorage class]) return;
    NSFont* font = [NSFont fontWithName:@"Menlo" size:13]
                ?: [NSFont monospacedSystemFontOfSize:13 weight:NSFontWeightRegular];

    sBase    = @{ NSFontAttributeName: font,
                  NSForegroundColorAttributeName: [NSColor colorWithWhite:0.92 alpha:1] };
    sKw      = @{ NSFontAttributeName: font,
                  NSForegroundColorAttributeName:
                      [NSColor colorWithRed:0.62 green:0.90 blue:0.75 alpha:1] };
    sDrawCmd = @{ NSFontAttributeName: font,
                  NSForegroundColorAttributeName:
                      [NSColor colorWithRed:0.70 green:0.85 blue:0.55 alpha:1] };
    sNum     = @{ NSFontAttributeName: font,
                  NSForegroundColorAttributeName:
                      [NSColor colorWithRed:0.75 green:0.88 blue:0.65 alpha:1] };
    sComment = @{ NSFontAttributeName: font,
                  NSForegroundColorAttributeName: [NSColor colorWithWhite:0.42 alpha:1] };
}

- (instancetype)init {
    self = [super init];
    if (self) _backing = [NSMutableAttributedString new];
    return self;
}
- (NSString*)string { return _backing.string; }
- (NSDictionary*)attributesAtIndex:(NSUInteger)i effectiveRange:(NSRangePointer)r {
    return [_backing attributesAtIndex:i effectiveRange:r];
}
- (void)replaceCharactersInRange:(NSRange)r withString:(NSString*)s {
    [self beginEditing];
    [_backing replaceCharactersInRange:r withString:s];
    [self edited:NSTextStorageEditedCharacters range:r
  changeInLength:(NSInteger)s.length-(NSInteger)r.length];
    [self endEditing];
}
- (void)setAttributes:(NSDictionary*)attrs range:(NSRange)r {
    [self beginEditing];
    [_backing setAttributes:attrs range:r];
    [self edited:NSTextStorageEditedAttributes range:r changeInLength:0];
    [self endEditing];
}
- (void)processEditing { [self highlight]; [super processEditing]; }

- (void)highlight {
    NSString* str = _backing.string;
    NSUInteger len = str.length;
    if (len == 0) return;
    NSRange all = NSMakeRange(0, len);
    [_backing setAttributes:sBase range:all];

    void (^apply)(NSString*, NSDictionary*) = ^(NSString* pat, NSDictionary* attrs) {
        NSRegularExpression* re = [NSRegularExpression
            regularExpressionWithPattern:pat options:0 error:nil];
        [re enumerateMatchesInString:str options:0 range:all
            usingBlock:^(NSTextCheckingResult* m, NSMatchingFlags f, BOOL* stop) {
                [_backing setAttributes:attrs range:m.range];
            }];
    };

    apply(@"\\b(rule|draw_rule|when|symbol|global|float|return|if|else|while|null)\\b", sKw);
    apply(@"\\b(line|turn_yaw|turn_pitch|turn_roll|fill)\\b", sDrawCmd);
    apply(@"(?<![\\w])-?\\d+(\\.\\d+)?", sNum);
    apply(@"//[^\n]*", sComment);
}
@end

// ─── data model ───────────────────────────────────────────────────────────────

@interface Program : NSObject
@property (nonatomic, copy)   NSString*  name;
@property (nonatomic, copy)   NSString*  code;
@property (nonatomic, copy)   NSString*  axiom;
@property (nonatomic, assign) NSInteger  steps;
@end
@implementation Program @end

// ─── editor controller ────────────────────────────────────────────────────────

@interface EditorController : NSObject
    <NSTableViewDelegate, NSTableViewDataSource, NSTextViewDelegate>

@property (nonatomic, strong) NSPanel*                  panel;
@property (nonatomic, strong) NSTableView*              table;
@property (nonatomic, strong) NSTextView*               textView;
@property (nonatomic, strong) NSTextField*              axiomField;
@property (nonatomic, strong) NSTextField*              stepsField;
@property (nonatomic, strong) NSMutableArray<Program*>* programs;
@property (nonatomic, assign) NSInteger                 selectedIndex;
@property (nonatomic, copy)   void (^onRun)(NSString* code, NSString* axiom, NSInteger steps);

- (instancetype)initWithRunCallback:(void(^)(NSString*, NSString*, NSInteger))onRun;
- (void)show;
- (void)hide;
@end

@implementation EditorController

- (instancetype)initWithRunCallback:(void(^)(NSString*, NSString*, NSInteger))onRun {
    self = [super init];
    if (!self) return nil;
    self.onRun         = onRun;
    self.selectedIndex = -1;
    self.programs      = [NSMutableArray array];
    [self loadFromDisk];
    [self buildPanel];
    if (self.programs.count == 0) [self addProgram];
    return self;
}

// ─── persistence ──────────────────────────────────────────────────────────────

- (NSString*)savePath {
    NSString* dir = [NSHomeDirectory()
        stringByAppendingPathComponent:@"Library/Application Support/plantground"];
    [[NSFileManager defaultManager]
        createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    return [dir stringByAppendingPathComponent:@"programs.json"];
}

- (void)saveToDisk {
    NSMutableArray* arr = [NSMutableArray array];
    for (Program* p in self.programs)
        [arr addObject:@{@"name":p.name?:@"",@"code":p.code?:@"",
                         @"axiom":p.axiom?:@"",@"steps":@(p.steps)}];
    NSData* data = [NSJSONSerialization dataWithJSONObject:arr
                       options:NSJSONWritingPrettyPrinted error:nil];
    [data writeToFile:[self savePath] atomically:YES];
}

- (void)loadFromDisk {
    NSData* data = [NSData dataWithContentsOfFile:[self savePath]];
    if (!data) return;
    NSArray* arr = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![arr isKindOfClass:[NSArray class]]) return;
    for (NSDictionary* d in arr) {
        Program* p = [Program new];
        p.name  = d[@"name"]  ?: @"untitled";
        p.code  = d[@"code"]  ?: @"";
        p.axiom = d[@"axiom"] ?: @"stem(18)";
        p.steps = [d[@"steps"] integerValue] ?: 5;
        [self.programs addObject:p];
    }
}

// ─── panel construction ───────────────────────────────────────────────────────

- (void)buildPanel {
    self.panel = [[NSPanel alloc]
        initWithContentRect: NSMakeRect(200, 200, 860, 540)
                  styleMask: (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                              NSWindowStyleMaskResizable | NSWindowStyleMaskUtilityWindow)
                    backing: NSBackingStoreBuffered defer: NO];
    self.panel.title              = @"plantground — editor";
    self.panel.floatingPanel      = YES;
    self.panel.hidesOnDeactivate  = NO;
    self.panel.releasedWhenClosed = NO;
    // allow panel to become key so text editing and copy/paste work
    self.panel.becomesKeyOnlyIfNeeded = NO;

    NSView* root = self.panel.contentView;

    // toolbar
    NSView* toolbar = [[NSView alloc] initWithFrame:NSMakeRect(0, 506, 860, 34)];
    toolbar.autoresizingMask      = NSViewWidthSizable | NSViewMinYMargin;
    toolbar.wantsLayer            = YES;
    toolbar.layer.backgroundColor = [[NSColor colorWithWhite:0.13 alpha:1] CGColor];

    NSButton* addBtn = [self toolbarButton:@"+ New"  action:@selector(addProgram)    x:8];
    NSButton* delBtn = [self toolbarButton:@"Delete" action:@selector(deleteProgram) x:72];
    NSButton* runBtn = [self toolbarButton:@"▶ Run"  action:@selector(runProgram)    x:860-80];
    runBtn.autoresizingMask = NSViewMinXMargin;
    [toolbar addSubview:addBtn];
    [toolbar addSubview:delBtn];
    [toolbar addSubview:runBtn];
    [root addSubview:toolbar];

    // split view
    NSSplitView* split = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 0, 860, 506)];
    split.dividerStyle     = NSSplitViewDividerStyleThin;
    split.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    split.vertical         = YES;

    // left: program list
    NSScrollView* leftScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0,0,180,506)];
    leftScroll.hasVerticalScroller = YES;
    leftScroll.autohidesScrollers  = YES;

    self.table = [[NSTableView alloc] initWithFrame:leftScroll.bounds];
    NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    col.width = 174;
    [self.table addTableColumn:col];
    self.table.headerView  = nil;
    self.table.delegate    = self;
    self.table.dataSource  = self;
    self.table.usesAlternatingRowBackgroundColors = NO;
    self.table.backgroundColor = [NSColor colorWithWhite:0.11 alpha:1];
    leftScroll.documentView = self.table;
    [split addSubview:leftScroll];

    // right pane
    CGFloat axiomH = 30, paneW = 680, paneH = 506;
    NSView* rightPane = [[NSView alloc] initWithFrame:NSMakeRect(0,0,paneW,paneH)];
    rightPane.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // axiom bar
    NSView* axiomBar = [[NSView alloc] initWithFrame:NSMakeRect(0,0,paneW,axiomH)];
    axiomBar.autoresizingMask     = NSViewWidthSizable;
    axiomBar.wantsLayer           = YES;
    axiomBar.layer.backgroundColor= [[NSColor colorWithWhite:0.11 alpha:1] CGColor];

    NSTextField* axiomLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(8,4,48,axiomH-8)];
    axiomLabel.stringValue = @"Axiom:"; axiomLabel.bezeled = NO;
    axiomLabel.drawsBackground = NO; axiomLabel.editable = NO;
    axiomLabel.textColor = [NSColor colorWithWhite:0.6 alpha:1];
    axiomLabel.font = [NSFont systemFontOfSize:12];

    NSTextField* axiomField = [[NSTextField alloc]
        initWithFrame:NSMakeRect(60, 4, paneW-68-170, axiomH-8)];
    axiomField.bezeled = YES; axiomField.bezelStyle = NSTextFieldSquareBezel;
    axiomField.drawsBackground = YES;
    axiomField.backgroundColor = [NSColor colorWithWhite:0.13 alpha:1];
    axiomField.textColor = [NSColor colorWithRed:0.75 green:0.88 blue:0.65 alpha:1];
    axiomField.font = [NSFont fontWithName:@"Menlo" size:12]
                   ?: [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    axiomField.placeholderString = @"e.g. stem(18)";
    axiomField.autoresizingMask = NSViewWidthSizable;
    axiomField.target = self; axiomField.action = @selector(axiomEdited:);
    self.axiomField = axiomField;

    NSTextField* stepsLabel = [[NSTextField alloc]
        initWithFrame:NSMakeRect(paneW-160, 4, 52, axiomH-8)];
    stepsLabel.stringValue = @"Steps:"; stepsLabel.bezeled = NO;
    stepsLabel.drawsBackground = NO; stepsLabel.editable = NO;
    stepsLabel.textColor = [NSColor colorWithWhite:0.6 alpha:1];
    stepsLabel.font = [NSFont systemFontOfSize:12];
    stepsLabel.autoresizingMask = NSViewMinXMargin;

    NSTextField* stepsField = [[NSTextField alloc]
        initWithFrame:NSMakeRect(paneW-108, 4, 60, axiomH-8)];
    stepsField.bezeled = YES; stepsField.bezelStyle = NSTextFieldSquareBezel;
    stepsField.drawsBackground = YES;
    stepsField.backgroundColor = [NSColor colorWithWhite:0.13 alpha:1];
    stepsField.textColor = [NSColor colorWithRed:0.75 green:0.88 blue:0.65 alpha:1];
    stepsField.font = [NSFont fontWithName:@"Menlo" size:12]
                   ?: [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    stepsField.placeholderString = @"e.g. 5";
    stepsField.autoresizingMask = NSViewMinXMargin;
    stepsField.target = self; stepsField.action = @selector(stepsEdited:);
    self.stepsField = stepsField;

    [axiomBar addSubview:axiomLabel];
    [axiomBar addSubview:axiomField];
    [axiomBar addSubview:stepsLabel];
    [axiomBar addSubview:stepsField];

    // editor scroll — sits ABOVE axiom bar (y = axiomH)
    NSScrollView* rightScroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(0, axiomH, paneW, paneH-axiomH)];
    rightScroll.hasVerticalScroller   = YES;
    rightScroll.hasHorizontalScroller = NO;
    rightScroll.autohidesScrollers    = NO;   // always show scrollbar so scroll works
    rightScroll.autoresizingMask      = NSViewWidthSizable | NSViewHeightSizable;
    rightScroll.scrollerStyle         = NSScrollerStyleOverlay;
    
    rightScroll.hasVerticalScroller = YES;
    rightScroll.hasHorizontalScroller = NO;
    rightScroll.autohidesScrollers = YES;

    SyntaxStorage*   storage   = [SyntaxStorage new];
    NSLayoutManager* layout    = [NSLayoutManager new];
    NSTextContainer* container = [[NSTextContainer alloc]
                                      initWithSize:NSMakeSize(FLT_MAX, FLT_MAX)];
    container.widthTracksTextView  = YES;
    container.heightTracksTextView = NO;
    [layout addTextContainer:container];
    [storage addLayoutManager:layout];

    self.textView = [[NSTextView alloc] initWithFrame:rightScroll.bounds
                                        textContainer:container];
    self.textView.verticallyResizable = YES;
    self.textView.horizontallyResizable = NO;
    self.textView.minSize = NSMakeSize(0, 0);
    self.textView.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);

    self.textView.textContainer.containerSize = NSMakeSize(rightScroll.contentSize.width, FLT_MAX);
    self.textView.textContainer.widthTracksTextView = YES;
    self.textView.textContainer.heightTracksTextView = NO;
    
    self.textView.autoresizingMask               = NSViewWidthSizable | NSViewHeightSizable;
    self.textView.backgroundColor                = [NSColor colorWithWhite:0.09 alpha:1];
    self.textView.insertionPointColor            = [NSColor colorWithRed:0.7 green:0.95 blue:0.7 alpha:1];
    self.textView.richText                       = NO;
    self.textView.delegate                       = self;
    self.textView.automaticQuoteSubstitutionEnabled  = NO;
    self.textView.automaticSpellingCorrectionEnabled = NO;
    self.textView.automaticDashSubstitutionEnabled   = NO;
    self.textView.automaticTextReplacementEnabled    = NO;
    self.textView.string                         = @"";
    self.textView.textContainerInset             = NSMakeSize(12, 12);
    // ensure copy/paste and selection work
    self.textView.selectable                     = YES;
    self.textView.editable                       = YES;
    self.textView.allowsUndo                     = YES;

    rightScroll.documentView = self.textView;

    [rightPane addSubview:axiomBar];
    [rightPane addSubview:rightScroll];
    [split addSubview:rightPane];

    [split setHoldingPriority:NSLayoutPriorityDefaultLow  forSubviewAtIndex:0];
    [split setHoldingPriority:NSLayoutPriorityDefaultHigh forSubviewAtIndex:1];
    [root addSubview:split];

    if (self.programs.count > 0) {
        [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
                byExtendingSelection:NO];
        [self loadProgram:0];
    }
}

- (NSButton*)toolbarButton:(NSString*)title action:(SEL)sel x:(CGFloat)x {
    NSButton* b = [[NSButton alloc] initWithFrame:NSMakeRect(x, 4, 60, 26)];
    b.title = title; b.bezelStyle = NSBezelStyleRounded;
    b.target = self; b.action = sel;
    return b;
}

// ─── show / hide ──────────────────────────────────────────────────────────────

- (void)show {
    // activate app so key events, copy/paste, and scrolling all work
    [NSApp activateIgnoringOtherApps:YES];
    if (!self.panel.isVisible) [self.panel makeKeyAndOrderFront:nil];
    else                       [self.panel makeKeyAndOrderFront:nil];
    // focus the text view so it's ready to type/scroll immediately
    [self.panel makeFirstResponder:self.textView];
}

- (void)hide { [self.panel orderOut:nil]; }

// ─── program management ───────────────────────────────────────────────────────

- (void)addProgram {
    [self saveCurrentEdit];
    Program* p = [Program new];
    p.name  = [NSString stringWithFormat:@"Program %lu", self.programs.count + 1];
    p.code  = default_code;
    p.axiom = @"stem(18)";
    p.steps = 5;
    [self.programs addObject:p];
    [self.table reloadData];
    NSInteger row = (NSInteger)self.programs.count - 1;
    [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
    [self loadProgram:row];
    [self saveToDisk];
}

- (void)deleteProgram {
    if (self.selectedIndex < 0) return;
    [self.programs removeObjectAtIndex:self.selectedIndex];
    self.selectedIndex = -1;
    [self.table reloadData];
    if (self.programs.count > 0) {
        NSInteger sel = MIN((NSInteger)self.programs.count - 1, 0);
        [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:sel] byExtendingSelection:NO];
        [self loadProgram:sel];
    } else {
        self.textView.string        = @"";
        self.axiomField.stringValue = @"";
        self.stepsField.stringValue = @"";
    }
    [self saveToDisk];
}

- (void)saveCurrentEdit {
    if (self.selectedIndex >= 0 && self.selectedIndex < (NSInteger)self.programs.count) {
        Program* p = self.programs[self.selectedIndex];
        p.code  = self.textView.string;
        p.axiom = self.axiomField.stringValue;
        p.steps = self.stepsField.integerValue;
    }
}

- (void)loadProgram:(NSInteger)index {
    self.selectedIndex = index;
    if (index >= 0 && index < (NSInteger)self.programs.count) {
        Program* p = self.programs[index];
        self.textView.string         = p.code  ?: @"";
        self.axiomField.stringValue  = p.axiom ?: @"stem(18)";
        self.stepsField.integerValue = p.steps > 0 ? p.steps : 5;
        // scroll back to top after loading
        [self.textView scrollPoint:NSMakePoint(0,0)];
    }
}

- (void)runProgram {
    [self saveCurrentEdit];
    [self saveToDisk];
    if (self.selectedIndex >= 0 && self.onRun) {
        Program* p = self.programs[self.selectedIndex];
        self.onRun(p.code, p.axiom ?: @"stem(18)", p.steps > 0 ? p.steps : 5);
    }
}

- (void)axiomEdited:(NSTextField*)sender { [self saveCurrentEdit]; [self saveToDisk]; }
- (void)stepsEdited:(NSTextField*)sender { [self saveCurrentEdit]; [self saveToDisk]; }

// ─── table data source ────────────────────────────────────────────────────────

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tv {
    return (NSInteger)self.programs.count;
}

- (NSView*)tableView:(NSTableView*)tv viewForTableColumn:(NSTableColumn*)col row:(NSInteger)row {
    NSTextField* cell = [tv makeViewWithIdentifier:@"cell" owner:self];
    if (!cell) {
        cell = [[NSTextField alloc] init];
        cell.identifier = @"cell"; cell.bezeled = NO;
        cell.drawsBackground = NO; cell.editable = YES;
        cell.textColor = [NSColor colorWithWhite:0.9 alpha:1];
        cell.font = [NSFont systemFontOfSize:13];
        cell.target = self; cell.action = @selector(cellEdited:);
    }
    cell.tag = row;
    cell.stringValue = self.programs[row].name;
    return cell;
}

- (void)cellEdited:(NSTextField*)sender {
    NSInteger row = sender.tag;
    if (row >= 0 && row < (NSInteger)self.programs.count) {
        self.programs[row].name = sender.stringValue;
        [self saveToDisk];
    }
}

// ─── table delegate ───────────────────────────────────────────────────────────

- (void)tableViewSelectionDidChange:(NSNotification*)n {
    [self saveCurrentEdit];
    [self loadProgram:self.table.selectedRow];
}

- (CGFloat)tableView:(NSTableView*)tv heightOfRow:(NSInteger)row { return 28; }
- (NSTableRowView*)tableView:(NSTableView*)tv rowViewForRow:(NSInteger)row { return nil; }

// ─── text view delegate ───────────────────────────────────────────────────────

- (void)textDidChange:(NSNotification*)n { [self saveCurrentEdit]; }

@end

// ─── globals ──────────────────────────────────────────────────────────────────

static NSStatusItem*     gStatusItem = nil;
static NSMenu*           gMenu       = nil;
static EditorController* gEditor     = nil;

// ─── public API ───────────────────────────────────────────────────────────────

void makeDesktopWindow(GLFWwindow* glfwWindow) {
    NSWindow* win = glfwGetCocoaWindow(glfwWindow);
    [win setLevel: kCGDesktopWindowLevel + 1];
    [win setStyleMask: NSWindowStyleMaskBorderless];
    [win setFrame: [NSScreen mainScreen].frame display: YES];
    [win setIgnoresMouseEvents: YES];
    [win setCollectionBehavior:
        NSWindowCollectionBehaviorCanJoinAllSpaces
        | NSWindowCollectionBehaviorStationary
        | NSWindowCollectionBehaviorIgnoresCycle];
    [win setCanHide: NO];
    [win setHidesOnDeactivate: NO];
}

void createStatusBarItem(std::function<void(const char*, const char*, int)> onRun) {
    gEditor = [[EditorController alloc]
        initWithRunCallback:^(NSString* code, NSString* axiom, NSInteger steps) {
            onRun([code UTF8String], [axiom UTF8String], (int)steps);
        }];

    gStatusItem = [[NSStatusBar systemStatusBar]
        statusItemWithLength: NSVariableStatusItemLength];
    gStatusItem.button.title  = @"🌿";
    gStatusItem.button.target = gEditor;
    gStatusItem.button.action = @selector(show);

    gMenu = [[NSMenu alloc] init];
    NSMenuItem* open = [[NSMenuItem alloc]
        initWithTitle:@"Open Editor..." action:@selector(show) keyEquivalent:@"e"];
    open.target = gEditor;
    [gMenu addItem:open];
    [gMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* q = [[NSMenuItem alloc]
        initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    q.target = NSApp;
    [gMenu addItem:q];

    gStatusItem.menu = nil;
}
