/*
 * mosfet.cpp - mosfet class implementation
 *
 * Copyright (C) 2004 Stefan Jahn <stefan@lkcc.org>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 *
 * $Id: mosfet.cpp,v 1.13 2004-10-16 16:42:31 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "complex.h"
#include "matrix.h"
#include "object.h"
#include "logging.h"
#include "node.h"
#include "circuit.h"
#include "net.h"
#include "component_id.h"
#include "constants.h"
#include "device.h"
#include "mosfet.h"

#define NODE_G 1 /* gate node   */
#define NODE_D 2 /* drain node  */
#define NODE_S 3 /* source node */
#define NODE_B 4 /* bulk node   */

// silicon bandgap as function of T
#define Egap(T) (1.16 - (7.02e-4 * sqr (T)) / ((T) + 1108))

mosfet::mosfet () : circuit (4) {
  rg = rs = rd = NULL;
  type = CIR_MOSFET;
}

void mosfet::calcSP (nr_double_t frequency) {
  setMatrixS (ytos (calcMatrixY (frequency)));
}

matrix mosfet::calcMatrixY (nr_double_t frequency) {

  // fetch computed operating points
  nr_double_t Cgd = getOperatingPoint ("Cgd");
  nr_double_t Cgs = getOperatingPoint ("Cgs");
  nr_double_t Cbd = getOperatingPoint ("Cbd");
  nr_double_t Cbs = getOperatingPoint ("Cbs");
  nr_double_t Cgb = getOperatingPoint ("Cgb");
  nr_double_t gbs = getOperatingPoint ("gbs");
  nr_double_t gbd = getOperatingPoint ("gbd");
  nr_double_t gds = getOperatingPoint ("gds");
  nr_double_t gm  = getOperatingPoint ("gm");
  nr_double_t gmb = getOperatingPoint ("gmb");

  // compute the models admittances
  complex Ygd = rect (0.0, 2.0 * M_PI * frequency * Cgd);
  complex Ygs = rect (0.0, 2.0 * M_PI * frequency * Cgs);
  complex Yds = gds;
  complex Ybd = rect (gbd, 2.0 * M_PI * frequency * Cbd);
  complex Ybs = rect (gbs, 2.0 * M_PI * frequency * Cbs);
  complex Ygb = rect (0.0, 2.0 * M_PI * frequency * Cgb);

  // build admittance matrix and convert it to S-parameter matrix
  matrix y (4);
  y.set (NODE_G, NODE_G, Ygd + Ygs + Ygb);
  y.set (NODE_G, NODE_D, -Ygd);
  y.set (NODE_G, NODE_S, -Ygs);
  y.set (NODE_G, NODE_B, -Ygb);
  y.set (NODE_D, NODE_G, gm - Ygd);
  y.set (NODE_D, NODE_D, Ygd + Yds + Ybd - DrainControl);
  y.set (NODE_D, NODE_S, -Yds - SourceControl);
  y.set (NODE_D, NODE_B, -Ybd + gmb);
  y.set (NODE_S, NODE_G, -Ygs - gm);
  y.set (NODE_S, NODE_D, -Yds + DrainControl);
  y.set (NODE_S, NODE_S, Ygs + Yds + Ybs + SourceControl);
  y.set (NODE_S, NODE_B, -Ybs - gmb);
  y.set (NODE_B, NODE_G, -Ygb);
  y.set (NODE_B, NODE_D, -Ybd);
  y.set (NODE_B, NODE_S, -Ybs);
  y.set (NODE_B, NODE_B, Ybd + Ybs + Ygb);

  return y;
}

void mosfet::calcNoise (nr_double_t frequency) {
  nr_double_t Kf  = getPropertyDouble ("Kf");
  nr_double_t Af  = getPropertyDouble ("Af");
  nr_double_t Ffe = getPropertyDouble ("Ffe");
  nr_double_t gm  = getOperatingPoint ("gm");
  nr_double_t Ids = getOperatingPoint ("Id");
  nr_double_t T   = getPropertyDouble ("Temp");

  /* compute channel noise and flicker noise generated by the DC
     transconductance and current flow from drain to source */
  nr_double_t i = 8 * kelvin (T) / T0 * gm / 3 +
    Kf * pow (Ids, Af) / pow (frequency, Ffe) / kB / T0;

  /* build noise current correlation matrix and convert it to
     noise-wave correlation matrix */
  matrix y = matrix (4);
  y.set (NODE_D, NODE_D, +i);
  y.set (NODE_S, NODE_S, +i);
  y.set (NODE_D, NODE_S, -i);
  y.set (NODE_S, NODE_D, -i);
  setMatrixN (cytocs (y * z0, getMatrixS ()));
}

