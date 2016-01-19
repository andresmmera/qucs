/*
* mimcap.cpp - Lossy capacitor implementation
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
#include "mimcap.h"


mimcap::mimcap()
{
  Description = QObject::tr("MIM capacitor");

  //Lines connection device and ports
  Lines.append(new Line( -35, 10, 10, 10,QPen(Qt::darkBlue,2)));
  Lines.append(new Line(  -25,-10,  1, -10,QPen(Qt::darkBlue,2)));

  Lines.append(new Line( -35, 10, -25, -10,QPen(Qt::darkBlue,2)));
  Lines.append(new Line( -5, 10, -5, 5,QPen(Qt::darkBlue,2)));
  Lines.append(new Line( 10, 10, 10, 5,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(-5, 5, 28, 5,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(-5, 5, 2, -12,QPen(Qt::darkBlue,2)));
  Lines.append(new Line(2, -12, 35, -12,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(28, 5, 35, -12,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(10, 10, 12, 5,QPen(Qt::darkBlue,2)));

 // Filling
 Lines.append(new Line(-5, 10, -3, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(-3, 10, -1, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(-1, 10, 1, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(1, 10, 3, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(3, 10, 5, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(5, 10, 7, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(7, 10, 9, 5,QPen(Qt::darkBlue,2)));
 Lines.append(new Line(9, 10, 11, 5,QPen(Qt::darkBlue,2)));


  Ports.append(new Port(-30,  0));
  Ports.append(new Port( 30,  0));

  x1 = -30; y1 = -13;
  x2 =  30; y2 =  13;

  tx = x1+4;
  ty = y2+4;
  Model = "MIMCAP";
  Name  = "MIMCAP";


  Props.append(new Property("W", "25 um", true,
		QObject::tr("Width of the line")));
  Props.append(new Property("h", "10 um", true,
		QObject::tr("Insulator thickness")));
  Props.append(new Property("l", "1 um", true,
		QObject::tr("Insulator length")));
  Props.append(new Property("tand", "0.0125", true,
		QObject::tr("Dielectric loss of the insulator")));
  Props.append(new Property("er", "9.8", true,
		QObject::tr("Relative dielectric coefficient of the insulator film")));
  Props.append(new Property("rho", "4.1e7", true,
		QObject::tr("Conductivity of the metal plates")));
  Props.append(new Property("t", "36 um", true,
		QObject::tr("Conductor thickness")));

}

mimcap::~mimcap()
{
}

Component* mimcap::newOne()
{
  return new mimcap();
}

Element* mimcap::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("MIM capacitor");
  BitmapFile = (char *) "mimcap";

  if(getNewOne)  return new mimcap();
  return 0;
}
