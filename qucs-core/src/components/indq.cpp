/*
 * indq.cpp - Lossy inductor class implementation
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
#include "indq.h"
#include "iostream"
using namespace qucs;

indq::indq () : circuit (2) {
  type = CIR_INDQ;
}
//------------------------------------------------------------------
// This function calculates the ABCD matrix of a lossy inductance
// Q = 2*pi*f*L/Rs
// where Rs is the series resistance and L the inductance
void indq::calcABCDparams(nr_double_t frequency)
{
 nr_double_t L = getPropertyDouble ("L");
 nr_double_t Q = getPropertyDouble ("Q");
 nr_double_t f = getPropertyDouble ("f");
 if ((f==0) || (Q==0)) Q = 1e10; //In case of unphysical inputs, the capacitor is forced to be ideal
 nr_double_t Qf=Q;

 if (!strcmp (getPropertyString ("Mode"), "Linear")) Qf*=frequency/f;
 if (!strcmp (getPropertyString ("Mode"), "Quadratic"))Qf*=qucs::sqrt(frequency/f);
 nr_double_t Rs = 1/(2*pi*frequency*L*Qf);

 ABCD = eye(2);
 ABCD.set(0,0,1);
 ABCD.set(0,1, Rs + 2*pi*L*frequency*nr_complex_t (0, 1));
 ABCD.set(1,0,0);
 ABCD.set(1,1,1);
}

void indq::calcSP (nr_double_t frequency) {
  calcABCDparams(frequency);
  matrix Stmp = qucs::atos(ABCD, z0, z0);
  setMatrixS(Stmp);
}



void indq::initDC (void) {
  //Short circuit
  setVoltageSources (1);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void indq::initAC (void) {
  setVoltageSources (0);
  allocMatrixMNA ();
}


void indq::initSP(void)
{
  allocMatrixS ();
}

void indq::calcAC (nr_double_t frequency) {
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
  { "L", PROP_REAL, { 1e-12, PROP_NO_STR }, PROP_POS_RANGE },
  { "Q", PROP_REAL, { 100, PROP_NO_STR }, PROP_POS_RANGE },
  { "f", PROP_REAL, { 100e6, PROP_NO_STR }, PROP_NO_RANGE },
  { "Mode", PROP_STR, { PROP_NO_VAL, "Proportional to freq" },
    PROP_RNG_STR3 ("Linear", "Quadratic", "Constant") },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t indq::cirdef =
  { "INDQ", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
