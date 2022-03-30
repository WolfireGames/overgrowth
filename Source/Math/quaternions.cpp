//-----------------------------------------------------------------------------
//           Name: quaternions.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "quaternions.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <cmath>

quaternion normalize(const quaternion &quat){
    float magnitude;


    magnitude = sqrtf((quat.entries[0] * quat.entries[0]) + 
                      (quat.entries[1] * quat.entries[1]) + 
                      (quat.entries[2] * quat.entries[2]) + 
                      (quat.entries[3] * quat.entries[3]));

    return quaternion(quat.entries[0]/magnitude,
                      quat.entries[1]/magnitude,
                      quat.entries[2]/magnitude,
                      quat.entries[3]/magnitude);
}

quaternion Quat_Mult(const quaternion &q1, const quaternion &q2)
{
        quaternion QResult;
        float a, b, c, d, e, f, g, h;
        a = (q1.entries[3] + q1.entries[0]) * (q2.entries[3] + q2.entries[0]);
        b = (q1.entries[2] - q1.entries[1]) * (q2.entries[1] - q2.entries[2]);
        c = (q1.entries[3] - q1.entries[0]) * (q2.entries[1] + q2.entries[2]);
        d = (q1.entries[1] + q1.entries[2]) * (q2.entries[3] - q2.entries[0]);
        e = (q1.entries[0] + q1.entries[2]) * (q2.entries[0] + q2.entries[1]);
        f = (q1.entries[0] - q1.entries[2]) * (q2.entries[0] - q2.entries[1]);
        g = (q1.entries[3] + q1.entries[1]) * (q2.entries[3] - q2.entries[2]);
        h = (q1.entries[3] - q1.entries[1]) * (q2.entries[3] + q2.entries[2]);
        QResult.entries[0] = a - (e + f + g + h) / 2;
        QResult.entries[1] = c + (e - f + g - h) / 2;
        QResult.entries[2] = d + (e - f - g + h) / 2;
        QResult.entries[3] = b + (-e - f + g + h) / 2;
        return QResult;
}

quaternion QuaternionMultiply(const quaternion * quat1, const quaternion * quat2) {
    quaternion result;
    
    result.entries[0] = (quat1->entries[0]*quat2->entries[3] + quat2->entries[0]*quat1->entries[3] + quat1->entries[1] * quat2->entries[2] - quat1->entries[2] * quat2->entries[1]);
    result.entries[1] = (quat1->entries[1]*quat2->entries[3] + quat2->entries[1]*quat1->entries[3] + quat1->entries[2] * quat2->entries[0] - quat1->entries[0] * quat2->entries[2]);
    result.entries[2] = (quat1->entries[2]*quat2->entries[3] + quat2->entries[2]*quat1->entries[3] + quat1->entries[0] * quat2->entries[1] - quat1->entries[1] * quat2->entries[0]);
    result.entries[3] = (quat1->entries[3]*quat2->entries[3]) - (quat1->entries[0]*quat2->entries[0]+quat1->entries[1]*quat2->entries[1]+quat1->entries[2]*quat2->entries[2]);

    return result;
}


quaternion QuaternionFromAxisAngle(vec3 axis, float angle) {
    quaternion result;

    float sinAngle;

    angle *= 0.5f;
    axis = normalize(axis);
    sinAngle = sinf(angle);
    result.entries[0] = (axis.entries[0] * sinAngle);
    result.entries[1] = (axis.entries[1] * sinAngle);
    result.entries[2] = (axis.entries[2] * sinAngle);
    result.entries[3] = cosf(angle);

    return result;
}

void QuaternionNormalize(quaternion * quat) {

    float magnitude;

    magnitude = sqrtf((quat->entries[0] * quat->entries[0]) + (quat->entries[1] * quat->entries[1]) + (quat->entries[2] * quat->entries[2]) + (quat->entries[3] * quat->entries[3]));
    quat->entries[0] /= magnitude;
    quat->entries[1] /= magnitude;
    quat->entries[2] /= magnitude;
    quat->entries[3] /= magnitude;
}