void mosfet::initDC (void) {

  // initialize starting values
  setV (NODE_G, 0.8);
  setV (NODE_D, 0.0);
  setV (NODE_S, 0.0);
  setV (NODE_B, 0.0);
  UgdPrev = real (getV (NODE_G) - getV (NODE_D));
  UgsPrev = real (getV (NODE_G) - getV (NODE_S));
  UbsPrev = real (getV (NODE_B) - getV (NODE_S));
  UbdPrev = real (getV (NODE_B) - getV (NODE_D));
  UdsPrev = UgsPrev - UgdPrev;

  // initialize the MOSFET
  initModel ();

  // get device temperature
  nr_double_t T = getPropertyDouble ("Temp");

  // possibly insert series resistance at source
  if (Rs != 0.0) {
    // create additional circuit if necessary and reassign nodes
    rs = splitResistance (this, rs, getNet (), "Rs", "source", NODE_S);
    rs->setProperty ("Temp", T);
    rs->setProperty ("R", Rs);
  }
  // no series resistance at source
  else {
    disableResistance (this, rs, getNet (), NODE_S);
  }

  // possibly insert series resistance at gate
  nr_double_t Rg = getPropertyDouble ("Rg");
  if (Rg != 0.0) {
    // create additional circuit if necessary and reassign nodes
    rg = splitResistance (this, rg, getNet (), "Rg", "gate", NODE_G);
    rg->setProperty ("Temp", T);
    rg->setProperty ("R", Rg);
  }
  // no series resistance at source
  else {
    disableResistance (this, rg, getNet (), NODE_G);
  }

  // possibly insert series resistance at drain
  if (Rd != 0.0) {
    // create additional circuit if necessary and reassign nodes
    rd = splitResistance (this, rd, getNet (), "Rd", "drain", NODE_D);
    rd->setProperty ("Temp", T);
    rd->setProperty ("R", Rd);
  }
  // no series resistance at drain
  else {
    disableResistance (this, rd, getNet (), NODE_D);
  }
}

