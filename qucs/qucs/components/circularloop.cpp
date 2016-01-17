/*
* circularloop.cpp - Circular loop inductor class implementation
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

#include "circularloop.h"


circularloop::circularloop()
{
  Description = QObject::tr("Printed loop inductor");

  //Loop
  Arcs.append(new Arc(-18, -25, 35, 35, -312, 16*220,QPen(Qt::gray,4)));

  Lines.append(new Line(-30,  0, -16,  0,QPen(Qt::gray,8)));
  Lines.append(new Line(  16,  0, 30,  0,QPen(Qt::gray,8)));

  Ports.append(new Port(-30, 0));
  Ports.append(new Port( 30, 0));

  x1 = -30; y1 =-30;
  x2 =  30; y2 = 5;

  tx = x2;
  ty = y1+20;
  Model = "CIRCULARLOOP";
  Name  = "CIRCULARLOOP";

  Props.append(new Property("Subst", "Subst1", true,
		QObject::tr("Substrate")));
  Props.append(new Property("W", "25 um", true,
		QObject::tr("Width of line")));
  Props.append(new Property("a", "200 um", true,
		QObject::tr("Radius")));

}

circularloop::~circularloop()
{
}

Component* circularloop::newOne()
{
  return new circularloop();
}

Element* circularloop::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Circular loop");
  BitmapFile = (char *) "circularloop";

  if(getNewOne)  return new circularloop();
  return 0;
}