void QuaternionToAxisAngle(quaternion quat, vec3 * axis, float * angle) {
    float sinAngle;

    QuaternionNormalize(&quat);
    sinAngle = sqrtf(1.0f - (quat.entries[3] * quat.entries[3]));
    if (fabs(sinAngle) < 0.0005f) sinAngle = 1.0f;
    axis->entries[0] = (quat.entries[0] / sinAngle);
    axis->entries[1] = (quat.entries[1] / sinAngle);
    axis->entries[2] = (quat.entries[2] / sinAngle);
    *angle = (acosf(quat.entries[3]) * 2.0f);
}


void QuaternionRotate(quaternion * quat, vec3 axis, float angle) {
    quaternion rotationQuat;
    rotationQuat = QuaternionFromAxisAngle(axis, angle);
    *quat = QuaternionMultiply(quat, &rotationQuat);
}

void QuaternionInvert(quaternion * quat) {
    float length;
    length = (1.0f / ((quat->entries[0] * quat->entries[0]) +
                      (quat->entries[1] * quat->entries[1]) +
                      (quat->entries[2] * quat->entries[2]) +
                      (quat->entries[3] * quat->entries[3])));
    quat->entries[0] *= -length;
    quat->entries[1] *= -length;
    quat->entries[2] *= -length;
    quat->entries[3] *= length;
}

quaternion invert(const quaternion &quat){
    quaternion inverted;
    inverted.entries[0] = -quat.entries[0];
    inverted.entries[1] = -quat.entries[1];
    inverted.entries[2] = -quat.entries[2];
    inverted.entries[3] = quat.entries[3];
    //QuaternionInvert(&inverted);
    return inverted;
}

quaternion invert_by_val(quaternion quat){
    quaternion inverted;
    inverted.entries[0] = -quat.entries[0];
    inverted.entries[1] = -quat.entries[1];
    inverted.entries[2] = -quat.entries[2];
    inverted.entries[3] = quat.entries[3];
    return inverted;
}


void QuaternionMultiply(const vec3 * quat1, const quaternion * quat2, quaternion * result) {
    result->entries[0] = (quat1->entries[0]*quat2->entries[3] + quat1->entries[1] * quat2->entries[2] - quat1->entries[2] * quat2->entries[1]);
    result->entries[1] = (quat1->entries[1]*quat2->entries[3] + quat1->entries[2] * quat2->entries[0] - quat1->entries[0] * quat2->entries[2]);
    result->entries[2] = (quat1->entries[2]*quat2->entries[3] + quat1->entries[0] * quat2->entries[1] - quat1->entries[1] * quat2->entries[0]);
    result->entries[3] = (quat1->entries[0]*quat2->entries[0]+quat1->entries[1]*quat2->entries[1]+quat1->entries[2]*quat2->entries[2])*-1;
}

void QuaternionMultiply(const quaternion * quat1, const quaternion * quat2, quaternion * result) {
    result->entries[0] = (quat1->entries[0]*quat2->entries[3] + quat2->entries[0]*quat1->entries[3] + quat1->entries[1] * quat2->entries[2] - quat1->entries[2] * quat2->entries[1]);
    result->entries[1] = (quat1->entries[1]*quat2->entries[3] + quat2->entries[1]*quat1->entries[3] + quat1->entries[2] * quat2->entries[0] - quat1->entries[0] * quat2->entries[2]);
    result->entries[2] = (quat1->entries[2]*quat2->entries[3] + quat2->entries[2]*quat1->entries[3] + quat1->entries[0] * quat2->entries[1] - quat1->entries[1] * quat2->entries[0]);
    result->entries[3] = (quat1->entries[3] * quat2->entries[3]) - (quat1->entries[0]*quat2->entries[0]+quat1->entries[1]*quat2->entries[1]+quat1->entries[2]*quat2->entries[2]);
}


