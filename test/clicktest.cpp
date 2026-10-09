#define TL_MOCK 1
#include "mock/DistrhoUI.hpp"
#include "../plugin/Params.hpp"
DisplayHistory* tlGetHistory(void*) { return nullptr; }
#define protected public
#define private public
#include "../plugin/TrapWinBetonUI.cpp"
#undef protected
#undef private
#include <cstdio>
using namespace DISTRHO;
int main(){
  TrapWinBetonUI ui; ui.w = 1386; ui.h = 249; float sc = 1386/1980.f;
  auto press=[&](float x,float y,uint b=1){ MouseEvent e{}; e.button=b; e.press=true; e.pos.x=x*sc; e.pos.y=y*sc; e.mod=0; return ui.onMouse(e); };
  auto rel=[&](float x,float y){ MouseEvent e{}; e.button=1; e.press=false; e.pos.x=x*sc; e.pos.y=y*sc; e.mod=0; ui.onMouse(e); };
  auto move=[&](float x,float y){ MotionEvent e{}; e.pos.x=x*sc; e.pos.y=y*sc; e.mod=0; ui.onMotion(e); };
  printf("gain before %.2f\n", ui.vals[kGain]);
  printf("press handled %d\n", press(315,145)); move(315,100); move(315,60); rel(315,60);
  printf("gain after %.2f\n", ui.vals[kGain]);
  press(1515, BTN_Y[2]); rel(1515,BTN_Y[2]); printf("mode %.0f\n", ui.vals[kMode]);
  press(905, BTN_Y[0]); rel(905,BTN_Y[0]); printf("clip %.0f shape %.0f\n", ui.vals[kClip], ui.vals[kClipShape]);
  press(TOG_X, 260); rel(TOG_X,260); printf("monosub %.0f\n", ui.vals[kMonoSub]);
}
