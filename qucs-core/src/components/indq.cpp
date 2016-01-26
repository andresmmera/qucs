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

//--------------------------------------------------------
// References:
// [1] Bahl. Lumped elements for RF and microwave circuits. Pg 22

indq::indq () : circuit (2) {
  type = CIR_INDQ;
}
//------------------------------------------------------------------
// This function calculates the ABCD matrix of a lossy inductance
// Q = 2*pi*f*L/Rs
// where Rs is the series resistance and L the inductance
void indq::calcSP (nr_double_t frequency) {
 nr_double_t L = getPropertyDouble ("L");
 nr_double_t Q = getPropertyDouble ("Q");
 nr_double_t f = getPropertyDouble ("f");
 nr_double_t Rs = 0;
 if ((f!=0) && (Q!=0))
 {
   nr_double_t Qf=Q;
   if (!strcmp (getPropertyString ("Mode"), "Linear")) Qf*=frequency/f;
   if (!strcmp (getPropertyString ("Mode"), "Quadratic"))Qf*=qucs::sqrt(frequency/f);
   Rs = 1./(2.*pi*frequency*L*Qf);
 }
 nr_complex_t Z = nr_complex_t (Rs, 2.*pi*L*frequency);
 qucs::matrix S;
 S = eye(2);
 S.set(0,0,Z/z0);
 S.set(0,1,2);
 S.set(1,0,2);
 S.set(1,1,Z/z0);
 S = S/(2.+Z/z0);
 setMatrixS(S);
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
  nr_double_t L = getPropertyDouble ("L");
  nr_double_t Q = getPropertyDouble ("Q");
  nr_double_t f = getPropertyDouble ("f");
  nr_double_t Rs = 0;
  if ((f!=0) && (Q!=0))
  {
   nr_double_t Qf=Q;
   if (!strcmp (getPropertyString ("Mode"), "Linear")) Qf*=frequency/f;
   if (!strcmp (getPropertyString ("Mode"), "Quadratic"))Qf*=qucs::sqrt(frequency/f);
   Rs = 1/(2.*pi*frequency*L*Qf);
  }
  nr_complex_t Z = nr_complex_t (Rs, 2*pi*L*frequency);

  nr_complex_t y11 = 1./Z;
  nr_complex_t y12 = -1./Z;
  nr_complex_t y21 = -1./Z;
  nr_complex_t y22 = 1./Z;
  setY (NODE_1, NODE_1, y11); setY (NODE_2, NODE_2, y22);
  setY (NODE_1, NODE_2, y12); setY (NODE_2, NODE_1, y21);
}


// properties
PROP_REQ [] = {
  { "L", PROP_REAL, { 1e-12, PROP_NO_STR }, PROP_POS_RANGE },
  { "Q", PROP_REAL, { 100, PROP_NO_STR }, PROP_POS_RANGE },
  { "f", PROP_REAL, { 100e6, PROP_NO_STR }, PROP_NO_RANGE },
  { "Mode", PROP_STR, { PROP_NO_VAL, "Linear" },
    PROP_RNG_STR3 ("Linear", "Quadratic", "Constant") },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t indq::cirdef =
  { "INDQ", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
