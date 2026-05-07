//
//  desktop_window.hpp
//  test
//
//  Created by Albert Yang on 2026-04-08.
//
#pragma once
#include <functional>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#pragma once
#include <functional>

struct GLFWwindow;

void makeDesktopWindow(GLFWwindow* w);

// Pure C++ interface (no NSString here!)
void createStatusBarItem(
    std::function<void(const char*, const char*, int)> onRun
);