void mosfet::initModel (void) {

  // get device temperature
  nr_double_t T = getPropertyDouble ("Temp");

  // apply polarity of MOSFET
  char * type = getPropertyString ("Type");
  pol = !strcmp (type, "pfet") ? -1 : 1;

  // calculate effective channel length
  nr_double_t L  = getPropertyDouble ("L");
  nr_double_t Ld = getPropertyDouble ("Ld");
  if ((Leff = L - 2 * Ld) <= 0) {
    logprint (LOG_STATUS, "WARNING: effective MOSFET channel length %g <= 0, "
	      "set to L = %g\n", Leff, L);
    Leff = L;
  }

  // calculate gate oxide overlap capacitance
  nr_double_t W   = getPropertyDouble ("W");
  nr_double_t Tox = getPropertyDouble ("Tox");
  if (Tox <= 0) {
    logprint (LOG_STATUS, "WARNING: disabling gate oxide capacitance, "
	      "Cox = 0\n");
    Cox = 0;
  } else {
    Cox = (ESiO2 * E0 / Tox);
  }

  // calculate DC transconductance coefficient
  nr_double_t Kp = getPropertyDouble ("Kp");
  nr_double_t Uo = getPropertyDouble ("Uo");
  if (Kp > 0) {
    beta = Kp * W / Leff;
  } else {
    if (Cox > 0 && Uo > 0) {
      beta = Uo * 1e-4 * Cox * W / Leff;
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Uo or Kp to get a valid "
		"transconductance coefficient\n");
      beta = 2e-5 * W / Leff;
    }
  }

  // calculate surface potential
  nr_double_t P    = getPropertyDouble ("Phi");
  nr_double_t Nsub = getPropertyDouble ("Nsub");
  nr_double_t Ut   = T0 * kBoverQ;
  if ((Phi = P) <= 0) {
    if (Nsub > 0) {
      if (Nsub * 1e6 >= Ni) {
	Phi = 2 * Ut * log (Nsub * 1e6 / Ni);
      } else {
	logprint (LOG_STATUS, "WARNING: substrate doping less than instrinsic "
		  "density, adjust Nsub >= %g\n", Ni / 1e6);
	Phi = 0.6;
      }
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Nsub or Phi to get a valid "
		"surface potential\n");
      Phi = 0.6;
    }
  }

  // calculate bulk threshold
  nr_double_t G = getPropertyDouble ("Gamma");
  if ((Ga = G) < 0) {
    if (Cox > 0 && Nsub > 0) {
      Ga = sqrt (2 * Q * ESi * E0 * Nsub * 1e6) / Cox;
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Nsub or Gamma to get a "
		"valid bulk threshold\n");
      Ga = 0.0;
    }
  }

  // calculate threshold voltage
  nr_double_t Vt0 = getPropertyDouble ("Vt0");
  if ((Vto = Vt0) < 0) {
    nr_double_t Tpg = getPropertyDouble ("Tpg");
    nr_double_t Nss = getPropertyDouble ("Nss");
    nr_double_t PhiMS, PhiG, Eg;
    // bandgap for silicon
    Eg = Egap (kelvin (T));
    if (Tpg != 0.0) { // n-poly or p-poly
      PhiG = 4.15 + Eg / 2 - pol * Tpg * Eg / 2;
    } else {          // alumina
      PhiG = 4.1;
    }
    PhiMS = PhiG - (4.15 + Eg / 2 + pol * Phi / 2);
    if (Nss >= 0 && Cox > 0) {
      Vto = PhiMS - Q * Nss * 1e4 / Cox + pol * (Phi + Ga * sqrt (Phi));
    } else {
      logprint (LOG_STATUS, "WARNING: adjust Tox, Nss or Vt0 to get a "
		"valid threshold voltage\n");
      Vto = 0.0;
    }
  }

  Cox = Cox * W * Leff;

  // calculate drain and source resistance if necessary
  nr_double_t Rsh = getPropertyDouble ("Rsh");
  nr_double_t Nrd = getPropertyDouble ("Nrd");
  nr_double_t Nrs = getPropertyDouble ("Nrs");
  Rd = getPropertyDouble ("Rd");
  Rs = getPropertyDouble ("Rs");
  if (Rsh > 0) {
    if (Nrd > 0) Rd += Rsh * Nrd;
    if (Nrs > 0) Rs += Rsh * Nrs;
  }

  // calculate zero-bias junction capacitance
  nr_double_t Cj = getPropertyDouble ("Cj");
  nr_double_t Pb = getPropertyDouble ("Pb");
  if (Cj <= 0) {
    if (Pb > 0 && Nsub > 0) {
      Cj = sqrt (ESi * E0 * Q * Nsub * 1e6 / 2 / Pb);
    }
    else {
      logprint (LOG_STATUS, "WARNING: adjust Pb, Nsub or Cj to get a "
		"valid square junction capacitance\n");
      Cj = 0.0;
    }
    setProperty ("Cj", Cj);
  }

  // calculate junction capacitances
  nr_double_t Cbd0 = getPropertyDouble ("Cbd");
  nr_double_t Cbs0 = getPropertyDouble ("Cbs");
  nr_double_t Ad   = getPropertyDouble ("Ad");
  nr_double_t As   = getPropertyDouble ("As");
  if (Cbd0 <= 0) {
    Cbd0 = Cj * Ad;
    setProperty ("Cbd", Cbd0);
  }
  if (Cbs0 <= 0) {
    Cbs0 = Cj * As;
    setProperty ("Cbs", Cbs0);
  }

  // calculate periphery junction capacitances
  nr_double_t Cjs = getPropertyDouble ("Cjsw");
  nr_double_t Pd  = getPropertyDouble ("Pd");
  nr_double_t Ps  = getPropertyDouble ("Ps");
  setProperty ("Cbds", Cjs * Pd);
  setProperty ("Cbss", Cjs * Ps);

  // calculate junction capacitances and saturation currents
  nr_double_t Js  = getPropertyDouble ("Js");
  nr_double_t Is  = getPropertyDouble ("Is");
  nr_double_t Isd = (Ad > 0) ? Js * Ad : Is;
  nr_double_t Iss = (As > 0) ? Js * As : Is;
  setProperty ("Isd", Isd);
  setProperty ("Iss", Iss);

