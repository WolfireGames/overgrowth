//-----------------------------------------------------------------------------
//           Name: geometry.cpp
//         Author: Freeglut
//        License: X-Consortium license
//    Description: Data and utility functions for rendering several useful 
//                 geometric shapes. This code is a modified version of the 
//                 code found in "freeglut_teapot.c" and "freeglut_geometry.c", 
//                 which is part of the open source project, Freeglut.
//                 http://freeglut.sourceforge.net/
//               
//                 Modified by Wolfire Games LLC for use in the project 
//                 Overgrowth.
//
// ----------------------------------------------------------------------------
// Freeglut Copyright
// ------------------
// 
// Freeglut code without an explicit copyright is covered by the following
// copyright :
// 
// Copyright(c) 1999 - 2000 Pawel W.Olszta.All Rights Reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies or substantial portions of the Software.
// 
// The above  copyright notice and this permission notice  shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE  IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
// FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.IN  NO EVENT  SHALL
// PAWEL W.OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN  AN ACTION  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF  OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of Pawel W.Olszta shall not be
// used  in advertising or otherwise to promote the sale, use or other dealings
// in this Software without prior written authorization from Pawel W.Olszta.
// ---------------------------------------------------------------------------
/*
 * freeglut_geometry.c
 *
 * Freeglut geometry rendering methods.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Fri Dec 3 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * freeglut_teapot.c
 *
 * Teapot(tm) rendering code.
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Fri Dec 24 1999
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Original teapot code copyright follows:
 */
/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 *
 * ALL RIGHTS RESERVED
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that
 * both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Silicon
 * Graphics, Inc. not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
 * "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
 * OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
 * EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
 * ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
 * INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
 * SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
 * NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 *
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227f.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer
 * Software clause at DFARS 252.227f-7013 and/or in similar or
 * successor clauses in the FAR or the DOD or NASA FAR
 * Supplement.  Unpublished-- rights reserved under the copyright
 * laws of the United States.  Contractor/manufacturer is Silicon
 * Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
 * 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
//
// The following functions are defined here:
//
// void renderWireTeapot(GLdouble size);
// void renderSolidTeapot(GLdouble size);
// void renderWireCube(GLdouble size);
// void renderSolidCube(GLdouble size);
// void renderWireSphere(GLdouble radius, GLint slices, GLint stacks);
// void renderSolidSphere(GLdouble radius, GLint slices, GLint stacks);
// void renderWireCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);
// void renderSolidCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);
// void renderWireCylinder(GLdouble base, GLdouble height, GLint slices, GLint stacks);
// void renderSolidCyliner(GLdouble base, GLdouble height, GLint slices, GLint stacks);
// void renderWireTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
// void renderSolidTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
// void renderWireDodecahedron(void);
// void renderSolidDodecahedron(void);
// void renderWireOctahedron(void);
// void renderSolidOctahedron(void);
// void renderWireTetrahedron(void);
// void renderSolidTetrahedron(void);
// void renderWireIcosahedron(void);
// void renderSolidIcosahedron(void);
// void renderWireSierpinskiSponge(int num_levels, GLdouble offset[3], GLdouble scale);
// void renderSolidSierpinskiSponge(int num_levels, GLdouble offset[3], GLdouble scale);
//-----------------------------------------------------------------------------

#include <Graphics/geometry.h>
#include <Graphics/graphics.h>

#include <Math/vec3math.h>
#include <Memory/allocation.h>

#include <cmath>

static void circleTable(double **sint,double **cost,const int n);

/* -- INTERFACE FUNCTIONS -------------------------------------------------- */

/*
 * Compute lookup table of cos and sin values forming a cirle
 *
 * Notes:
 *    It is the responsibility of the caller to free these tables
 *    The size of the table is (n+1) to form a connected loop
 *    The last entry is exactly the same as the first
 *    The sign of n can be flipped to get the reverse loop
 */

