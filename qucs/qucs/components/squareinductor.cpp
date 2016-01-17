/*
* squareinductor.cpp - Planar square inductor class implementation
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

#include "squareinductor.h"


squareinductor::squareinductor()
{
  Description = QObject::tr("Planar square inductor");

  //Square spiral
  Lines.append(new Line(-30,  0, 2,  0,QPen(Qt::black,4)));
  Lines.append(new Line(2,  0, 2,  10,QPen(Qt::gray,4)));
  Lines.append(new Line(-10,  10, 2,  10,QPen(Qt::gray,4)));
  Lines.append(new Line(-10,  10, -10,  -10,QPen(Qt::gray,4)));
  Lines.append(new Line(-10,  -10, 10,  -10,QPen(Qt::gray,4)));
  Lines.append(new Line(10,  -10, 10,  20,QPen(Qt::gray,4)));
  Lines.append(new Line(-20,  20, 10,  20,QPen(Qt::gray,4)));
  Lines.append(new Line(-20,  20, -20,  -25,QPen(Qt::gray,4)));
  Lines.append(new Line(-20,  -25, 20,  -25,QPen(Qt::gray,4)));
  Lines.append(new Line(20,  0, 20,  -25,QPen(Qt::gray,4)));

  Lines.append(new Line(  20,  0, 30,  0,QPen(Qt::darkBlue,4)));


  Ports.append(new Port(-30, 0));
  Ports.append(new Port( 30, 0));

  x1 = -30; y1 =-30;
  x2 =  30; y2 = 30;

  tx = x2;
  ty = y1+20;
  Model = "SQUAREIND";
  Name  = "SQUAREIND";

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

squareinductor::~squareinductor()
{
}

Component* squareinductor::newOne()
{
  return new squareinductor();
}

Element* squareinductor::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Square inductor");
  BitmapFile = (char *) "squareinductor";

  if(getNewOne)  return new squareinductor();
  return 0;
}