#if DEBUG
  logprint (LOG_STATUS, "NOTIFY: Cox=%g, Beta=%g Ga=%g, Phi=%g, Vto=%g\n",
	    Cox, beta, Ga, Phi, Vto);
#endif /* DEBUG */
}

void mosfet::calcDC (void) {

  // fetch device model parameters
  nr_double_t Isd = getPropertyDouble ("Isd");
  nr_double_t Iss = getPropertyDouble ("Iss");
  nr_double_t n   = getPropertyDouble ("N");
  nr_double_t l   = getPropertyDouble ("Lambda");
  nr_double_t T   = getPropertyDouble ("Temp");

  nr_double_t Ugs, Ugd, Ut, IeqBS, IeqBD, IeqDS, UbsCrit, UbdCrit;
  nr_double_t Uds, gtiny, Ubs, Ubd;

  T = kelvin (T);
  Ut = T * kBoverQ;
  Ugd = real (getV (NODE_G) - getV (NODE_D)) * pol;
  Ugs = real (getV (NODE_G) - getV (NODE_S)) * pol;
  Ubs = real (getV (NODE_B) - getV (NODE_S)) * pol;
  Ubd = real (getV (NODE_B) - getV (NODE_D)) * pol;
  Uds = Ugs - Ugd;

  // critical voltage necessary for bad start values
  UbsCrit = pnCriticalVoltage (Iss, Ut * n);
  UbdCrit = pnCriticalVoltage (Isd, Ut * n);

  // for better convergence
  if (Uds >= 0) {
    Ugs = fetVoltage (Ugs, UgsPrev, Vto * pol);
    Uds = Ugs - Ugd;
    Uds = fetVoltage (Uds, UdsPrev, Vto * pol);
    Ugd = Ugs - Uds;
  }
  else {
    Ugd = fetVoltage (Ugd, UgdPrev, Vto * pol);
    Uds = Ugs - Ugd;
    Uds = -fetVoltage (-Uds, -UdsPrev, Vto * pol);
    Ugs = Ugd + Uds;
  }
  if (Uds >= 0) {
    Ubs = pnVoltage (Ubs, UbsPrev, Ut * n, UbsCrit);
    Ubd = Ubs - Uds;
  }
  else {
    Ubd = pnVoltage (Ubd, UbdPrev, Ut * n, UbdCrit);
    Ubs = Ubd + Uds;
  }
  UgsPrev = Ugs; UgdPrev = Ugd; UbsPrev = Ubs; UbdPrev = Ubd; UdsPrev = Uds; 
  
  // parasitic bulk-source diode
  gtiny = Ubs < - 10 * Ut * n ? Iss : 0;
  gbs = pnConductance (Ubs, Iss, Ut * n) + gtiny;
  Ibs = pnCurrent (Ubs, Iss, Ut * n) + gtiny * Ubs;

  // parasitic bulk-drain diode
  gtiny = Ubd < - 10 * Ut * n ? Isd : 0;
  gbd = pnConductance (Ubd, Isd, Ut * n) + gtiny;
  Ibd = pnCurrent (Ubd, Isd, Ut * n) + gtiny * Ubd;

  // differentiate inverse and forward mode
  MOSdir = (Uds >= 0) ? +1 : -1;

  // first calculate sqrt (Upn - Phi)
  nr_double_t Upn = (MOSdir > 0) ? Ubs : Ubd;
  nr_double_t Sarg, Sphi = sqrt (Phi);
  if (Upn <= 0) {
    // take equation as is
    Sarg = sqrt (Phi - Upn);
  }
  else {
    // taylor series of "sqrt (x - 1)" -> continual at Ubs/Ubd = 0
    Sarg = Sphi - Upn / Sphi / 2;
    Sarg = MAX (Sarg, 0);
  }

  // calculate bias-dependent threshold voltage
  Uon = Vto * pol + Ga * (Sarg - Sphi);
  nr_double_t Utst = ((MOSdir > 0) ? Ugs : Ugd) - Uon;
  // no infinite backgate transconductance (if non-zero Ga)
  nr_double_t arg = (Sarg != 0.0) ? (Ga / Sarg / 2) : 0;

  // cutoff region
  if (Utst <= 0) {
    Ids = 0;
    gm  = 0;
    gds = 0;
    gmb = 0;
  }
  else {
    nr_double_t Vds = Uds * MOSdir;
    nr_double_t b   = beta * (1 + l * Vds);
    // saturation region
    if (Utst <= Vds) {
      Ids = b * Utst * Utst / 2;
      gm  = b * Utst;
      gds = l * beta * Utst * Utst / 2;
    }
    // linear region
    else {
      Ids = b * Vds * (Utst - Vds / 2);
      gm  = b * Vds;
      gds = b * (Utst - Vds) + l * beta * Vds * (Utst - Vds / 2);
    }
    gmb = gm * arg;
  }
  Udsat = pol * MAX (Utst, 0);
  Ids = MOSdir * Ids;
  Uon = pol * Uon;

  // compute autonomic current sources
  IeqBD = Ibd - gbd * Ubd;
  IeqBS = Ibs - gbs * Ubs;

  // exchange controlling nodes if necessary
  SourceControl = (MOSdir > 0) ? (gm + gmb) : 0;
  DrainControl  = (MOSdir < 0) ? (gm + gmb) : 0;
  if (MOSdir > 0) {
    IeqDS = Ids - gm * Ugs - gmb * Ubs - gds * Uds;
  } else {
    IeqDS = Ids - gm * Ugd - gmb * Ubd - gds * Uds;
  }

  setI (NODE_G, 0);
  setI (NODE_D, (+IeqBD - IeqDS) * pol);
  setI (NODE_S, (+IeqBS + IeqDS) * pol);
  setI (NODE_B, (-IeqBD - IeqBS) * pol);

  // apply admittance matrix elements
  setY (NODE_G, NODE_G, 0);
  setY (NODE_G, NODE_D, 0);
  setY (NODE_G, NODE_S, 0);
  setY (NODE_G, NODE_B, 0);
  setY (NODE_D, NODE_G, gm);
  setY (NODE_D, NODE_D, gds + gbd - DrainControl);
  setY (NODE_D, NODE_S, -gds - SourceControl);
  setY (NODE_D, NODE_B, gmb - gbd);
  setY (NODE_S, NODE_G, -gm);
  setY (NODE_S, NODE_D, -gds + DrainControl);
  setY (NODE_S, NODE_S, gbs + gds + SourceControl);
  setY (NODE_S, NODE_B, -gbs - gmb);
  setY (NODE_B, NODE_G, 0);
  setY (NODE_B, NODE_D, -gbd);
  setY (NODE_B, NODE_S, -gbs);
  setY (NODE_B, NODE_B, gbs + gbd);
}

