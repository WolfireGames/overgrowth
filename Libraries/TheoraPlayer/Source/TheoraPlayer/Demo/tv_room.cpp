/************************************************************************************
This source file is part of the Theora Video Playback Library
For latest info, see http://libtheoraplayer.sourceforge.net/
*************************************************************************************
Copyright (c) 2008-2010 Kresimir Spes (kreso@cateia.com)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License (LGPL) as published by the
Free Software Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
*************************************************************************************/

/************************************************************************************
COPYRIGHT INFO: The room 3D models and textures are licensed under the terms of the
                GNU General Public License (GPL).
*************************************************************************************/
#define __3D_PROJECTION
#define __DEV_IL
#define __ZBUFFER
#include "../demo_basecode.h"
#include "TheoraPlayer.h"
#include "TheoraDataSource.h"
#include "../ObjModel.h"
#include <math.h>

unsigned int tex_id;
TheoraVideoManager* mgr;
TheoraVideoClip* clip;
std::string window_name="tv_room";
bool started=1;
int window_w=800,window_h=600;

ObjModel chair1,chair2,tv,room,table;
float anglex=0,angley=0;
unsigned int r=0,g=0,b=0;
void draw()
{
	glBindTexture(GL_TEXTURE_2D,tex_id);

	glLoadIdentity();
	gluLookAt(sin(anglex)*400-200,angley,cos(anglex)*400,  -200,150,0,  0,1,0);

	TheoraVideoFrame* f=clip->getNextFrame();
	if (f)
	{
		unsigned char* data=f->getBuffer();
		unsigned int n=clip->getWidth()*f->getHeight();

		r=g=b=0;
		for (unsigned int i=0;i<n;i++)
		{
			r+=data[i*3];
			g+=data[i*3+1];
			b+=data[i*3+2];
		}
		r/=n; g/=n; b/=n;
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,clip->getWidth(),f->getHeight(),GL_RGB,GL_UNSIGNED_BYTE,f->getBuffer());
		clip->popFrame();
	}


	float w=clip->getWidth(),h=clip->getHeight();
	float tw=nextPow2(w),th=nextPow2(h);

	glEnable(GL_TEXTURE_2D);
	if (shader_on) enable_shader();
	glColor3f(1,1,1);


	glPushMatrix();
	glRotatef(90,0,1,0);
	glTranslatef(0,0,-415);
	drawTexturedQuad(-2*30,190,4*30,-3*25,w/tw,h/th);
	glPopMatrix();

	if (shader_on) disable_shader();
	glColor3f(0.2f+0.8f*(r/255.0f),
		      0.2f+0.8f*(g/255.0f),
		      0.2f+0.8f*(b/255.0f));

	chair1.draw();
	chair2.draw();
	table.draw();
	tv.draw();
	glEnable(GL_CULL_FACE);
	room.draw();
	glDisable(GL_CULL_FACE);

}

void update(float time_increase)
{
	float x,y;
	getCursorPos(&x,&y);
	anglex=-4*3.14f*x/window_w;
	angley=1500*(y-300)/window_h;

	if (started)
	{
		// let's wait until the system caches up a few frames on startup
		if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames()*0.5f)
			return;
		started=0;
	}
	mgr->update(time_increase);
}

void OnKeyPress(int key)
{
	if (key == ' ')
    { if (clip->isPaused()) clip->play(); else clip->pause(); }

	if (key == 5) clip->setOutputMode(TH_RGB);
	if (key == 6) clip->setOutputMode(TH_YUV);
	if (key == 7) clip->setOutputMode(TH_GREY3);
}

void OnClick(float x,float y)
{
	if (y > 570)
		clip->seek((x/window_w)*clip->getDuration());
}

void setDebugTitle(char* out)
{
	int nDropped=clip->getNumDroppedFrames(),nDisplayed=clip->getNumDisplayedFrames();
	float percent=100*((float) nDropped/nDisplayed);
	sprintf(out," (%dx%d) %d precached, %d displayed, %d dropped (%.1f %%)",clip->getWidth(),
		                                                                    clip->getHeight(),
		                                                                    clip->getNumReadyFrames(),
		                                                                    nDisplayed,
		                                                                    nDropped,
			                                                                percent);
}

void init()
{
	mgr=new TheoraVideoManager();
	clip=mgr->createVideoClip("../media/bunny.ogg",TH_RGB);
//  use this if you want to preload the file into ram and stream from there
//	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("../media/short.ogg"),TH_RGB);
	clip->setAutoRestart(1);

	tex_id=createTexture(nextPow2(clip->getWidth()),nextPow2(clip->getHeight()));

	chair1.load("../media/chair1.obj",loadTexture("../media/chair1.jpg"));
	chair2.load("../media/chair2.obj",loadTexture("../media/chair2.jpg"));
	room.load("../media/room.obj",loadTexture("../media/room.jpg"));
	table.load("../media/table.obj",loadTexture("../media/table.jpg"));
	tv.load("../media/tv.obj",loadTexture("../media/tv.jpg"));

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_COLOR_MATERIAL);
}

void destroy()
{
	delete mgr;
}