static void circleTable(double **sint,double **cost,const int n)
{
  int i;
  
  /* Table size, the sign of n flips the circle direction */
  
  const int size = abs(n);
  
  /* Determine the angle between samples */
  
  const double angle = 2*PI/(double)n;
  
  /* Allocate memory for n samples, plus duplicate of first entry at the end */
  
  *sint = (double *) calloc(sizeof(double), size+1);
  *cost = (double *) calloc(sizeof(double), size+1);
  
  /* Bail out if memory allocation fails, fgError never returns */
  
  if (!(*sint) || !(*cost))
  {
    OG_FREE(*sint);
    *sint = NULL;
    OG_FREE(*cost);
    *cost = NULL;
    FatalError("Error", "Failed to allocate memory in circleTable");
  }
  
  /* Compute cos and sin around the circle */
  
  for (i=0; i<size; i++)
  {
    (*sint)[i] = sin(angle*i);
    (*cost)[i] = cos(angle*i);
  }
  
  /* Last sample is duplicate of the first */
  
  (*sint)[size] = (*sint)[0];
  (*cost)[size] = (*cost)[0];
}

void GetWireCylinderVertArray(GLint slices, std::vector<vec3> &data)
{
    // x right, y up, z forward
    float rotationStep = (2.0f*PI_f)/(float)slices;
    float radius = 0.5f;
    float halfheight = 0.5f;

    vec3 centerTop( 0,halfheight,0 );
    vec3 centerBottom( 0, -halfheight,0);
   
    data.reserve( slices * 2 * 10 );

    for( int i = 0; i < slices; i++ )
    {
        float rotation1 = i*rotationStep;
        float rotation2 = (i+1)*rotationStep;

        vec3 offset1(radius*(std::cos(rotation1)-std::sin(rotation1)), 0.0f, radius*(std::sin(rotation1)+std::cos(rotation1)));
        vec3 pointTop1 = offset1+centerTop;
        vec3 pointBottom1 = offset1+centerBottom;

        vec3 offset2(radius*(std::cos(rotation2)-std::sin(rotation2)), 0.0f, radius*(std::sin(rotation2)+std::cos(rotation2)));
        vec3 pointTop2 = offset2+centerTop;
        vec3 pointBottom2 = offset2+centerBottom;

        data.push_back( centerTop );
        data.push_back( pointTop1 );

        data.push_back( pointTop1 );
        data.push_back( pointTop2 );

        data.push_back( pointTop1 );
        data.push_back( pointBottom1 );

        data.push_back( pointBottom1 );
        data.push_back( pointBottom2 );

        data.push_back( pointBottom1 );
        data.push_back( centerBottom );
    }      
}

void GetWireBoxVertArray( std::vector<vec3> &data )
{
    vec3 corners[] =
    {
        //Upper four corners
        vec3( 0.5f,   0.5f, 0.5f ),
        vec3( 0.5f,   0.5f, -0.5f ),
        vec3( -0.5f,  0.5f, -0.5f ),
        vec3( -0.5f,  0.5f, 0.5f ),

        //Lower four corners
        vec3( 0.5f,  -0.5f, 0.5f ),
        vec3( 0.5f,  -0.5f, -0.5f ),
        vec3( -0.5f, -0.5f, -0.5f ),
        vec3( -0.5f, -0.5f, 0.5f ),
    };

    //We start with spinning around the top and the bottom.
    for( int i = 1; i <= 4; i++ )
    {
        int prev_index = i - 1;
        int cur_index = i % 4;

        data.push_back( corners[prev_index] );
        data.push_back( corners[cur_index] );
    }

    for( int i = 1; i <= 4; i++ )
    {
        int prev_index = 4 + i - 1;
        int cur_index = 4 + i % 4;

        data.push_back( corners[prev_index] );
        data.push_back( corners[cur_index] );
    }

    //Then we match each bottom point with the one right above.
    for( int i = 0; i < 4; i++ )
    {
        data.push_back( corners[i] );
        data.push_back( corners[i+4] );
    }
}