void mosfet::calcOperatingPoints (void) {

  // fetch device model parameters
  nr_double_t Cbd0 = getPropertyDouble ("Cbd");
  nr_double_t Cbs0 = getPropertyDouble ("Cbs");
  nr_double_t Cbds = getPropertyDouble ("Cbds");
  nr_double_t Cbss = getPropertyDouble ("Cbss");
  nr_double_t Cgso = getPropertyDouble ("Cgso");
  nr_double_t Cgdo = getPropertyDouble ("Cgdo");
  nr_double_t Cgbo = getPropertyDouble ("Cgbo");
  nr_double_t Pb   = getPropertyDouble ("Pb");
  nr_double_t M    = getPropertyDouble ("Mj");
  nr_double_t Ms   = getPropertyDouble ("Mjsw");
  nr_double_t Fc   = getPropertyDouble ("Fc");
  nr_double_t Tt   = getPropertyDouble ("Tt");
  nr_double_t W    = getPropertyDouble ("W");
  
  nr_double_t Ubs, Ubd, Cbs, Cbd, Ugs, Ugd, Uds;
  nr_double_t Cgd, Cgb, Cgs;

  Ugd = real (getV (NODE_G) - getV (NODE_D)) * pol;
  Ugs = real (getV (NODE_G) - getV (NODE_S)) * pol;
  Ubs = real (getV (NODE_B) - getV (NODE_S)) * pol;
  Ubd = real (getV (NODE_B) - getV (NODE_D)) * pol;
  Uds = Ugs - Ugd;

  // capacitance of bulk-drain diode
  Cbd = gbd * Tt + pnCapacitance (Ubd, Cbd0, Pb, M, Fc) +
    pnCapacitance (Ubd, Cbds, Pb, Ms, Fc);
  Qbd = Ibd * Tt + pnCharge (Ubd, Cbd0, Pb, M, Fc) +
    pnCharge (Ubd, Cbds, Pb, Ms, Fc);

  // capacitance of bulk-source diode
  Cbs = gbs * Tt + pnCapacitance (Ubs, Cbs0, Pb, M, Fc) +
    pnCapacitance (Ubs, Cbss, Pb, Ms, Fc);
  Qbs = Ibs * Tt + pnCharge (Ubs, Cbs0, Pb, M, Fc) +
    pnCharge (Ubs, Cbss, Pb, Ms, Fc);

  // calculate bias-dependent MOS overlap capacitances
  if (MOSdir > 0) {
    fetCapacitanceMeyer (Ugs, Ugd, Uon, Udsat, Phi, Cox, Cgs, Cgd, Cgb);
  } else {
    fetCapacitanceMeyer (Ugd, Ugs, Uon, Udsat, Phi, Cox, Cgd, Cgs, Cgb);
  }
  Cgs += Cgso * W;
  Cgd += Cgdo * W;
  Cgb += Cgbo * Leff;

  // save operating points
  setOperatingPoint ("Id", Ids);
  setOperatingPoint ("gm", gm);
  setOperatingPoint ("gmb", gmb);
  setOperatingPoint ("gds", gds);
  setOperatingPoint ("Vth", Vto);
  setOperatingPoint ("Vdsat", Udsat);
  setOperatingPoint ("gbs", gbs);
  setOperatingPoint ("gbd", gbd);
  setOperatingPoint ("Vgs", Ugs);
  setOperatingPoint ("Vgd", Ugd);
  setOperatingPoint ("Vbs", Ubs);
  setOperatingPoint ("Vbd", Ubd);
  setOperatingPoint ("Vds", Ugs - Ugd);
  setOperatingPoint ("Cbd", Cbd);
  setOperatingPoint ("Cbs", Cbs);
  setOperatingPoint ("Cgs", Cgs);
  setOperatingPoint ("Cgd", Cgd);
  setOperatingPoint ("Cgb", Cgb);
}

