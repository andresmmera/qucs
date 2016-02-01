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
// [3] Gabriele Grandi, Marian K. Kazimierczuk, Antonio Massarini, and Ugo Reggiani, „Stray Capacitances of Single-Layer 
// Solenoid Air-Core Inductors”, IEEE Transactions on Industry Application, Vol. 35, No.5, September/October 1999
// [4] Peter Csurgai, Miklos Kuczmann, Comparison of various high-frequency models of RF chip inductors, 
// “Szécheny István” University, Lab. of Electromagnetic Field, Dept. of Telecommunications, Hungary

// Equivalent circuit:
//  >------- L ----- R --------<
//        |_____ C _____|
 
coil::coil () : circuit (2) {
  type = CIR_COIL;
}


nr_complex_t coil::calcZ(nr_double_t frequency)
{
 nr_double_t omega = 2*pi*frequency;
 nr_double_t N = getPropertyDouble ("N");//Number of turns
 nr_double_t Coil_Radius = getPropertyDouble ("Coil_Radius");//Turn radius
 nr_double_t Cond_Radius = getPropertyDouble ("Cond_Radius");//Radius of the winding wire
 nr_double_t mu_r = getPropertyDouble ("mu_r");//Relative permeability
 nr_double_t rho_0 = getPropertyDouble ("rho_0");//Conductor resistance measured at a given temperature
 nr_double_t T_0 = getPropertyDouble ("T_0");//Temperature at which rho was measured
 nr_double_t T = getPropertyDouble ("T");//Current temperature
 nr_double_t pitch = getPropertyDouble ("pitch");//Pitch, spacing between turns
 nr_double_t er = getPropertyDouble("e_r");//Dielectric permittivity of the material between turns, typically air.
 nr_double_t fH = getPropertyDouble("fH");//3dB Frequency threshold. It indicates at which frequency real(mu) = .5*mu_0 of the core material
 nr_double_t alpha = getPropertyDouble("alpha");//Temperature coefficient of the conductor resistivity


 nr_double_t length = N*(2*Cond_Radius) + (N-1)*pitch;//Length of the solenoid
 if (pitch <= 2*Cond_Radius)//The spacing between turns must be more than the wire diameter
 {
   pitch = 2.1*Cond_Radius;
   logprint(LOG_ERROR,"Warning: The pitch cannot be less than the wire diameter.\nPitch = %g will be used instead",pitch);
 }

 nr_double_t L, mu_0 = 4e-7*pi;


// Core permeability ([1], page 139)
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

// Inductance calculation ([1], page 45)
// The high frequency model of a solenoid states that H and B vectors are not parallel, so the permeability is no longer a scalar. The real
// part of mu is related to the inductance and the imaginary part indicates the resistive loss caused by the magnetic core because of 
// the hysteris ([1], page 145)

  if (!strcmp (getPropertyString ("L_formula"), "Wheeler"))
  {
    L = real(mu)*Coil_Radius*Coil_Radius*pi*N*N/(length*(1+.9*Coil_Radius));
  }
  else{//Improved solenoid formula [4]
    L = real(mu)*Coil_Radius*Coil_Radius*pi*N*N/(qucs::sqrt(length*length + 4*Coil_Radius*Coil_Radius));
  }

  // Series resistance
  nr_double_t rho_t = rho_0*(1+alpha*(T-T_0));//Calculation of the resistivity as a function of the frequency. ([1], page 165)
  nr_double_t R_DC = rho_t*(2*Coil_Radius)/(Cond_Radius*Cond_Radius);
  nr_double_t skin_depth = qucs::sqrt((2*rho_t)/(omega*mu_0));//mu_r = 1 \forall freq for common conductors, like Copper
  // Dowell's method ([1], page 296) [2]
  nr_double_t A = qucs::pow(pi/4, .75)*(Cond_Radius/skin_depth)*qucs::sqrt(Cond_Radius/pitch);//Normalised thickness wrt the skin effect
  nr_double_t F = A*(qucs::sinh(2*A)+qucs::sin(2*A))/(qucs::cosh(2*A) - qucs::cos(2*A));//Skin effect contribution to the increasement of the AC resistance
  nr_double_t Rhisteresis = -omega*imag(mu)*L;//Loss caused by the core hysteresis
  nr_double_t R = R_DC*F + Rhisteresis;


  // Capacitance equivalent. [3]
  nr_double_t epsilon = 8.854187817620389851e-12*er;
  nr_double_t Ctt = pi*pi*(2*Coil_Radius)*epsilon/(qucs::log((.5*pitch/Cond_Radius) + qucs::sqrt(-1 + .25*pitch*pitch/(Cond_Radius*Cond_Radius))) );//Turn to turn capacitance
  nr_double_t C = Ctt/(N-1);

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
    PROP_RNG_STR2 ("Wheeler", "Improved_solenoid_equation") },
    PROP_NO_PROP };
PROP_OPT [] = {  PROP_NO_PROP };
struct define_t coil::cirdef =
  { "coil", 2, PROP_COMPONENT, PROP_NO_SUBSTRATE, PROP_LINEAR, PROP_DEF };
