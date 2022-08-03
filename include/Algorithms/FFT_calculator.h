#ifndef EPLACEPRACTICE_SRC_ALGORITHMS_FFT_calculator_H_
#define EPLACEPRACTICE_SRC_ALGORITHMS_FFT_calculator_H_

namespace ePlace {
class FFT_calculator {
 public:
  // The following FFT library came from
  // http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html

  // 1D fftsg
  void cdft(int n, int isgn, float *a, int *ip, float *w);
  void rdft(int n, int isgn, float *a, int *ip, float *w);
  void ddct(int n, int isgn, float *a, int *ip, float *w);
  void ddst(int n, int isgn, float *a, int *ip, float *w);
  void dfct(int n, float *a, float *t, int *ip, float *w);
  void dfst(int n, float *a, float *t, int *ip, float *w);

  void makewt(int nw, int *ip, float *w);
  void makeipt(int nw, int *ip);
  void makect(int nc, int *ip, float *c);

  void cftfsub(int n, float *a, int *ip, int nw, float *w);
  void cftbsub(int n, float *a, int *ip, int nw, float *w);
  void bitrv2(int n, int *ip, float *a);
  void bitrv2conj(int n, int *ip, float *a);
  void bitrv216(float *a);
  void bitrv216neg(float *a);
  void bitrv208(float *a);
  void bitrv208neg(float *a);
  void cftf1st(int n, float *a, float *w);
  void cftb1st(int n, float *a, float *w);

  void cftrec4(int n, float *a, int nw, float *w);
  int cfttree(int n, int j, int k, float *a, int nw, float *w);
  void cftleaf(int n, int isplt, float *a, int nw, float *w);
  void cftmdl1(int n, float *a, float *w);
  void cftmdl2(int n, float *a, float *w);
  void cftfx41(int n, float *a, int nw, float *w);
  void cftf161(float *a, float *w);
  void cftf162(float *a, float *w);
  void cftf081(float *a, float *w);
  void cftf082(float *a, float *w);
  void cftf040(float *a);
  void cftb040(float *a);
  void cftx020(float *a);
  void rftfsub(int n, float *a, int nc, float *c);
  void rftbsub(int n, float *a, int nc, float *c);
  void dctsub(int n, float *a, int nc, float *c);
  void dstsub(int n, float *a, int nc, float *c);

  // 2D fftsg
  void cdft2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void rdft2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void rdft2dsort(int n1, int n2, int isgn, float **a);
  void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void ddct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void ddst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void cdft2d_sub(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
  void rdft2d_sub(int n1, int isgn, float **a);
  void ddxt2d_sub(int n1, int n2, int ics, int isgn, float **a, float *t, int *ip, float *w);
};

}

#endif //EPLACEPRACTICE_SRC_ALGORITHMS_FFT_calculator_H_
