/*
 * coil.cpp - Coil class implementation
 *
 * Copyright (C) 2016 Andres Martinez-Mera <andresmartinezmera@gmail.com>
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
#include "coil.h"

#include <fstream>
#include <string>
#include <iostream>
using namespace qucs;

//--------------------------------------------------------
// References:
// [1] Marian K. Kazimierczuk, “High-Frequency Magnetic Components”, John Wiley & Sons, Ltd. 2009 
// [2] Alberto Reatti and Marian K. Kazimierczuk, „Comparison of Various Methods for Calculating 
// the AC Resistance of Inductors”, IEEE Transactions on Magnetics, Vol. 38, No. 3, May 2002 
// [3] R. Lundin, "A Handbook Formula for the Inductance of a Single-Layer Circular Coil," Proc. IEEE, 
// vol. 73, no. 9, pp. 1428-1429, Sep. 1985.


// Equivalent circuit:
//  >------- L ----- R --------<
//        |_____ C _____|
 
coil::coil () : circuit (2) {
  type = CIR_COIL;
}

// ([1], page 165)
nr_double_t coil::calculateSeriesR(nr_double_t rho_0, nr_double_t T_0, nr_double_t T, nr_double_t alpha, nr_double_t r, nr_double_t D, nr_double_t pitch, nr_double_t frequency, nr_complex_t mu)
{
  nr_double_t omega = 2*pi*frequency;
  // Series resistance
  nr_double_t rho_t = rho_0*(1+alpha*(T-T_0));//Calculation of the resistivity as a function of the frequency. 
  nr_double_t R_DC = rho_t*D/(r*r);
  nr_double_t skin_depth = qucs::sqrt((2*rho_t)/(omega*4e-7*pi));//mu_r = 1 \forall freq for common conductors, like Copper
  // Dowell's method ([1], page 296) [2]
  nr_double_t A = qucs::pow(pi/4, .75)*(r/skin_depth)*qucs::sqrt(r/pitch);//Normalised thickness wrt the skin effect
  nr_double_t F = A*(qucs::sinh(2*A)+qucs::sin(2*A))/(qucs::cosh(2*A) - qucs::cos(2*A));//Skin effect contribution to the increasement of the AC resistance
  nr_double_t Rhisteresis = -omega*imag(mu)*L;//Loss caused by the core hysteresis
  return R = R_DC*F + Rhisteresis;
}

nr_double_t coil::f1(nr_double_t x)
{
   return (1+.383901*x+.017108*x*x)/(1+ .258952*x);
}


nr_double_t coil::f2(nr_double_t x)
{
   return .093842*x + .002029*x*x - .000801*x*x*x;
}

//Inductance calculation according to Lundin's formula
nr_double_t coil::getL_Lundin(nr_double_t a, nr_double_t b, nr_double_t mu, int N)
{
int ab_ratio = .25*b*b/(a*a);
 if ((ab_ratio > 0)&&(ab_ratio <= 1))
 {
    if (2*a<=b)
    {
      L = (mu*N*N*pi*a*a/b)*(f1(4*a*a/(b*b)) - 8*a/(3*pi*b));
    }
    else
    {
      L = mu*N*N*a*(f1(.25*b*b/(a*a))*(qucs::log(8*a/b)-.5) - f2(.25*b*b/(a*a)))
    }
 }
 else
 {
   L = -1;
 }
 return L;
}


//([1], page 139)
nr_complex_t coil::getPermeability(nr_double_t fH, nr_double_t frequency)
{
 nr_double_t mu_0 = 4e-7*pi;
 nr_complex_t mu;
 if (fH > 0)
 {
 nr_double_t mu_rr = mu_r/(1+ (frequency*frequency)/(fH*fH));//Real part of the relative permeability as a function of frequency
 nr_double_t mu_ri = (frequency/fH)*mu_r/(1+ (frequency*frequency)/(fH*fH));
 mu = mu_0*nr_complex_t(mu_rr, -mu_ri);
 }
 else
 {//mu_r is supposed to be constant vs frequency. The imaginary part of the permeability is not estimated => the loss caused by
  // the hysteresis effect is not taken into consideration.
  mu = nr_complex_t(mu_0*mu_r,0);
 }
return mu;
}

nr_double_t coil::getSelfCapacitance(nr_double_t D, nr_double_t len, nr_double_t e_rx, nr_double_t e_ri, nr_double_t pitch_angle)
{
 nr_double_t Kc = 0.717439*(D/len) + .933048*qucs::pow(D/len, .66666667) + .106*(D*D/(len*len));
 return (4*e_0*e_rx*len/pi)*qucs::cos(pitch_angle)*qucs::cos(pitch_angle)*(1+.5*Kc*(1+e_ri/e_rx)); 
}

nr_complex_t coil::calcZ(nr_double_t frequency)
{
 nr_double_t omega = 2*pi*frequency;
 nr_double_t N = getPropertyDouble ("N");//Number of turns
 nr_double_t Coil_Radius = getPropertyDouble ("D");//Coil diameter
 nr_double_t Cond_Radius = getPropertyDouble ("d");//Wire diameter
 nr_double_t mu_r = getPropertyDouble ("mu_r");//Relative permeability of the core
 nr_double_t rho_0 = getPropertyDouble ("rho_0");//Wire resistivity measured at a given temperature
 nr_double_t T_0 = getPropertyDouble ("T_0");//Temperature at which rho_0 was measured
 nr_double_t T = getPropertyDouble ("T");//Current temperature
 nr_double_t alpha = getPropertyDouble("alpha");//Temperature coefficient of the conductor resistivity
 nr_double_t pitch = getPropertyDouble ("pitch");//Pitch, spacing between turns
 nr_double_t pitch_angle = getPropertyDouble("pitch_angle");//Pitch angle (in degrees)
 nr_double_t erx = getPropertyDouble("e_rx");//Dielectric permittivity of the coil former
 nr_double_t eri = getPropertyDouble("e_ri");//Dielectric permittivity of the coil hollow
 nr_double_t fH = getPropertyDouble("fH");//In case the core is a ferromagnetic material, this field states for the 3dB frequency threshold. It indicates at which frequency real(mu) = .5*mu_0 of the core material. Otherwise, this parameter must be < 0.


 nr_complex_t mu = getPermeability(fH, frequency);// Core permeability 

// Equivalent circuit elements
 nr_double_t L = getL_Lundin(a, b, real(mu), nN);
 nr_double_t R = calculateSeriesR(rho_0, T_0, T, alpha, d/2, D, pitch, frequency, mu);
 nr_double_t C = getSelfCapacitance();

  return nr_complex_t (R, omega*L)/nr_complex_t(-omega*omega*L*C+1, omega*R*C);
}

void coil::calcSP (nr_double_t frequency) {
 nr_complex_t Z = calcZ(frequency);
 qucs::matrix S;
 S = eye(2);
 S.set(0,0,Z/z0);
 S.set(0,1,2);
 S.set(1,0,2);
 S.set(1,1,Z/z0);
 S = S/(2.+Z/z0);
 setMatrixS(S);
}



void coil::initDC (void) {
  //Short circuit
  setVoltageSources (1);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void coil::initAC (void) {
  setVoltageSources (0);
  allocMatrixMNA ();
}


void coil::initSP(void)
{
  allocMatrixS ();
}

void coil::calcAC (nr_double_t frequency) {
  nr_complex_t Z = calcZ(frequency);

  nr_complex_t y11 = 1./Z;
  nr_complex_t y12 = -1./Z;
  nr_complex_t y21 = -1./Z;
  nr_complex_t y22 = 1./Z;
  setY (NODE_1, NODE_1, y11); setY (NODE_2, NODE_2, y22);
  setY (NODE_1, NODE_2, y12); setY (NODE_2, NODE_1, y21);
}


// properties
PROP_REQ [] = {
  { "N", PROP_REAL, { 5, PROP_NO_STR }, PROP_POS_RANGE },
  { "Coil_Radius", PROP_REAL, { 5e-4, PROP_NO_STR }, PROP_NO_RANGE },
  { "Cond_Radius", PROP_REAL, { 5e-5, PROP_NO_STR }, PROP_NO_RANGE },
  { "pitch", PROP_REAL, { 10e-6, PROP_NO_STR }, PROP_NO_RANGE },
  { "rho_0", PROP_REAL, { 1.724e-8, PROP_NO_STR }, PROP_NO_RANGE },
  { "T_0", PROP_REAL, { 293, PROP_NO_STR }, PROP_NO_RANGE },
  { "T", PROP_REAL, { 300, PROP_NO_STR }, PROP_NO_RANGE },
  { "alpha", PROP_REAL, { 0.00393, PROP_NO_STR }, PROP_NO_RANGE },
  { "e_r", PROP_REAL, { 1, PROP_NO_STR }, PROP_NO_RANGE },
  { "mu_r", PROP_REAL, { 1, PROP_NO_STR }, PROP_NO_RANGE },
  { "fH", PROP_REAL, { 1e6, PROP_NO_STR }, PROP_NO_RANGE },
  { "L_formula", PROP_STR, { PROP_NO_VAL, "Wheeler" },
    PROP_RNG_STR2 ("Wheeler", "Lundin") },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t coil::cirdef =
  { "coil", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
