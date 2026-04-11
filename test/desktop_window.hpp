//
//  desktop_window.hpp
//  test
//
//  Created by Albert Yang on 2026-04-08.
//

#pragma once
#include <functional>
struct GLFWwindow;

void makeDesktopWindow(GLFWwindow* w);
void createStatusBarItem(std::function<void(int)> onChange);
