//-----------------------------------------------------------------------------
//           Name: collisiondetection.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This file handles collision detection functions
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
#include "collisiondetection.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <cmath>
#include <cstddef> //For NULL

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

bool DistancePointLine( const vec3 &point, const vec3 &line_start, const vec3 &line_end, float *the_distance, vec3 *intersection )
{
    float LineMag;
    float U;
 
    LineMag = distance( line_end, line_start );
 
    U = ( ( ( point.x() - line_start.x() ) * ( line_end.x() - line_start.x() ) ) +
        ( ( point.y() - line_start.y() ) * ( line_end.y() - line_start.y() ) ) +
        ( ( point.z() - line_start.z() ) * ( line_end.z() - line_start.z() ) ) ) /
        ( LineMag * LineMag );
 
    if( U < 0.0f || U > 1.0f ){
        if(distance_squared(point, line_start)<distance_squared(point,line_end)){
            *intersection = line_start;
            *the_distance = distance(point, line_start);
        }
        else {
            *intersection = line_end;
            *the_distance = distance(point, line_end);
        }
        return 0;   // closest point does not fall within the line segment
    }

    intersection->x() = line_start.x() + U * ( line_end.x() - line_start.x() );
    intersection->y() = line_start.y() + U * ( line_end.y() - line_start.y() );
    intersection->z() = line_start.z() + U * ( line_end.z() - line_start.z() );
 
    *the_distance = distance( point, *intersection );
 
    return 1;
}

bool sphere_line_intersection (
    const vec3& p1, const vec3& p2, const vec3& p3, const float &r, vec3 *ret )
{
     // x1,p1->y,p1->z  P1 coordinates (point of line)
     // p2->x,p2->y,p2->z  P2 coordinates (point of line)
     // p3->x,p3->y,p3->z, r  P3 coordinates and radius (sphere)
     // x,y,z   intersection coordinates
     //
     // This function returns a pointer array which first index indicates
     // the number of intersection point, followed by coordinate pairs.

    if(p1.x()>p3.x()+r&&p2.x()>p3.x()+r)return(0);
    if(p1.x()<p3.x()-r&&p2.x()<p3.x()-r)return(0);
    if(p1.y()>p3.y()+r&&p2.y()>p3.y()+r)return(0);
    if(p1.y()<p3.y()-r&&p2.y()<p3.y()-r)return(0);
    if(p1.z()>p3.z()+r&&p2.z()>p3.z()+r)return(0);
    if(p1.z()<p3.z()-r&&p2.z()<p3.z()-r)return(0);
    
    float a, b, c, i ;
    
    a =  square(p2.x() - p1.x()) + square(p2.y() - p1.y()) + square(p2.z() - p1.z());
    b =  2* ( (p2.x() - p1.x())*(p1.x() - p3.x())
          + (p2.y() - p1.y())*(p1.y() - p3.y())
          + (p2.z() - p1.z())*(p1.z() - p3.z()) ) ;
    c =  square(p3.x()) + square(p3.y()) +
          square(p3.z()) + square(p1.x()) +
          square(p1.y()) + square(p1.z()) -
          2* ( p3.x()*p1.x() + p3.y()*p1.y() + p3.z()*p1.z() ) - square(r) ;
    i =   b * b - 4 * a * c ;

     if ( i < 0.0f )
     {
      // no intersection
      return(0);
     }
     else
     {
         if(ret){
             float mu = (-b - sqrtf(i)) / (2*a);
             ret->x() = p1.x() + mu*(p2.x()-p1.x());
             ret->y() = p1.y() + mu*(p2.y()-p1.y());
             ret->z() = p1.z() + mu*(p2.z()-p1.z());
         }
      return(1);
     }
}

//Check if a point is in a triangle
bool inTriangle(const vec3 &pointv, const vec3 &normal, const vec3 &p1v, const vec3 &p2v, const vec3 &p3v)
{
    float u0, u1, u2;
    float v0, v1, v2;
    float a, b;
    float maximum;
    int i=0, j=0;
    bool bInter = 0;

    vec3 new_norm;
    new_norm.x()=fabsf(normal.x());
    new_norm.y()=fabsf(normal.y());
    new_norm.z()=fabsf(normal.z());

    if(new_norm.x()>new_norm.y())maximum = new_norm.x();
    else maximum = new_norm.y();
    if(maximum<new_norm.z())maximum=new_norm.z();

    if (maximum == std::fabs(normal.x())) {i = 1; j = 2;}
    if (maximum == std::fabs(normal.y())) {i = 0; j = 2;}
    if (maximum == std::fabs(normal.z())) {i = 0; j = 1;}

    u0 = pointv[i] - p1v[i];
    v0 = pointv[j] - p1v[j];
    u1 = p2v[i] - p1v[i];
    v1 = p2v[j] - p1v[j];
    u2 = p3v[i] - p1v[i];
    v2 = p3v[j] - p1v[j];

    if (u1 > -0.00001f && u1 < 0.00001f)
    {
        b = u0 / u2;
        if (0.0f <= b && b <= 1.0f)
        {
            a = (v0 - b * v2) / v1;
            if ((a >= 0.0f) && (( a + b ) <= 1.0f))
                bInter = 1;
        }
    }
    else
    {
        b = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
        if (0.0f <= b && b <= 1.0f)
        {
            a = (u0 - b * u2) / u1;
            if ((a >= 0.0f) && (( a + b ) <= 1.0f ))
                bInter = 1;
        }
    }

    return bInter;
}

