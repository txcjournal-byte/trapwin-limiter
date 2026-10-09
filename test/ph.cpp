#include "../plugin/Dsp.hpp"
#include <cstdio>
int main(){ const double* h=tl::tpCoeffs().h; for(int p=0;p<4;++p){double s=0; for(int k=0;4*k+p<tl::kTPTaps;++k) s+=h[4*k+p]; printf("phase %d dc %.4f\n",p,s);} 
 const double* g=tl::hbCoeffs().h; double e=0,o=0; for(int k=0;k<tl::kHBTaps;++k) (k%2?o:e)+=g[k]; printf("hb even %.4f odd %.4f\n",e,o);}