void QuaternionMultiplyVector(const quaternion * quat, vec3 * vector) {
    quaternion inverseQuat, resultQuat;
    
    inverseQuat = *quat;
    QuaternionInvert(&inverseQuat);
    QuaternionMultiply(vector, &inverseQuat, &resultQuat);
    resultQuat = QuaternionMultiply(quat, &resultQuat);

    vector->entries[0] = resultQuat.entries[0];
    vector->entries[1] = resultQuat.entries[1];
    vector->entries[2] = resultQuat.entries[2];
}

void QuaternionMultiplyVector(quaternion * quat, quaternion * inverseQuat, vec3 * vector) {
    quaternion resultQuat;
    
    QuaternionMultiply(vector, inverseQuat, &resultQuat);
    resultQuat = QuaternionMultiply(quat, &resultQuat);

    vector->entries[0] = resultQuat.entries[0];
    vector->entries[1] = resultQuat.entries[1];
    vector->entries[2] = resultQuat.entries[2];
}

void QuaternionMultiplyVector(quaternion * quat, quaternion * inverseQuat, quaternion * resultQuat, vec3 * vector) {
    QuaternionMultiply(vector, inverseQuat, resultQuat);
    *resultQuat = QuaternionMultiply(quat, resultQuat);

    vector->entries[0] = resultQuat->entries[0];
    vector->entries[1] = resultQuat->entries[1];
    vector->entries[2] = resultQuat->entries[2];
}

quaternion::quaternion(vec4 Ang_Ax) {
    // From the Quaternion Powers article on gamedev.net
    float sin_angle = sinf(Ang_Ax.entries[3] / 2);
    entries[0] = Ang_Ax.entries[0] * sin_angle;
    entries[1] = Ang_Ax.entries[1] * sin_angle;
    entries[2] = Ang_Ax.entries[2] * sin_angle;
    entries[3] = cosf(Ang_Ax.entries[3] / 2);
}

float Length2(const quaternion& quat) {
    return 
          quat.entries[0] * quat.entries[0]
        + quat.entries[1] * quat.entries[1]
        + quat.entries[2] * quat.entries[2]
        + quat.entries[3] * quat.entries[3];
}

vec4 Quat_2_AA(quaternion Quat)
{
        vec4 Ang_Ax;
        float scale, tw;
        if(Quat.entries[3] >= 1.0f){
            Ang_Ax.entries[0]=1.0f; Ang_Ax.entries[1] = 0.0f; Ang_Ax.entries[2] = 0.0f; Ang_Ax.angle() = 0.0f;
            return Ang_Ax;
        }
        tw = (float)acos(Quat.entries[3]) * 2;
        scale = (float)sin(tw / 2.0f);
        Ang_Ax.entries[0] = Quat.entries[0] / scale;
        Ang_Ax.entries[1] = Quat.entries[1] / scale;
        Ang_Ax.entries[2] = Quat.entries[2] / scale;
        
        Ang_Ax.angle() = 2.0f * acosf(Quat.entries[3]);///(float)PI*180;
        return Ang_Ax;
}

quaternion::quaternion(bool In_Degrees, vec3 Euler)
{
        // From the gamasutra quaternion article
        quaternion Quat;
        float cr, cp, cy, sr, sp, sy, cpcy, spsy;
        //If we are in Degree mode, convert to Radians
        if (In_Degrees) {
                Euler.entries[0] = Euler.entries[0] * (float)PI / 180;
                Euler.entries[1] = Euler.entries[1] * (float)PI / 180;
                Euler.entries[2] = Euler.entries[2] * (float)PI / 180;
        }
        //Calculate trig identities
        //Formerly roll, pitch, yaw
        cr = float(cos(Euler.entries[0]/2));
        cp = float(cos(Euler.entries[1]/2));
        cy = float(cos(Euler.entries[2]/2));
        sr = float(sin(Euler.entries[0]/2));
        sp = float(sin(Euler.entries[1]/2));
        sy = float(sin(Euler.entries[2]/2));

        
        cpcy = cp * cy;
        spsy = sp * sy;
        entries[0] = sr * cpcy - cr * spsy;
        entries[1] = cr * sp * cy + sr * cp * sy;
        entries[2] = cr * cp * sy - sr * sp * cy;
        entries[3] = cr * cpcy + sr * spsy;
}

