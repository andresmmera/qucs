/*
 * inductor.cpp - inductor class implementation
 *
 * Copyright (C) 2003, 2004, 2005, 2006 Stefan Jahn <stefan@lkcc.org>
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
 * $Id: inductor.cpp,v 1.18 2008-01-10 20:00:00 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#define __USE_BSD
#define __USE_XOPEN
#include <stdio.h>
#include <stdlib.h>

#include "complex.h"
#include "object.h"
#include "node.h"
#include "circuit.h"
#include "component_id.h"
#include "constants.h"
#include "inductor.h"

inductor::inductor () : circuit (2) {
  type = CIR_INDUCTOR;
  setISource (true);
}

void inductor::calcSP (nr_double_t frequency) {
  nr_double_t l = getPropertyDouble ("L") / z0;
  nr_complex_t z = rect (0, 2.0 * M_PI * frequency * l);
  setS (NODE_1, NODE_1, z / (z + 2.0));
  setS (NODE_2, NODE_2, z / (z + 2.0));
  setS (NODE_1, NODE_2, 2.0 / (z + 2.0));
  setS (NODE_2, NODE_1, 2.0 / (z + 2.0));
}

void inductor::initDC (void) {
  setVoltageSources (1);  
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void inductor::calcDC (void) {
  clearY ();
}

void inductor::initAC (void) {
  nr_double_t l = getPropertyDouble ("L");

  // for non-zero inductance usual MNA entries
  if (l != 0.0) {
    setVoltageSources (0);
    allocMatrixMNA ();
  }
  // for zero inductance create a zero voltage source
  else {
    initDC ();
    calcDC ();
  }
}

void inductor::calcAC (nr_double_t frequency) {
  nr_double_t l = getPropertyDouble ("L");

  // for non-zero inductance usual MNA entries
  if (l != 0.0) {
    nr_complex_t y = rect (0, -1 / (2.0 * M_PI * frequency * l));
    setY (NODE_1, NODE_1, +y); setY (NODE_2, NODE_2, +y);
    setY (NODE_1, NODE_2, -y); setY (NODE_2, NODE_1, -y);
  }
}

void inductor::initTR (void) {
  initDC ();
  clearY ();
  setStates (2);
}

#define fState 0 // flux state
#define vState 1 // voltage state

void inductor::calcTR (nr_double_t) {
  nr_double_t l = getPropertyDouble ("L");
  nr_double_t r, v;
  nr_double_t i = real (getJ (VSRC_1));

  /* apply initial condition if requested */
  if (getMode () == MODE_INIT && isPropertyGiven ("I")) {
    i = getPropertyDouble ("I");
  }

  setState (fState, i * l);
  integrate (fState, l, r, v);
  setD (VSRC_1, VSRC_1, -r);
  setE (VSRC_1, v);
}

void inductor::initHB (void) {
  setVoltageSources (1);
  setInternalVoltageSource (1);
  allocMatrixMNA ();
  voltageSource (VSRC_1, NODE_1, NODE_2);
}

void inductor::calcHB (nr_double_t frequency) {
  nr_double_t l = getPropertyDouble ("L");
  setD (VSRC_1, VSRC_1, -l * 2 * M_PI * frequency);
}
