/*
 * mimcap.cpp - Lossy capacitor class implementation
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
#include "mimcap.h"

using namespace qucs;

mimcap::mimcap () : circuit (2) {
  type = CIR_MIMCAP;
}

//------------------------------------------------------------------
// References:
// [1] Inder Bahl. Lumped elements for RF and Microwave Circuits. Artech House 2003

//------------------------------------------------------------------
// This function calculates the ABCD matrix of a MIM capacitor
void mimcap::calcABCDparams(nr_double_t frequency)
{
 nr_double_t W = getPropertyDouble ("W");
 nr_double_t l = getPropertyDouble ("l");
 nr_double_t h = getPropertyDouble ("h");
 nr_double_t er = getPropertyDouble ("er");
 nr_double_t tand = getPropertyDouble ("tand");
 nr_double_t rho = getPropertyDouble ("rho");
 nr_double_t t = getPropertyDouble ("t");

 nr_double_t Kg = 0.57 - 0.145*std::log(W/h);
 nr_double_t e0 = 8.854187817e-12;

 nr_double_t C = e0*er*W*l*1e6/h;
 nr_double_t Rs = rho/t;
 nr_double_t R = .6666*Rs*l/W;
 nr_double_t G = 2.*pi*frequency*C*tand;
 nr_double_t L = 2e-7*l*(std::log(l/(W+t)) + 1.193 + (W+t)/(3.*l))*Kg;
 

 ABCD = eye(2);
 nr_complex_t I = nr_complex_t(0,1);
 ABCD.set(0,0,(2.*I*pi*L*frequency + R)*((2.*I*pi*C*frequency - G)*(2.*I*pi*C*frequency - G) - (-2.*I*pi*C*frequency + G)*(-2.*I*pi*C*frequency + G))/(2.*I*pi*C*frequency - G) + 1. );
 ABCD.set(0,1, 2.*I*pi*L*frequency + R - 1./(2.*I*pi*C*frequency - G));
 ABCD.set(1,0,((2.*I*pi*C*frequency - G)*(2.*I*pi*C*frequency - G) - (-2.*I*pi*C*frequency + G)*(-2.*I*pi*C*frequency + G))/(2.*I*pi*C*frequency - G));
 ABCD.set(1,1,1);
}

void mimcap::calcSP (nr_double_t frequency) {
  calcABCDparams(frequency);
  matrix Stmp = qucs::atos(ABCD, z0, z0);
  setMatrixS(Stmp);
}



void mimcap::initDC (void) {
  allocMatrixMNA ();
  // open circuit
  clearY ();
}

void mimcap::initAC (void) {
  setVoltageSources (0);
  allocMatrixMNA ();
}


void mimcap::initSP(void)
{
  allocMatrixS ();
}

void mimcap::calcAC (nr_double_t frequency) {
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
  { "W", PROP_REAL, { 25e-6, PROP_NO_STR }, PROP_POS_RANGE },
  { "h", PROP_REAL, { 5e-6, PROP_NO_STR }, PROP_POS_RANGE },
  { "l", PROP_REAL, { 10e-6, PROP_NO_STR }, PROP_POS_RANGE },
  { "tand", PROP_REAL, { 0.0125, PROP_NO_STR }, PROP_NO_RANGE },
  { "er", PROP_REAL, { 9.8, PROP_NO_STR }, PROP_NO_RANGE },
  { "rho", PROP_REAL, { 4.1e7, PROP_NO_STR }, PROP_NO_RANGE },
  { "t", PROP_REAL, { 35e-6, PROP_NO_STR }, PROP_NO_RANGE },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t mimcap::cirdef =
  { "MIMCAP", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