quaternion::quaternion()
{
    entries[0] = 0.0f;
    entries[1] = 0.0f;
    entries[2] = 0.0f;
    entries[3] = 1.0f;
}

quaternion::quaternion( float x, float y, float z, float w )
{
    entries[0] = x;
    entries[1] = y;
    entries[2] = z;
    entries[3] = w;
}

quaternion::quaternion( const quaternion &other )
{
    entries[0] = other.entries[0];
    entries[1] = other.entries[1];
    entries[2] = other.entries[2];
    entries[3] = other.entries[3];
}

quaternion& quaternion::operator+=( const quaternion &b )
{
    (*this) = (*this) + b;
    return (*this);
}

quaternion QNormalize(quaternion Quat)
{
        float norm;
        norm =  Quat.entries[0] * Quat.entries[0] + 
                Quat.entries[1] * Quat.entries[1] + 
                Quat.entries[2] * Quat.entries[2] + 
                Quat.entries[3] * Quat.entries[3];
        Quat.entries[0] = float(Quat.entries[0] / norm);
        Quat.entries[1] = float(Quat.entries[1] / norm);
        Quat.entries[2] = float(Quat.entries[2] / norm);
        Quat.entries[3] = float(Quat.entries[3] / norm);
        return Quat;
}

vec3 Quat2Vector(quaternion Quat)
{
    QNormalize(Quat);

    float fW = Quat.entries[3];
    float fX = Quat.entries[0];
    float fY = Quat.entries[1];
    float fZ = Quat.entries[2];
    
    vec3 tempvec;

    tempvec.entries[0] = 2.0f*(fX*fZ-fW*fY);
    tempvec.entries[1] = 2.0f*(fY*fZ+fW*fX);
    tempvec.entries[2] = 1.0f-2.0f*(fX*fX+fY*fY);
    
    return tempvec;
}

#define DELTA 0.01f
quaternion Slerp(quaternion start, quaternion end, float t) {
    quaternion res;

    float     to1[4];
    float    omega, cosom, sinom, scale0, scale1;

    quaternion *from = &start;
    quaternion *to = &end;

    // calc cosine
    cosom = from->entries[0] * to->entries[0] + from->entries[1] * to->entries[1] + from->entries[2] * to->entries[2]
        + from->entries[3] * to->entries[3];

    // adjust signs (if necessary)
    if ( cosom <0.0f ){ 
        cosom = -cosom; 
        to1[0] = - to->entries[0];
        to1[1] = - to->entries[1];
        to1[2] = - to->entries[2];
        to1[3] = - to->entries[3];
    } else  {
        to1[0] = to->entries[0];
        to1[1] = to->entries[1];
        to1[2] = to->entries[2];
        to1[3] = to->entries[3];
    }

    // calculate coefficients
    if ( (1.0f - cosom) > DELTA ) {
        // standard case (slerp)
        omega = acosf(cosom);
        sinom = sinf(omega);
        scale0 = sinf((1.0f - t) * omega) / sinom;
        scale1 = sinf(t * omega) / sinom;
    } else {        
        // "from" and "to" quaternions are very close 
        //  ... so we can do a linear interpolation
        scale0 = 1.0f - t;
        scale1 = t;
    }
    // calculate final values
    res.entries[0] = scale0 * from->entries[0] + scale1 * to1[0];
    res.entries[1] = scale0 * from->entries[1] + scale1 * to1[1];
    res.entries[2] = scale0 * from->entries[2] + scale1 * to1[2];
    res.entries[3] = scale0 * from->entries[3] + scale1 * to1[3];
    
    return res;
}

