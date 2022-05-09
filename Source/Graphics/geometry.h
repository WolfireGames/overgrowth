//-----------------------------------------------------------------------------
//           Name: geometry.h
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
// ---------------------------------------------------------------------------
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
#pragma once

#include <stdlib.h>
#include <cmath>
#include "Math/enginemath.h"
#include "opengl.h"
#include <vector>

/* -- INTERFACE FUNCTION PROTOTYPES -------------------------------------------------- */

void GetWireCylinderVertArray(GLint slices, std::vector<vec3> &data);
void GetWireSphereVertArray(GLdouble radius, GLint slices, GLint stacks, std::vector<float> &data);
void GetWireBoxVertArray(std::vector<vec3> &data);
void GetUnitBoxVertArray(std::vector<float> &verts, std::vector<unsigned> &faces);
void GetUnitBoxVertArray(std::vector<float> &verts);
