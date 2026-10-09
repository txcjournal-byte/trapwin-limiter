#define main dsp_main
#include "dsptest.cpp"
#undef main
int main(){
  std::vector<float> L,R,oL,oR; makeTrap(L,R,8.0);
  for (int variant=0; variant<3; ++variant){
  Host h; h.set(kGain,8); if(variant==0) h.set(k808,80); if(variant==1) h.set(kMonoSub,1); if(variant==2){h.set(k808,80); h.set(kClip,0);}
  h.process(L,R,oL,oR);
  tl::TruePeak tp; float m=0; size_t at=0; for(size_t i=4800;i<oL.size();++i){float v=tp.process(oL[i]); if(v>m){m=v;at=i;}}
  int flat=0; for(size_t i=4800;i<oL.size();++i) if(std::fabs(std::fabs(oL[i])-0.96605f)<1e-5f) flat++;
  printf("variant %d maxTP %.2f dB at %zu; samples at ceiling: %d\n", variant, 20*log10(m), at, flat);
  for(size_t i=at-9;i<at-2;++i) printf("  %zu %.5f\n", i, oL[i]);
  }
}
