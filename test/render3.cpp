#define TL_MOCK 1
#include "mock/DistrhoUI.hpp"
#include "../plugin/Params.hpp"
DisplayHistory* tlGetHistory(void*) { return nullptr; }
#define protected public
#define private public
#include "../plugin/TrapWinBetonUI.cpp"
#undef protected
#undef private
#include <fstream>
using namespace DISTRHO;
int main(int argc, char** argv) {
  int variant = atoi(argv[1]); int W = atoi(argv[3]); int H = W * 356 / 1980;
  TrapWinBetonUI ui; ui.w = W; ui.h = H;
  float vals[][2] = {{kOutInL,-2.f},{kOutInR,-2.5f},{kOutOutL,-0.4f},{kOutOutR,-0.5f},{kOutGR,-1.5f},{kOutClipGR,-2.5f},{kOutLufsM,-5.1f},{kOutLufsS,-5.8f},
                     {kOutLufsI,-6.2f},{kOutLRA,4.8f},{kOutTP,-0.3f},{kGain,8.5f},{kCeiling,-0.3f},{kDrive,4.f},{kRelease,80.f},{kSat,35.f},{k808,55.f},{kMode,3},{kClipShape,1},{kMonoSub,1}};
  for (auto& v : vals) ui.parameterChanged((uint32_t)v[0], v[1]);
  ui.currentPreset = "Brutal 808";
  if (variant == 1) ui.overlay = 4;
  if (variant == 2) { setenv("HOME","/tmp/tlhome",1); ui.refreshMyPresets(); ui.overlay = 2; ui.presetTab = 1; }
  if (variant == 3) { ui.dragging = true; ui.dragKnob = 0; }
  ui.onNanoDisplay();
  std::ofstream f(argv[2]);
  f << "<!doctype html><html><head><style>@font-face{font-family:'mono-bold';src:url('file:///home/claude/traplimit/plugin/res/LiberationMono-Bold.ttf');}@font-face{font-family:'serif-bold';src:url('file:///home/claude/traplimit/plugin/res/DejaVuSerif-Bold.ttf');}body{margin:0;background:#2a2b2e}</style></head><body>"
    << "<canvas id=cv width=" << W*2 << " height=" << H*2 << " style='width:" << W << "px;height:" << H << "px'></canvas><script>\n"
    << "const c=document.getElementById('cv').getContext('2d');c.scale(2,2);\n"
    << "function lg(a,b,d,e,i,o){const g=c.createLinearGradient(a,b,d,e);g.addColorStop(0,i);g.addColorStop(1,o);return g;}\n"
    << "function rg(x,y,r0,r1,i,o){const g=c.createRadialGradient(x,y,r0,x,y,r1);g.addColorStop(0,i);g.addColorStop(1,o);return g;}\n"
    << "function bg(x,y,w,h,r,f,i,o){const cx=x+w/2,cy=y+h/2;const g=c.createRadialGradient(cx,cy,Math.max(1,Math.min(w,h)/2-f),cx,cy,Math.max(w,h)/2+f/2);g.addColorStop(0,i);g.addColorStop(1,o);return g;}\n"
    << "function tx(x,y,s,sz,ha,va,bl,fam,sp){c.save();c.font=sz+'px \"'+fam+'\", sans-serif';c.letterSpacing=sp+'px';c.textAlign=ha;c.textBaseline=va;if(bl>0){c.shadowColor=c.fillStyle;c.shadowBlur=bl*2;}c.fillText(s,x,y);c.restore();}\n"
    << "const TEX=new Image();TEX.src='file:///home/claude/traplimit/plugin/res/concrete.png';\n"
    << "function draw(){\n" << ui.js << "}\nPromise.all([document.fonts.load('20px \"mono-bold\"'),document.fonts.load('20px \"serif-bold\"'),new Promise(r=>TEX.onload=r)]).then(()=>{draw();document.title='done';});</script></body></html>";
}
