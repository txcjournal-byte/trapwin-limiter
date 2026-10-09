#include "mock/DistrhoUI.hpp"
#include "../plugin/Params.hpp"
static DisplayHistory gHist;
DisplayHistory* tlGetHistory(void* p) { return p ? &gHist : nullptr; }
#define protected public
#define private public
#include "../plugin/TrapLimitUI.cpp"
#undef protected
#undef private
#include <fstream>
using namespace DISTRHO;
static const char* HEAD = R"(<!doctype html><html><body style="margin:0;background:#1c1d20"><canvas id=cv width=960 height=1536 style="width:480px;height:768px"></canvas><script>
const c=document.getElementById('cv').getContext('2d');c.scale(2,2);
function lg(a,b,d,e,i,o){const g=c.createLinearGradient(a,b,d,e);g.addColorStop(0,i);g.addColorStop(1,o);return g;}
function rg(x,y,r0,r1,i,o){const g=c.createRadialGradient(x,y,r0,x,y,r1);g.addColorStop(0,i);g.addColorStop(1,o);return g;}
function bg(x,y,w,h,r,f,i,o){const cx=x+w/2,cy=y+h/2;const g=c.createRadialGradient(cx,cy,Math.max(1,Math.min(w,h)/2-f),cx,cy,Math.max(w,h)/2+f/2);g.addColorStop(0,i);g.addColorStop(1,o);return g;}
function tx(x,y,s,sz,ha,va,bl){c.save();c.font=sz+'px "DejaVu Sans", sans-serif';c.textAlign=ha;c.textBaseline=va;if(bl>0){c.shadowColor=c.fillStyle;c.shadowBlur=bl*2;}c.fillText(s,x,y);c.restore();}
)";
int main(int argc, char** argv) {
  int variant = argc > 1 ? atoi(argv[1]) : 0;
  // fake waveform history
  uint32_t seed = 7;
  for (int i = 0; i < kHistoryLen; ++i) {
    seed = seed * 1664525u + 1013904223u; float r = (seed >> 9) / 8388608.f;
    float beat = std::fmod(i, 45.f) / 45.f;
    float env = 0.55f + 0.4f * std::exp(-beat * 5.f);
    gHist.peak[i] = std::fmin(0.96f, env * (0.75f + 0.25f * r));
    gHist.gr[i] = -(beat < 0.08f ? 7.f + 3.f * r : 1.5f + 2.f * std::exp(-beat * 4.f) + r);
  }
  gHist.writePos = 0;
  TrapLimitUI ui; ui.pluginPtr = &gHist; ui.w = 480; ui.h = 768;
  float vals[][2] = {{kOutInL,-2.f},{kOutInR,-2.5f},{kOutOutL,-0.4f},{kOutOutR,-0.5f},{kOutGR,-2.0f},{kOutClipGR,-4.0f},{kOutLufsM,-5.1f},{kOutLufsS,-5.8f},
                     {kOutLufsI,-6.2f},{kOutLRA,4.8f},{kOutTP,-0.3f},{kOutCPU,3.f},{kOutLatency,6.1f},{kGain,8.5f},{kClip,1},{kTruePeak,1},{kOversample,1}};
  for (auto& v : vals) ui.parameterChanged((uint32_t)v[0], v[1]);
  if (variant == 1) { MouseEvent e{1, true, {600*(87+27)/600.0*1.0, 544+30}, 0}; e.pos.x = 30 + 2*54 + 25; e.pos.y = 580; ui.onMouse(e); e.press=false; ui.onMouse(e); ui.parameterChanged(kStream,1); ui.parameterChanged(kPunch,40); }
  if (variant == 2) { ui.parameterChanged(kMode,2); MouseEvent e{1,true,{30+9*54+20, 760},0}; ui.onMouse(e); e.press=false; ui.onMouse(e); /* LOAD? */ }
  if (variant == 4) { setenv("HOME","/tmp/tlhome",1); ui.doAction(A_Save); for (const char* c="Brutal 808"; *c; ++c) {} ui.saveName = "Brutal 808"; ui.doSave();
     ui.saveName="Dark Trap Master"; ui.doSave(); ui.saveName="Vox Bus"; ui.doSave(); ui.doAction(A_Load); }
  if (variant == 5) { setenv("HOME","/tmp/tlhome",1); ui.doAction(A_Save); CharacterInputEvent ce{'X',0}; ui.onCharacterInput(ce); }
  if (variant == 3) { MouseEvent e{1,true,{190,500},0}; ui.onMouse(e); e.press=false; ui.onMouse(e); }
  ui.onNanoDisplay();
  std::ofstream f(argv[2]); f << HEAD << ui.js << "</script></body></html>";
}
