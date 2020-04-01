#pragma once
#include <iostream>
#include<stdlib.h>

inline void DrawTexture_Plane(GLfloat size,GLuint texture,
	GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	glBindTexture(GL_TEXTURE_2D, texture);

	glPushMatrix();//=========================
	{
		glColor4f(r, g, b, a);
		glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(size / 2, size / 2, 0);//4
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(size / 2, -size / 2, 0);//8
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(-size / 2, -size / 2, 0);//5
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-size / 2, size / 2, 0);//1
		glEnd();
	}
	glPopMatrix();//=========================
}

inline void drawRect(float size, float x, float y, float z)
{
	glPushMatrix(); {
		glTranslatef(x, y, z);
		glBegin(GL_QUADS);
		//À­¸é
		glVertex3f(-size / 2, size / 2, -size / 2); //1
		glVertex3f(-size / 2, size / 2, size / 2);// 2
		glVertex3f(size / 2, size / 2, size / 2);// 3
		glVertex3f(size / 2, size / 2, -size / 2);//4
												  //¾Õ¸é		 
		glVertex3f(-size / 2, size / 2, size / 2);//2
		glVertex3f(-size / 2, -size / 2, size / 2);//6
		glVertex3f(size / 2, -size / 2, size / 2);//7
		glVertex3f(size / 2, size / 2, size / 2);//3
												 //¿À¸¥ÂÊ¿·¸é
		glVertex3f(size / 2, size / 2, size / 2);//3
		glVertex3f(size / 2, -size / 2, size / 2);//7
		glVertex3f(size / 2, -size / 2, -size / 2);//8
		glVertex3f(size / 2, size / 2, -size / 2);//4
												  //¿ÞÂÊ¿·¸é
		glVertex3f(-size / 2, size / 2, -size / 2);//1
		glVertex3f(-size / 2, -size / 2, -size / 2);//5
		glVertex3f(-size / 2, -size / 2, size / 2);//6
		glVertex3f(-size / 2, size / 2, size / 2);//2
												  //µÞ¸é
		glVertex3f(size / 2, size / 2, -size / 2);//4
		glVertex3f(size / 2, -size / 2, -size / 2);//8
		glVertex3f(-size / 2, -size / 2, -size / 2);//5
		glVertex3f(-size / 2, size / 2, -size / 2);//1

												   //¹Ù´Ú¸é
		glVertex3f(-size / 2, -size / 2, size / 2);//6
		glVertex3f(-size / 2, -size / 2, -size / 2);//5
		glVertex3f(size / 2, -size / 2, -size / 2);//8
		glVertex3f(size / 2, -size / 2, size / 2);//7

		glEnd();
	}glPopMatrix();
}
inline void drawRectText(float size, int i)
{
	glPushMatrix(); {
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_QUADS);
		if (i == 0) {
			//À­¸é
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-size / 2, size / 2, -size / 2); //1
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-size / 2, size / 2, size / 2);// 2
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(size / 2, size / 2, size / 2);// 3
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(size / 2, size / 2, -size / 2);//4
		}
		else if (i == 1) {
			//¾Õ¸é		 
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-size / 2, size / 2, size / 2);//2
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-size / 2, -size / 2, size / 2);//6
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(size / 2, -size / 2, size / 2);//7
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(size / 2, size / 2, size / 2);//3
		}
		else if (i == 2) {
			//¿À¸¥ÂÊ¿·¸é
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(size / 2, size / 2, size / 2);//3
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(size / 2, -size / 2, size / 2);//7
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(size / 2, -size / 2, -size / 2);//8
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(size / 2, size / 2, -size / 2);//4
		}
		else if (i == 3) {
			//¿ÞÂÊ¿·¸é
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-size / 2, size / 2, -size / 2);//1
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-size / 2, -size / 2, -size / 2);//5
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(-size / 2, -size / 2, size / 2);//6
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(-size / 2, size / 2, size / 2);//2
		}
		else if (i == 4) {
			//µÞ¸é
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(size / 2, size / 2, -size / 2);//4
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(size / 2, -size / 2, -size / 2);//8
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(-size / 2, -size / 2, -size / 2);//5
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(-size / 2, size / 2, -size / 2);//1
		}
		else if (i == 5) {
			//¹Ù´Ú¸é
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(size / 2, -size / 2, -size / 2);//8

			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(size / 2, -size / 2, size / 2);//7

			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(-size / 2, -size / 2, size / 2);//6
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(-size / 2, -size / 2, -size / 2);//5
		}
		glEnd();
	}glPopMatrix();
}