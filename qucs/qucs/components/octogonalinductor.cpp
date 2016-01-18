/*
* octogonalinductor.cpp - Planar octogonal inductor class implementation
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

#include "octogonalinductor.h"


octogonalinductor::octogonalinductor()
{
  Description = QObject::tr("Planar octogonal inductor");

  //Octogonal spiral
  Lines.append(new Line(0,  0, 5,  -2,QPen(Qt::gray,4)));
  Lines.append(new Line(5,  -2, 5,  -7,QPen(Qt::gray,4)));
  Lines.append(new Line(5,  -7, 0,  -12,QPen(Qt::gray,4)));
  Lines.append(new Line(0,  -12, -7,  -12,QPen(Qt::gray,4)));
  Lines.append(new Line(-7,  -12, -12,  -7,QPen(Qt::gray,4)));
  Lines.append(new Line(-12,  -7, -12,  -2,QPen(Qt::gray,4)));
  Lines.append(new Line(-12,  -2, -7, 7, QPen(Qt::gray,4)));
  Lines.append(new Line(-7,  7, 7, 7, QPen(Qt::gray,4)));

  Lines.append(new Line(7,  7, 15, 0, QPen(Qt::gray,4)));
  Lines.append(new Line(15,  0, 15, -10, QPen(Qt::gray,4)));
  Lines.append(new Line(15,  -10, 5, -20, QPen(Qt::gray,4)));
  Lines.append(new Line(5,  -20, -12, -20, QPen(Qt::gray,4)));
  Lines.append(new Line(-12,  -20, -23, -10, QPen(Qt::gray,4)));
  Lines.append(new Line(-23,  -10, -23, 0, QPen(Qt::gray,4)));
  Lines.append(new Line(-23,  0, -12, 17, QPen(Qt::gray,4)));
  Lines.append(new Line(-12,  17, 8, 17, QPen(Qt::gray,4)));

  Lines.append(new Line(8, 17, 23, 5, QPen(Qt::gray,4)));
  Lines.append(new Line(23, 5, 23, 0, QPen(Qt::gray,4)));


  Lines.append(new Line(-30, 0, 0, 0, QPen(Qt::black,4)));
  Lines.append(new Line(23, 0, 30, 0, QPen(Qt::black,2)));

  Ports.append(new Port(-30, 0));
  Ports.append(new Port( 30, 0));


  x1 = -30; y1 =-30;
  x2 =  30; y2 = 30;

  tx = x1;
  ty = y1+50;
  Model = "OCTOGONALIND";
  Name  = "OCTOGONALIND";

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

octogonalinductor::~octogonalinductor()
{
}

Component* octogonalinductor::newOne()
{
  return new octogonalinductor();
}

Element* octogonalinductor::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Octogonal inductor");
  BitmapFile = (char *) "octogonalinductor";

  if(getNewOne)  return new octogonalinductor();
  return 0;
}
