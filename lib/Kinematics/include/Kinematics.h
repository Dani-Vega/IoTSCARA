#ifndef _KINEMATICS_H
#define _KINEMATICS_H

#include <math.h>

#define DEG2RAD 0.0174533f
#define RAD2DEG 57.29578f

class Kinematics
{
public:
    Kinematics();

    ~Kinematics();

    void setup(float DH_Parameters[][4], uint8_t num_DoF);
    void calculateFK(float pose[6]);
    void updateJointVariables(float joint_variables[4]); //4DoF
    void calculateDK(float joint_vel[4], float operational_vel[6]); //Differential Kinematics
    int calculateIK(float operational_var[4], float solutons[2][4]);
    int selectBestIK(float solutions[2][4]);
    bool isNearSingularity(float joint_angles[4]);

private:
    float (*_DH)[4];
    uint8_t _num_DoF;
};

#endif