quaternion QuatScale(quaternion start, float alpha) {
    quaternion neutral;
    neutral.entries[0] = 0.0f;
    neutral.entries[1] = 0.0f;
    neutral.entries[2] = 0.0f;
    neutral.entries[3] = 1.0f;
    
    return Slerp(neutral,start,alpha);
}

// Adapted from a paper by ID Software 
// http://cache-www.intel.com/cd/00/00/29/37/293748_293748.pdf
quaternion QuaternionFromMat4( const mat4 &R )
{
    const float *m = R.entries;
    quaternion quat;

    if ( m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f ) {
        float t = + m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f;
        float s = sqrtf( t ) * 0.5f;
        quat.entries[3] = s * t;
        quat.entries[2] = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s;
        quat.entries[1] = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s;
        quat.entries[0] = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s;
    } else if ( m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2] ) {
        float t = + m[0 * 4 + 0] - m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f;
        float s = sqrtf( t ) * 0.5f;
        quat.entries[0] = s * t;
        quat.entries[1] = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s;
        quat.entries[2] = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s;
        quat.entries[3] = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s;
    } else if ( m[1 * 4 + 1] > m[2 * 4 + 2] ) {
        float t = - m[0 * 4 + 0] + m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f;
        float s = sqrtf( t ) * 0.5f;
        quat.entries[1] = s * t;
        quat.entries[0] = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s;
        quat.entries[3] = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s;
        quat.entries[2] = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s;
    } else {
        float t = - m[0 * 4 + 0] - m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f;
        float s = sqrtf( t ) * 0.5f;
        quat.entries[2] = s * t;
        quat.entries[3] = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s;
        quat.entries[0] = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s;
        quat.entries[1] = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s;
    }

    QuaternionNormalize(&quat);

    return quat;
}

mat4 Mat4FromQuaternion( const quaternion &q )
{
    mat4 matrix;
    float *m = matrix.entries;
    float x2 = q.entries[0] + q.entries[0];
    float y2 = q.entries[1] + q.entries[1];
    float z2 = q.entries[2] + q.entries[2];
    {
        float xx2 = q.entries[0] * x2;
        float yy2 = q.entries[1] * y2;
        float zz2 = q.entries[2] * z2;
        m[0*4+0] = 1.0f - yy2 - zz2;
        m[1*4+1] = 1.0f - xx2 - zz2;
        m[2*4+2] = 1.0f - xx2 - yy2;
    }
    {
        float yz2 = q.entries[1] * z2;
        float wx2 = q.entries[3] * x2;
        m[2*4+1] = yz2 - wx2;
        m[1*4+2] = yz2 + wx2;
    }
    {
        float xy2 = q.entries[0] * y2;
        float wz2 = q.entries[3] * z2;
        m[1*4+0] = xy2 - wz2;
        m[0*4+1] = xy2 + wz2;
    }
    {
        float xz2 = q.entries[0] * z2;
        float wy2 = q.entries[3] * y2;
        m[0*4+2] = xz2 - wy2;
        m[2*4+0] = xz2 + wy2;
    }
    return matrix;
}

const quaternion operator*( const quaternion &a, const quaternion &b )
{
    return Quat_Mult(a,b);
}

const vec3 operator*( const quaternion &a, const vec3 &b )
{
    vec3 result = b;
    QuaternionMultiplyVector(&a, &result);
    return result;
}

const quaternion operator*( const quaternion &a, float b )
{
    return quaternion(a.entries[0] * b,
                      a.entries[1] * b,
                      a.entries[2] * b,
                      a.entries[3] * b);
}

