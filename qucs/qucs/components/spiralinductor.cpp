/*
* spiralinductor.cpp - Planar spiral inductor class implementation
*
* copyright (C) 2015 Andres Martinez-Mera <andresmartinezmera@gmail.com>
*
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this package; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
* Boston, MA 02110-1301, USA.
*
*
*/

#include "spiralinductor.h"


spiralinductor::spiralinductor()
{
  Description = QObject::tr("Planar spiral inductor");

  //Spiral
  Arcs.append(new Arc(-5, 0, 10, 10, -16*90, 16*180,QPen(Qt::darkBlue,2)));
  Arcs.append(new Arc(-10, -10, 20, 20, 16*90, 16*180,QPen(Qt::darkBlue,2)));
  Arcs.append(new Arc(-15, -10, 30, 30, -16*90, 16*180,QPen(Qt::darkBlue,2)));
  Arcs.append(new Arc(-20, -20, 40, 40, 16*90, 16*180,QPen(Qt::darkBlue,2)));
  Arcs.append(new Arc(-20, -20, 40, 40, 0, 16*90,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(-30,  0, 0,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new Line( 20,  0, 30,  0,QPen(Qt::darkBlue,2)));

  Ports.append(new Port(-30, 0));
  Ports.append(new Port( 30, 0));

  x1 = -30; y1 =-11;
  x2 =  11; y2 = 30;

  tx = x2;
  ty = y1+20;
  Model = "SPIRALIND";
  Name  = "SPIRALIND";

  Props.append(new Property("Subst", "Subst1", true,
		QObject::tr("Substrate")));
  Props.append(new Property("W", "25 um", true,
		QObject::tr("Width of line")));
  Props.append(new Property("Di", "200 um", true,
		QObject::tr("Inner diameter")));
  Props.append(new Property("S", "25 um", true,
		QObject::tr("Spacing between turns")));
  Props.append(new Property("N", "3", true,
		QObject::tr("Number of turns")));

}

spiralinductor::~spiralinductor()
{
}

Component* spiralinductor::newOne()
{
  return new spiralinductor();
}

Element* spiralinductor::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Spiral inductor");
  BitmapFile = (char *) "spiralinductor";

  if(getNewOne)  return new spiralinductor();
  return 0;
}
