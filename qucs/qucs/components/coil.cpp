/*
* coil.cpp - Coil implementation
*
* copyright (C) 2016 Andres Martinez-Mera <andresmartinezmera@gmail.com>
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
#include "coil.h"


coil::coil()
{
  Description = QObject::tr("Coil");

  //Spiral
  Arcs.append(new Arc(-15, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Arcs.append(new Arc( -11, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Arcs.append(new Arc(  -7, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Arcs.append(new Arc(  -3, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Arcs.append(new Arc(  1, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Arcs.append(new Arc(  5, -10, 12, 12,  0, 16*360,QPen(Qt::darkBlue,1)));
  Lines.append(new Line(-30,  0,-18,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new Line( 18,  0, 30,  0,QPen(Qt::darkBlue,2)));

  Lines.append(new Line(-18,  0, -10,  2,QPen(Qt::darkBlue,1)));
  Lines.append(new Line( 10,  2, 18,  0,QPen(Qt::darkBlue,1)));
  
  //Core
  Lines.append(new Line(-15,  -4, 18,  -4,QPen(Qt::red,1)));
  Lines.append(new Line( -15,  -6, 18,  -6,QPen(Qt::red,1)));
  Lines.append(new Line( -15,  -2, 18,  -2,QPen(Qt::red,1)));



  Ports.append(new Port(-30,  0));
  Ports.append(new Port( 30,  0));

  x1 = -30; y1 = -13;
  x2 =  30; y2 =  13;

  tx = x1+4;
  ty = y2+4;
  Model = "coil";
  Name  = "coil";

  Props.append(new Property("N", "5", true,
		QObject::tr("Number of turns")));
  Props.append(new Property("Coil_Radius ", "0.5 mm", true,
		QObject::tr("Mean radius of the solenoid")));
  Props.append(new Property("Cond_Radius ", "50 um", true,
		QObject::tr("Mean radius of the conductor")));
  Props.append(new Property("pitch", "0.5 mm", true,
		QObject::tr("Spacing between turns")));
  Props.append(new Property("rho_0", "1.724e-8", true,
		QObject::tr("Conductor's resistivity")));
  Props.append(new Property("T_0", "293 K", true,
		QObject::tr("Temperature at which the resistivity of the conductor was measured")));
  Props.append(new Property("T", "300 K", true,
		QObject::tr("Temperature")));
  Props.append(new Property("alpha", "0.00393", true,
		QObject::tr("Temperature coefficient of the conductor")));
  Props.append(new Property("e_r", "1", true,
		QObject::tr("Relative permitivity of the material between turns")));
  Props.append(new Property("mu_r", "1", true,
		QObject::tr("Relative permeability of the core")));
  Props.append(new Property("fH", "0 MHz", true,
		QObject::tr("Frequency at which the mu_r of the core decays 3dB")));
  Props.append(new Property("L_formula", "Wheeler", true,
		QObject::tr("Inductance formula")+
		" [Wheeler, Improved_solenoid_equation]"));
}
coil::~coil()
{
}

Component* coil::newOne()
{
  return new coil();
}

Element* coil::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Coil");
  BitmapFile = (char *) "coil";

  if(getNewOne)  return new coil();
  return 0;
}
