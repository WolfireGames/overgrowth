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

        vec3 offset1(radius*(cos(rotation1)-sin(rotation1)), 0.0f, radius*(sin(rotation1)+cos(rotation1)));
        vec3 pointTop1 = offset1+centerTop;
        vec3 pointBottom1 = offset1+centerBottom;

        vec3 offset2(radius*(cos(rotation2)-sin(rotation2)), 0.0f, radius*(sin(rotation2)+cos(rotation2)));
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

/*
 * Draws a solid sphere
 */
void renderSolidSphere(GLdouble radius, GLint slices, GLint stacks)
{
  int i,j;
  
  /* Adjust z and radius as stacks are drawn. */
  
  double z0,z1;
  double r0,r1;
  
  /* Pre-computed circle */
  
  double *sint1,*cost1;
  double *sint2,*cost2;
  circleTable(&sint1,&cost1,-slices);
  circleTable(&sint2,&cost2,stacks*2);
  
  /* The top stack is covered with a triangle fan */
  
  z0 = 1.0f;
  z1 = cost2[1];
  r0 = 0.0f;
  r1 = sint2[1];
  
  glBegin(GL_TRIANGLE_FAN);
  
  glNormal3d(0,0,1);
  glVertex3d(0,0,radius);
  
  for (j=slices; j>=0; j--)
  {       
    glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
    glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);
  }
  
  glEnd();
  
  /* Cover each stack with a quad strip, except the top and bottom stacks */
  
  for( i=1; i<stacks-1; i++ )
  {
    z0 = z1; z1 = cost2[i+1];
    r0 = r1; r1 = sint2[i+1];
    
    glBegin(GL_QUAD_STRIP);
    
    for(j=0; j<=slices; j++)
    {
      glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
      glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);
      glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
      glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
    }
    
    glEnd();
  }
  
  /* The bottom stack is covered with a triangle fan */
  
  z0 = z1;
  r0 = r1;
  
  glBegin(GL_TRIANGLE_FAN);
  
  glNormal3d(0,0,-1);
  glVertex3d(0,0,-radius);
  
  for (j=0; j<=slices; j++)
  {
    glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
    glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
  }
  
  glEnd();
  
  /* Release sin and cos tables */
  
  OG_FREE(sint1);
  OG_FREE(cost1);
  OG_FREE(sint2);
  OG_FREE(cost2);
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

/*
 * Draws a solid cylinder
 */
void renderSolidCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks)
{
  int i,j;
  
  /* Step in z and radius as stacks are drawn. */
  
  
  double z0,z1;
  const double zStep = height/stacks;
  
  /* Pre-computed circle */
  
  double *sint,*cost;
  circleTable(&sint,&cost,-slices);
  
  /* Cover the base and top */
  
  glBegin(GL_TRIANGLE_FAN);
  glNormal3d(0.0f, 0.0f, -1.0f );
  glVertex3d(0.0f, 0.0f,  -height/2 );
  for (j=0; j<=slices; j++)
    glVertex3d(cost[j]*radius, sint[j]*radius, -height/2);
  glEnd();
  
  glBegin(GL_TRIANGLE_FAN);
  glNormal3d(0.0f, 0.0f, 1.0f   );
  glVertex3d(0.0f, 0.0f, height/2);
  for (j=slices; j>=0; j--)
    glVertex3d(cost[j]*radius, sint[j]*radius, height/2);
  glEnd();
  
  /* Do the stacks */
  
  z0 = -height/2;
  z1 = zStep-height/2;
  
  for (i=1; i<=stacks; i++)
  {
    if (i==stacks)
      z1 = height/2;
    
    glBegin(GL_QUAD_STRIP);
    for (j=0; j<=slices; j++ )
    {
      glNormal3d(cost[j],        sint[j],        0.0f );
      glVertex3d(cost[j]*radius, sint[j]*radius, z0  );
      glVertex3d(cost[j]*radius, sint[j]*radius, z1  );
    }
    glEnd();
    
    z0 = z1; z1 += zStep;
  }
  
  /* Release sin and cos tables */
  
  OG_FREE(sint);
  OG_FREE(cost);
}

/*
 * Draws a wire cylinder
 */
void renderWireCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks)
{
  int i,j;
  
  /* Step in z and radius as stacks are drawn. */
  
  double z = 0.0f;
  const double zStep = height/stacks;
  
  /* Pre-computed circle */
  
  double *sint,*cost;
  circleTable(&sint,&cost,-slices);
  
  /* Draw the stacks... */
  
  for (i=0; i<=stacks; i++)
  {
    if (i==stacks)
      z = height;
    
    glBegin(GL_LINE_LOOP);    
    for( j=0; j<slices; j++ ) {
      glNormal3d(cost[j],        sint[j],        0.0f);
      glVertex3d(cost[j]*radius, sint[j]*radius, z  );
    }    
    glEnd();
    
    z += zStep;
  }
  
  /* Draw the slices */
  
  glBegin(GL_LINES);
  
  for (j=0; j<slices; j++)
  {
    glNormal3d(cost[j],        sint[j],        0.0f   );
    glVertex3d(cost[j]*radius, sint[j]*radius, 0.0f   );
    glVertex3d(cost[j]*radius, sint[j]*radius, height);
  }
  
  glEnd();
  
  /* Release sin and cos tables */
  
  OG_FREE(sint);
  OG_FREE(cost);
}


#undef num_faces


#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "opengl.h"

GLuint GetBoxDisplayList( const vec3 &sides )
{
    GLuint display_list = glGenLists(1);
    glNewList(display_list,GL_COMPILE);
    glBegin(GL_QUADS);
        glNormal3f(0,-1,0);
        glVertex3f(-sides[0]*0.5f,-sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f(-sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);

        glNormal3f(0,1,0);
        glVertex3f(-sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f(-sides[0]*0.5f, sides[1]*0.5f,-sides[2]*0.5f);

        glNormal3f(1,0,0);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);

        glNormal3f(-1,0,0);
        glVertex3f( -sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( -sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( -sides[0]*0.5f, sides[1]*0.5f,-sides[2]*0.5f);
        glVertex3f( -sides[0]*0.5f,-sides[1]*0.5f,-sides[2]*0.5f);

        glNormal3f(0,0,-1);
        glVertex3f(-sides[0]*0.5f,-sides[1]*0.5f, -sides[2]*0.5f);
        glVertex3f(-sides[0]*0.5f, sides[1]*0.5f, -sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f, -sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f, -sides[2]*0.5f);

        glNormal3f(0,0,1);
        glVertex3f( sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f( sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f(-sides[0]*0.5f, sides[1]*0.5f, sides[2]*0.5f);
        glVertex3f(-sides[0]*0.5f,-sides[1]*0.5f, sides[2]*0.5f);
    glEnd();
    glEndList();

    return display_list;
}



GLuint GetMultiSphereDisplayList( const vec3 *positions, const float *radii, int num_spheres )
{
    CHECK_GL_ERROR();
    GLuint display_list = glGenLists(1);
    CHECK_GL_ERROR();
    glNewList(display_list,GL_COMPILE);
    CHECK_GL_ERROR();

    for(int i=0; i<num_spheres; i++){
        glPushMatrix();
            glTranslatef(positions[i][0], positions[i][1], positions[i][2]);
            renderSolidSphere(radii[i],12,12);
        glPopMatrix();
    }
    CHECK_GL_ERROR();
    for(int i=0; i<num_spheres; i++){
        for(int j=i+1; j<num_spheres; j++){
            vec3 connection = normalize(positions[j]-positions[i]);
            vec3 right;// = normalize(vec3(1.0f,1.0f,1.0f));
            vec3 up;// = normalize(cross(right,connection));
            //right = normalize(cross(up,connection));
    
            PlaneSpace(connection, right, up);

            vec3 avg_pos = (positions[j]+positions[i])*0.5f;
            
            mat4 transform;
            transform.SetColumn(0,right);
            transform.SetColumn(1,up);
            transform.SetColumn(2,connection);
            transform.SetColumn(3,avg_pos);

            glPushMatrix();
                glMultMatrixf(transform);
                renderSolidCylinder(radii[i],
                                    distance(positions[i], positions[j]),
                                    12,12);
            glPopMatrix();
        }
    }
    CHECK_GL_ERROR();
    glEndList();
    return display_list;
}

GLuint GetWireCylinderDisplayList( float radius, float height, GLint slices, GLint stacks )
{
    GLuint display_list = glGenLists(1);
    glNewList(display_list,GL_COMPILE);
    glPushMatrix();
    glTranslatef(0.0f,height*0.5f,0.0f);
    glRotatef(90,1,0,0);
    renderWireCylinder(radius,height,slices,stacks);
    glPopMatrix();
    glEndList();
    return display_list;
}

GLuint GetSphereDisplayList( float radius )
{
    GLuint display_list = glGenLists(1);
    glNewList(display_list,GL_COMPILE);
    glPushMatrix();
    renderSolidSphere(radius,12,12);
    glPopMatrix();
    glEndList();
    return display_list;
}
