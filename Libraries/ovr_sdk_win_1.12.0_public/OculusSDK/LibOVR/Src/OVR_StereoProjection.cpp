/************************************************************************************

Filename    :   OVR_StereoProjection.cpp
Content     :   Stereo rendering functions
Created     :   November 30, 2013
Authors     :   Tom Fosyth

Copyright   :   Copyright 2014-2016 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.3 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.3 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#include <Extras/OVR_StereoProjection.h>


namespace OVR {


ScaleAndOffset2D CreateNDCScaleAndOffsetFromFov ( FovPort tanHalfFov )
{
    float projXScale = 2.0f / ( tanHalfFov.LeftTan + tanHalfFov.RightTan );
    float projXOffset = ( tanHalfFov.LeftTan - tanHalfFov.RightTan ) * projXScale * 0.5f;
    float projYScale = 2.0f / ( tanHalfFov.UpTan + tanHalfFov.DownTan );
    float projYOffset = ( tanHalfFov.UpTan - tanHalfFov.DownTan ) * projYScale * 0.5f;

    ScaleAndOffset2D result;
    result.Scale    = Vector2f(projXScale, projYScale);
    result.Offset   = Vector2f(projXOffset, projYOffset);
    // Hey - why is that Y.Offset negated?
    // It's because a projection matrix transforms from world coords with Y=up,
    // whereas this is from NDC which is Y=down.

    return result;
}



Matrix4f CreateProjection( bool leftHanded, bool isOpenGL, FovPort tanHalfFov, StereoEye /*eye*/, 
                           float zNear /*= 0.01f*/, float zFar /*= 10000.0f*/,
                           bool flipZ /*= false*/, bool farAtInfinity /*= false*/)
{
    if(!flipZ && farAtInfinity)
    {
        //OVR_ASSERT_M(false, "Cannot push Far Clip to Infinity when Z-order is not flipped"); Assertion disabled because this code no longer has access to LibOVRKernel assertion functionality.
        farAtInfinity = false;
    }

    // A projection matrix is very like a scaling from NDC, so we can start with that.
    ScaleAndOffset2D scaleAndOffset = CreateNDCScaleAndOffsetFromFov ( tanHalfFov );

    float handednessScale = leftHanded ? 1.0f : -1.0f;

    Matrix4f projection;
    // Produces X result, mapping clip edges to [-w,+w]
    projection.M[0][0] = scaleAndOffset.Scale.x;
    projection.M[0][1] = 0.0f;
    projection.M[0][2] = handednessScale * scaleAndOffset.Offset.x;
    projection.M[0][3] = 0.0f;

    // Produces Y result, mapping clip edges to [-w,+w]
    // Hey - why is that YOffset negated?
    // It's because a projection matrix transforms from world coords with Y=up,
    // whereas this is derived from an NDC scaling, which is Y=down.
    projection.M[1][0] = 0.0f;
    projection.M[1][1] = scaleAndOffset.Scale.y;
    projection.M[1][2] = handednessScale * -scaleAndOffset.Offset.y;
    projection.M[1][3] = 0.0f;

    // Produces Z-buffer result - app needs to fill this in with whatever Z range it wants.
    // We'll just use some defaults for now.
    projection.M[2][0] = 0.0f;
    projection.M[2][1] = 0.0f;

    if (farAtInfinity)
    {
        if (isOpenGL)
        {
            // It's not clear this makes sense for OpenGL - you don't get the same precision benefits you do in D3D.
            projection.M[2][2] = -handednessScale;
            projection.M[2][3] = 2.0f * zNear;
        }
        else
        {
            projection.M[2][2] = 0.0f;
            projection.M[2][3] = zNear;
        }
    }
    else
    {
        if (isOpenGL)
        {
            // Clip range is [-w,+w], so 0 is at the middle of the range.
            projection.M[2][2] = -handednessScale * (flipZ ? -1.0f : 1.0f) * (zNear + zFar) / (zNear - zFar);
            projection.M[2][3] =                    2.0f * ((flipZ ? -zFar : zFar) * zNear) / (zNear - zFar);
        }
        else
        {
            // Clip range is [0,+w], so 0 is at the start of the range.
            projection.M[2][2] = -handednessScale * (flipZ ? -zNear : zFar)                 / (zNear - zFar);
            projection.M[2][3] =                           ((flipZ ? -zFar : zFar) * zNear) / (zNear - zFar);
        }
    }

    // Produces W result (= Z in)
    projection.M[3][0] = 0.0f;
    projection.M[3][1] = 0.0f;
    projection.M[3][2] = handednessScale;
    projection.M[3][3] = 0.0f;

    return projection;
}