vec3 ASMult( quaternion a, vec3 b )
{
    vec3 result = b;
    QuaternionMultiplyVector(&a, &result);
    return result;
}

const quaternion operator+( const quaternion &a, const quaternion &b )
{
    return quaternion(a.entries[0] + b.entries[0],
                      a.entries[1] + b.entries[1],
                      a.entries[2] + b.entries[2],
                      a.entries[3] + b.entries[3]);
}

float dot( const quaternion &a, const quaternion &b )
{
    return a.entries[0] * b.entries[0] +
           a.entries[1] * b.entries[1] +
           a.entries[2] * b.entries[2] +
           a.entries[3] * b.entries[3];
}

mat3 Mat3FromQuaternion( const quaternion &quat ) {
    mat3 matrix;
    float *m = matrix.entries;
    const float *q = quat.entries;
    float x2 = q[0] + q[0];
    float y2 = q[1] + q[1];
    float z2 = q[2] + q[2];
    float xx2 = q[0] * x2;
    float yy2 = q[1] * y2;
    float zz2 = q[2] * z2;
    m[0*3+0] = 1.0f - yy2 - zz2;
    m[1*3+1] = 1.0f - xx2 - zz2;
    m[2*3+2] = 1.0f - xx2 - yy2;
    float yz2 = q[1] * z2;
    float wx2 = q[3] * x2;
    m[2*3+1] = yz2 - wx2;
    m[1*3+2] = yz2 + wx2;
    float xy2 = q[0] * y2;
    float wz2 = q[3] * z2;
    m[1*3+0] = xy2 - wz2;
    m[0*3+1] = xy2 + wz2;
    float xz2 = q[0] * z2;
    float wy2 = q[3] * y2;
    m[0*3+2] = xz2 - wy2;
    m[2*3+0] = xz2 + wy2;
    return matrix;
}

bool operator!=(const quaternion &a, const quaternion &b) {
    for(int i=0; i<4; ++i){
        if(a[i] != b[i]){
            return true;
        }
    }
    return false;
}

bool operator==(const quaternion &a, const quaternion &b) {
    return !(a != b);
}

// Funcion assumes xyzw order!
vec3 QuaternionToEuler(const quaternion& quat) {
    vec3 euler_angles;
    quaternion q(quat[3], quat[0], quat[1], quat[2]);
    euler_angles[0] = atan2(2*(q[0]*q[1] + q[2]*q[3]), 1 - 2*(q[1]*q[1] + q[2]*q[2]));
    float sinval = 2*(q[0]*q[2] - q[3]*q[1]);
    if(fabs(sinval) >= 1.0f)
        if(sinval >= 0.0f)
            euler_angles[1] = 3.14159266f / 2.0f;
        else
            euler_angles[1] = 3.14159266f / -2.0f;
    else
        euler_angles[1] = asin(2*(q[0]*q[2] - q[3]*q[1]));
    euler_angles[2] = atan2(2*(q[0]*q[3] + q[1]*q[2]), 1 - 2*(q[2]*q[2] + q[3]*q[3]));
    return euler_angles;
}

quaternion EulerToQuaternion(const vec3& euler) {
    quaternion q;
    vec3 eu = euler * 0.5f; // makes conversion simpler
    q[3] = cos(eu[0])*cos(eu[1])*cos(eu[2]) +
        sin(eu[0])*sin(eu[1])*sin(eu[2]);
    q[0] = sin(eu[0])*cos(eu[1])*cos(eu[2]) -
        cos(eu[0])*sin(eu[1])*sin(eu[2]);
    q[1] = cos(eu[0])*sin(eu[1])*cos(eu[2]) +
        sin(eu[0])*cos(eu[1])*sin(eu[2]);
    q[2] = cos(eu[0])*cos(eu[1])*sin(eu[2]) -
        sin(eu[0])*sin(eu[1])*cos(eu[2]);
    return q;
}
