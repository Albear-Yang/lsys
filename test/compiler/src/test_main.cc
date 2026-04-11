//// main.cpp
//// Build: see CMakeLists.txt
//// Deps: SDL2, Dear ImGui (with SDL2+OpenGL3 backends)
////
//// Layout:
////   ┌─────────────────┬──────────────────────────────┐
////   │  CODE EDITOR    │                              │
////   │  (textarea)     │     L-SYSTEM CANVAS          │
////   │                 │     (turtle graphics)        │
////   │  AXIOM input    │                              │
////   │  [Run] [Step]   │                              │
////   │  [Reset]        │                              │
////   │  Gen: N         │                              │
////   └─────────────────┴──────────────────────────────┘
//
//#include <SDL2/SDL.h>
//#include <GL/gl.h>
//
//#include "imgui.h"
//#include "imgui_impl_sdl2.h"
//#include "imgui_impl_opengl3.h"
//
//#include <cmath>
//#include <stack>
//#include <string>
//#include <vector>
//#include <memory>
//#include <sstream>
//#include <stdexcept>
//#include <algorithm>
//
//#include "lsyslex.hpp"
//#include "lsysparse.hpp"
//#include "lsystype.hpp"
//#include "lsysgen.hpp"
//
//// ─── simple 2D turtle ─────────────────────────────────────────────────────────
//// Projects 3D turtle moves onto the XY plane for display.
//// Yaw rotates in-plane; pitch/roll are ignored for the 2D preview.
//
//struct Line2D { float x0, y0, x1, y1; float r, g, b, a; };
//
//struct Turtle2D : public Turtle {
//   float x = 0, y = 0;
//   float angle = -90.0f;         // degrees, 0 = right, -90 = up
//   float cur_r = 1, cur_g = 1, cur_b = 1, cur_a = 1;
//
//   struct State { float x,y,angle,r,g,b,a; };
//   std::stack<State> stack_;
//   std::vector<Line2D> lines;
//
//   void reset(float sx=0, float sy=0, float sa=-90.0f){
//       x=sx; y=sy; angle=sa;
//       cur_r=cur_g=cur_b=cur_a=1;
//       while(!stack_.empty()) stack_.pop();
//       lines.clear();
//   }
//
//   void forward(float dist) override {
//       float rad = angle * 3.14159265f / 180.0f;
//       float nx = x + dist * std::cos(rad);
//       float ny = y + dist * std::sin(rad);
//       lines.push_back({x, y, nx, ny, cur_r, cur_g, cur_b, cur_a});
//       x = nx; y = ny;
//   }
//   void yaw(float deg)   override { angle += deg; }
//   void pitch(float)     override {}   // ignored in 2D
//   void roll(float)      override {}   // ignored in 2D
//   void fill(float r, float g, float b, float a) override {
//       cur_r=r; cur_g=g; cur_b=b; cur_a=a;
//   }
//   void push() override { stack_.push({x,y,angle,cur_r,cur_g,cur_b,cur_a}); }
//   void pop()  override {
//       if(stack_.empty()) return;
//       auto s = stack_.top(); stack_.pop();
//       x=s.x; y=s.y; angle=s.angle;
//       cur_r=s.r; cur_g=s.g; cur_b=s.b; cur_a=s.a;
//   }
//};
//
//// ─── parse axiom ──────────────────────────────────────────────────────────────
//// Axiom format: "F(1.0) G B(0.5, 2.0)"
//
//static std::vector<SymbolInstance> parse_axiom(const std::string &src) {
//   std::vector<SymbolInstance> axiom;
//   size_t i = 0;
//   auto skip = [&]{ while(i<src.size()&&std::isspace(src[i]))++i; };
//   while(i < src.size()){
//       skip();
//       if(i >= src.size()) break;
//       if(!std::isalpha(src[i]) && src[i]!='_') { ++i; continue; }
//       size_t s = i;
//       while(i<src.size()&&(std::isalnum(src[i])||src[i]=='_')) ++i;
//       SymbolInstance inst;
//       inst.name = src.substr(s, i-s);
//       skip();
//       if(i<src.size() && src[i]=='('){
//           ++i;
//           skip();
//           while(i<src.size() && src[i]!=')'){
//               size_t ns = i;
//               if(i<src.size()&&src[i]=='-') ++i;
//               while(i<src.size()&&(std::isdigit(src[i])||src[i]=='.')) ++i;
//               float val = std::stof(src.substr(ns, i-ns));
//               inst.args.push_back([val](const Env&){ return val; });
//               skip();
//               if(i<src.size()&&src[i]==',') { ++i; skip(); }
//           }
//           if(i<src.size()) ++i; // consume ')'
//       }
//       axiom.push_back(std::move(inst));
//   }
//   return axiom;
//}
//
//// ─── application state ────────────────────────────────────────────────────────
//
//struct AppState {
//   char code_buf[1024*32] = {};
//   char axiom_buf[512]    = "F";
//   char error_buf[1024]   = {};
//   bool has_error         = false;
//
//   std::unique_ptr<Lsystem>              lsys;
//   std::vector<SymbolInstance>           current_string;
//   int                                   generation = 0;
//
//   Turtle2D                              turtle;
//   float                                 zoom   = 1.0f;
//   float                                 pan_x  = 0.0f;
//   float                                 pan_y  = 0.0f;
//   bool                                  panning = false;
//   ImVec2                                pan_start{};
//   float                                 pan_ox = 0, pan_oy = 0;
//
//   void compile_and_reset() {
//       has_error = false;
//       error_buf[0] = '\0';
//       try {
//           auto toks        = tokenize(std::string(code_buf));
//           std::string pinput = tokens_to_parser_input(toks);
//           Parser parser;
//           cout << pinput << endl;
//           int rc = parser.parse_input(pinput);
//           if(rc != 0) throw std::runtime_error("Parse error");
//
//           // build tree from result string
//           std::istringstream iss(parser.result);
//           // TypeChecker also builds the tree from the linearised string
//           TypeChecker tc(parser.result);
//           tc.type_check();
//
//           lsys = std::make_unique<Lsystem>(tc.tree());
//           current_string = parse_axiom(std::string(axiom_buf));
//           generation = 0;
//           redraw();
//       } catch(const std::exception &e){
//           has_error = true;
//           snprintf(error_buf, sizeof(error_buf), "%s", e.what());
//           lsys.reset();
//       }
//   }
//
//   void step_forward() {
//       if(!lsys) return;
//       current_string = lsys->step(current_string);
//       ++generation;
//       redraw();
//   }
//
//   void redraw() {
//       if(!lsys) return;
//       turtle.reset(0, 0, -90.0f);
//       lsys->draw(current_string, turtle);
//       fit_to_lines();
//   }
//
//   void fit_to_lines() {
//       if(turtle.lines.empty()){ zoom=1; pan_x=pan_y=0; return; }
//       float mn_x=1e9,mn_y=1e9,mx_x=-1e9,mx_y=-1e9;
//       for(auto &l : turtle.lines){
//           mn_x=std::min({mn_x,l.x0,l.x1});
//           mn_y=std::min({mn_y,l.y0,l.y1});
//           mx_x=std::max({mx_x,l.x0,l.x1});
//           mx_y=std::max({mx_y,l.y0,l.y1});
//       }
//       float cx=(mn_x+mx_x)*0.5f, cy=(mn_y+mx_y)*0.5f;
//       pan_x=-cx; pan_y=-cy;
//       float w=mx_x-mn_x+1, h=mx_y-mn_y+1;
//       zoom = std::min(400.0f/w, 400.0f/h);
//       if(zoom > 20.0f) zoom=20.0f;
//   }
//};
//
//// ─── main ─────────────────────────────────────────────────────────────────────
//
//int main(int, char**) {
//   SDL_Init(SDL_INIT_VIDEO);
//   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
//   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
//
//   SDL_Window *win = SDL_CreateWindow("L-System Studio",
//       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
//       1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
//   SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
//   SDL_GL_MakeCurrent(win, gl_ctx);
//   SDL_GL_SetSwapInterval(1);
//
//   IMGUI_CHECKVERSION();
//   ImGui::CreateContext();
//   ImGuiIO &io = ImGui::GetIO();
//   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
//   io.Fonts->AddFontFromFileTTF(
//       "/System/Library/Fonts/Menlo.ttc", 14.0f);
//
//   // dark theme with green accent
//   ImGui::StyleColorsDark();
//   ImGuiStyle &sty = ImGui::GetStyle();
//   sty.WindowRounding = 4;
//   sty.FrameRounding  = 3;
//   sty.Colors[ImGuiCol_Header]        = ImVec4(0.2f,0.5f,0.3f,0.8f);
//   sty.Colors[ImGuiCol_Button]        = ImVec4(0.2f,0.45f,0.3f,1.0f);
//   sty.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f,0.6f,0.4f,1.0f);
//   sty.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.1f,0.35f,0.2f,1.0f);
//   sty.Colors[ImGuiCol_FrameBg]       = ImVec4(0.1f,0.1f,0.1f,1.0f);
//
//   ImGui_ImplSDL2_InitForOpenGL(win, gl_ctx);
//   ImGui_ImplOpenGL3_Init("#version 330");
//
//   AppState app;
//
//   // seed the code editor with an example
//   const char *example =
//       "symbol F(float x)\n"
//       "\n"
//       "rule: F(float x) -> F(x * 1.1) LBRACK F(x * 0.7) RBRACK F(x * 0.9)\n"
//       "\n"
//       "draw_rule: F {\n"
//       "    line x;\n"
//       "    turn_yaw 25.0;\n"
//       "}\n";
//   strncpy(app.code_buf, example, sizeof(app.code_buf)-1);
//   strncpy(app.axiom_buf, "F(1.0)", sizeof(app.axiom_buf)-1);
//
//   bool running = true;
//   while(running){
//       SDL_Event ev;
//       while(SDL_PollEvent(&ev)){
//           ImGui_ImplSDL2_ProcessEvent(&ev);
//           if(ev.type == SDL_QUIT) running = false;
//       }
//
//       ImGui_ImplOpenGL3_NewFrame();
//       ImGui_ImplSDL2_NewFrame();
//       ImGui::NewFrame();
//
//       int win_w, win_h;
//       SDL_GetWindowSize(win, &win_w, &win_h);
//       ImGui::SetNextWindowPos({0,0});
//       ImGui::SetNextWindowSize({(float)win_w,(float)win_h});
//       ImGui::Begin("##root", nullptr,
//           ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
//           ImGuiWindowFlags_NoBringToFrontOnFocus);
//
//       // ── left panel ──────────────────────────────────────────────────────
//       float panel_w = 340.0f;
//       ImGui::BeginChild("##left", {panel_w, 0}, true);
//
//       ImGui::TextColored({0.4f,0.9f,0.5f,1}, "L-SYSTEM STUDIO");
//       ImGui::Separator();
//
//       ImGui::Text("Code");
//       ImGui::InputTextMultiline("##code", app.code_buf, sizeof(app.code_buf),
//           {panel_w-16, 320},
//           ImGuiInputTextFlags_AllowTabInput);
//
//       ImGui::Spacing();
//       ImGui::Text("Axiom");
//       ImGui::SetNextItemWidth(panel_w - 16);
//       ImGui::InputText("##axiom", app.axiom_buf, sizeof(app.axiom_buf));
//
//       ImGui::Spacing();
//       ImGui::Separator();
//       ImGui::Spacing();
//
//       if(ImGui::Button("  Run  ", {90,28}))  app.compile_and_reset();
//       ImGui::SameLine();
//       if(ImGui::Button("  Step  ", {90,28})) app.step_forward();
//       ImGui::SameLine();
//       if(ImGui::Button(" Reset ", {90,28})){
//           if(app.lsys){
//               app.current_string = parse_axiom(app.axiom_buf);
//               app.generation = 0;
//               app.redraw();
//           }
//       }
//
//       ImGui::Spacing();
//       ImGui::Text("Generation: %d", app.generation);
//       ImGui::Text("Symbols:    %zu", app.current_string.size());
//       ImGui::Text("Lines:      %zu", app.turtle.lines.size());
//
//       if(app.has_error){
//           ImGui::Spacing();
//           ImGui::Separator();
//           ImGui::TextColored({1,0.3f,0.3f,1}, "Error:");
//           ImGui::TextWrapped("%s", app.error_buf);
//       }
//
//       ImGui::Spacing();
//       ImGui::Separator();
//       ImGui::Text("Canvas");
//       ImGui::SliderFloat("Zoom", &app.zoom, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
//       if(ImGui::Button("Fit")) app.fit_to_lines();
//
//       ImGui::EndChild(); // left panel
//
//       ImGui::SameLine();
//
//       // ── canvas panel ────────────────────────────────────────────────────
//       ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
//       float  canvas_w   = (float)win_w - panel_w - 20;
//       float  canvas_h   = (float)win_h - 20;
//
//       ImGui::BeginChild("##canvas", {canvas_w, canvas_h}, false);
//
//       ImDrawList *dl = ImGui::GetWindowDrawList();
//       ImVec2 cp = ImGui::GetCursorScreenPos();
//
//       // background
//       dl->AddRectFilled(cp, {cp.x+canvas_w, cp.y+canvas_h}, IM_COL32(18,18,22,255));
//       dl->AddRect(cp, {cp.x+canvas_w, cp.y+canvas_h}, IM_COL32(50,80,60,255));
//
//       // centre of canvas in screen space
//       float cx = cp.x + canvas_w * 0.5f + app.pan_x * app.zoom;
//       float cy = cp.y + canvas_h * 0.5f + app.pan_y * app.zoom;
//
//       // clip to canvas
//       dl->PushClipRect(cp, {cp.x+canvas_w, cp.y+canvas_h}, true);
//
//       // draw turtle lines
//       for(auto &l : app.turtle.lines){
//           float sx = cx + l.x0 * app.zoom;
//           float sy = cy + l.y0 * app.zoom;
//           float ex = cx + l.x1 * app.zoom;
//           float ey = cy + l.y1 * app.zoom;
//           ImU32 col = IM_COL32(
//               (int)(l.r*220), (int)(l.g*220), (int)(l.b*220),
//               (int)(l.a*200));
//           dl->AddLine({sx,sy},{ex,ey}, col, 1.2f);
//       }
//
//       // origin crosshair
//       dl->AddLine({cx-5,cy},{cx+5,cy}, IM_COL32(80,80,80,180), 1);
//       dl->AddLine({cx,cy-5},{cx,cy+5}, IM_COL32(80,80,80,180), 1);
//
//       dl->PopClipRect();
//
//       // pan with mouse drag inside canvas
//       ImGui::InvisibleButton("##canvas_input", {canvas_w, canvas_h});
//       if(ImGui::IsItemActive() && ImGui::IsMouseDragging(0)){
//           ImVec2 d = ImGui::GetMouseDragDelta(0, 0);
//           app.pan_x = app.pan_ox + d.x / app.zoom;
//           app.pan_y = app.pan_oy + d.y / app.zoom;
//       } else {
//           app.pan_ox = app.pan_x;
//           app.pan_oy = app.pan_y;
//       }
//       // scroll to zoom
//       if(ImGui::IsItemHovered()){
//           float wheel = io.MouseWheel;
//           if(wheel != 0) app.zoom *= (wheel > 0 ? 1.1f : 0.9f);
//       }
//
//       // overlay: generation counter
//       dl->AddText({cp.x+8, cp.y+8}, IM_COL32(80,180,100,200),
//           ("Gen " + std::to_string(app.generation)).c_str());
//
//       ImGui::EndChild();
//       ImGui::End();
//
//       // render
//       ImGui::Render();
//       glViewport(0, 0, win_w, win_h);
//       glClearColor(0.07f, 0.07f, 0.09f, 1.0f);
//       glClear(GL_COLOR_BUFFER_BIT);
//       ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//       SDL_GL_SwapWindow(win);
//   }
//
//   ImGui_ImplOpenGL3_Shutdown();
//   ImGui_ImplSDL2_Shutdown();
//   ImGui::DestroyContext();
//   SDL_GL_DeleteContext(gl_ctx);
//   SDL_DestroyWindow(win);
//   SDL_Quit();
//   return 0;
//}