//Check if a point is in a triangle
vec3 barycentric(const vec3 &pointv, const vec3 &normal, const vec3 &p1v, const vec3 &p2v, const vec3 &p3v)
{
    float u0, u1, u2;
    float v0, v1, v2;
    float a=0, b=0;
    float maximum;
    int i=0, j=0;

    vec3 new_norm;
    new_norm.x()=fabsf(normal.x());
    new_norm.y()=fabsf(normal.y());
    new_norm.z()=fabsf(normal.z());

    if(new_norm.x()>new_norm.y())maximum = new_norm.x();
    else maximum = new_norm.y();
    if(maximum<new_norm.z())maximum=new_norm.z();

    if (maximum == std::fabs(normal.x())) {i = 1; j = 2;}
    if (maximum == std::fabs(normal.y())) {i = 0; j = 2;}
    if (maximum == std::fabs(normal.z())) {i = 0; j = 1;}

    u0 = pointv[i] - p1v[i];
    v0 = pointv[j] - p1v[j];
    u1 = p2v[i] - p1v[i];
    v1 = p2v[j] - p1v[j];
    u2 = p3v[i] - p1v[i];
    v2 = p3v[j] - p1v[j];

    if (u1 > -0.00001f && u1 < 0.00001f)
    {
        b = u0 / u2;
        //if (0.0f <= b && b <= 1.0f)
        //{
            a = (v0 - b * v2) / v1;
            //if ((a >= 0.0f) && (( a + b ) <= 1.0f))
                //bInter = 1;
        //}
    }
    else
    {
        b = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
        //if (0.0f <= b && b <= 1.0f)
        //{
            a = (u0 - b * u2) / u1;
            //if ((a >= 0.0f) && (( a + b ) <= 1.0f ))
                //bInter = 1;
        //}
    }

    return vec3(1-a-b, a, b);
}


//Check if a line collides with a triangle
int LineFacet(const vec3 &p1,const vec3 &p2,const vec3 &pa,const vec3 &pb,const vec3 &pc, vec3 *p, const vec3 &n)
{
    /*vec3 n;
    n.x() = (pb.y() - pa.y())*(pc.z() - pa.z()) - (pb.z() - pa.z())*(pc.y() - pa.y());
   n.y() = (pb.z() - pa.z())*(pc.x() - pa.x()) - (pb.x() - pa.x())*(pc.z() - pa.z());
   n.z() = (pb.x() - pa.x())*(pc.y() - pa.y()) - (pb.y() - pa.y())*(pc.x() - pa.x());
   Normalise(&n);

   if(n!=an)
       int blah=5;*/

    //Calculate the position on the line that intersects the plane 
    float denom = n.x() * (p2.x() - p1.x()) + n.y() * (p2.y() - p1.y()) + n.z() * (p2.z() - p1.z());
    if (std::fabs(denom) < 0.0000001f)        // Line and plane don't intersect 
        return 0;
    float d = - n.x() * pa.x() - n.y() * pa.y() - n.z() * pa.z();
    float mu = - (d + n.x() * p1.x() + n.y() * p1.y() + n.z() * p1.z()) / denom;
    if (mu < 0 || mu > 1)   // Intersection not along line segment 
        return 0;

    p->x() = p1.x() + mu * (p2.x() - p1.x());
    p->y() = p1.y() + mu * (p2.y() - p1.y());
    p->z() = p1.z() + mu * (p2.z() - p1.z());
    
    return inTriangle( *p, n, pa, pb, pc);
}

//Check if a line collides with a triangle against the front face only
int LineFacetNoBackface(const vec3 &p1,const vec3 &p2,const vec3 &pa,const vec3 &pb,const vec3 &pc, vec3 *p, const vec3 &n)
{
    /*vec3 n;
    n.x() = (pb.y() - pa.y())*(pc.z() - pa.z()) - (pb.z() - pa.z())*(pc.y() - pa.y());
   n.y() = (pb.z() - pa.z())*(pc.x() - pa.x()) - (pb.x() - pa.x())*(pc.z() - pa.z());
   n.z() = (pb.x() - pa.x())*(pc.y() - pa.y()) - (pb.y() - pa.y())*(pc.x() - pa.x());
   Normalise(&n);

   if(n!=an)
       int blah=5;*/
    
    vec3 dir = p2 - p1;
    float n_dir_dot_prod = n.x()*dir.x() + n.y()*dir.y() + n.z()*dir.z();
    if (n_dir_dot_prod > 0)            // omit backface collisions
        return 0;

    //Calculate the position on the line that intersects the plane 
    float denom = n.x() * (p2.x() - p1.x()) + n.y() * (p2.y() - p1.y()) + n.z() * (p2.z() - p1.z());
    if (std::fabs(denom) < 0.0000001f)   // Line and plane don't intersect 
        return 0;
    float d = - n.x() * pa.x() - n.y() * pa.y() - n.z() * pa.z();
    float mu = - (d + n.x() * p1.x() + n.y() * p1.y() + n.z() * p1.z()) / denom;
    if (mu < 0 || mu > 1)            // Intersection not along line segment 
        return 0;
    
    p->x() = p1.x() + mu * (p2.x() - p1.x());
    p->y() = p1.y() + mu * (p2.y() - p1.y());
    p->z() = p1.z() + mu * (p2.z() - p1.z());
    
    return inTriangle( *p, n, pa, pb, pc);
}

//Check if a line collides with a triangle
int LineFacet(const vec3 &p1,const vec3 &p2,const vec3 &pa,const vec3 &pb,const vec3 &pc, vec3 *p)
{
   vec3 n;
   n.x() = (pb.y() - pa.y())*(pc.z() - pa.z()) - (pb.z() - pa.z())*(pc.y() - pa.y());
   n.y() = (pb.z() - pa.z())*(pc.x() - pa.x()) - (pb.x() - pa.x())*(pc.z() - pa.z());
   n.z() = (pb.x() - pa.x())*(pc.y() - pa.y()) - (pb.y() - pa.y())*(pc.x() - pa.x());
   n = normalize(n);

   return LineFacet(p1, p2, pa, pb, pc, p, n);
}


