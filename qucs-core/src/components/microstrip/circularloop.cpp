/*
 * circularloop.cpp - Circular loop inductor class implementation
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
#include "circularloop.h"
#include "substrate.h"

using namespace qucs;


//------------------------------------------------------------------------
// References:
// [1] I. Bahl, Fundamentals of RF and Microwave Transistor Amplifiers, John Wiley and Sons, 2009.  
//------------------------------------------------------------------------
circularloop::circularloop () : circuit (2) {
  type = CIR_CIRCULARLOOP;
}
//------------------------------------------------------------------
// This function calculates the ABCD matrix of the spiral inductance
void circularloop::calcABCDparams(nr_double_t frequency)
{
 nr_double_t Z0, ere, F, eta = 120.*pi;
 nr_double_t W = getPropertyDouble ("W");//Width
 nr_double_t a = getPropertyDouble ("a");//Radius
 substrate * subst = getSubstrate ();
  
 nr_double_t h = subst->getPropertyDouble ("h");
 nr_double_t rho = subst->getPropertyDouble ("rho");
 nr_double_t t = subst->getPropertyDouble ("t");
 nr_double_t er = subst->getPropertyDouble ("er");


// [1] Page 127
 if (W/h <= 1.)
 {
   F = 1./qucs::sqrt(1.+12.*h/W) + 0.041*(1.-W/h)*(1.-W/h);
   ere = (er+1.)/2. + F*(er-1.)/2.;
   Z0 = (eta/(2.*pi*qucs::sqrt(ere)))*std::log((8.*h/W) + 0.25*W/h);
 }
 else
 {
   F = 1./qucs::sqrt(1.+12.*h/W);
   ere = (er+1.)/2. + F*(er-1.)/2.;
   Z0 = (eta/qucs::sqrt(ere))*(1./((W/h) + 1.393 + 0.667*std::log(W/h + 1.444)));
 }
   
// [1] Page 138 
 nr_double_t Kg = 0.57 - 0.145*std::log(W/h);
 nr_double_t K = 1.4 + 0.217*std::log(W/(5.*t));// Correction factor to include the effect of the current bunching at the corners

 nr_double_t L = 1.257e-12*(a*1e6)*(std::log(a/(W+t)) + 0.078)*Kg;
 nr_double_t Rs = rho/t;
 nr_double_t R = pi*a*K*Rs/(W+t);
 nr_double_t C = 33.33e-16*pi*a*1e6*qucs::sqrt(ere)/Z0;


 //ABCD matrix
 nr_complex_t I = nr_complex_t(0,1);
 ABCD = eye(2);
 ABCD.set(0,0,2.*I*pi*(2.*I*pi*L*frequency + R)*C*frequency + 1. );
 ABCD.set(0,1,2.*I*pi*L*frequency + R);
 ABCD.set(1,0,-2.*I*pi*(4.*pi*pi*C*L*frequency*frequency - 2.*I*pi*C*R*frequency - 1.)*C*frequency + 2.*I*pi*C*frequency);
 ABCD.set(1,1, -4.*pi*pi*C*L*frequency*frequency + 2.*I*pi*C*R*frequency + 1.);
}

void circularloop::calcSP (nr_double_t frequency) {
  calcABCDparams(frequency);
  matrix Stmp = qucs::atos(ABCD, z0, z0);
  setMatrixS(Stmp);
}



void circularloop::initDC (void) {
  //Short circuit
  setVoltageSources (1);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void circularloop::initAC (void) {
  setVoltageSources (0);
  allocMatrixMNA ();
}


void circularloop::initSP(void)
{
  allocMatrixS ();
}

void circularloop::calcAC (nr_double_t frequency) {
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
  { "a", PROP_REAL, { 200e-6, PROP_NO_STR }, PROP_POS_RANGE },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t circularloop::cirdef =
  { "CIRCULARLOOP", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
