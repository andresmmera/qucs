/***************************************************************************
                          matchsettingsdialog.h
                             ---------------
    begin                : Nov 2018
    copyright            : (C) 2018 by Andres Martinez-Mera, The Qucs Team
    email                : andresmartinezmera@gmail.com

-----------------------------------------------------------------------------

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MATCHSETTINGSDIALOG_H
#define MATCHSETTINGSDIALOG_H

#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>

#include <QDialog>
#include <QGridLayout>
#include "matchsubstratedialog.h"

#define LSECTION                           0
#define SINGLESTUB                         1
#define DOUBLESTUB                         2
#define MULTISTAGEL4                       3
#define CASCADEDLSECTIONS                  4
#define L8L4                               5
#define PI_TYPE                            6
#define TEE_TYPE                           7
#define TAPPED_C                           8
#define TAPPED_L                           9
#define DOUBLE_TAPPED                     10
#define SINGLE_TUNED_TRANSFORMER          11
#define PARALLEL_DOUBLE_TUNED_TRANSFORMER 12

#define CHEBYSHEV_WEIGHTING 0
#define BINOMIAL_WEIGHTING  1

#define CALCULATE_INPUT  0
#define CALCULATE_OUTPUT 1

enum NETWORK_TYPE{TWO_PORT_INPUT, TWO_PORT_OUTPUT, SINGLE_PORT};
enum RESPONSE_TYPE{LOWPASS, HIGHPASS};

struct ImplementationParams {
    bool BalancedStubs=false, open_short=true;//Stub implementation
    int order=3;//Number of section
    double gamma_MAX=0.1;//Maximum ripple for the impedance transformer
    int network_type = 0;//Network topology
    double Q = 5;//Q of the overall matching network. Only for Pi/Tee matching
    RESPONSE_TYPE network_response = LOWPASS;//Response type for Pi/Tee matching networks
    int weighting_type = 0;//Weighting of the impedance transformer: Chebyshev or binomial
    double CAPQ = 1000;//Capacitor quality factor, Q = X/R = 1/(w·C·R)
    double INDQ = 1000;//Inductor quality factor, Q = X/R = (w·L) / R
    double L2 = 5e-9;//L2 parameter for the double tapped resonator
    double k = 0.95;//Coupling coefficient of the transformer
    int coupled_L_Equivalent=0;//Use coupled inductor or its uncoupled equivalent
    double BW = 20e6;// Bandwidth for the double-tuned transformer matching method
};


struct NetworkParams {
  double S11real, S11imag, S12real, S12imag, S21real, S21imag, S22real, S22imag;
  double Z1, Z2, freq;
  tSubstrate Substrate;
  NETWORK_TYPE network;
  double DetReal, DetImag;
  struct ImplementationParams InputNetwork, OutputNetwork;
  bool micro_syn, SP_block;
  double r_real, r_imag;
};

class MatchSettingsDialog : public QDialog {
  Q_OBJECT
public:
  MatchSettingsDialog(QWidget *parent = 0, int topology=LSECTION);
  ~MatchSettingsDialog();
  struct ImplementationParams GetOptions();

private:
  QLabel *Order_Label, *Network_Response_Label, *QualityFactor_Label, *maxRipple_Label, *Weighting_Type_Label,
         *Stub_Type_Label, *Stub_Implementation_Label, *CapacitorQ_Label, *InductorQ_Label, *L2_Double_Tapped_Resonator_Label,
         *k_Transformer_Label, *coupled_L_Label, *BW_Label, *f2_Label;
  QComboBox *Network_Response_Combo, *Stub_Type_Combo, *Weighting_Type_Combo, *Stub_Implementation_Combo, *L2_Double_Tapped_Resonator_Scale_Combo,
            *coupled_L_Combo, *BW_Scale_Combo, *f2_Scale_Combo;
  QSpinBox *Order_Spinbox, *BW_Spinbox, *f2_Spinbox;
  QDoubleSpinBox *Quality_Factor_Spinbox, *maxRipple_Spinbox, *CapacitorQ_Spinbox, *InductorQ_Spinbox, *L2_Double_Tapped_Resonator_SpinBox,
                 *k_Transformer_Spinbox;
  QPushButton *OK_Button, *Cancel_Button;
  struct ImplementationParams params;

  double getScale(QString);

public slots:
  void slot_save_settings();
  void slot_cancel_settings();  
};
#endif