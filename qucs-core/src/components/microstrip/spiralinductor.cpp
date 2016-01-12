/*
 * spiralinductor.cpp - Spiral inductor class implementation
 *
 * Copyright (C) 2015 Andres Martinez-Mera <andresmartinezmera@gmail.com>
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
 * $Id$
 *
 */


#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "component.h"
#include "spiralinductor.h"
#include "substrate.h"
#include <iostream>
#include <fstream>
using namespace qucs;


//------------------------------------------------------------------------
// References:
// [1] I. Bahl, Lumped Elements for RF and Microwave Circuits, Artech House, Norwood, MA, 2003.
// [2] I. Bahl, Fundamentals of RF and Microwave Transistor Amplifiers, John Wiley and Sons, 2009.  
//------------------------------------------------------------------------
spiralinductor::spiralinductor () : circuit (2) {
  type = CIR_SPIRALIND;
}
//------------------------------------------------------------------
// This function calculates the ABCD matrix of the spiral inductance
void spiralinductor::calcABCDparams(nr_double_t frequency)
{
 nr_double_t N = getPropertyDouble ("N");//Number of turns
 nr_double_t Di = getPropertyDouble ("Di");//Inner diameter
 nr_double_t Do = getPropertyDouble ("Do");//Outer diameter
 nr_double_t W = getPropertyDouble ("W");//Width
 nr_double_t S = getPropertyDouble ("S");//Spacing between turns
 substrate * subst = getSubstrate ();

 nr_double_t a = (Di+Do)/4.;
 nr_double_t c = (Do-Di)/2.;
  
 nr_double_t h = subst->getPropertyDouble ("h");
 nr_double_t rho = subst->getPropertyDouble ("rho");
 nr_double_t t = subst->getPropertyDouble ("t");
 
   
 nr_double_t Kg = 0.57 - 0.145*std::log(W/h);
 nr_double_t K = 1.+0.333*std::pow(1.+S/W, -1.7); 

 nr_double_t L = (0.03937)*(a*a*N*N*1e12)*Kg*1e-9/(8.e6*a+11.e6*c);
 nr_double_t Rs = rho/t;
 nr_double_t R = K*pi*a*N*Rs/W;
 nr_double_t C = (3.5e-5*Do+0.06)*1e-12;

 //ABCD matrix
 nr_complex_t I = nr_complex_t(0,1);
 ABCD = eye(2);
 ABCD.set(0,0,2.*I*pi*(2.*I*pi*L*frequency + R)*C*frequency + 1. );
 ABCD.set(0,1,2.*I*pi*L*frequency + R);
 ABCD.set(1,0,-2.*I*pi*(4.*pi*pi*C*L*frequency*frequency - 2.*I*pi*C*R*frequency - 1.)*C*frequency + 2.*I*pi*C*frequency);
 ABCD.set(1,1, -4.*pi*pi*C*L*frequency*frequency + 2.*I*pi*C*R*frequency + 1.);
}

void spiralinductor::calcSP (nr_double_t frequency) {
  calcABCDparams(frequency);
  matrix Stmp = qucs::atos(ABCD, z0, z0);
  setMatrixS(Stmp);
}



void spiralinductor::initDC (void) {
  //Short circuit
  setVoltageSources (1);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void spiralinductor::initAC (void) {
  setVoltageSources (0);
  allocMatrixMNA ();
}


void spiralinductor::initSP(void)
{
  allocMatrixS ();
}

void spiralinductor::calcAC (nr_double_t frequency) {
  calcABCDparams(frequency);
  nr_complex_t y11 = ABCD.get(1,1)/ABCD.get(0,1);
  nr_complex_t y12 = -det(ABCD)/ABCD.get(0,1);
  nr_complex_t y21 = -1./ABCD.get(0,1);
  nr_complex_t y22 = ABCD.get(0,0)/ABCD.get(0,1);
  setY (NODE_1, NODE_1, y11); setY (NODE_2, NODE_2, y22);
  setY (NODE_1, NODE_2, y12); setY (NODE_2, NODE_1, y21);
}


// properties
PROP_REQ [] = {
  { "Subst", PROP_STR, { PROP_NO_VAL, "Subst1" }, PROP_NO_RANGE },
  { "W", PROP_REAL, { 25e-6, PROP_NO_STR }, PROP_POS_RANGE },
  { "Di", PROP_REAL, { 200e-6, PROP_NO_STR }, PROP_POS_RANGE },
  { "Do", PROP_REAL, { 400e-6, PROP_NO_STR }, PROP_NO_RANGE },
  { "S", PROP_REAL, { 25e-6, PROP_NO_STR }, PROP_NO_RANGE },
  { "N", PROP_REAL, { 3, PROP_NO_STR }, PROP_NO_RANGE },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t spiralinductor::cirdef =
  { "SPIRALIND", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