bool lineBox(const vec3 &start, const vec3 &end,const vec3 &box_min,const vec3 &box_max, float *time)
{
   float st,et,fst = 0,fet = 1;
   float const *bmin = &box_min.x();
   float const *bmax = &box_max.x();
   float const *si = &start.x();
   float const *ei = &end.x();

   for (int i = 0; i < 3; i++) {
      if (*si < *ei) {
         if (*si > *bmax || *ei < *bmin)
            return false;
         float di = *ei - *si;
         st = (*si < *bmin)? (*bmin - *si) / di: 0;
         et = (*ei > *bmax)? (*bmax - *si) / di: 1;
      }
      else {
         if (*ei > *bmax || *si < *bmin)
            return false;
         float di = *ei - *si;
         st = (*si > *bmax)? (*bmax - *si) / di: 0;
         et = (*ei < *bmin)? (*bmin - *si) / di: 1;
      }

      if (st > fst) fst = st;
      if (et < fet) fet = et;
      if (fet < fst)
         return false;
      bmin++; bmax++;
      si++; ei++;
   }

   if(time)*time = fst;
   return true;
}

#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0]; 

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

int planeBoxOverlap(float normal[3],float d, float maxbox[3])
{
  int q;
  float vmin[3],vmax[3];
  for(q=X;q<=Z;q++)
  {
    if(normal[q]>0.0f)
    {
      vmin[q]=-maxbox[q];
      vmax[q]=maxbox[q];
    }
    else
    {
      vmin[q]=maxbox[q];
      vmax[q]=-maxbox[q];
    }
  }
  if(DOT(normal,vmin)+d>0.0f) return 0;
  if(DOT(normal,vmax)+d>=0.0f) return 1;
  
  return 0;
}


/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)               \
    p0 = a*v0[Y] - b*v0[Z];                          \
    p2 = a*v2[Y] - b*v2[Z];                          \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)               \
    p0 = a*v0[Y] - b*v0[Z];                       \
    p1 = a*v1[Y] - b*v1[Z];                          \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)               \
    p0 = -a*v0[X] + b*v0[Z];                     \
    p2 = -a*v2[X] + b*v2[Z];                             \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)               \
    p0 = -a*v0[X] + b*v0[Z];                     \
    p1 = -a*v1[X] + b*v1[Z];                           \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)               \
    p1 = a*v1[X] - b*v1[Y];                       \
    p2 = a*v2[X] - b*v2[Y];                          \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)               \
    p0 = a*v0[X] - b*v0[Y];                   \
    p1 = a*v1[X] - b*v1[Y];                       \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return 0;

bool triBoxOverlap(const vec3 &box_min, const vec3 &box_max, const vec3 &vert1, const vec3 &vert2, const vec3 &vert3)
{

  /*    use separating axis theorem to test overlap between triangle and box */
  /*    need to test for overlap in these directions: */
  /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
  /*       we do not even need to test these) */
  /*    2) normal of the triangle */
  /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
  /*       this gives 3x3=9 more tests */
   float boxcenter[3], boxhalfsize[3], triverts[3][3];
   boxcenter[0] = (box_min.x()+box_max.x())/2;
   boxcenter[1] = (box_min.y()+box_max.y())/2;
   boxcenter[2] = (box_min.z()+box_max.z())/2;
   boxhalfsize[0] = (box_max.x()-box_min.x())/2;
   boxhalfsize[1] = (box_max.y()-box_min.y())/2;
   boxhalfsize[2] = (box_max.z()-box_min.z())/2;
   
   triverts[0][0] = vert1.x();
   triverts[0][1] = vert1.y();
   triverts[0][2] = vert1.z();
   triverts[1][0] = vert2.x();
   triverts[1][1] = vert2.y();
   triverts[1][2] = vert2.z();
   triverts[2][0] = vert3.x();
   triverts[2][1] = vert3.y();
   triverts[2][2] = vert3.z();

   float v0[3],v1[3],v2[3];
   //float axis[3];
   float min,max,d,p0,p1,p2,rad,fex,fey,fez;  
   float normal[3],e0[3],e1[3],e2[3];

   /* This is the fastest branch on Sun */
   /* move everything so that the boxcenter is in (0,0,0) */
   SUB(v0,triverts[0],boxcenter);
   SUB(v1,triverts[1],boxcenter);
   SUB(v2,triverts[2],boxcenter);

   /* compute triangle edges */
   SUB(e0,v1,v0);      /* tri edge 0 */
   SUB(e1,v2,v1);      /* tri edge 1 */
   SUB(e2,v0,v2);      /* tri edge 2 */

   /* Bullet 3:  */
   /*  test the 9 tests first (this was faster) */
   fex = fabsf(e0[X]);
   fey = fabsf(e0[Y]);
   fez = fabsf(e0[Z]);
   AXISTEST_X01(e0[Z], e0[Y], fez, fey);
   AXISTEST_Y02(e0[Z], e0[X], fez, fex);
   AXISTEST_Z12(e0[Y], e0[X], fey, fex);

   fex = fabsf(e1[X]);
   fey = fabsf(e1[Y]);
   fez = fabsf(e1[Z]);
   AXISTEST_X01(e1[Z], e1[Y], fez, fey);
   AXISTEST_Y02(e1[Z], e1[X], fez, fex);
   AXISTEST_Z0(e1[Y], e1[X], fey, fex);

   fex = fabsf(e2[X]);
   fey = fabsf(e2[Y]);
   fez = fabsf(e2[Z]);
   AXISTEST_X2(e2[Z], e2[Y], fez, fey);
   AXISTEST_Y1(e2[Z], e2[X], fez, fex);
   AXISTEST_Z12(e2[Y], e2[X], fey, fex);

   /* Bullet 1: */
   /*  first test overlap in the {x,y,z}-directions */
   /*  find min, max of the triangle each direction, and test for overlap in */
   /*  that direction -- this is equivalent to testing a minimal AABB around */
   /*  the triangle against the AABB */

   /* test in X-direction */
   FINDMINMAX(v0[X],v1[X],v2[X],min,max);
   if(min>boxhalfsize[X] || max<-boxhalfsize[X]) return 0;

   /* test in Y-direction */
   FINDMINMAX(v0[Y],v1[Y],v2[Y],min,max);
   if(min>boxhalfsize[Y] || max<-boxhalfsize[Y]) return 0;

   /* test in Z-direction */
   FINDMINMAX(v0[Z],v1[Z],v2[Z],min,max);
   if(min>boxhalfsize[Z] || max<-boxhalfsize[Z]) return 0;

   /* Bullet 2: */
   /*  test if the box intersects the plane of the triangle */
   /*  compute plane equation of triangle: normal*x+d=0 */
   CROSS(normal,e0,e1);
   d=-DOT(normal,v0);  /* plane eq: normal.x()+d=0 */
   if(!planeBoxOverlap(normal,d,boxhalfsize)) return 0;

   return 1;   /* box and triangle overlaps */
}

