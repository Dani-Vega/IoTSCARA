#include "Kinematics.h"

Kinematics::Kinematics() {};

Kinematics::~Kinematics() {};

void Kinematics::setup(float DH_Parameters[][4], uint8_t num_DoF)
{
    _DH = DH_Parameters;
    _num_DoF = num_DoF;
}

void Kinematics::calculateFK(float pose[6])
{
    // Initialize HM with the identity matrix, it will accumulate the results
    float HM_total[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}};
    // Apply for each DoF
    for (uint8_t i = 0; i < _num_DoF; i++)
    {
        // Obtain the HM from DH parameters of each joint
        float t = DEG2RAD * _DH[i][0]; // theta
        float d = _DH[i][1];
        float a = DEG2RAD * _DH[i][2]; // alpha
        float r = _DH[i][3];
        float ct = cos(t);
        float ca = cos(a);
        float st = sin(t);
        float sa = sin(a);
        float HM_DoF[][4] =
            {{ct, -ca * st, sa * st, r * ct},
             {st, ca * ct, -sa * ct, r * st},
             {0, sa, ca, d},
             {0, 0, 0, 1}};
        // Temp HM row and apply matrix mult
        float HM_row_temp[4];
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                HM_row_temp[col] = 0;
                for (int k = 0; k < 4; k++)
                {
                    HM_row_temp[col] += HM_total[row][k] * HM_DoF[k][col];
                }
            }
            // Update each row
            for (int col = 0; col < 4; col++)
                HM_total[row][col] = HM_row_temp[col];
        }
    }
    // Convert final HM to pose
    pose[0] = RAD2DEG * atan2(HM_total[1][0], HM_total[0][0]); //roll
    pose[1] = RAD2DEG * atan2(-HM_total[2][0], sqrt(HM_total[0][0] * HM_total[0][0] + HM_total[1][0] * HM_total[1][0])); //pitch
    pose[2] = RAD2DEG * atan2(HM_total[2][1], HM_total[2][2]); //yaw
    pose[3] = HM_total[0][3]; //X
    pose[4] = HM_total[1][3]; //Y
    pose[5] = HM_total[2][3]; //Z
}

void Kinematics::updateJointVariables(float joint_variables[4])
{
    _DH[0][0] = joint_variables[0]; // θ1
    _DH[1][0] = joint_variables[1]; // θ2
    _DH[2][1] = joint_variables[2]; // d
    _DH[3][0] = joint_variables[3]; // θ3
}

void Kinematics::calculateDK(float joint_vel[4], float operational_vel[6]) // Differential Kinematics
{
    float theta1 = DEG2RAD * _DH[1][0];
    float theta2 = DEG2RAD * _DH[2][0];
    float L1 = _DH[1][2];
    float L2 = _DH[2][2];
    float c1 = cos(theta1);
    float c12 = cos(theta1 + theta2);
    float s1 = sin(theta1);
    float s12 = sin(theta1 + theta2);
    float Jacobian[6][4] = {
        // Linear velocity
        {-L1 * s1 - L2 * s12, -L2 * s12, 0, 0},
        {L1 * c1 + L2 * c12, L2 * c12, 0, 0},
        {0, 0, 1, 0},
        // Angular velocity just around z
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {1, 1, 0, 1}};
    for (int row = 0; row < 6; row++)
    {
        operational_vel[row] = 0;
        for (int col = 0; col < 4; col++)
            operational_vel[row] += Jacobian[row][col] * joint_vel[col];
    }
}

int Kinematics::calculateIK(float operational_var[4], float solutions[2][4])
{
    float x = operational_var[0];
    float y = operational_var[1];
    float z = operational_var[2];
    float theta_tool = DEG2RAD * operational_var[3];
    float L1 = _DH[1][2];
    float L2 = _DH[2][2];
    float P = sqrt(x * x + y * y);
    float gamma = atan2(y, x);
    // Check reachability
    if (P > (L1 + L2) || P < fabs(L1 - L2))
        return 0;
    float alpha = acos((L1 * L1 + P * P - L2 * L2) / (2 * L1 * P));
    float beta = acos((L1 * L1 + L2 * L2 - P * P) / (2 * L1 * L2));
    float theta1_1 = gamma - alpha;
    float theta2_1 = M_PI - beta;
    float theta1_2 = gamma + alpha;
    float theta2_2 = -(M_PI - beta);
    float theta3_1 = theta_tool - (theta1_1 + theta2_1);
    float theta3_2 = theta_tool - (theta1_2 + theta2_2);
    // Save both solutions [θ1, θ2, d, θ3] in degrees
    solutions[0][0] = RAD2DEG * theta1_1;
    solutions[0][1] = RAD2DEG * theta2_1;
    solutions[0][2] = z;
    solutions[0][3] = RAD2DEG * theta3_1;
    solutions[1][0] = RAD2DEG * theta1_2;
    solutions[1][1] = RAD2DEG * theta2_2;
    solutions[1][2] = z;
    solutions[1][3] = RAD2DEG * theta3_2;
    return 2; // always returns 2 possible solutions if reachable
}

int Kinematics::selectBestIK(float solutions[2][4])
{
    float current_theta1 = _DH[1][0];
    float current_theta2 = _DH[2][0];
    float current_theta3 = _DH[3][0];
    float best_error = 1e6;
    int best_index = -1;
    for (int i = 0; i < 2; i++)
    {
        if (isNearSingularity(solutions[i]))
            continue; // Saltar esta solución
        float theta1 = solutions[i][0];
        float theta2 = solutions[i][1];
        float theta3 = solutions[i][3];
        float error =
            2.0 * fabs(theta1 - current_theta1) +
            1.5 * fabs(theta2 - current_theta2) +
            1.0 * fabs(theta3 - current_theta3);
        if (error < best_error)
        {
            best_error = error;
            best_index = i;
        }
    }
    return best_index;
}

bool Kinematics::isNearSingularity(float joint_angles[4])
{
    float theta1 = DEG2RAD * joint_angles[0];
    float theta2 = DEG2RAD * joint_angles[1];
    float L1 = _DH[1][2];
    float L2 = _DH[2][2];

    float det = L1 * L2 * sin(theta2); // Simplificación de la determinante jacobiano
    return fabs(det) < 1e-3;           // Si está cerca de cero, estás cerca de singularidad
}
