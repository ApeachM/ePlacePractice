#ifndef EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_
#define EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_

namespace ePlace{
class Bin {
 public:
  float electricDensity;  // means ρ_DCT in eq (22)
  float electricPotential;  // means phi_DCT in eq (23)
  float electricForce_x;  // means ξ_x_DSCT in eq (24)
  float electricForce_y;  // means ξ_y_DSCT in eq (24)

  float wx, wy; // means w_u, w_v each in eq (22)
  float wx_sq, wy_sq; // means w_u^2, w_v^2 each in eq (23)
};

}

#endif //EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_