#undef X
#undef Y
#undef Z
#undef CROSS
#undef DOT
#undef SUB
#undef FINDMINMAX
#undef AXISTEST_X01
#undef AXISTEST_X2    
#undef AXISTEST_Y02
#undef AXISTEST_Y1
#undef AXISTEST_Z12
#undef AXISTEST_Z0

/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 * updated: 2001-06-20 (added line of intersection)
 *
 * int tri_tri_intersect(float V0[3],float V1[3],float V2[3],
 *                       float U0[3],float U1[3],float U2[3])
 *
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 * Here is a version withouts divisions (a little faster)
 * int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
 *                      float U0[3],float U1[3],float U2[3]);
 * 
 * This version computes the line of intersection as well (if they are not coplanar):
 * int tri_tri_intersect_with_isectline(float V0[3],float V1[3],float V2[3], 
 *                        float U0[3],float U1[3],float U2[3],int *coplanar,
 *                        float isectpt1[3],float isectpt2[3]);
 * coplanar returns whether the tris are coplanar
 * isectpt1, isectpt2 are the endpoints of the line of intersection
 */

#include <math.h>

//#define FABS(x) ((float)fabs(x))        /* implement as is fastest on your machine */

/* if USE_EPSILON_TEST is true then we do a check: 
         if |dv|<EPSILON then dv=0.0f;
   else no check is done (which is less robust)
*/
#define USE_EPSILON_TEST TRUE
#ifndef EPSILON
#define EPSILON 0.000001    // should be replaced by a const float; the previous def of EPSILON is 0.01
#endif


/* some macros */
#define CROSS(dest,v1,v2)                      \
              dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
              dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
              dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) dest[0]=v1[0]-v2[0]; dest[1]=v1[1]-v2[1]; dest[2]=v1[2]-v2[2]; 

#define ADD(dest,v1,v2) dest[0]=v1[0]+v2[0]; dest[1]=v1[1]+v2[1]; dest[2]=v1[2]+v2[2]; 

#define MULT(dest,v,factor) dest[0]=factor*v[0]; dest[1]=factor*v[1]; dest[2]=factor*v[2];

#define SET(dest,src) dest[0]=src[0]; dest[1]=src[1]; dest[2]=src[2]; 

/* sort so that a<=b */
/*#define SORT(a,b)       \
             if(a>b)    \
             {          \
               float c; \
               c=a;     \
               a=b;     \
               b=c;     \
             }*/
void sort(float& a, float& b) {
  if (a > b) {
    float c = a;
    a = b;
    b = c;
  }
}

#define ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1) \
              isect0=VV0+(VV1-VV0)*D0/(D0-D1);    \
              isect1=VV0+(VV2-VV0)*D0/(D0-D2);