Matrix4f CreateOrthoSubProjection ( bool /*rightHanded*/, StereoEye eyeType,
                                    float tanHalfFovX, float tanHalfFovY,
                                    float unitsX, float unitsY,
                                    float distanceFromCamera, float interpupillaryDistance,
                                    Matrix4f const &projection,
                                    float zNear /*= 0.0f*/, float zFar /*= 0.0f*/,
                                    bool flipZ /*= false*/, bool farAtInfinity /*= false*/)
{
    if(!flipZ && farAtInfinity)
    {
        //OVR_ASSERT_M(false, "Cannot push Far Clip to Infinity when Z-order is not flipped");  Assertion disabled because this code no longer has access to LibOVRKernel assertion functionality.
        farAtInfinity = false;
    }

    float orthoHorizontalOffset = interpupillaryDistance * 0.5f / distanceFromCamera;
    switch ( eyeType )
    {
    case StereoEye_Left:
        break;
    case StereoEye_Right:
        orthoHorizontalOffset = -orthoHorizontalOffset;
        break;
    case StereoEye_Center:
        orthoHorizontalOffset = 0.0f;
        break;
    default: 
        break;
    }

    // Current projection maps real-world vector (x,y,1) to the RT.
    // We want to find the projection that maps the range [-FovPixels/2,FovPixels/2] to
    // the physical [-orthoHalfFov,orthoHalfFov]
    // Note moving the offset from M[0][2]+M[1][2] to M[0][3]+M[1][3] - this means
    // we don't have to feed in Z=1 all the time.
    // The horizontal offset math is a little hinky because the destination is
    // actually [-orthoHalfFov+orthoHorizontalOffset,orthoHalfFov+orthoHorizontalOffset]
    // So we need to first map [-FovPixels/2,FovPixels/2] to
    //                         [-orthoHalfFov+orthoHorizontalOffset,orthoHalfFov+orthoHorizontalOffset]:
    // x1 = x0 * orthoHalfFov/(FovPixels/2) + orthoHorizontalOffset;
    //    = x0 * 2*orthoHalfFov/FovPixels + orthoHorizontalOffset;
    // But then we need the sam mapping as the existing projection matrix, i.e.
    // x2 = x1 * Projection.M[0][0] + Projection.M[0][2];
    //    = x0 * (2*orthoHalfFov/FovPixels + orthoHorizontalOffset) * Projection.M[0][0] + Projection.M[0][2];
    //    = x0 * Projection.M[0][0]*2*orthoHalfFov/FovPixels +
    //      orthoHorizontalOffset*Projection.M[0][0] + Projection.M[0][2];
    // So in the new projection matrix we need to scale by Projection.M[0][0]*2*orthoHalfFov/FovPixels and
    // offset by orthoHorizontalOffset*Projection.M[0][0] + Projection.M[0][2].

    float orthoScaleX = 2.0f * tanHalfFovX / unitsX;
    float orthoScaleY = 2.0f * tanHalfFovY / unitsY;
    Matrix4f ortho;
    ortho.M[0][0] = projection.M[0][0] * orthoScaleX;
    ortho.M[0][1] = 0.0f;
    ortho.M[0][2] = 0.0f;
    ortho.M[0][3] = -projection.M[0][2] + ( orthoHorizontalOffset * projection.M[0][0] );

    ortho.M[1][0] = 0.0f;
    ortho.M[1][1] = -projection.M[1][1] * orthoScaleY;       // Note sign flip (text rendering uses Y=down).
    ortho.M[1][2] = 0.0f;
    ortho.M[1][3] = -projection.M[1][2];

    const float zDiff = zNear - zFar;
    if (fabsf(zDiff) < 0.001f)
    {
        ortho.M[2][0] = 0.0f;
        ortho.M[2][1] = 0.0f;
        ortho.M[2][2] = 0.0f;
        ortho.M[2][3] = flipZ ? zNear : zFar;
    }
    else
    {
        ortho.M[2][0] = 0.0f;
        ortho.M[2][1] = 0.0f;

        if(farAtInfinity)
        {
            ortho.M[2][2] = 0.0f;
            ortho.M[2][3] = zNear;
        }
        else if (zDiff != 0.0f)
        {
            ortho.M[2][2] = (flipZ ? zNear : zFar) / zDiff;
            ortho.M[2][3] = ((flipZ ? -zFar : zFar) * zNear) / zDiff;
        }
    }

    // No perspective correction for ortho.
    ortho.M[3][0] = 0.0f;
    ortho.M[3][1] = 0.0f;
    ortho.M[3][2] = 0.0f;
    ortho.M[3][3] = 1.0f;

    return ortho;
}



} //namespace OVR