void mosfet::initAC (void) {
  clearI ();
}

void mosfet::calcAC (nr_double_t frequency) {
  setMatrixY (calcMatrixY (frequency));
}

#define qgdState 0 // gate-drain charge state
#define cgdState 1 // gate-drain current state
#define qgsState 2 // gate-source charge state
#define cgsState 3 // gate-source current state
#define qbdState 4 // bulk-drain charge state
#define cbdState 5 // bulk-drain current state
#define qbsState 6 // bulk-source charge state
#define cbsState 7 // bulk-source current state
#define qgbState 8 // gate-bulk charge state
#define cgbState 9 // gate-bulk current state

void mosfet::initTR (void) {
  setStates (10);
  initDC ();
}

void mosfet::calcTR (nr_double_t) {
  calcDC ();
  calcOperatingPoints ();

  nr_double_t Cgd = getOperatingPoint ("Cgd");
  nr_double_t Cgs = getOperatingPoint ("Cgs");
  nr_double_t Cbd = getOperatingPoint ("Cbd");
  nr_double_t Cbs = getOperatingPoint ("Cbs");
  nr_double_t Cgb = getOperatingPoint ("Cgb");
  nr_double_t Ugd = getOperatingPoint ("Vgd");
  nr_double_t Ugs = getOperatingPoint ("Vgs");
  nr_double_t Ubd = getOperatingPoint ("Vbd");
  nr_double_t Ubs = getOperatingPoint ("Vbs");
  nr_double_t Ugb = Ugs - Ubs;

  transientCapacitance (qgdState, NODE_G, NODE_D, Cgd, Ugd, Cgd * Ugd);
  transientCapacitance (qgsState, NODE_G, NODE_S, Cgs, Ugs, Cgs * Ugs);
  transientCapacitance (qbdState, NODE_B, NODE_D, Cbd, Ubd, Qbd);
  transientCapacitance (qbsState, NODE_B, NODE_S, Cbs, Ubs, Qbs);
  transientCapacitance (qgbState, NODE_G, NODE_B, Cgb, Ugb, Cgb * Ugb);
}
