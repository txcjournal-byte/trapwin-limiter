#define main dsp_main
#include "dsptest.cpp"
#undef main
int main(){ std::vector<float> L,R,oL,oR; makeTrap(L,R,4.0);
 for (float g : {2.f, 4.f, 8.f}) { for(auto&v:L) v*=g; for(auto&v:R) v*=g;
  for (int os=0; os<3; ++os) { Host h; h.set(kOversample,(float)os);
   h.process(L,R,oL,oR);
   printf("in x%.0f os %d: in pk %.1f dB  out pk %.2f  outTP %.2f  meterTP %.2f GR %.1f\n", g, os, peakDb(L), peakDb(oL), truePeakDb(oL,0), h.get(kOutTP), h.get(kOutGR)); }
  for(auto&v:L) v/=g; for(auto&v:R) v/=g; } }