#define COMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1) \
  if(D0D1>0.0f)                                         \
  {                                                     \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else if(D0D2>0.0f)                                    \
  {                                                     \
    /* here we know that d0d1<=0.0 */                   \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if(D1*D2>0.0f || D0!=0.0f)                       \
  {                                                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1);          \
  }                                                     \
  else if(D1!=0.0f)                                     \
  {                                                     \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if(D2!=0.0f)                                     \
  {                                                     \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else                                                  \
  {                                                     \
    /* triangles are coplanar */                        \
    return coplanar_tri_tri_OG(N1,V0,V1,V2,U0,U1,U2);   \
  }



/* this edge to edge test is based on Franlin Antonio's gem:
   "Faster Line Segment Intersection", in Graphics Gems III,
   pp. 199-202 */ 
#define EDGE_EDGE_TEST(V0,U0,U1)                      \
  Bx=U0[i0]-U1[i0];                                   \
  By=U0[i1]-U1[i1];                                   \
  Cx=V0[i0]-U0[i0];                                   \
  Cy=V0[i1]-U0[i1];                                   \
  f=Ay*Bx-Ax*By;                                      \
  d=By*Cx-Bx*Cy;                                      \
  if((f>0 && d>=0 && d<=f) || (f<0 && d<=0 && d>=f))  \
  {                                                   \
    e=Ax*Cy-Ay*Cx;                                    \
    if(f>0)                                           \
    {                                                 \
      if(e>=0 && e<=f) return 1;                      \
    }                                                 \
    else                                              \
    {                                                 \
      if(e<=0 && e>=f) return 1;                      \
    }                                                 \
  }                                

#define EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2) \
{                                              \
  float Ax,Ay,Bx,By,Cx,Cy,e,d,f;               \
  Ax=V1[i0]-V0[i0];                            \
  Ay=V1[i1]-V0[i1];                            \
  /* test edge U0,U1 against V0,V1 */          \
  EDGE_EDGE_TEST(V0,U0,U1);                    \
  /* test edge U1,U2 against V0,V1 */          \
  EDGE_EDGE_TEST(V0,U1,U2);                    \
  /* test edge U2,U1 against V0,V1 */          \
  EDGE_EDGE_TEST(V0,U2,U0);                    \
}

#define POINT_IN_TRI(V0,U0,U1,U2)           \
{                                           \
  float a,b,c,d0,d1,d2;                     \
  /* is T1 completly inside T2? */          \
  /* check if V0 is inside tri(U0,U1,U2) */ \
  a=U1[i1]-U0[i1];                          \
  b=-(U1[i0]-U0[i0]);                       \
  c=-a*U0[i0]-b*U0[i1];                     \
  d0=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U2[i1]-U1[i1];                          \
  b=-(U2[i0]-U1[i0]);                       \
  c=-a*U1[i0]-b*U1[i1];                     \
  d1=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U0[i1]-U2[i1];                          \
  b=-(U0[i0]-U2[i0]);                       \
  c=-a*U2[i0]-b*U2[i1];                     \
  d2=a*V0[i0]+b*V0[i1]+c;                   \
  if(d0*d1>0.0f)                             \
  {                                         \
    if(d0*d2>0.0f) return 1;                 \
  }                                         \
}

int coplanar_tri_tri_OG(float N[3],float V0[3],float V1[3],float V2[3],
                        float U0[3],float U1[3],float U2[3])
{
   float A[3];
   short i0,i1;
   /* first project onto an axis-aligned plane, that maximizes the area */
   /* of the triangles, compute indices: i0,i1. */
   A[0]=fabs(N[0]);
   A[1]=fabs(N[1]);
   A[2]=fabs(N[2]);
   if(A[0]>A[1])
   {
      if(A[0]>A[2])  
      {
          i0=1;      /* A[0] is greatest */
          i1=2;
      }
      else
      {
          i0=0;      /* A[2] is greatest */
          i1=1;
      }
   }
   else   /* A[0]<=A[1] */
   {
      if(A[2]>A[1])
      {
          i0=0;      /* A[2] is greatest */
          i1=1;                                           
      }
      else
      {
          i0=0;      /* A[1] is greatest */
          i1=2;
      }
    }               
                
    /* test all edges of triangle 1 against the edges of triangle 2 */
    EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGES(V1,V2,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGES(V2,V0,U0,U1,U2);
                
    /* finally, test if tri1 is totally contained in tri2 or vice versa */
    POINT_IN_TRI(V0,U0,U1,U2);
    POINT_IN_TRI(U0,V0,V1,V2);

    return 0;
}


int tri_tri_intersect(float V0[3],float V1[3],float V2[3],
                      float U0[3],float U1[3],float U2[3])
{
  float E1[3],E2[3];
  float N1[3],N2[3],d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  float D[3];
  float isect1[2], isect2[2];
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float b,c,max;

  /* compute plane equation of triangle(V0,V1,V2) */
  SUB(E1,V1,V0);
  SUB(E2,V2,V0);
  CROSS(N1,E1,E2);
  d1=-DOT(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0=DOT(N1,U0)+d1;
  du1=DOT(N1,U1)+d1;
  du2=DOT(N1,U2)+d1;

  /* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
  if(fabsf(du0)<EPSILON) du0=0.0f;
  if(fabsf(du1)<EPSILON) du1=0.0f;
  if(fabsf(du2)<EPSILON) du2=0.0f;
#endif
  du0du1=du0*du1;
  du0du2=du0*du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  SUB(E1,U1,U0);
  SUB(E2,U2,U0);
  CROSS(N2,E1,E2);
  d2=-DOT(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0=DOT(N2,V0)+d2;
  dv1=DOT(N2,V1)+d2;
  dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
  if(fabsf(dv0)<EPSILON) dv0=0.0f;
  if(fabsf(dv1)<EPSILON) dv1=0.0f;
  if(fabsf(dv2)<EPSILON) dv2=0.0f;
#endif

  dv0dv1=dv0*dv1;
  dv0dv2=dv0*dv2;
        
  if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  CROSS(D,N1,N2);

  /* compute and index to the largest component of D */
  max=fabsf(D[0]);
  index=0;
  b=fabsf(D[1]);
  c=fabsf(D[2]);
  if(b>max) max=b,index=1;
  if(c>max) max=c,index=2;

  /* this is the simplified projection onto L*/
  vp0=V0[index];
  vp1=V1[index];
  vp2=V2[index];
  
  up0=U0[index];
  up1=U1[index];
  up2=U2[index];

  /* compute interval for triangle 1 */
  COMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,isect1[0],isect1[1]);

  /* compute interval for triangle 2 */
  COMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,isect2[0],isect2[1]);

  sort(isect1[0],isect1[1]);
  sort(isect2[0],isect2[1]);

  if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;
  return 1;
}


#define NEWCOMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,A,B,C,X0,X1) \
{ \
        if(D0D1>0.0f) \
        { \
                /* here we know that D0D2<=0.0 */ \
            /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else if(D0D2>0.0f)\
        { \
                /* here we know that d0d1<=0.0 */ \
            A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D1*D2>0.0f || D0!=0.0f) \
        { \
                /* here we know that d0d1<=0.0 or that D0!=0.0 */ \
                A=VV0; B=(VV1-VV0)*D0; C=(VV2-VV0)*D0; X0=D0-D1; X1=D0-D2; \
        } \
        else if(D1!=0.0f) \
        { \
                A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D2!=0.0f) \
        { \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else \
        { \
                /* triangles are coplanar */ \
                return coplanar_tri_tri_OG(N1,V0,V1,V2,U0,U1,U2); \
        } \
}



int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
                     float U0[3],float U1[3],float U2[3])
{
  float E1[3],E2[3];
  float N1[3],N2[3],d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  float D[3];
  float isect1[2], isect2[2];
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float bb,cc,max;
  float a,b,c,x0,x1;
  float d,e,f,y0,y1;
  float xx,yy,xxyy,tmp;

  /* compute plane equation of triangle(V0,V1,V2) */
  SUB(E1,V1,V0);
  SUB(E2,V2,V0);
  CROSS(N1,E1,E2);
  d1=-DOT(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0=DOT(N1,U0)+d1;
  du1=DOT(N1,U1)+d1;
  du2=DOT(N1,U2)+d1;

  /* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
  if(fabsf(du0)<EPSILON) du0=0.0f;
  if(fabsf(du1)<EPSILON) du1=0.0f;
  if(fabsf(du2)<EPSILON) du2=0.0f;
#endif
  du0du1=du0*du1;
  du0du2=du0*du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  SUB(E1,U1,U0);
  SUB(E2,U2,U0);
  CROSS(N2,E1,E2);
  d2=-DOT(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0=DOT(N2,V0)+d2;
  dv1=DOT(N2,V1)+d2;
  dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
  if(fabsf(dv0)<EPSILON) dv0=0.0f;
  if(fabsf(dv1)<EPSILON) dv1=0.0f;
  if(fabsf(dv2)<EPSILON) dv2=0.0f;
#endif

  dv0dv1=dv0*dv1;
  dv0dv2=dv0*dv2;

  if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  CROSS(D,N1,N2);

  /* compute and index to the largest component of D */
  max=(float)fabsf(D[0]);
  index=0;
  bb=(float)fabsf(D[1]);
  cc=(float)fabsf(D[2]);
  if(bb>max) max=bb,index=1;
  if(cc>max) max=cc,index=2;

  /* this is the simplified projection onto L*/
  vp0=V0[index];
  vp1=V1[index];
  vp2=V2[index];

  up0=U0[index];
  up1=U1[index];
  up2=U2[index];

  /* compute interval for triangle 1 */
  NEWCOMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,a,b,c,x0,x1);

  /* compute interval for triangle 2 */
  NEWCOMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,d,e,f,y0,y1);

  xx=x0*x1;
  yy=y0*y1;
  xxyy=xx*yy;

  tmp=a*xxyy;
  isect1[0]=tmp+b*x1*yy;
  isect1[1]=tmp+c*x0*yy;

  tmp=d*xxyy;
  isect2[0]=tmp+e*xx*y1;
  isect2[1]=tmp+f*xx*y0;

  sort(isect1[0],isect1[1]);
  sort(isect2[0],isect2[1]);

  if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;
  return 1;
}

/* sort so that a<=b */
/*#define SORT2(a,b,smallest)       \
             if(a>b)       \
             {             \
               float c;    \
               c=a;        \
               a=b;        \
               b=c;        \
               smallest=1; \
             }             \
             else smallest=0;*/
void sort2(float& a, float& b, int& smallest) {
  if (a > b) {
    float c = a;
    a = b;
    b = c;
    smallest = 1;
  }
  else smallest = 0;
}


inline void isect2(float VTX0[3],float VTX1[3],float VTX2[3],float VV0,float VV1,float VV2,
        float D0,float D1,float D2,float *isect0,float *isect1,float isectpoint0[3],float isectpoint1[3]) 
{
  float tmp=D0/(D0-D1);          
  float diff[3];
  *isect0=VV0+(VV1-VV0)*tmp;         
  SUB(diff,VTX1,VTX0);              
  MULT(diff,diff,tmp);               
  ADD(isectpoint0,diff,VTX0);        
  tmp=D0/(D0-D2);                    
  *isect1=VV0+(VV2-VV0)*tmp;          
  SUB(diff,VTX2,VTX0);                   
  MULT(diff,diff,tmp);                 
  ADD(isectpoint1,VTX0,diff);          
}


#if 0
#define ISECT2(VTX0,VTX1,VTX2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1) \
              tmp=D0/(D0-D1);                    \
              isect0=VV0+(VV1-VV0)*tmp;          \
          SUB(diff,VTX1,VTX0);               \
        MULT(diff,diff,tmp);               \
              ADD(isectpoint0,diff,VTX0);        \
              tmp=D0/(D0-D2);
/*              isect1=VV0+(VV2-VV0)*tmp;          \ */
/*              SUB(diff,VTX2,VTX0);               \     */
/*              MULT(diff,diff,tmp);               \   */
/*              ADD(isectpoint1,VTX0,diff);           */
#endif

inline int compute_intervals_isectline(float VERT0[3],float VERT1[3],float VERT2[3],
                       float VV0,float VV1,float VV2,float D0,float D1,float D2,
                       float D0D1,float D0D2,float *isect0,float *isect1,
                       float isectpoint0[3],float isectpoint1[3])
{
  if(D0D1>0.0f)                                        
  {                                                    
    /* here we know that D0D2<=0.0 */                  
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
  } 
  else if(D0D2>0.0f)                                   
    {                                                   
    /* here we know that d0d1<=0.0 */             
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);
  }                                                  
  else if(D1*D2>0.0f || D0!=0.0f)   
  {                                   
    /* here we know that d0d1<=0.0 or that D0!=0.0 */
    isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1);   
  }                                                  
  else if(D1!=0.0f)                                  
  {                                               
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1); 
  }                                         
  else if(D2!=0.0f)                                  
  {                                                   
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);     
  }                                                 
  else                                               
  {                                                   
    /* triangles are coplanar */    
    return 1;
  }
  return 0;
}

