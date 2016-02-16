/*
 * coil.h - Coil class definition
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

#ifndef COIL_H
#define COIL_H
#include "matrix.h"


class coil : public qucs::circuit
{
 public:
  CREATOR (coil);
  void calcSP (nr_double_t);
  void initDC (void);
  void initAC (void);
  void initSP (void);
  void calcAC (nr_double_t);
 private:
  nr_complex_t calcZ(nr_double_t frequency);
  nr_double_t getL_Lundin(nr_double_t, nr_double_t, nr_double_t, int);
  nr_double_t f1(nr_double_t);
  nr_double_t f2(nr_double_t);
  nr_complex_t getPermeability(nr_double_t, nr_double_t);
  nr_double_t  calculateSeriesR(nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_complex_t);
  nr_double_t getSelfCapacitance(nr_double_t, nr_double_t, nr_double_t, nr_double_t, nr_double_t);
};

#endif /* __indq_H__ */