// Get interleaved vert array (3 verts) for GL_LINES
void GetWireSphereVertArray(GLdouble radius, GLint slices, GLint stacks, std::vector<float> &data) {
    double r,x,y,z;
    double *sint1,*cost1;
    double *sint2,*cost2;
    circleTable(&sint1,&cost1,-slices  );
    circleTable(&sint2,&cost2, stacks*2);
    for (int i=1; i<stacks; i++) {
        z = cost2[i];
        r = sint2[i];
        for(int j=0; j<=slices; j++){
            x = cost1[j];
            y = sint1[j];
            //data.push_back((float)x);
            //data.push_back((float)y);
            //data.push_back((float)z);
            data.push_back((float)(x*r*radius));
            data.push_back((float)(y*r*radius));
            data.push_back((float)(z*radius));
            if(j!=0 && j!=slices){
                //data.push_back((float)x);
                //data.push_back((float)y);
                //data.push_back((float)z);
                data.push_back((float)(x*r*radius));
                data.push_back((float)(y*r*radius));
                data.push_back((float)(z*radius));
            }
        }
    }
    for (int i=0; i<slices; i++) {
        for(int j=0; j<=stacks; j++){
            x = cost1[i]*sint2[j];
            y = sint1[i]*sint2[j];
            z = cost2[j];
            //data.push_back((float)x);
            //data.push_back((float)y);
            //data.push_back((float)z);
            data.push_back((float)(x*radius));
            data.push_back((float)(y*radius));
            data.push_back((float)(z*radius));
            if(j!=0 && j!=stacks){
                //data.push_back((float)x);
                //data.push_back((float)y);
                //data.push_back((float)z);
                data.push_back((float)(x*radius));
                data.push_back((float)(y*radius));
                data.push_back((float)(z*radius));
            }
        }
    }
    OG_FREE(sint1);
    OG_FREE(cost1);
    OG_FREE(sint2);
    OG_FREE(cost2);
}

void GetUnitBoxVertArray(std::vector<float> &verts) 
{
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(-0.5f); 
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(0.5f); 
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(0.5f);
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(0.5f); 
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(-0.5f); 
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(0.5f); 
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(0.5f); 
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(-0.5f); 
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(0.5f); 
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(-0.5f); verts.push_back(-0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(-0.5f);
    verts.push_back(-0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
    verts.push_back(0.5f); verts.push_back(0.5f); verts.push_back(0.5f);
}

void GetUnitBoxVertArray(std::vector<float> &verts, std::vector<unsigned> &faces) 
{

    verts.push_back(0.500000);
    verts.push_back(-0.500000);
    verts.push_back(-0.500000);

    verts.push_back(0.500000);
    verts.push_back(-0.500000);
    verts.push_back(0.500000);

    verts.push_back(-0.500000);
    verts.push_back(-0.500000);
    verts.push_back(0.500000);

    verts.push_back(-0.500000);
    verts.push_back(-0.500000);
    verts.push_back(-0.500000);

    verts.push_back(0.500000);
    verts.push_back(0.500000);
    verts.push_back(-0.500000);

    verts.push_back(0.500000);
    verts.push_back(0.500000);
    verts.push_back(0.500000);

    verts.push_back(-0.500000);
    verts.push_back(0.500000);
    verts.push_back(0.500000);

    verts.push_back(-0.500000);
    verts.push_back(0.500000);
    verts.push_back(-0.500000);

    faces.push_back(1-1);
    faces.push_back(2-1);
    faces.push_back(4-1);
    faces.push_back(5-1);
    faces.push_back(8-1);
    faces.push_back(6-1);
    faces.push_back(1-1);
    faces.push_back(5-1);
    faces.push_back(2-1);
    faces.push_back(2-1);
    faces.push_back(6-1);
    faces.push_back(3-1);
    faces.push_back(3-1);
    faces.push_back(7-1);
    faces.push_back(4-1);
    faces.push_back(5-1);
    faces.push_back(1-1);
    faces.push_back(8-1);
    faces.push_back(2-1);
    faces.push_back(3-1);
    faces.push_back(4-1);
    faces.push_back(8-1);
    faces.push_back(7-1);
    faces.push_back(6-1);
    faces.push_back(5-1);
    faces.push_back(6-1);
    faces.push_back(2-1);
    faces.push_back(6-1);
    faces.push_back(7-1);
    faces.push_back(3-1);
    faces.push_back(7-1);
    faces.push_back(8-1);
    faces.push_back(4-1);
    faces.push_back(1-1);
    faces.push_back(4-1);
    faces.push_back(8-1);
}