#define COMPUTE_INTERVALS_ISECTLINE(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1,isectpoint0,isectpoint1) \
  if(D0D1>0.0f)                                         \
  {                                                     \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     
#if 0
  else if(D0D2>0.0f)                                    \
  {                                                     \
    /* here we know that d0d1<=0.0 */                   \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1*D2>0.0f || D0!=0.0f)                       \
  {                                                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1!=0.0f)                                     \
  {                                                     \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D2!=0.0f)                                     \
  {                                                     \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else                                                  \
  {                                                     \
    /* triangles are coplanar */                        \
    coplanar=1;                                         \
    return coplanar_tri_tri_OG(N1,V0,V1,V2,U0,U1,U2);   \
  }
#endif

//  int coplanar_tri_tri_OG(float N[3],float V0[3],float V1[3],float V2[3],
//                          float U0[3],float U1[3],float U2[3])
//{
//   float A[3];
//   short i0,i1;
//   /* first project onto an axis-aligned plane, that maximizes the area */
//   /* of the triangles, compute indices: i0,i1. */
//   A[0]=fabsf(N[0]);
//   A[1]=fabsf(N[1]);
//   A[2]=fabsf(N[2]);
//   if(A[0]>A[1])
//   {
//      if(A[0]>A[2])  
//      {
//          i0=1;      /* A[0] is greatest */
//          i1=2;
//      }
//      else
//      {
//          i0=0;      /* A[2] is greatest */
//          i1=1;
//      }
//   }
//   else   /* A[0]<=A[1] */
//   {
//      if(A[2]>A[1])
//      {
//          i0=0;      /* A[2] is greatest */
//          i1=1;                                           
//      }
//      else
//      {
//          i0=0;      /* A[1] is greatest */
//          i1=2;
//      }
//    }               
//                
//    /* test all edges of triangle 1 against the edges of triangle 2 */
//    EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2);
//    EDGE_AGAINST_TRI_EDGES(V1,V2,U0,U1,U2);
//    EDGE_AGAINST_TRI_EDGES(V2,V0,U0,U1,U2);
//                
//    /* finally, test if tri1 is totally contained in tri2 or vice versa */
//    POINT_IN_TRI(V0,U0,U1,U2);
//    POINT_IN_TRI(U0,V0,V1,V2);
//
//    return 0;
//}

int tri_tri_intersect_with_isectline(float V0[3],float V1[3],float V2[3],
                     float U0[3],float U1[3],float U2[3],int *coplanar,
                     float *isectpt1,float *isectpt2)
{
  float E1[3],E2[3];
  float N1[3],N2[3],d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  float D[3];
  float isect1[2] = {0.0f,0.0f}, isect2[2] = {0.0f,0.0f};
  float isectpointA1[3],isectpointA2[3];
  float isectpointB1[3] = {0.0f,0.0f,0.0f},isectpointB2[3] = {0.0f,0.0f,0.0f};
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float b,c,max;
  //float tmp,diff[3];
  int smallest1,smallest2;
  
  /* compute plane equation of triangle(V0,V1,V2) */
  SUB(E1,V1,V0);
  SUB(E2,V2,V0);
  CROSS(N1,E1,E2);
  d1=-DOT(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0=DOT(N1,U0)+d1;
  du1=DOT(N1,U1)+d1;
  du2=DOT(N1,U2)+d1;

  /* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
  if(fabsf(du0)<EPSILON) du0=0.0f;
  if(fabsf(du1)<EPSILON) du1=0.0f;
  if(fabsf(du2)<EPSILON) du2=0.0f;
#endif
  du0du1=du0*du1;
  du0du2=du0*du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  SUB(E1,U1,U0);
  SUB(E2,U2,U0);
  CROSS(N2,E1,E2);
  d2=-DOT(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0=DOT(N2,V0)+d2;
  dv1=DOT(N2,V1)+d2;
  dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
  if(fabsf(dv0)<EPSILON) dv0=0.0f;
  if(fabsf(dv1)<EPSILON) dv1=0.0f;
  if(fabsf(dv2)<EPSILON) dv2=0.0f;
#endif

  dv0dv1=dv0*dv1;
  dv0dv2=dv0*dv2;
        
  if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  CROSS(D,N1,N2);

  /* compute and index to the largest component of D */
  max=fabsf(D[0]);
  index=0;
  b=fabsf(D[1]);
  c=fabsf(D[2]);
  if(b>max) max=b,index=1;
  if(c>max) max=c,index=2;

  /* this is the simplified projection onto L*/
  vp0=V0[index];
  vp1=V1[index];
  vp2=V2[index];
  
  up0=U0[index];
  up1=U1[index];
  up2=U2[index];

  /* compute interval for triangle 1 */
  *coplanar=compute_intervals_isectline(V0,V1,V2,vp0,vp1,vp2,dv0,dv1,dv2,
                       dv0dv1,dv0dv2,&isect1[0],&isect1[1],isectpointA1,isectpointA2);
  if(*coplanar) return coplanar_tri_tri_OG(N1,V0,V1,V2,U0,U1,U2);     


  /* compute interval for triangle 2 */
  compute_intervals_isectline(U0,U1,U2,up0,up1,up2,du0,du1,du2,
                  du0du1,du0du2,&isect2[0],&isect2[1],isectpointB1,isectpointB2);

  sort2(isect1[0],isect1[1],smallest1);
  sort2(isect2[0],isect2[1],smallest2);

  if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;

  /* at this point, we know that the triangles intersect */

  if(isect2[0]<isect1[0])
  {
    if(smallest1==0) { SET(isectpt1,isectpointA1); }
    else { SET(isectpt1,isectpointA2); }

    if(isect2[1]<isect1[1])
    {
      if(smallest2==0) { SET(isectpt2,isectpointB2); }
      else { SET(isectpt2,isectpointB1); }
    }
    else
    {
      if(smallest1==0) { SET(isectpt2,isectpointA2); }
      else { SET(isectpt2,isectpointA1); }
    }
  }
  else
  {
    if(smallest2==0) { SET(isectpt1,isectpointB1); }
    else { SET(isectpt1,isectpointB2); }

    if(isect2[1]>isect1[1])
    {
      if(smallest1==0) { SET(isectpt2,isectpointA2); }
      else { SET(isectpt2,isectpointA1); }      
    }
    else
    {
      if(smallest2==0) { SET(isectpt2,isectpointB2); }
      else { SET(isectpt2,isectpointB1); } 
    }
  }
  return 1;
}

bool PointInTriangle(vec3 &point, vec3 *tri_point[3])
{
    if((((tri_point[1]->z() - tri_point[0]->z()) * (tri_point[0]->x() - point.x()) -
        (tri_point[1]->x() - tri_point[0]->x()) * (tri_point[0]->z() - point.z()))<0) &&
        (((tri_point[2]->z() - tri_point[1]->z()) * (tri_point[1]->x() - point.x()) -
        (tri_point[2]->x() - tri_point[1]->x()) * (tri_point[1]->z() - point.z()))<0) &&
        (((tri_point[0]->z() - tri_point[2]->z()) * (tri_point[2]->x() - point.x()) -
        (tri_point[0]->x() - tri_point[2]->x()) * (tri_point[2]->z() - point.z()))<0))
    {
        return true;
    }

    return false;
}

// Check for intersection between segments p1-p2 and p3-p4
// Based on http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
bool LineIntersection2D(const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4)
{
    float denom = (p4.z() - p3.z())*(p2.x() - p1.x()) - (p4.x() - p3.x())*(p2.z() - p1.z());
    float ua = ((p4.x() - p3.x())*(p1.z() - p3.z()) - (p4.z() - p3.z())*(p1.x() - p3.x())) / denom;
    float ub = ((p2.x() - p1.x())*(p1.z() - p3.z()) - (p2.z() - p1.z())*(p1.x() - p3.x())) / denom;

    if( ua>=0 && ua<=1 && ub>=0 && ub<=1 )
        return true;
    return false;
}

bool LineSquareIntersection2D(vec3 &min, vec3 &max, vec3 &start, vec3 &end)
{
    // Discard lines that are entirely on one side of a box plane
    if(start.x() < min.x() && end.x() < min.x())return false;
    if(start.z() < min.z() && end.z() < min.z())return false;
    if(start.x() > max.x() && end.x() > max.x())return false;
    if(start.z() > max.z() && end.z() > max.z())return false;

    // Check planes
    if((start.x() > max.x()) != (end.x() > max.x()))
        if(LineIntersection2D(vec3(max.x(),0,min.z()), vec3(max.x(),0,max.z()), start, end))
            return true;
    if((start.x() > min.x()) != (end.x() > min.x()))
        if(LineIntersection2D(vec3(min.x(),0,min.z()), vec3(min.x(),0,max.z()), start, end))
            return true;
    if((start.z() > max.z()) != (end.z() > max.z()))
        if(LineIntersection2D(vec3(min.x(),0,max.z()), vec3(max.x(),0,max.z()), start, end))
            return true;
    if((start.z() > min.z()) != (end.z() > min.z()))
        if(LineIntersection2D(vec3(min.x(),0,min.z()), vec3(max.x(),0,min.z()), start, end))
            return true;

    return false;
}

bool TriangleSquareIntersection2D(vec3 &min, vec3 &max, vec3 *tri_point[3])
{
    // Discard triangles that are entirely on one side of a box plane
    if(tri_point[0]->x() < min.x() && tri_point[1]->x() < min.x() && tri_point[2]->x() < min.x())return false;
    if(tri_point[0]->z() < min.z() && tri_point[1]->z() < min.z() && tri_point[2]->z() < min.z())return false;
    if(tri_point[0]->x() > max.x() && tri_point[1]->x() > max.x() && tri_point[2]->x() > max.x())return false;
    if(tri_point[0]->z() > max.z() && tri_point[1]->z() > max.z() && tri_point[2]->z() > max.z())return false;

    // Accept all triangles that have a vertex in the square
    for (int i = 0; i<3; i++){
        if(tri_point[i]->x() < max.x() && tri_point[i]->z() < max.z() &&
            tri_point[i]->x() > min.x() && tri_point[i]->z() > min.z()){
                return true;
        }
    }

    // Accept all triangles that completely enclose the square
    if(PointInTriangle(min, tri_point)){
        return true;
    }

    // If we are still here, check line-square collisions
    if(LineSquareIntersection2D(min, max, *tri_point[0], *tri_point[1])) return true;
    if(LineSquareIntersection2D(min, max, *tri_point[1], *tri_point[2])) return true;
    if(LineSquareIntersection2D(min, max, *tri_point[0], *tri_point[2])) return true;

    return false;
}

Collision::Collision() : hit(false), hit_what(NULL), hit_how(-1) {

}
