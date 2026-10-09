#define main dsp_main
#include "dsptest.cpp"
#undef main
int main(){ std::vector<float> L,R,oL,oR; makeTrap(L,R,4.0);
 for (int shape=0; shape<3; ++shape){ Host h; h.set(kGain,7); h.set(kClipShape,(float)shape); h.set(kMode,3);
  size_t bs=512; float minClip=0,minLim=0; oL.assign(L.size(),0); oR.assign(R.size(),0);
  for(size_t s=0;s+bs<=L.size();s+=bs){const float* in[2]={&L[s],&R[s]}; float* out[2]={&oL[s],&oR[s]}; h.p->run(in,out,bs); minClip=std::min(minClip,h.get(kOutClipGR)); minLim=std::min(minLim,h.get(kOutGR));}
  printf("shape %d: max clipper reduction %.1f dB, max limiter GR %.1f dB\n", shape, minClip, minLim);} }
