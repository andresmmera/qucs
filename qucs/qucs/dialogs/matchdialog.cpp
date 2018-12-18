/***************************************************************************
                               matchdialog.cpp
                              -----------------
    begin                : Fri Jul 22 2005
    copyright            : (C) 2005 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
    copyright            : (C) 2012, 2013, 2016 by Qucs Team (see AUTHORS file)

-----------------------------------------------------------------------------
    Update (2017)        : New impedance matching techniques: Single matching,
                           double stub matching, real to real broadband transformers, 
                           cascaded L-sections and lambda/8 + lambda/4 matching

                          Andres Martinez-Mera <andresmartinezmera@gmail.com>
                          Claudio Girardi      <claudio.girardi@virgilio.it>
------------------------------------------------------------------------------

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "matchdialog.h"
#include "misc.h"
#include "qucs.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

MatchDialog::MatchDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(tr("Create Matching Circuit"));
  DoubleVal = new QDoubleValidator(this);

  all = new QHBoxLayout(this);

  QVBoxLayout *matchFrame = new QVBoxLayout();   // Matching circuit design panel
  QVBoxLayout *micro_layout = new QVBoxLayout(); // Substrate properties
  all->addLayout(matchFrame);
  all->addLayout(micro_layout);

  MethodBox = new QGroupBox(tr("Implementation"));
  matchFrame->addWidget(MethodBox);
  MethodLayout = new QVBoxLayout();

  QGridLayout *MatchingMethod_Layout = new QGridLayout();
  TopoLabel = new QLabel(tr("Input matching:"));
  MatchingMethod_Layout->addWidget(TopoLabel, 0, 0);

  // Matching network topology
  QStringList matching_methods;
  matching_methods.append(tr("L-section"));
  matching_methods.append(tr("Single stub"));
  matching_methods.append(tr("Double stub"));
  matching_methods.append(QString("%1 %2/4").arg(tr("Multistage ")).arg(QString(QChar(0xBB, 0x03))));
  matching_methods.append(tr("Cascaded L-sections"));
  matching_methods.append(QString("%1/4 line").arg(QChar(0xBB, 0x03)));
  matching_methods.append(QString("%1/8 + %1/4 line").arg(QChar(0xBB, 0x03)));
  matching_methods.append(QString("%1-type").arg(QChar(0xC0, 0x03)));
  matching_methods.append(tr("Tee-Type"));
  matching_methods.append(tr("Tapped C transformer"));
  matching_methods.append(tr("Tapped L transformer"));
  matching_methods.append(tr("Double tapped resonator"));
  matching_methods.append(tr("Single tuned transformer"));
  matching_methods.append(tr("Parallel double-tuned transformer"));
  matching_methods.append(tr("Series double-tuned transformer"));

  //List of matching methos that may be implemented using a microstrip line
  Transmission_Line_Topologies.append(SINGLESTUB);
  Transmission_Line_Topologies.append(DOUBLESTUB);
  Transmission_Line_Topologies.append(MULTISTAGEL4);
  Transmission_Line_Topologies.append(QUARTER_WAVE_LINE);
  Transmission_Line_Topologies.append(L8L4);

  TopoCombo_Input = new QComboBox();
  TopoCombo_Input->setFixedWidth(220);
  TopoCombo_Input->addItems(matching_methods);
  connect(TopoCombo_Input, SIGNAL(currentIndexChanged(int)), SLOT(slot_InputTopologyChanged(int)));

  MatchingMethod_Layout->addWidget(TopoCombo_Input, 0, 1);

  //Button for setting the input matching network parameters
  InputMatchingSettings_Button = new QPushButton("Settings...");
  connect(InputMatchingSettings_Button, SIGNAL(clicked(bool)), SLOT(slot_InputMatchingSettings()));
  MatchingMethod_Layout->addWidget(InputMatchingSettings_Button, 0, 2);


  TopoLabel_Output = new QLabel(tr("Output matching:"));
  MatchingMethod_Layout->addWidget(TopoLabel_Output, 1, 0);

  // Matching network topology
  TopoCombo_Output = new QComboBox();
  TopoCombo_Output->setFixedWidth(220);
  TopoCombo_Output->addItems(matching_methods);
  connect(TopoCombo_Output, SIGNAL(currentIndexChanged(int)), SLOT(slot_OutputTopologyChanged(int)));

  MatchingMethod_Layout->addWidget(TopoCombo_Output, 1, 1);
  MethodLayout->addLayout(MatchingMethod_Layout);

  //Button for setting the output matching network parameters
  OutputMatchingSettings_Button = new QPushButton("Settings...");
  connect(OutputMatchingSettings_Button, SIGNAL(clicked(bool)), SLOT(slot_OutputMatchingSettings()));
  MatchingMethod_Layout->addWidget(OutputMatchingSettings_Button, 1, 2);

  QGridLayout *OptLayout = new QGridLayout();

  Substrate_Button = new QPushButton(tr("Substrate settings..."));
  Substrate_Button->setEnabled(false);
  connect(Substrate_Button, SIGNAL(clicked(bool)), SLOT(slot_SubtrateSettings()));

  MethodBox->setLayout(MethodLayout);
  TwoCheck = new QCheckBox(tr("Calculate two-port matching"));
  TwoCheck->setChecked(true);
  AddSPBlock =
      new QCheckBox(tr("Add S-Parameter simulation")); // The user can choose
                                                       // whether add a S-param
                                                       // simulation or not
  AddSPBlock->setChecked(true);
  MicrostripCheck = new QCheckBox(tr("Synthesize microstrip lines"));
  MicrostripCheck->setEnabled(false);
  MicrostripCheck->setChecked(false);
  connect(MicrostripCheck, SIGNAL(clicked(bool)), SLOT(slot_MicrostripCheckChanged()));

  OptLayout->addWidget(TwoCheck, 0, 0);
  OptLayout->addWidget(MicrostripCheck, 0, 1);
  OptLayout->addWidget(AddSPBlock, 1, 0);
  OptLayout->addWidget(Substrate_Button, 1, 1);
  matchFrame->addLayout(OptLayout);

  connect(TwoCheck, SIGNAL(toggled(bool)), SLOT(slotSetTwoPort(bool)));

  // ...........................................................
  QGroupBox *ImpBox = new QGroupBox(tr("Reference Impedance"));
  matchFrame->addWidget(ImpBox);
  QHBoxLayout *ImpLayout = new QHBoxLayout();
  Port1Label = new QLabel(tr("Port 1"));
  Ref1Edit = new QLineEdit("50");
  Ref1Edit->setMaximumWidth(75);
  Ref1Edit->setValidator(DoubleVal);
  Ohm1Label = new QLabel(QString(QChar(0xA9, 0x03)));
  connect(Ref1Edit, SIGNAL(textChanged(const QString &)),
          SLOT(slotImpedanceChanged(const QString &)));
  Port2Label = new QLabel(tr("Port 2"));
  Ref2Edit = new QLineEdit("50");
  Ref2Edit->setMaximumWidth(75);
  Ref2Edit->setValidator(DoubleVal);
  Ohm2Label = new QLabel(QString(QChar(0xA9, 0x03)));
  ImpLayout->addWidget(Port1Label);
  ImpLayout->addWidget(Ref1Edit);
  ImpLayout->addWidget(Ohm1Label);
  ImpLayout->addSpacing(50);
  ImpLayout->addWidget(Port2Label);
  ImpLayout->addWidget(Ref2Edit);
  ImpLayout->addWidget(Ohm2Label);
  ImpBox->setLayout(ImpLayout);

  // ...........................................................
  QGroupBox *SParBox = new QGroupBox(tr("S Parameter"));
  matchFrame->addWidget(SParBox);
  QVBoxLayout *SParLayout = new QVBoxLayout();
  SParBox->setLayout(SParLayout);

  QHBoxLayout *h1 = new QHBoxLayout();
  h1->setSpacing(3);
  FormatLabel = new QLabel(tr("Input format"));
  h1->addWidget(FormatLabel);
  FormatCombo = new QComboBox();
  h1->addWidget(FormatCombo);
  FormatCombo->setFixedWidth(140);
  FormatCombo->addItem(tr("Real/Imag"));
  FormatCombo->addItem(tr("mag/deg"));
  connect(FormatCombo, SIGNAL(activated(int)), SLOT(slotChangeMode(int)));
  h1->addStretch(5);
  SParLayout->addLayout(h1);

  QHBoxLayout *h3 = new QHBoxLayout();
  h3->setSpacing(3);
  QVBoxLayout *VBox1 = new QVBoxLayout();
  h3->addLayout(VBox1);
  S11Label = new QLabel(tr("S11"));
  S21Label = new QLabel(tr("S21"));
  VBox1->addWidget(S11Label);
  VBox1->addWidget(S21Label);
  QVBoxLayout *VBox2 = new QVBoxLayout();
  h3->addLayout(VBox2);
  S11magEdit = new QLineEdit("0.5");
  S11magEdit->setValidator(DoubleVal);
  S11magEdit->setMaximumWidth(75);
  S21magEdit = new QLineEdit("0.5");
  S21magEdit->setValidator(DoubleVal);
  S21magEdit->setMaximumWidth(75);

  VBox2->addWidget(S11magEdit);
  VBox2->addWidget(S21magEdit);
  QVBoxLayout *VBox3 = new QVBoxLayout();
  h3->addLayout(VBox3);
  S11sLabel = new QLabel("+j");
  S21sLabel = new QLabel("+j");
  VBox3->addWidget(S11sLabel);
  VBox3->addWidget(S21sLabel);
  QVBoxLayout *VBox4 = new QVBoxLayout();
  h3->addLayout(VBox4);
  S11degEdit = new QLineEdit("0");
  S11degEdit->setValidator(DoubleVal);
  S11degEdit->setMaximumWidth(75);
  S21degEdit = new QLineEdit("0");
  S21degEdit->setValidator(DoubleVal);
  S21degEdit->setMaximumWidth(75);
  VBox4->addWidget(S11degEdit);
  VBox4->addWidget(S21degEdit);
  QVBoxLayout *VBox5 = new QVBoxLayout();
  h3->addLayout(VBox5);
  S11uLabel = new QLabel(" ");
  S21uLabel = new QLabel(" ");
  VBox5->addWidget(S11uLabel);
  VBox5->addWidget(S21uLabel);
  h3->addStretch(5);
  QVBoxLayout *VBox6 = new QVBoxLayout();
  h3->addLayout(VBox6);
  S12Label = new QLabel(tr("S12"));
  S22Label = new QLabel(tr("S22"));
  VBox6->addWidget(S12Label);
  VBox6->addWidget(S22Label);
  QVBoxLayout *VBox7 = new QVBoxLayout();
  h3->addLayout(VBox7);
  S12magEdit = new QLineEdit("0");
  S12magEdit->setValidator(DoubleVal);
  S12magEdit->setMaximumWidth(75);
  S22magEdit = new QLineEdit("0.5");
  S22magEdit->setValidator(DoubleVal);
  S22magEdit->setMaximumWidth(75);
  VBox7->addWidget(S12magEdit);
  VBox7->addWidget(S22magEdit);
  QVBoxLayout *VBox8 = new QVBoxLayout();
  h3->addLayout(VBox8);
  S12sLabel = new QLabel("+j");
  S22sLabel = new QLabel("+j");
  VBox8->addWidget(S12sLabel);
  VBox8->addWidget(S22sLabel);
  QVBoxLayout *VBox9 = new QVBoxLayout();
  h3->addLayout(VBox9);
  S12degEdit = new QLineEdit("0");
  S12degEdit->setMaximumWidth(75);
  S12degEdit->setValidator(DoubleVal);
  S22degEdit = new QLineEdit("0");
  S22degEdit->setMaximumWidth(75);
  S22degEdit->setValidator(DoubleVal);
  VBox9->addWidget(S12degEdit);
  VBox9->addWidget(S22degEdit);
  QVBoxLayout *VBox0 = new QVBoxLayout();
  h3->addLayout(VBox0);
  S12uLabel = new QLabel(" ");
  S22uLabel = new QLabel(" ");
  VBox0->addWidget(S12uLabel);
  VBox0->addWidget(S22uLabel);
  SParLayout->addLayout(h3);

  // set tab order to a more natural mode
  setTabOrder(S11magEdit, S11degEdit);
  setTabOrder(S11degEdit, S12magEdit);
  setTabOrder(S12magEdit, S12degEdit);
  setTabOrder(S12degEdit, S21magEdit);
  setTabOrder(S21magEdit, S21degEdit);
  setTabOrder(S21degEdit, S22magEdit);
  setTabOrder(S22magEdit, S22degEdit);

  connect(S21magEdit, SIGNAL(textChanged(const QString &)),
          SLOT(slotImpedanceChanged(const QString &)));
  connect(S21degEdit, SIGNAL(textChanged(const QString &)),
          SLOT(slotImpedanceChanged(const QString &)));
  connect(S11magEdit, SIGNAL(textChanged(const QString &)),
          SLOT(slotReflectionChanged(const QString &)));
  connect(S11degEdit, SIGNAL(textChanged(const QString &)),
          SLOT(slotReflectionChanged(const QString &)));

  QHBoxLayout *h2 = new QHBoxLayout();
  h2->setSpacing(3);
  FrequencyLabel = new QLabel(tr("Frequency:"));
  FrequencyEdit = new QLineEdit();
  FrequencyEdit->setValidator(DoubleVal);
  FrequencyEdit->setMaximumWidth(75);
  h2->addWidget(FrequencyLabel);
  h2->addWidget(FrequencyEdit);
  UnitCombo = new QComboBox();
  UnitCombo->addItem("Hz");
  UnitCombo->addItem("kHz");
  UnitCombo->addItem("MHz");
  UnitCombo->addItem("GHz");
  UnitCombo->setFixedWidth(100);
  h2->addWidget(UnitCombo);
  h2->addStretch(5);
  SParLayout->addLayout(h2);

  // ...........................................................
  QHBoxLayout *h0 = new QHBoxLayout();
  h0->setSpacing(5);
  matchFrame->addLayout(h0);
  h0->addStretch(5);
  QPushButton *buttCreate = new QPushButton(tr("Create"));
  QPushButton *buttCancel = new QPushButton(tr("Cancel"));
  h0->addWidget(buttCreate);
  h0->addWidget(buttCancel);
  connect(buttCreate, SIGNAL(clicked()), SLOT(slotButtCreate()));
  connect(buttCancel, SIGNAL(clicked()), SLOT(reject()));

  slotReflectionChanged(""); // calculate impedance
  setFrequency(1e9);        // set 1GHz
}

MatchDialog::~MatchDialog() {
  delete all;
  delete DoubleVal;
}

// -----------------------------------------------------------------------
void MatchDialog::setFrequency(double Freq_) {
  int Expo = int(log10(Freq_) / 3.0);
  if (Expo < 0)
    Expo = 0;
  else if (Expo > 3)
    Expo = 3;
  UnitCombo->setCurrentIndex(Expo);
  Freq_ /= pow(10.0, double(3 * Expo));
  FrequencyEdit->setText(QString::number(Freq_));
}

// Set visibility of LineEdits and Labels associated with two-port matching
void MatchDialog::set2PortWidgetsVisible(bool visible) {
  S12magEdit->setVisible(visible);
  S22magEdit->setVisible(visible);
  S12degEdit->setVisible(visible);
  S22degEdit->setVisible(visible);
  S12Label->setVisible(visible);
  S22Label->setVisible(visible);
  S12sLabel->setVisible(visible);
  S22sLabel->setVisible(visible);
  S12degEdit->setVisible(visible);
  S22degEdit->setVisible(visible);
  S12uLabel->setVisible(visible);
  S22uLabel->setVisible(visible);
  Port2Label->setVisible(visible);
  Ref2Edit->setVisible(visible);
  Ohm2Label->setVisible(visible);
}
// -----------------------------------------------------------------------
// Is called when the checkbox for two-port matching changes.
void MatchDialog::slotSetTwoPort(bool on) {
  if (on) { // two-port matching ?
    S11Label->setText(tr("S11"));
    S21Label->setText(tr("S21"));
    // restore the previous S21 values
    setS21LineEdits(tmpS21mag, tmpS21deg);
    set2PortWidgetsVisible(true);
    TopoLabel_Output->setVisible(true);
    TopoCombo_Output->setVisible(true);
    OutputMatchingSettings_Button->setVisible(true);
  } else {
    S11Label->setText(tr("Reflection Coefficient"));
    S21Label->setText(QString("Impedance (%1)").arg(QChar(0xA9, 0x03)));
    set2PortWidgetsVisible(false);
    // save S21 values, as these will be overwritten with the impedance value
    tmpS21mag = S21magEdit->text().toDouble();
    tmpS21deg = S21degEdit->text().toDouble();
    slotReflectionChanged(""); // calculate impedance
    TopoLabel_Output->setVisible(false);
    TopoCombo_Output->setVisible(false);
    OutputMatchingSettings_Button->setVisible(false);
  }
}

// -----------------------------------------------------------------------
// Is called when the combobox changes between mag/deg and real/imag.
void MatchDialog::slotChangeMode(int Index) {
  if (Index) { // polar ?
    S11sLabel->setText("/");
    S12sLabel->setText("/");
    S21sLabel->setText("/");
    S22sLabel->setText("/");
    S11uLabel->setText(QString::fromUtf8("°"));
    S12uLabel->setText(QString::fromUtf8("°"));
    S21uLabel->setText(QString::fromUtf8("°"));
    S22uLabel->setText(QString::fromUtf8("°"));

    double Real = S11magEdit->text().toDouble();
    double Imag = S11degEdit->text().toDouble();
    c2p(Real, Imag);
    setS11LineEdits(Real, Imag);

    Real = S12magEdit->text().toDouble();
    Imag = S12degEdit->text().toDouble();
    c2p(Real, Imag);
    setS12LineEdits(Real, Imag);

    Real = S21magEdit->text().toDouble();
    Imag = S21degEdit->text().toDouble();
    c2p(Real, Imag);
    setS21LineEdits(Real, Imag);
    // convert also temp entries for future use
    c2p(tmpS21mag, tmpS21deg);

    Real = S22magEdit->text().toDouble();
    Imag = S22degEdit->text().toDouble();
    c2p(Real, Imag);
    setS22LineEdits(Real, Imag);
  } else { // cartesian
    S11sLabel->setText("+j");
    S12sLabel->setText("+j");
    S21sLabel->setText("+j");
    S22sLabel->setText("+j");
    S11uLabel->setText(" ");
    S12uLabel->setText(" ");
    S21uLabel->setText(" ");
    S22uLabel->setText(" ");

    double Mag = S11magEdit->text().toDouble();
    double Phase = S11degEdit->text().toDouble();
    p2c(Mag, Phase);
    setS11LineEdits(Mag, Phase);

    Mag = S12magEdit->text().toDouble();
    Phase = S12degEdit->text().toDouble();
    p2c(Mag, Phase);
    setS12LineEdits(Mag, Phase);

    Mag = S21magEdit->text().toDouble();
    Phase = S21degEdit->text().toDouble();
    p2c(Mag, Phase);
    setS21LineEdits(Mag, Phase);
    // convert also temp entries for future use
    p2c(tmpS21mag, tmpS21deg);

    Mag = S22magEdit->text().toDouble();
    Phase = S22degEdit->text().toDouble();
    p2c(Mag, Phase);
    setS22LineEdits(Mag, Phase);
  }
}

// -----------------------------------------------------------------------
// Is called if the user changed the impedance. -> The Reflection
// coefficient is calculated.
void MatchDialog::slotImpedanceChanged(const QString &) {
  if (TwoCheck->isChecked())
    return;

  double Z0 = Ref1Edit->text().toDouble();
  double Real = S21magEdit->text().toDouble();
  double Imag = S21degEdit->text().toDouble();

  if (FormatCombo->currentIndex()) { // entries in polar format
    p2c(Real, Imag);
    z2r(Real, Imag, Z0);
    c2p(Real, Imag);
  } else {
    z2r(Real, Imag, Z0);
  }

  setS11LineEdits(Real, Imag);
}

// -----------------------------------------------------------------------
// Is called if the user changed the Reflection coefficient. -> The impedance
// is calculated.
void MatchDialog::slotReflectionChanged(const QString &) {
  if (TwoCheck->isChecked())
    return;

  double Z0 = Ref1Edit->text().toDouble();
  double Real = S11magEdit->text().toDouble();
  double Imag = S11degEdit->text().toDouble();

  if (FormatCombo->currentIndex()) { // entries in polar format
    p2c(Real, Imag);
    r2z(Real, Imag, Z0);
    c2p(Real, Imag);
  } else {
    r2z(Real, Imag, Z0);
  }
  setS21LineEdits(Real, Imag);
}

void MatchDialog::setS11LineEdits(double Real, double Imag) {
  S11magEdit->blockSignals(true); // do not call slot for "textChanged"
  S11magEdit->setText(QString::number(Real));
  S11magEdit->blockSignals(false);
  S11degEdit->blockSignals(true); // do not call slot for "textChanged"
  S11degEdit->setText(QString::number(Imag));
  S11degEdit->blockSignals(false);
}

void MatchDialog::setS12LineEdits(double Real, double Imag) {
  S12magEdit->setText(QString::number(Real));
  S12degEdit->setText(QString::number(Imag));
}

void MatchDialog::setS21LineEdits(double Real, double Imag) {
  S21magEdit->blockSignals(true); // do not call slot for "textChanged"
  S21magEdit->setText(QString::number(Real));
  S21magEdit->blockSignals(false);
  S21degEdit->blockSignals(true); // do not call slot for "textChanged"
  S21degEdit->setText(QString::number(Imag));
  S21degEdit->blockSignals(false);
}

void MatchDialog::setS22LineEdits(double Real, double Imag) {
  S22magEdit->setText(QString::number(Real));
  S22degEdit->setText(QString::number(Imag));
}

// -----------------------------------------------------------------------
// Is called if the "Create"-button is pressed.
void MatchDialog::slotButtCreate() {  
  params.Z1 = Ref1Edit->text().toDouble(); // Port 1 impedance
  params.Z2 = Ref2Edit->text().toDouble(); // Port 2 impedance
  params.freq = FrequencyEdit->text().toDouble() *
                pow(10.0, 3.0 * UnitCombo->currentIndex());
  params.micro_syn = MicrostripCheck->isChecked();
  params.SP_block = AddSPBlock->isChecked();

  // S matrix
  params.S11real = S11magEdit->text().toDouble();
  params.S11imag = S11degEdit->text().toDouble();
  params.S12real = S12magEdit->text().toDouble();
  params.S12imag = S12degEdit->text().toDouble();
  params.S21real = S21magEdit->text().toDouble();
  params.S21imag = S21degEdit->text().toDouble();
  params.S22real = S22magEdit->text().toDouble();
  params.S22imag = S22degEdit->text().toDouble();

  if (FormatCombo->currentIndex()) { // are they polar ?
    p2c(params.S11real, params.S11imag);
    p2c(params.S12real, params.S12imag);
    p2c(params.S21real, params.S21imag);
    p2c(params.S22real, params.S22imag);
  }

  bool success = true;

  // Set topology: the topology is set here so as to avoid writing a slot function. The parameters are updated by the Settings... button.
  input_network.network_type = TopoCombo_Input->currentIndex();
  output_network.network_type = TopoCombo_Output->currentIndex();

  params.InputNetwork = input_network;
  params.OutputNetwork = output_network;

  if (TwoCheck->isChecked()) { // two-port matching ?
    // determinant of S-parameter matrix
    params.DetReal = params.S11real * params.S22real - params.S11imag * params.S22imag - params.S12real * params.S21real +
                     params.S12imag * params.S21imag;
    params.DetImag = params.S11real * params.S22imag + params.S11imag * params.S22real - params.S12real * params.S21imag -
                     params.S12imag * params.S21real;
    success = calc2PortMatch(params);
  } else {
    success = calcMatchingCircuit(params);
  }

  QucsMain->slotEditPaste(success);
  accept();
}

// -----------------------------------------------------------------------
// transform real/imag into mag/deg (cartesian to polar)
void MatchDialog::c2p(double &Real, double &Imag) {
  double Real_ = Real;
  Real = sqrt(Real * Real + Imag * Imag); // magnitude
  Imag = 180.0 / pi * atan2(Imag, Real_); // phase in degree
}

// -----------------------------------------------------------------------
// transform mag/deg into real/imag (polar to cartesian)
void MatchDialog::p2c(double &Real, double &Imag) {
  double Real_ = Real;
  Real = Real_ * cos(Imag * pi / 180.0); // real part
  Imag = Real_ * sin(Imag * pi / 180.0); // imaginary part
}

// -----------------------------------------------------------------------
// transform reflection coefficient into impedance
void MatchDialog::r2z(double &Real, double &Imag, double Z0) {
  double tmp = Z0 / ((1.0 - Real) * (1.0 - Real) + Imag * Imag);
  Real = (1.0 - Real * Real - Imag * Imag) * tmp;
  Imag *= 2.0 * tmp;
}

// -----------------------------------------------------------------------
// transform impedance into reflection coefficient
void MatchDialog::z2r(double &Real, double &Imag, double Z0) {
  double tmp = (Real + Z0) * (Real + Z0) + Imag * Imag;
  Real = (Real * Real + Imag * Imag - Z0 * Z0) / tmp;
  Imag *= 2.0 * Z0 / tmp;
}

// This function retrieves the path schematic image of the L-section matching according to the
// reactance and susceptance values.
// first_series: Indicates if the first element is place in series
// X           : Reactance
// B           : Susceptance
QString MatchDialog::getImageFrom_XB(bool first_series, double X, double B)
{
  if (first_series == true)//First series, Z0 < ZL
  {
      if (X < 0)//Series capacitor
      {
          if (B < 0)
              return ":/bitmaps/matching/CSLP.png";
          else
              return ":/bitmaps/matching/CSCP.png";
      }
      else//Series inductor
      {
          if (B < 0)
              return ":/bitmaps/matching/LSLP.png";
          else
              return ":/bitmaps/matching/LSCP.png";
      }
  }
  else//First shunt, Z0 > ZL
  {
      if (B < 0)//Shunt inductor
      {
          if (X > 0)
              return ":/bitmaps/matching/LPLS.png";
          else
              return ":/bitmaps/matching/LPCS.png";
      }
      else//Shunt inductor
      {
          if (X > 0)
              return ":/bitmaps/matching/CPLS.png";
          else
              return ":/bitmaps/matching/CPCS.png";
      }
  }
}

// -----------------------------------------------------------------------
// This function calculates a LC matching section
// Reference:
// Microwave Circuit Design Using Linear and Nonlinear Techniques. George D. Vendelin,
// Antonio M. Pavio, Ulrich P. Rohde. 2nd Edition. p. 251-252. Wiley
QString MatchDialog::calcMatchingLC(struct NetworkParams params) {
  double RL, XL, Z0;
  QMessageBox msgBox;//Informs the user about the two candidate networks
  QString message;

  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      (params.network == TWO_PORT_INPUT) ? Z0 = params.Z1 : Z0 = params.Z2;
  }

  r2z(RL, XL, Z0);

  if (RL < 0.0) {
    if (RL < -1e-13) {
      QMessageBox::critical(
          0, tr("Error"),
          tr("Real part of impedance must be greater zero,\nbut is %1 !")
              .arg(RL));
      return QString(""); // matching not possible
    }

    // In high-Q circuits, Zreal often becomes somewhat about -1e-16
    // because of numerical inaccuracy.
    RL = 0.0;
  }

  double w0 = 2.0 * pi * params.freq;
  double X1, X2, B1, B2;
  double L, C, X, B;
  int solution;
  QString laddercode = "";

  if (Z0 > RL) {
    // ZS -------- X -- ZL
    //       |
    //       B
    //       |
    //      ---

    // Solution 1
      X1 = sqrt(RL*(Z0-RL))-XL;
      B1 = sqrt((Z0-RL)/RL)/Z0;
    // Solution 2
      X2 = -sqrt(RL*(Z0-RL))-XL;
      B2 = -sqrt((Z0-RL)/RL)/Z0;

    switch (params.network){
    case SINGLE_PORT:
        message = "L-section design";
        break;
    case TWO_PORT_INPUT:
        message = "L-section design: Input matching";
        break;
    case TWO_PORT_OUTPUT:
        message = "L-section design: Output matching";
        break;
    }

    msgBox.setText(message);
    msgBox.setInformativeText("Please select a solution");
    //Add buttons
    //Solution 1
    QAbstractButton* Solution1_button = msgBox.addButton(tr(""), QMessageBox::AcceptRole);
    Solution1_button->setIcon(QIcon(getImageFrom_XB(false, X1, B1)));
    Solution1_button->setIconSize(QSize(300, 300));

    //Solution 2
    QAbstractButton* Solution2_button = msgBox.addButton(tr(""), QMessageBox::RejectRole);
    Solution2_button->setIcon(QIcon(getImageFrom_XB(false, X2, B2)));
    Solution2_button->setIconSize(QSize(300, 300));

    msgBox.setDefaultButton(QMessageBox::SaveAll);
    solution = msgBox.exec();


    if (solution == 0) X = X1, B = B1;
    else               X = X2, B = B2;


    if (B > 0)  //Capacitor
        C = B/w0,         laddercode += QString("CP:%1;").arg(C);
    else        //Inductor
        L = -1/(w0*B),    laddercode += QString("LP:%1;").arg(L);

    if (X < 0)  //Capacitor
        C = -1/(w0*X),    laddercode += QString("CS:%1;").arg(C);
    else        //Inductor
        L = X/w0,         laddercode += QString("LS:%1;").arg(L);

  } else {
      // Z0 < RL
      // ZS --- X  ------- ZL
      //              |
      //              B
      //              |
      //             ---

    // Solution 1
      B1 = (XL + sqrt(RL/Z0)*sqrt(RL*RL + XL*XL - Z0*RL))/(RL*RL + XL*XL);
      X1 = 1/B1 + XL*Z0/RL - Z0/(B1*RL);
    // Solution 2
      B2 = (XL - sqrt(RL/Z0)*sqrt(RL*RL + XL*XL - Z0*RL))/(RL*RL + XL*XL);
      X2 = 1/B2 + XL*Z0/RL - Z0/(B2*RL);

    switch (params.network){
    case SINGLE_PORT:
        message = "L-section design";
        break;
    case TWO_PORT_INPUT:
        message = "L-section design: Input matching";
        break;
    case TWO_PORT_OUTPUT:
        message = "L-section design: Output matching";
        break;
    }

    msgBox.setText(message);
    msgBox.setInformativeText("Please select a solution");
    //Add buttons
    //Solution 1
    QAbstractButton* Solution1_button = msgBox.addButton(tr(""), QMessageBox::AcceptRole);
    Solution1_button->setIcon(QIcon(getImageFrom_XB(true, X1, B1)));
    Solution1_button->setIconSize(QSize(300, 300));

    //Solution 2
    QAbstractButton* Solution2_button = msgBox.addButton(tr(""), QMessageBox::RejectRole);
    Solution2_button->setIcon(QIcon(getImageFrom_XB(true, X2, B2)));
    Solution2_button->setIconSize(QSize(300, 300));

    solution = msgBox.exec();

    if (solution == 0) X = X1, B = B1;
    else               X = X2, B = B2;

    if (X < 0)  //Capacitor
        C = -1/(w0*X),         laddercode += QString("CS:%1;").arg(C);
    else        //Inductor
        L = X/w0,              laddercode += QString("LS:%1;").arg(L);

    if (B > 0)  //Capacitor
        C = B/w0,              laddercode += QString("CP:%1;").arg(C);
    else        //Inductor
        L = -1/(w0*B),         laddercode += QString("LP:%1;").arg(L);
  }

  return laddercode;
}

// -----------------------------------------------------------------------
// This function calls the specific matching network function so as to get the
// desired matching topology Returns true if the synthesis worked as expected or
// false if it wasn't
bool MatchDialog::calcMatchingCircuit(struct NetworkParams params) {
  params.network = SINGLE_PORT;
  QString laddercode = SynthesizeMatchingNetwork(params);

  if (laddercode.isEmpty())
    return false;

  double RL = params.S11real, XL = params.S11imag;
  QString wirestr = "";
  QString componentstr = "";
  QString paintingstr = "";
  int x_pos = 0;

  r2z(RL, XL, params.Z1);

  if (params.SP_block) {
    laddercode.append("S2P:Freq;");
    laddercode.prepend(QString("P1:%1;").arg(params.Z1));
    laddercode.append(QString("ZL:%1#%2;").arg(RL).arg(XL));
  } else // Add tags instead of a S-param simulation
  {
    laddercode.prepend("LBL:Port 1;");
    laddercode.append("LBL:Port 2;");
  }
  SchematicParser(laddercode, x_pos, params);
  return true; // The schematic was successfully created
}

// -----------------------------------------------------------------------
// Fundamental calculations for concurrent 2-port matching.
QString MatchDialog::calcBiMatch(struct NetworkParams params) {

  double S11real, S11imag, S22real, S22imag;

  if (params.network == TWO_PORT_OUTPUT){
     S11real = params.S22real;
     S11imag = params.S22imag;
     S22real = params.S11real;
     S22imag = params.S11imag;
  }
  else  {
     S11real = params.S11real;
     S11imag = params.S11imag;
     S22real = params.S22real;
     S22imag = params.S22imag;
  }
  double B = 1.0 + S11real * S11real + S11imag * S11imag - S22real * S22real -
             S22imag * S22imag - params.DetReal * params.DetReal - params.DetImag * params.DetImag;
  double Creal = S11real - S22real * params.DetReal - S22imag * params.DetImag;
  double Cimag = S22real * params.DetImag - S11imag - S22imag * params.DetReal;
  double Cmag = 2.0 * (Creal * Creal + Cimag * Cimag);
  Creal /= Cmag;
  Cimag /= Cmag;

  double Rreal = B * B - 2.0 * Cmag;
  double Rimag;
  if (Rreal < 0.0) {
    Rimag = Cimag * B - Creal * sqrt(-Rreal);
    Rreal = Creal * B + Cimag * sqrt(-Rreal);
  } else {
    Rreal = B - sqrt(Rreal);
    Rimag = Cimag * Rreal;
    Rreal *= Creal;
  }

  params.r_real = Rreal;
  params.r_imag = -Rimag;

  return SynthesizeMatchingNetwork(params);
}

QString MatchDialog::SynthesizeMatchingNetwork(struct NetworkParams params)
{
    QString laddercode;
    int topology;
    if ((params.network == TWO_PORT_INPUT) || (params.network == SINGLE_PORT))
        topology = params.InputNetwork.network_type;
    else
        topology = params.OutputNetwork.network_type;

    switch (topology)
    {
    case LSECTION: // LC
      laddercode = calcMatchingLC(params);
      break;
    case SINGLESTUB: // Single stub
      laddercode = calcSingleStub(params);
      break;
    case DOUBLESTUB: // Double stub
      laddercode = calcDoubleStub(params);
      break;
    case MULTISTAGEL4: // Quarter wave cascaded sections
       laddercode = calcTransmissionLineTransformer(params);
      break;
    case CASCADEDLSECTIONS: // Cascaded LC sections
      laddercode = calcMatchingCascadedLCSections(params);
      break;
    case QUARTER_WAVE_LINE:
        laddercode = calcMatchingLambda4(params);
        break;
    case L8L4: // Lambda/8 + Lambda/4 transformer
      laddercode = calcMatchingLambda8Lambda4(params);
      break;
    case PI_TYPE: //Pi-type network
      laddercode = calcPiType(params);
      break;
    case TEE_TYPE: //Tee-type network
      laddercode = calcTeeType(params);
      break;
    case TAPPED_C: //Tapped-C transformer
      laddercode = calcTappedCTransformer(params);
      break;
    case TAPPED_L: //Tapped-L transformer
      laddercode = calcTappedLTransformer(params);
      break;
    case DOUBLE_TAPPED: //Double tapped resonator
      laddercode = calcDoubleTappedResonator(params);
      break;
    case SINGLE_TUNED_TRANSFORMER: //Single tuned transformer
      laddercode = calcSingleTunedTransformer(params);
      break;
    case PARALLEL_DOUBLE_TUNED_TRANSFORMER: //Parallel double-tuned transformer
      laddercode = calcParallelDoubleTunedTransformer(params);
      break;
    case SERIES_DOUBLE_TUNED_TRANSFORMER: //Series double-tuned transformer
      laddercode = calcSeriesDoubleTunedTransformer(params);
      break;
    }
    return SimplifySeriesParallelConnections(laddercode);
}

// -----------------------------------------------------------------------
// This function calculates both the input and the output matching networks. In
// this sense, it calls 'calcBiMatch' function to get the input/output
// impedance. Then it synthesize the network according to the user choice
bool MatchDialog::calc2PortMatch(struct NetworkParams params) {
  // Input port network
  // The result is a string which gives the structure of the matching network
  params.network = TWO_PORT_INPUT;
  QString InputLadderCode = calcBiMatch(params);
  if (InputLadderCode.isEmpty())
    return false; // Synthesis error

  // Output port network
  params.network = TWO_PORT_OUTPUT;
  QString OutputLadderCode = calcBiMatch(params);
  if (OutputLadderCode.isEmpty())
    return false; // Synthesis error
  else
    OutputLadderCode = flipLadderCode(OutputLadderCode);

  if (params.SP_block) {
    InputLadderCode.prepend(QString("S2P:%1;").arg(
        params.freq)); // Add the s-param simulation to the circuit code
    InputLadderCode.prepend(QString("P1:%1;").arg(params.Z1)); // Port 1
    OutputLadderCode.append(QString("P2:%1;").arg(params.Z2)); // Port 2
  } else { // Use labels instead of the S-param simulation
    InputLadderCode.prepend(QString("LBL:Port 1;")); // Port 1
    OutputLadderCode.append(QString("LBL:Port 2;")); // Port 2
  }
  QString laddercode = InputLadderCode + QString("DEV:0") + OutputLadderCode;
  int x_pos = 0;
  SchematicParser(laddercode, x_pos, params);

  return true;
}

// This function flips the ladder structure so as to correctly process the
// output matching network
QString MatchDialog::flipLadderCode(QString laddercode) {
  QStringList strlist = laddercode.split(";"), auxlist;
  std::reverse(strlist.begin(), strlist.end());

  QString flipped_laddercode = "";
  // Build the flipped ladder code
  for (int i = 0; i < strlist.length(); i++) {
      if (strlist.at(i).mid(0, 5) == "LCOUP"){//In case of having coupled inductors L11 and L22 must also be swapped
          QString lcoup = strlist.at(i).mid(6);
          QStringList lcoup_params = lcoup.split("#");
          QString L11 = lcoup_params.at(0);
          QString L22 = lcoup_params.at(1);
          QString k = lcoup_params.at(2);
          flipped_laddercode += QString("LCOUP:%1#%2#%3;").arg(L22).arg(L11).arg(k);
      }else{
          flipped_laddercode += strlist.at(i) + ";";
      }
  }
  return flipped_laddercode;
}

// FUNCTIONS FOR THE MICROSTRIP LINE SYNTHESIS. JUST COPIED FROM THE QUCS-FILTER
// TOOL
/////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_ERROR 1e-7
void calcMicrostrip(tSubstrate *substrate, double width, double freq,
                    double &er_eff, double &zl) {
  double a, b;
  double h = substrate->height;
  double t = substrate->thickness;
  double er = substrate->er;
  double Wh = width / h;
  t /= h;

  // quasi-static models by Hammerstad

  double w1 = Wh;
  if (t > 1e-100) { // width correction due to metal thickness?
    a = coth(sqrt(6.517 * Wh));
    b = t / pi * log(1.0 + 10.873127 / t / a / a);
    w1 += b;
    Wh += 0.5 * b * (1.0 + sech(sqrt(er - 1.0)));
  }

  // relative effective permittivity
  a = Wh * Wh;
  b = a * a;
  er_eff = -0.564 * pow((er - 0.9) / (er + 3.0), 0.053);
  er_eff *= 1.0 + log((b + a / 2704.0) / (b + 0.432)) / 49.0 +
            log(1.0 + a * Wh / 5929.741) / 18.7;
  er_eff = (er + 1.0) / 2.0 + (er - 1.0) / 2.0 * pow(1.0 + 10.0 / Wh, er_eff);

  // characteristic impedance
  zl = 6.0 + 0.2831853 * exp(-pow(30.666 / Wh, 0.7528));
  zl = Z_FIELD / 2.0 / pi * log(zl / Wh + sqrt(1.0 + 4.0 / Wh / Wh));

  // characteristic impedance (same again for "w1")
  a = 6.0 + 0.2831853 * exp(-pow(30.666 / w1, 0.7528));
  a = Z_FIELD / 2.0 / pi * log(a / w1 + sqrt(1.0 + 4.0 / w1 / w1));

  a /= zl;
  zl /= sqrt(er_eff);
  er_eff *= a * a;

  // dispersion models by Kirschning

  freq *= h / 1e6; // normalize frequency into GHz*mm

  // relative effective permittivity
  a = 0.0363 * exp(-4.6 * Wh) * (1.0 - exp(-pow(freq / 38.7, 4.97)));
  a *= 1.0 + 2.751 * (1.0 - exp(-pow(er / 15.916, 8.0)));
  a = pow((0.1844 + a) * freq, 1.5763);
  a *= 0.27488 + Wh * (0.6315 + 0.525 / pow(1.0 + 0.0157 * freq, 20.0)) -
       0.065683 * exp(-8.7513 * Wh);
  a *= 0.33622 * (1.0 - exp(-0.03442 * er));
  double er_freq = er - (er - er_eff) / (1.0 + a);

  // characteristic impedance
  a = -0.03891 * pow(er, 1.4);
  b = -0.267 * pow(Wh, 7.0);
  double R7 = 1.206 - 0.3144 * exp(a) * (1.0 - exp(b));

  a = 0.016 + pow(0.0514 * er, 4.524);
  b = pow(freq / 28.843, 12.0);
  a = 5.086 * a * b / (0.3838 + 0.386 * a) / (1.0 + 1.2992 * b);
  b = -22.2 * pow(Wh, 1.92);
  a *= exp(b);
  b = pow(er - 1.0, 6.0);
  double R9 = a * b / (1.0 + 10.0 * b);

  a = 4.766 * exp(-3.228 * pow(Wh, 0.641)); // = R3
  a = 1.0 + 1.275 * (1.0 - exp(-0.004625 * a * pow(er, 1.674) *
                               pow(freq / 18.365, 2.745))); // = R8

  b = 0.9408 * pow(er_freq, a) - 0.9603; // = R13
  b /= (0.9408 - R9) * pow(er_eff, a) - 0.9603;
  R9 = b; // = R13 / R14

  a = 0.00044 * pow(er, 2.136) + 0.0184; // = R10
  a *= 0.707 * pow(freq / 12.3, 1.097);  // = R15
  a = exp(-0.026 * pow(freq, 1.15656) - a);
  b = pow(freq / 19.47, 6.0);
  b /= 1.0 + 0.0962 * b;                                            // = R11
  b = 1.0 + 0.0503 * er * er * b * (1.0 - exp(-pow(Wh / 15, 6.0))); // = R16
  R7 *= (1.0 - 1.1241 * a / b / (1.0 + 0.00245 * Wh * Wh));         // = R17

  zl *= pow(R9, R7);
  er_eff = er_freq;
}

// -------------------------------------------------------------------
// Calculates the width 'width' and the relative effective permittivity 'er_eff'
// of a microstrip line. It uses an iterative search algorithm because
// synthesis equations doesn't exist.
void MatchDialog::getMicrostrip(double Z0, double freq, tSubstrate *substrate,
                                double &width, double &er_eff) {
  int iteration = 0; // iteration counter
  double Z0_current, Z0_result, increment;

  width = 1e-3; // start with 1mm

  do {
    // compute line parameters
    calcMicrostrip(substrate, width, freq, er_eff, Z0_current);

    if (fabs(Z0 - Z0_current) < MAX_ERROR)
      break; // wanted value was found

    increment = width / 100.0;
    width += increment;

    // compute line parameters
    calcMicrostrip(substrate, width, freq, er_eff, Z0_result);

    // Newton iteration: w(n+1) = w(n) - f(w(n))/f'(w(n))
    //   with f(w(n))  = Z0_current - Z0
    //   and  f'(w(n)) = (Z0_result - Z0_current) / increment
    width -= (Z0_current - Z0) / (Z0_result - Z0_current) * increment;
    if (width <= 0.0)
      width = increment;

    iteration++;
  } while (iteration < 150);
}
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------
// Calculates a matching network according to the stub+line method
// Reference: 'Microwave Engineering'. David Pozar. John Wiley and Sons. 4th
// Edition. Pg 234-241
QString MatchDialog::calcSingleStub(struct NetworkParams params) {
  double t = 0, t1 = 0, t2 = 0;
  double dl, dl1, dl2, B;
  double B1, B2, d, lstub, ll;
  double lambda = SPEED_OF_LIGHT / params.freq;
  double RL, XL, Z0;

  struct ImplementationParams ImplParams;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT){
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }
      else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }

  r2z(RL, XL, Z0);

  // Stub+line method formulas
  if (RL == Z0) {
    t = -XL / (2 * Z0);
    (t < 0) ? dl = (pi + atan(t)) / (2 * pi) : dl = (atan(t)) / (2 * pi);
    B = (RL * RL * t - (Z0 - XL * t) * (Z0 * t + XL)) /
        (Z0 * (RL * RL + (Z0 * t + XL) * (Z0 * t + XL)));
  } else {
    t1 = (XL + sqrt(((RL / Z0) * fabs((Z0 - RL) * (Z0 - RL) + XL * XL)))) /
         (RL - Z0);
    (t1 < 0) ? dl1 = (pi + atan(t1)) / (2 * pi) : dl1 = (atan(t1)) / (2 * pi);
    B1 = (RL * RL * t1 - (Z0 - XL * t1) * (Z0 * t1 + XL)) /
         (Z0 * (RL * RL + (Z0 * t1 + XL) * (Z0 * t1 + XL)));

    t2 = (XL - sqrt((RL * fabs((Z0 - RL) * (Z0 - RL) + XL * XL)) / (Z0))) /
         (RL - Z0);
    (t2 < 0) ? dl2 = (pi + atan(t2)) / (2 * pi) : dl2 = (atan(t2)) / (2 * pi);
    B2 = (RL * RL * t2 - (Z0 - XL * t2) * (Z0 * t2 + XL)) /
         (Z0 * (RL * RL + (Z0 * t2 + XL) * (Z0 * t2 + XL)));
  }

  if (t != 0) {
    d = dl * lambda;
    (ImplParams.open_short) ? ll = -(atan(B * Z0)) / (2 * pi)
                 : ll = (atan(1. / (B * Z0))) / (2 * pi);
    if ((ImplParams.open_short) && (ll < 0))
      ll += 0.5;
    if ((!ImplParams.open_short) && (ll > 0.5))
      ll -= 0.5;
    lstub = ll * lambda;
  }

  if (t1 != 0) {
    d = dl1 * lambda;
    (ImplParams.open_short) ? ll = -(atan(B1 * Z0)) / (2 * pi)
                 : ll = (atan(1. / (1. * B1 * Z0))) / (2 * pi);
    if ((ImplParams.open_short) && (ll < 0))
      ll += 0.5;
    if ((!ImplParams.open_short) && (ll > 0.5))
      ll -= 0.5;
    lstub = ll * lambda;

  } else {
    if (t2 != 0) {
      d = dl2 * lambda;
      (ImplParams.open_short) ? ll = -(atan(B2 * Z0)) / (2 * pi)
                   : ll = (atan(1. / (1. * B2 * Z0))) / (2 * pi);
      if ((ImplParams.open_short) && (ll < 0))
        ll += 0.5;
      if ((!ImplParams.open_short) && (ll > 0.5))
        ll -= 0.5;
      lstub = ll * lambda;
    }
  }

  if (ImplParams.BalancedStubs) { // Balanced stub implementation
    double K;
    (ImplParams.open_short) ? K = 0.5 : K = 2;
    lstub = (lambda / (2 * pi)) * atan(K * tan((2 * pi * lstub) / lambda));
    if (lstub < 0)
      lstub += 0.5 * lambda;
  }

  // String code
  QString laddercode;
  if ((ImplParams.open_short) && (!ImplParams.BalancedStubs))
  {
      laddercode = QString("OL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub); // Line + Open stub
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);
  }
  if ((ImplParams.open_short) && (ImplParams.BalancedStubs))
  {
      laddercode = QString("OU:%1#%2;OL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub); // Open circuit balanced stubs
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);
  }
  if ((!ImplParams.open_short) && (!ImplParams.BalancedStubs))
  {
      laddercode = QString("SL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub); // Line + Short circuited stub
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);
  }
  if ((!ImplParams.open_short) && (ImplParams.BalancedStubs))
  {
      laddercode = QString("SU:%1#%2;SL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub); // Short circuited balanced stubs
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);
  }
  return laddercode;
}

//--------------------------------------------------------------------------------
// Calculates a matching network according to the stub+line+stub method
// Reference: 'Microwave Engineering', David Pozar. John Wiley and Sons. 4th
// Edition. Pg 241-245
QString MatchDialog::calcDoubleStub(struct NetworkParams params) {

  double RL, XL, Z0;
  struct ImplementationParams ImplParams;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT){
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }
      else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }

  r2z(RL, XL, Z0);
  double Y0 = 1. / Z0;
  double GL = (1 / ((RL * RL) + (XL * XL))) * RL;
  double BL = -(1 / ((RL * RL) + (XL * XL))) * XL;
  double lambda = SPEED_OF_LIGHT / params.freq;
  double beta = (2 * pi) / lambda;
  double d = lambda / 8;
  double t = tan(beta * d);
  double ll1, ll2;

  // Double stub method formulas
  if (GL > Y0 * ((1 + t * t) / (2 * t * t))) // Not every load can be match
                                             // using the double stub technique.
  {
    QString str = QString(
        "It is not possible to match this load using the double stub method");
    QMessageBox::warning(0, QObject::trUtf8("Error"),
                         QObject::trUtf8(str.toUtf8()));
    return QString("");
  }

  // Stubs susceptance
  double B11 = -BL + (Y0 + sqrt((1 + t * t) * GL * Y0 - GL * GL * t * t)) /
                         (t); // 1st solution
  // double B12 = -BL + (Y0 - sqrt((1+t*t)*GL*Y0 - GL*GL*t*t))/(t); // 2nd
  // solution

  double B21 =
      ((Y0 * sqrt((1 + t * t) * GL * Y0 - GL * GL * t * t)) + GL * Y0) /
      (GL * t); // 1st solution
  // double B22 = ((-Y0*sqrt((1+t*t)*GL*Y0 - GL*GL*t*t)) + GL*Y0)/(GL*t);// 2nd
  // solution

  // Open circuit solution
  (ImplParams.open_short) ? ll1 = (atan(B11 * Z0)) / (2 * pi)
               : ll1 = -(atan(1. / (1. * B11 * Z0))) / (2 * pi);
  (ImplParams.open_short) ? ll2 = (atan(B21 * Z0)) / (2 * pi)
               : ll2 = -(atan(1. / (1. * B21 * Z0))) / (2 * pi);

  if (ll1 < 0)
    ll1 += 0.5;
  if (ll2 < 0)
    ll2 += 0.5;
  if ((!ImplParams.open_short) && (ll1 > 0.5))
    ll1 -= 0.5;
  if ((!ImplParams.open_short) && (ll2 > 0.5))
    ll2 -= 0.5;

  double lstub1 = ll1 * lambda, lstub2 = ll2 * lambda;

  if (ImplParams.BalancedStubs) { // Balanced stub implementation
    double K;
    (ImplParams.open_short) ? K = 0.5 : K = 2;
    lstub1 = (lambda / (2 * pi)) * atan(K * tan((2 * pi * lstub1) / lambda));
    lstub2 = (lambda / (2 * pi)) * atan(K * tan((2 * pi * lstub2) / lambda));
    if (lstub1 < 0)
      lstub1 += 0.5 * lambda;
    if (lstub2 < 0)
      lstub2 += 0.5 * lambda;
  }

  QString laddercode;
  if ((ImplParams.open_short) && (ImplParams.BalancedStubs))
  {
      laddercode = QString("OU:%1#%2;OL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub2);
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);

      laddercode += QString("OU:%1#%2;OL:%1#%2;").arg(Z0).arg(lstub1);
  }
  if ((ImplParams.open_short) && (!ImplParams.BalancedStubs))
  {  laddercode = QString("OL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub2);
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);

      laddercode += QString("OL:%1#%2;").arg(Z0).arg(lstub1);
  }
  if ((!ImplParams.open_short) && (ImplParams.BalancedStubs))
  {
      laddercode = QString("SU:%1#%2;SL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub2);
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);

      laddercode += QString("SU:%1#%2;SL:%1#%2;").arg(Z0).arg(lstub1);
  }
  if ((!ImplParams.open_short) && (!ImplParams.BalancedStubs))
  {
      laddercode = QString("SL:%1#%2;")
                     .arg(Z0)
                     .arg(lstub2);
      if (ImplParams.use_TL_lumped_equivalent)
          laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Z0, ImplParams.use_TL_lumped_equivalent, d);
      else
          laddercode += QString("TL:%1#%2;").arg(Z0).arg(d);

      laddercode += QString("SL:%1#%2;").arg(Z0).arg(lstub1);
  }

  return laddercode;
}

// -----------------------------------------------------------------------------------------------
// Binomial coefficients using the multiplicative formula (more efficient than
// the recursive one)
int BinomialCoeffs(int n, int k) {
  double coeff = 1;
  for (int i = 1; i <= k; i++) {
    coeff *= (n + (1 - i)) / (1.0 * i);
  }
  return (int)coeff;
}

QString MatchDialog::calcTransmissionLineTransformer(struct NetworkParams params)
{
  QString laddercode;
 if (params.network == TWO_PORT_OUTPUT){
   (params.OutputNetwork.weighting_type) ? laddercode = calcChebyLines(params) : laddercode = calcBinomialLines(params);
 }
 else{
   (params.InputNetwork.weighting_type) ?  laddercode = calcChebyLines(params) : laddercode = calcBinomialLines(params);
 }
 return laddercode;
}

//-----------------------------------------------------------------------------------
// This function calculates a multistage lambda/4 matching using binomial
// weigthing Reference: 'Microwave Engineering'. David Pozar. John Wiley and
// Sons. 4th Edition. Pg 252-256
QString MatchDialog::calcBinomialLines(struct NetworkParams params) {
  double RL, XL, Z0;
  struct ImplementationParams ImplParams;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT){
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }
      else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }

  r2z(RL, XL, Z0);
  if (RL == 0) {
    QMessageBox::warning(
        0, QObject::tr("Error"),
        QObject::tr("The load has not resistive part. It cannot be matched "
                    "using the quarter wavelength method"));
    return NULL;
  }
  if (XL != 0) {
    QMessageBox::warning(0, QObject::tr("Warning"),
                         QObject::tr("Reactive loads cannot be matched. Only "
                                     "the real part will be matched"));
  }
  double l4 = SPEED_OF_LIGHT / (4. * params.freq);
  double Ci, Zi, Zaux = Z0;
  QString laddercode;
  for (int i = 1; i < ImplParams.order; i++) {
    Ci = BinomialCoeffs(ImplParams.order - 1, i - 1);
    Zi = exp(log(Zaux) + (Ci / pow(2, ImplParams.order - 1)) * log(RL / Z0));
    Zaux = Zi;
    laddercode += QString("TL:%1#%2;").arg(Zi).arg(l4);
  }
  return laddercode;
}

//-----------------------------------------------------------------------------------
// This function calculates a multistage lambda/4 matching using the Chebyshev
// weigthing. Reference 'Microwave Engineering'. David Pozar. John Wiley and
// Sons. 4th Edition. Pg 256-261
QString MatchDialog::calcChebyLines(struct NetworkParams params) {
   double RL, XL, Z0;
   struct ImplementationParams ImplParams;
   if (params.network == SINGLE_PORT)
   {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
   }
   else//Two-port matching
   {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT){
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }
      else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
    }
  int N = ImplParams.order - 1; // Number of sections
  if (N > 7)         // So far, it is only available Chebyshev weighting up to 7
             // sections. Probably, it makes no sense to use a higher number of
             // sections because of the losses
  {
    QMessageBox::warning(
        0, QObject::tr("Error"),
        QObject::tr("Chebyshev weighting for N>7 is not available"));
    return QString("");
  }
  QString laddercode;

  r2z(RL, XL, Z0); // Calculation of the load impedance given the reflection coeffient
  double sec_theta_m; // =
                      // cosh((1/(1.*N))*acosh((1/gamma)*fabs((RL-Z0)/(Z0+RL)))
                      // );
  // double sec_theta_m = cosh((1/(1.*N))*acosh(fabs(log(RL/Z0)/(2*gamma))) );
  (fabs(log(RL / Z0) / (2 * ImplParams.gamma_MAX)) < 1)
      ? sec_theta_m = 0
      : sec_theta_m =
            cosh((1 / (1. * N)) * acosh(fabs(log(RL / Z0) / (2 * ImplParams.gamma_MAX))));

  double w[N];

  switch (N) // The weights are calculated by equating the reflection coeffient
             // formula to the N-th Chebyshev polinomial
  {
  case 1:
    w[0] = sec_theta_m;
    break;
  case 2:
    w[0] = sec_theta_m * sec_theta_m;
    w[1] = 2 * (sec_theta_m * sec_theta_m - 1);
    break;
  case 3:
    w[0] = pow(sec_theta_m, 3);
    w[1] = 3 * (pow(sec_theta_m, 3) - sec_theta_m);
    w[2] = w[1];
    break;
  case 4:
    w[0] = pow(sec_theta_m, 4);
    w[1] = 4 * sec_theta_m * sec_theta_m * (sec_theta_m * sec_theta_m - 1);
    w[2] = 2 * (1 - 4 * sec_theta_m * sec_theta_m + 3 * pow(sec_theta_m, 4));
    w[3] = w[1];
    break;
  case 5:
    w[0] = pow(sec_theta_m, 5);
    w[1] = 5 * (pow(sec_theta_m, 5) - pow(sec_theta_m, 3));
    w[2] =
        10 * pow(sec_theta_m, 5) - 15 * pow(sec_theta_m, 3) + 5 * sec_theta_m;
    w[3] = w[2];
    w[4] = w[1];
    break;
  case 6:
    w[0] = pow(sec_theta_m, 6);
    w[1] = 6 * pow(sec_theta_m, 4) * (sec_theta_m * sec_theta_m - 1);
    w[2] = 15 * pow(sec_theta_m, 6) - 24 * pow(sec_theta_m, 4) +
           9 * sec_theta_m * sec_theta_m;
    w[3] = 2 * (10 * pow(sec_theta_m, 6) - 18 * pow(sec_theta_m, 4) +
                9 * sec_theta_m * sec_theta_m - 1);
    w[4] = w[2];
    w[5] = w[1];
    break;
  case 7:
    w[0] = pow(sec_theta_m, 7);
    w[1] = 7 * pow(sec_theta_m, 5) * (sec_theta_m * sec_theta_m - 1);
    w[2] = 21 * pow(sec_theta_m, 7) - 35 * pow(sec_theta_m, 5) +
           14 * pow(sec_theta_m, 3);
    w[3] = 35 * pow(sec_theta_m, 7) - 70 * pow(sec_theta_m, 5) +
           42 * pow(sec_theta_m, 3) - 7 * sec_theta_m;
    w[4] = w[3];
    w[5] = w[2];
    w[6] = w[1];
    break;
  }
  double l4 = SPEED_OF_LIGHT / (4. * params.freq);
  double Zaux = Z0, Zi;
  for (int i = 0; i < N; i++) {
    (RL < Z0) ? Zi = exp(log(Zaux) - ImplParams.gamma_MAX * w[i])
              : Zi = exp(log(Zaux) + ImplParams.gamma_MAX * w[i]); // When RL<Z0, Z_{i}<Z_{i-1}
    Zaux = Zi;
    if (ImplParams.use_l4_lumped_equivalent == 0)
       laddercode += QString("TL:%1#%2;").arg(Zi).arg(l4);
    else
       laddercode += CalcTransmissionLineLumpedEquivalent(params.freq, Zaux, ImplParams.use_l4_lumped_equivalent, l4);
  }
  return laddercode;
}

//--------------------------------------------------------------------------
// It calculates a cascaded LC matching network (only real impedances)
// Reference: Inder J. Bahl. "Fundamentals of RF and microwave transistor
// amplifiers". John Wiley and Sons. 2009. Pages 169 - 170
QString MatchDialog::calcMatchingCascadedLCSections(struct NetworkParams params) {
  double RL, XL, Z0;
  struct ImplementationParams ImplParams;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT){
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }
      else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }
  double N = ImplParams.order - 1;
  double RS = params.Z1;
  double w = 2 * pi * params.freq, Q, C, L;

  r2z(RL, XL, Z0);
  double Raux, R, R1, R2;
  QString s = "";

  if (RL == 0) {
    QMessageBox::warning(
        0, QObject::tr("Error"),
        QObject::tr("The load is reactive. It cannot be matched "
                    "using the quarter wavelength method"));
    return NULL;
  }
  if (XL != 0) {
    QMessageBox::warning(0, QObject::tr("Warning"),
                         QObject::tr("Reactive loads cannot be matched. Only "
                                     "the real part will be matched"));
  }

  if (RL > RS)//The design equations were tailored for the RS > RL case. In case of RL > RS, the ports impedance are swapped
    R1 = RL, R2 = RS;
  else
    R1 = RS, R2 = RL;

  Raux = R1;
  for (int i = 0; i < N - 1; i++) {
    R = pow(R1, 1. * (N - (i + 1)) / N) * pow(R2, 1. * (i + 1) / N);
    Q = sqrt(Raux / R - 1);
    if (ImplParams.network_response == LOWPASS){
        C = Q / (w * Raux);
        L = Q * R / w;
        s += QString("CP:%1;LS:%2;").arg(C).arg(L);
    }else{//Highpass
        C = 1/(w*Q*R);
        L = Raux/(w*Q);
        s += QString("LP:%1;CS:%2;").arg(L).arg(C);
    }
    Raux = R;
  }

  Q = sqrt(R / R2 - 1);

  if (ImplParams.network_response == LOWPASS){
      C = Q / (w * R);
      L = Q * R2 / (w);
      s += QString("CP:%1;LS:%2;").arg(C).arg(L);
  }else{
      L = R/(w*Q);
      C = 1/(w*Q*R2);
      s += QString("LP:%1;CS:%2;").arg(L).arg(C);
  }
  if (RL > RS) // Flip string
  {
    QString temp = "";
    QStringList strlist = s.split(";");
    for (int i = strlist.count() - 1; i >= 0; i--)
      temp += strlist.at(i) + ";";
    s = temp;
  }

  return s;
}

QString MatchDialog::calcPiParameters(struct ImplementationParams ImplParams, double RL, double Z0) {
    double Q1 = (2*ImplParams.Q*Z0 - sqrt(4*ImplParams.Q*ImplParams.Q*Z0*RL - (Z0 - RL)*(Z0 - RL)))/(Z0 - RL);
    double Q2 = (2*ImplParams.Q*RL - sqrt(4*ImplParams.Q*ImplParams.Q*Z0*RL - (RL - Z0)*(RL - Z0)))/(RL - Z0);
    double B1 = Q1/Z0;
    double B2 = Q2/RL;
    double Xc = 2*ImplParams.Q*Z0/(1+Q1*Q1);
    return QString("%1;%2;%3").arg(B1).arg(Xc).arg(B2);
}

//--------------------------------------------------------------------------
// It calculates pi-type matching network
QString MatchDialog::calcPiType(struct NetworkParams params) {
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    QString pi_params = calcPiParameters(ImplParams, RL, Z0);
    double w0 = 2*pi*params.freq;
    QString laddercode;
    QStringList pi_comp_val = pi_params.split(";");//Get the parameters calculated by calcPiParameters()

    if (ImplParams.network_response == LOWPASS)
    {//Lowpass pi-type matching network
        double C1 = pi_comp_val.at(0).toDouble()/w0;// B1/w0
        double C2 = pi_comp_val.at(2).toDouble()/w0;// B2/w0
        double L = pi_comp_val.at(1).toDouble()/w0;//  X/w0

        //Build the schematic description
        laddercode += QString("CP:%1;").arg(C1);
        laddercode += QString("LS:%1;").arg(L);
        laddercode += QString("CP:%1;").arg(C2);
    }
    else
    {//Highpass pi-type matching network
        double L1 = 1/(w0*pi_comp_val.at(0).toDouble());// 1/(w0*B1)
        double L2 = 1/(w0*pi_comp_val.at(2).toDouble());// 1/(w0*B2)
        double C = 1/(w0*pi_comp_val.at(1).toDouble());//  1/(w0*X)

        //Build the schematic description
        laddercode += QString("LP:%1;").arg(L1);
        laddercode += QString("CS:%1;").arg(C);
        laddercode += QString("LP:%1;").arg(L2);
    }

    return laddercode;
}

//Synthesis of a tapped-C transformer
QString MatchDialog::calcTappedCTransformer(struct NetworkParams params)
{
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    double w0 = 2*pi*params.freq;

    // Design equations
    double Q2 = sqrt((RL/Z0)*(ImplParams.Q*ImplParams.Q + 1)-1);
    double L = Z0/(w0*ImplParams.Q);
    double C2 = Q2/(RL*w0);
    double C1 = C2*(Q2*Q2 + 1)/(ImplParams.Q*Q2 - Q2*Q2);

    //Build the schematic description
    QString laddercode;
    laddercode += QString("LP:%1;").arg(L);
    laddercode += QString("CS:%1;").arg(C1);
    laddercode += QString("CP:%1;").arg(C2);

    return laddercode;
}

//Synthesis of a tapped-L transformer
QString MatchDialog::calcTappedLTransformer(struct NetworkParams params)
{
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    double w0 = 2*pi*params.freq;

    // Design equations
    double Q2 = sqrt((RL/Z0)*(ImplParams.Q*ImplParams.Q + 1)-1);
    double C = ImplParams.Q/(w0*Z0);
    double L2 = RL/(Q2*w0);
    double L1 = L2*(ImplParams.Q*Q2 - Q2*Q2)/(Q2*Q2 + 1);

    //Build the schematic description
    QString laddercode;
    laddercode += QString("CP:%1;").arg(C);
    laddercode += QString("LS:%1;").arg(L1);
    laddercode += QString("LP:%1;").arg(L2);

    return laddercode;
}

//Synthesis of a double tapped transformer
QString MatchDialog::calcDoubleTappedResonator(struct NetworkParams params)
{
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    double w0 = 2*pi*params.freq;

    // Design equations
    double L1 = Z0/(w0*ImplParams.Q);
    double Qsq = ImplParams.Q*ImplParams.Q;
    double Q2 = sqrt((RL/Z0)*(Qsq + 1) - 1);
    double Leq = ((L1*Qsq)/(1+Qsq))+ImplParams.L2;
    double Ceq = 1/(w0*w0*Leq);
    double C2 = Q2/(w0*RL);
    double C2_ = C2*(1+Q2*Q2)/(Q2*Q2);
    double C1 = (Ceq*C2_)/(C2_ - Ceq);

    //Build the schematic description
    QString laddercode;
    laddercode += QString("LP:%1;").arg(L1);
    laddercode += QString("LS:%1;").arg(ImplParams.L2);
    laddercode += QString("CS:%1;").arg(C1);
    laddercode += QString("CP:%1;").arg(C2);

    return laddercode;
}


//--------------------------------------------------------------------------
// It calculates tee-type matching network
QString MatchDialog::calcTeeType(struct NetworkParams params) {
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    QString pi_params = calcPiParameters(ImplParams, RL, Z0);
    double w0 = 2*pi*params.freq;
    QString laddercode;
    QStringList pi_comp_val = pi_params.split(";");//Get the parameters calculated by calcPiParameters()
    double B1 = pi_comp_val.at(0).toDouble();
    double X =  pi_comp_val.at(1).toDouble();
    double B2 = pi_comp_val.at(2).toDouble();

    if (ImplParams.network_response == LOWPASS)
    {//Lowpass tee-type matching network
        double L1 = -B2*X/((B1*B2*X - B1 - B2)*w0);
        double L2 = -B1*X/((B1*B2*X - B1 - B2)*w0);
        double C = -(B1*B2*X - B1 - B2)/w0;

        //Build the schematic description
        laddercode += QString("LS:%1;").arg(L1);
        laddercode += QString("CP:%1;").arg(C);
        laddercode += QString("LS:%1;").arg(L2);
    }
    else
    {//Highpass tee-type matching network
        double C1 = -(B1*B2*X - B1 - B2)/(B2*X*w0);
        double C2 = -(B1*B2*X - B1 - B2)/(B1*X*w0);
        double L = -1/((B1*B2*X - B1 - B2)*w0);

        //Build the schematic description
        laddercode += QString("CS:%1;").arg(C1);
        laddercode += QString("LP:%1;").arg(L);
        laddercode += QString("CS:%1;").arg(C2);
    }

    return laddercode;
}

// This function implements the design equations of a single tuned transformer
QString MatchDialog::calcSingleTunedTransformer(struct NetworkParams params) {
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    double w0 = 2*pi*params.freq;

    //Design equations
    double L11 = (Z0*RL*ImplParams.k*ImplParams.k)/(w0*ImplParams.Q*(Z0+RL*ImplParams.k*ImplParams.k));
    double L22 = (L11*RL/Z0)/(ImplParams.k*ImplParams.k);
    double C = 1/(L11*w0*w0);

    QString laddercode;
    laddercode += QString("CP:%1;").arg(C);

    if (ImplParams.coupled_L_Equivalent==0){//Use coupled inductors
        laddercode += QString("LCOUP:%1#%2#%3;").arg(L11).arg(L22).arg(ImplParams.k);
    }else{
        double M = ImplParams.k*sqrt(L11*L22);
        if (ImplParams.coupled_L_Equivalent==1)
        {
        //Use the uncoupled equivalent circuit (tee)
        laddercode += QString("LS:%1;").arg(L11-M);
        laddercode += QString("LP:%1;").arg(M);
        laddercode += QString("LS:%1;").arg(L22-M);
        }
        else{
            //Use the uncoupled equivalent circuit (pi)
            double delta = L11*L22-M*M;
            laddercode += QString("LP:%1;").arg(delta/(L22-M));
            laddercode += QString("LS:%1;").arg(delta/M);
            laddercode += QString("LP:%1;").arg(delta/(L11-M));
        }
    }

    return laddercode;
}

// This function implements the design equations of a parallel double-tuned transformer
QString MatchDialog::calcParallelDoubleTunedTransformer(struct NetworkParams params) {
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    //Design equations
    double f0 = params.freq;
    double BW = ImplParams.BW;
    double w1 = 2*pi*(f0-0.5*BW);
    double w2 = 2*pi*(f0+0.5*BW);
    double gt = pow(10, (-ImplParams.gamma_MAX*0.1));
    double r = (1+sqrt(abs(1-gt)))/(1-sqrt(abs(1-gt)));
    double Q2m1 = abs(sqrt(r*w1/w2 - 1));
    double Q2m2 = abs(sqrt(r*w2/w1 - 1));
    double K1 = abs(Q2m1) * Z0/(1+Q2m1*Q2m1);
    double K2 = abs(Q2m2) * Z0/(1+Q2m2*Q2m2);
    double C2_ = (w2/(w1*w1) - 1/w2)/(K2+K1*w2/w1);
    double L2_ = -(K1-1/(w1*C2_))/w1;
    double RL_ = (1+Q2m1*Q2m1)/(w1*w1*C2_*C2_*Z0);
    double GL_ = 1/RL_;
    double Bm1 = -(C2_*C2_*L2_*w1*w1 + GL_*GL_*L2_ - C2_)*w1/(C2_*C2_*L2_*L2_*w1*w1*w1*w1 +
                                                         GL_*GL_*L2_*L2_*w1*w1 - 2*C2_*L2_*w1*w1 + 1);
    double Bm2 = -(C2_*C2_*L2_*w2*w2 + GL_*GL_*L2_ - C2_)*w2/(C2_*C2_*L2_*L2_*w2*w2*w2*w2 +
                                                              GL_*GL_*L2_*L2_*w2*w2 - 2*C2_*L2_*w2*w2 + 1);
    double L11 = ((1/w2) -(w2/(w1*w1)))/(-abs(Bm2) - abs(Bm1)*w2/w1);
    double C1 = -(abs(Bm1) - 1/(w1*L11))/w1;
    double k = 1/sqrt(1 + L2_/L11);
    double L22 = L11*RL/(k*k*RL_);
    double C2 = C2_*L11/(k*k*L22);

    QString laddercode;
    laddercode += QString("CP:%1;").arg(C1);

    if (ImplParams.coupled_L_Equivalent==0){//Use coupled inductors
        laddercode += QString("LCOUP:%1#%2#%3;").arg(L11).arg(L22).arg(k);
    }else{
        double M = k*sqrt(L11*L22);
        if (ImplParams.coupled_L_Equivalent==1)
        {
        //Use the uncoupled equivalent circuit (tee)
        laddercode += QString("LS:%1;").arg(L11-M);
        laddercode += QString("LP:%1;").arg(M);
        laddercode += QString("LS:%1;").arg(L22-M);
        }
        else{
            //Use the uncoupled equivalent circuit (pi)
            double delta = L11*L22-M*M;
            laddercode += QString("LP:%1;").arg(delta/(L22-M));
            laddercode += QString("LS:%1;").arg(delta/M);
            laddercode += QString("LP:%1;").arg(delta/(L11-M));
        }
    }
    laddercode += QString("CP:%1;").arg(C2);

    return laddercode;
}

// This function implements the design equations of a series double-tuned transformer
QString MatchDialog::calcSeriesDoubleTunedTransformer(struct NetworkParams params) {
    double RL, XL, Z0;
    struct ImplementationParams ImplParams;
    if (params.network == SINGLE_PORT)
    {
        RL = params.S11real, XL = params.S11imag;
        Z0 = params.Z1;
        ImplParams = params.InputNetwork;
    }
    else//Two-port matching
    {
        RL = params.r_real, XL = params.r_imag;
        if (params.network == TWO_PORT_INPUT){
            Z0 = params.Z1;
            ImplParams = params.InputNetwork;
        }
        else{
            Z0 = params.Z2;
            ImplParams = params.OutputNetwork;
        }
    }
    r2z(RL, XL, Z0);

    //Design equations
    double f0 = params.freq;
    double w0 = 2*pi*params.freq;
    double BW = ImplParams.BW;
    double f1 = f0 - 0.5*BW;
    double f2 = f0 + 0.5*BW;
    double M = (f1*f1+f2*f2)/(f1*f2);
    double gt = pow(10, (-ImplParams.gamma_MAX*0.1));
    double kQ = 1/sqrt(gt) + sqrt((1/gt)-1);
    double kQsq = kQ*kQ;
    double A = (kQsq*kQsq)*(M*M) - 4*kQsq;
    double B = (4-M*M)*kQsq;
    double k = (-A + sqrt(A*A - 4*B))/2;
    double Q = kQ/k;

    double L11 = Q*Z0/w0;
    double C1 = 1/(w0*Z0*Q);
    double L22 = Q*RL/w0;
    double C2 = 1/(w0*RL*Q);

    QString laddercode;
    laddercode += QString("CS:%1;").arg(C1);

    if (ImplParams.coupled_L_Equivalent==0){//Use coupled inductors
        laddercode += QString("LCOUP:%1#%2#%3;").arg(L11).arg(L22).arg(k);
    }else{
        double M = k*sqrt(L11*L22);
        if (ImplParams.coupled_L_Equivalent==1)
        {
        //Use the uncoupled equivalent circuit (tee)
        laddercode += QString("LS:%1;").arg(L11-M);
        laddercode += QString("LP:%1;").arg(M);
        laddercode += QString("LS:%1;").arg(L22-M);
        }
        else{
            //Use the uncoupled equivalent circuit (pi)
            double delta = L11*L22-M*M;
            laddercode += QString("LP:%1;").arg(delta/(L22-M));
            laddercode += QString("LS:%1;").arg(delta/M);
            laddercode += QString("LP:%1;").arg(delta/(L11-M));
        }
    }
    laddercode += QString("CS:%1;").arg(C2);

    return laddercode;
}

//--------------------------------------------------------------------------
// It calculates a lambda/8 + lambda/4 transformer to match a complex load to a
// real source
QString MatchDialog::calcMatchingLambda8Lambda4(struct NetworkParams params) {
  double l4 = SPEED_OF_LIGHT / (4. * params.freq);
  double l8 = .5 * l4;
  struct ImplementationParams ImplParams;
  double RL, XL, Z0;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT) {
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }

  r2z(RL, XL, Z0);
  double Zmm = sqrt(RL * RL + XL * XL);
  double Zm = sqrt((Z0 * RL * Zmm) / (Zmm - XL));

  QString str;

  if (ImplParams.use_l4_lumped_equivalent == 0){
      str += QString("TL:%1#%2;").arg(Zm).arg(l4);
  } else{
      str += QString("%1").arg(CalcTransmissionLineLumpedEquivalent(params.freq, Zm, ImplParams.use_l4_lumped_equivalent, l4));
  }

  if (ImplParams.use_l8_lumped_equivalent == 0){
      str += QString("TL:%1#%2;").arg(Zmm).arg(l8);
  } else{
      str += QString("%1").arg(CalcTransmissionLineLumpedEquivalent(params.freq, Zmm, ImplParams.use_l8_lumped_equivalent, l8));
  }
return str;
}

//--------------------------------------------------------------------------
// This function calculates a lambda/4 transformer to match a real load to a
// real source
QString MatchDialog::calcMatchingLambda4(struct NetworkParams params) {
  double l4 = SPEED_OF_LIGHT / (4. * params.freq);
  double RL, XL, Z0;
  struct ImplementationParams ImplParams;
  if (params.network == SINGLE_PORT)
  {
      RL = params.S11real, XL = params.S11imag;
      Z0 = params.Z1;
      ImplParams = params.InputNetwork;
  }
  else//Two-port matching
  {
      RL = params.r_real, XL = params.r_imag;
      if (params.network == TWO_PORT_INPUT) {
          Z0 = params.Z1;
          ImplParams = params.InputNetwork;
      }else{
          Z0 = params.Z2;
          ImplParams = params.OutputNetwork;
      }
  }

  r2z(RL, XL, Z0);
  double Zm = sqrt(Z0*RL);
  if (ImplParams.use_l4_lumped_equivalent == 0)
     return QString("TL:%1#%2;").arg(Zm).arg(l4);
  else
     return CalcTransmissionLineLumpedEquivalent(params.freq, Zm, ImplParams.use_l4_lumped_equivalent, l4);
}

// Given a string code of inductors, capacitors and transmission lines, it
// generates the Qucs network. Notice that the schematic is split into three
// parts: components, wires and paintings, all of them are passed by reference.
void MatchDialog::SchematicParser(QString laddercode, int &x_pos, struct NetworkParams params) {
  QStringList strlist = laddercode.split(";");//Slipt the string code to get the components
  QString component, tag, label;
  qDebug() << laddercode;
  double value, value2, value3, er, width;
  int x_series = 120, x_shunt = 100; // x-axis spacing depending on whether the
                                    // component is placed in a series or shunt
                                    // configuration

  double Freq = params.freq;
  bool microsyn = params.micro_syn;
  tSubstrate Substrate = params.Substrate;
  double CAPQ, INDQ;
  CAPQ = params.InputNetwork.CAPQ;
  INDQ = params.InputNetwork.INDQ;

  // Clear schematic strings
  QString componentstr = "", wirestr = "", paintingstr = "";

  // The string format is as follows: "XX<value>;XX<value2>;...XX<valueN>;"
  // where XX, YY, ZZ define the type of component and its configuration.
  //    LS: Series inductance
  //    CS: Series capacitor
  //    LP: Shunt inductance
  //    CP: Shunt capacitor
  //    TL: Series transmission line
  //    OU: Open stub (facing up)
  //    OL: Open stub (facing down)
  //    SU: Short circuited stub (facing up)
  //    SL: Short circuited stub (facing down)
  //    P1: Port 1
  //    LBL: LBL
  //    P2: Port 2
  //    DEV: Device label
  //    S2P: S-param simulation block
  //    LCOUP: Coupled inductors

  for (int i = 0; i < strlist.count(); i++) {
    // Each token of the string descriptor has the following format:
    // 'tag:<value>;''tag:<value1>#<value2>;'
    // First, extract the tag
    component = strlist.at(i);
    int index_colon = component.indexOf(":");
    tag = component.mid(0, index_colon);

    // Now we remove the tag and the colon from the string
    component.remove(0, index_colon + 1);
    // At this point, we have the component parameters. In case of having more
    // than one parameter, they are separated by #
    int index = component.indexOf("#");
    // Transmission lines are defined by its impedance and its length
    // whereas capacitors and inductors only depend on
    // its capacitance and inductance, respectively. So, the code below is aimed
    // to handle such difference
    if (index != -1) // The component has two values
    {
      double index2 = component.indexOf("#", index+1);
      value = component.mid(0, index).toDouble();
      if (index2 == -1)
      {//Just two parameters
      value2 = component.mid(index + 1).toDouble();
      }
      else
      {//Three parameters: Coupled inductors
          value2 = component.mid(index + 1, index2-index-1).toDouble();
          value3 = component.mid(index2 + 1).toDouble();
      }
    } else {
      if (!tag.compare("LBL")) // The value is a string
      {
        label = component;
      } else // The value is a double number. For a clearer representation in
             // the schematic, it will be shown with a limited precision
      {
        value = component.toDouble();
      }
    }

    // The following if-else structure is responsible for placing the
    // components, wires and paintings in the schematic
    if (!tag.compare("P1")) // Port 1 component
    {
      componentstr += QString("<Pac P1 1 %2 -30 18 -26 0 1 \"1\" 1 \"%1\" 1 "
                              "\"0 dBm\" 0 \"1 GHz\" 0>\n")
                          .arg(misc::num2str(value, 3, "Ohm")) // reference impedance
                          .arg(x_pos);
      componentstr += QString("<GND * 1 %1 0 0 0 0 0>\n").arg(x_pos);
      wirestr += QString("<%1 -60 %1 -120>\n").arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120>\n").arg(x_pos).arg(x_pos + 120);
      x_pos += 120;
    } else if (!tag.compare("LBL")) // Label
    {
      paintingstr += QString("<Text %1 -150 12 #000000 0 \"%2\">\n")
                         .arg(x_pos)
                         .arg(component); // Add 'Port 1' or 'Port 2' label
      x_pos += 50;
    } else if (!tag.compare("P2")) // Port 2 component (it need to be different
                                   // from P1 because of the wiring)
    {
      x_pos += 100;
      componentstr += QString("<Pac P2 1 %2 -30 18 -26 0 1 \"1\" 1 \"%1\" 1 "
                              "\"0 dBm\" 0 \"1 GHz\" 0>\n")
                          .arg(misc::num2str(value, 3, "Ohm")) // reference impedance
                          .arg(x_pos);
      componentstr += QString("<GND * 1 %1 0 0 0 0 0>\n").arg(x_pos);
      wirestr += QString("<%1 -60 %1 -120>\n").arg(x_pos); // Vertical wire
      wirestr += QString("<%1 -120 %2 -120>\n")
                     .arg(x_pos - 100)
                     .arg(x_pos);   // Horizontal wire
    } else if (!tag.compare("DEV")) // Device painting
    {
      paintingstr += QString("<Text %1 -150 12 #000000 0 \"Device\">\n")
                         .arg(x_pos + 70); // Add 'Device' label
      paintingstr +=
          QString("<Rectangle %1 -160 90 50 #000000 0 1 #c0c0c0 1 0>\n")
              .arg(x_pos + 50); // Box surrounding the 'Device' label
      x_pos += 200;

      //Update CAPQ and INDQ data
      CAPQ = params.OutputNetwork.CAPQ;
      INDQ = params.OutputNetwork.INDQ;
    } else if (!tag.compare("LS")) // Series inductor
    {
      QString val = misc::num2str(value, 3, "H"); // Add prefix, unit - 3 significant digits
      if (INDQ == 1000){//Use an ideal inductor
          componentstr += QString("<L L1 1 %1 -120 -26 10 0 0 \"%2\" 1 "
                                  " 0>\n")
                              .arg(x_pos + 60)
                              .arg(val);
      }else{
      componentstr += QString("<INDQ INDQ1 1 %1 -120 -26 10 0 0 \"%2\" 1 \"%3\" 1 "
                              " 0>\n")
                          .arg(x_pos + 60)
                          .arg(val)
                          .arg(INDQ);
      }
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos)
                     .arg(x_pos + 30);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos + 90)
                     .arg(x_pos + x_series);
      x_pos += x_series;
    } else if (!tag.compare("CS")) // Series capacitor
    {
      QString val = misc::num2str(value, 3, "F"); // Add prefix, unit - 3 significant digits
      if (CAPQ == 1000){//Use an ideal capacitor
          componentstr += QString("<C C1 1 %1 -120 -26 17 0 0 \"%2\" 1 "
                                  " 0>\n")
                              .arg(x_pos + 60)
                              .arg(val);
      }
      else{
          componentstr += QString("<CAPQ CAPQ1 1 %1 -120 -26 17 0 0 \"%2\" 1 \"%3\" 1 "
                                  " 0>\n")
                              .arg(x_pos + 60)
                              .arg(val)
                              .arg(CAPQ);
      }
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos)
                     .arg(x_pos + 30);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos + 90)
                     .arg(x_pos + x_series);
      x_pos += x_series;
    } else if (!tag.compare("LP")) // Shunt inductor
    {
      QString val = misc::num2str(value, 3, "H"); // Add prefix, unit - 3 significant digits
      componentstr += QString("<GND * 1 %1 0 0 0 0 0>\n").arg(x_pos);
      if (INDQ == 1000){
          componentstr += QString("<L L1 1 %1 -30 5 -20 0 1 \"%2\" 1 "
                                  " 0>\n")
                              .arg(x_pos)
                              .arg(val);
      }
      else{
          componentstr += QString("<INDQ INDQ1 1 %1 -30 5 -20 0 1 \"%2\" 1 \"%3\" 1 "
                                  " 0>\n")
                              .arg(x_pos)
                              .arg(val)
                              .arg(INDQ);
      }

      wirestr += QString("<%1 -60 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos - 20)
                     .arg(x_pos + x_shunt);
      x_pos += x_shunt;
    } else if (!tag.compare("CP")) // Shunt capacitor
    {
      QString val = misc::num2str(value, 3, "F"); // Add prefix, unit - 3 significant digits
      componentstr += QString("<GND * 1 %1 0 0 0 0 0>\n").arg(x_pos);
      if (CAPQ == 1000){//Use an ideal capacitor
          componentstr += QString("<C C1 1 %1 -30 15 -20 0 1 \"%2\" 1 "
                                  " 0>\n")
                              .arg(x_pos)
                              .arg(val);
      }
      else{
          componentstr += QString("<CAPQ CAPQ1 1 %1 -30 15 -20 0 1 \"%2\" 1 \"%3\" 1 "
                                  " 0>\n")
                              .arg(x_pos)
                              .arg(val)
                              .arg(CAPQ);
      }

      wirestr += QString("<%1 -60 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos - 20)
                     .arg(x_pos + x_shunt);
      x_pos += x_shunt;
    } else if (!tag.compare("LCOUP")) // Coupled inductors
    {
        x_pos += 120;
        QString L11_val = misc::num2str(value, 3, "H"); // Add prefix, unit - 3 significant digits
        QString L22_val = misc::num2str(value2, 3, "H"); // Add prefix, unit - 3 significant digits
        QString k = misc::num2str(value3);
        componentstr += QString("<MUT Tr1 1 %1 -90 40 -20 0 0 \"%2\" 1 \"%3\" 1 \"%4\" 1 "
                                " 0>\n")
                            .arg(x_pos)
                            .arg(L11_val)
                            .arg(L22_val)
                            .arg(k);
        componentstr += QString("<GND * 1 %1 -60 0 0 0 0>\n").arg(x_pos-30);
        componentstr += QString("<GND * 1 %1 -60 0 0 0 0>\n").arg(x_pos+30);

        wirestr += QString("<%1 -120 %2 -120 "
                           " 0 0 0 "
                           ">\n")
                       .arg(x_pos - 120)
                       .arg(x_pos - 30);

        wirestr += QString("<%1 -120 %2 -120 "
                           " 0 0 0 "
                           ">\n")
                       .arg(x_pos + 30)
                       .arg(x_pos + 140);

        x_pos += 140;

    }else if (!tag.compare("TL")) // Transmission line
    {
      if (microsyn) // Microstrip implementation
      {
        er = Substrate.er;
        getMicrostrip(value, Freq, &Substrate, width, er);
        QString val_width =
          misc::num2str(width, 3, "m"); // Add prefix, unit - 3 significant digits
        QString val_length =
          misc::num2str(value2 / sqrt(er), 3, "m"); // Add prefix, unit - 3 significant digits
        componentstr +=
            QString("<MLIN MS1 1 %3 -120 -26 20 0 0 \"Sub1\" 1 \"%1\" 1 \"%2\" "
                    "1 \"Hammerstad\" 0 \"Kirschning\" 0 \"26.85\" 0>\n")
                .arg(val_width)
                .arg(val_length)
                .arg(x_pos + 60);
      } else {
        // Add a prefix (although rarely needed here) and unit - 3 significant digits
        QString val_impedance = misc::num2str(value, 3, "Ohm");
        // Add prefix, unit - 3 significant digits
        QString val_length = misc::num2str(value2, 3, "m");
        componentstr += QString("<TLIN Line1 1 %3 -120 -26 20 0 0 \"%1\" 1 "
                                "\"%2\" 1 \"0 dB\" 0 \"26.85\" 0>\n")
                            .arg(val_impedance)
                            .arg(val_length)
                            .arg(x_pos + 60);
      }
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos)
                     .arg(x_pos + 30);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos + 90)
                     .arg(x_pos + x_series);
      x_pos += x_series;
    } else if (!tag.compare("OU")) // Open stub (upper)
    {
      if (microsyn) // Microstrip implementation
      {
        er = Substrate.er;
        getMicrostrip(value, Freq, &Substrate, width, er);
        // Add prefix, unit - 3 significant digits
        QString val_width = misc::num2str(width, 3, "m");
        // Add prefix, unit - 3 significant digits
        QString val_length = misc::num2str(value2 / sqrt(er), 3, "m");
        componentstr +=
            QString("<MLIN MS1 1 %3 -180 30 -30 0 1 \"Sub1\" 1 \"%1\" 1 \"%2\" "
                    "1 \"Hammerstad\" 0 \"Kirschning\" 0 \"26.85\" 0>\n")
                .arg(val_width)
                .arg(val_length)
                .arg(x_pos);
      } else {
        // Add a prefix (although rarely needed here) and unit - 3 significant digits
        QString val_impedance = misc::num2str(value, 3, "Ohm");
        // Add prefix, unit - 3 significant digits
        QString val_length = misc::num2str(value2, 3, "m");
        componentstr += QString("<TLIN Line1 1 %3 -180 30 -30 0 1 \"%1\" 1 "
                                "\"%2\" 1 \"0 dB\" 0 \"26.85\" 0>\n")
                            .arg(val_impedance)
                            .arg(val_length)
                            .arg(x_pos);
      }
      wirestr += QString("<%1 -150 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos)
                     .arg(x_pos + x_shunt);
      // Here x_pos is not incremented since upper stubs does not overlap any
      // other component
    } else if (!tag.compare("OL")) // Open stub (lower)
    {
      if (microsyn) // Microstrip implementation
      {
        er = Substrate.er;
        getMicrostrip(value, Freq, &Substrate, width, er);
        // Add suffix mm, cm - 3 significant digits
        QString val_width = misc::num2str(width, 3, "m");
        // Add suffix mm, cm - 3 significant digits
        QString val_length = misc::num2str(value2 / sqrt(er), 3, "m");
        componentstr +=
            QString("<MLIN MS1 1 %3 -60 -26 30 0 1 \"Sub1\" 1 \"%1\" 1 \"%2\" "
                    "1 \"Hammerstad\" 0 \"Kirschning\" 0 \"26.85\" 0>\n")
                .arg(val_width)
                .arg(val_length)
                .arg(x_pos);
      } else {
        // Add a prefix (although rarely needed here) and unit - 3 significant digits
        QString val_impedance = misc::num2str(value, 3, "Ohm");
        // Add suffix mm, cm - 3 significant digits
        QString val_length = misc::num2str(value2, 3, "m");
        componentstr += QString("<TLIN Line1 1 %3 -60 -26 30 0 1 \"%1\" 1 "
                                "\"%2\" 1 \"0 dB\" 0 \"26.85\" 0>\n")
                            .arg(val_impedance)
                            .arg(val_length)
                            .arg(x_pos);
      }
      wirestr += QString("<%1 -90 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos - 20)
                     .arg(x_pos + x_shunt);
      x_pos += x_shunt;
    } else if (!tag.compare("SU")) // Short circuited stub (upper)
    {
      if (microsyn) // Microstrip implementation
      {
        er = Substrate.er;
        getMicrostrip(value, Freq, &Substrate, width, er);
        QString val_width = misc::num2str(width, 3, "m");
        QString val_length = misc::num2str(value2 / sqrt(er), 3, "m");
        componentstr +=
            QString("<MLIN MS1 1 %3 -180 30 -30 0 1 \"Sub1\" 1 \"%1\" 1 \"%2\" "
                    "1 \"Hammerstad\" 0 \"Kirschning\" 0 \"26.85\" 0>\n")
                .arg(val_width)
                .arg(val_length)
                .arg(x_pos);
      } else {
        QString val_impedance = misc::num2str(value, 3, "Ohm");
        QString val_length = misc::num2str(value2, 3, "m");
        componentstr += QString("<TLIN Line1 1 %3 -180 30 -30 0 1 \"%1\" 1 "
                                "\"%2\" 1 \"0 dB\" 0 \"26.85\" 0>\n")
                            .arg(val_impedance)
                            .arg(val_length)
                            .arg(x_pos);
      }
      componentstr += QString("<GND * 1 %1 -210 0 0 0 2>\n").arg(x_pos);
      wirestr += QString("<%1 -150 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos)
                     .arg(x_pos + x_shunt);
      // Here x_pos is not incremented since upper stubs does not overlap any
      // other component
    } else if (!tag.compare("SL")) // Short circuited stub (lower)
    {
      if (microsyn) // Microstrip implementation
      {
        er = Substrate.er;
        getMicrostrip(value, Freq, &Substrate, width, er);
        QString val_width = misc::num2str(width, 3, "m");
        QString val_length = misc::num2str(value2 / sqrt(er), 3, "m");
        componentstr +=
            QString("<MLIN MS1 1 %3 -60 30 -30 0 1 \"Sub1\" 1 \"%1\" 1 \"%2\" "
                    "1 \"Hammerstad\" 0 \"Kirschning\" 0 \"26.85\" 0>\n")
                .arg(val_width)
                .arg(val_length)
                .arg(x_pos);
      } else {
        QString val_impedance = misc::num2str(value, 3, "Ohm");
        QString val_length = misc::num2str(value2, 3, "m");
        componentstr += QString("<TLIN Line1 1 %3 -60 20 30 0 1 \"%1\" 1 "
                                "\"%2\" 1 \"0 dB\" 0 \"26.85\" 0>\n")
                            .arg(val_impedance)
                            .arg(val_length)
                            .arg(x_pos);
      }
      componentstr += QString("<GND * 1 %1 -30 0 0 0 0>\n").arg(x_pos);
      wirestr += QString("<%1 -90 %1 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos);
      wirestr += QString("<%1 -120 %2 -120 "
                         " 0 0 0 "
                         ">\n")
                     .arg(x_pos - 20)
                     .arg(x_pos + x_shunt);
      x_pos += x_shunt;
    } else if (!tag.compare("S2P")) // S-param simulation block
    {
      // Add the frequency range for the S-param simulation
      //   cover 1 octave below and 1 above, user will adjust if needed...
      double freq_start = Freq / 2.0;
      double freq_stop = 2.0 *Freq;
      QString val_freq_start = misc::num2str(freq_start, 3, "Hz");
      QString val_freq_stop = misc::num2str(freq_stop, 3, "Hz");

      componentstr +=
          QString("<.SP SP1 1 0 100 0 67 0 0 \"lin\" 1 \"%1\" 1 \"%2\" 1 "
                  "\"300\" 1 \"no\" 0 \"1\" 0 \"2\" 0>\n")
              .arg((val_freq_start))
              .arg((val_freq_stop));

      if (laddercode.indexOf("P2") == -1) // One port simulation
        componentstr += QString("<Eqn Eqn1 1 200 100 -28 15 0 0 "
                                "\"S11_dB=dB(S[1,1])\" 1 \"yes\" 0>\n");
      else // Two ports simulation
        componentstr += QString("<Eqn Eqn1 1 200 100 -28 15 0 0 "
                                "\"S11_dB=dB(S[1,1])\" 1 \"S21_dB=dB(S[2,1])\" "
                                "1 \"S22_dB=dB(S[2,2])\" 1 \"yes\" 0>\n");
    } else if (!tag.compare("ZL")) // Complex load
    {
      double RL = value;
      double XL = value2;
      x_pos += 100;
      if ((RL > 1e-3) && (XL < -1e-3)) // R + C
      {
        QString val_Res = misc::num2str(RL, 3, "Ohm");
        // Need to use abs() because XL < 0
        QString val_Cap = misc::num2str(1 / (fabs(XL) * 2 * pi * Freq), 3, "F");
        componentstr +=
            QString(
                "<R R1 1 %1 -30 15 -26 0 -1 \"%2\" 1 \"26.85\" 0 \"US\" 0>\n")
                .arg(x_pos)
                .arg(val_Res);
        componentstr += QString("<C C1 1 %1 -90 15 -26 0 -1 \"%2\" 1 0>\n")
                            .arg(x_pos)
                            .arg(val_Cap);
        paintingstr +=
            QString("<Text %1 50 12 #000000 0 \"%4-j%5 %2 @ %3\">\n")
                .arg(x_pos)
                .arg(QChar(0x2126))
                .arg(misc::num2str(Freq, 3, "Hz"))
                .arg(RL)
                .arg(fabs(XL));
      } else if ((RL > 1e-3) && (XL > 1e-3)) // R + L
      {
        QString val_Res = misc::num2str(RL, 3, "Ohm");
        QString val_Ind = misc::num2str(XL / (2 * pi * Freq), 3, "H");
        componentstr +=
            QString(
                "<R R1 1 %1 -30 15 -26 0 -1 \"%2\" 1 \"26.85\" 0 \"US\" 0>\n")
                .arg(x_pos)
                .arg(val_Res);
        componentstr += QString("<L L1 1 %1 -90 15 -26 0 -1 \"%2\" 1 0>\n")
                            .arg(x_pos)
                            .arg(val_Ind);
        paintingstr +=
            QString("<Text %1 50 12 #000000 0 \"%4+j%5 %2 @ %3\">\n")
                .arg(x_pos)
                .arg(QChar(0x2126))
                .arg(misc::num2str(Freq, 3, "Hz"))
                .arg(RL)
                .arg(XL);
      } else if ((RL > 1e-3) && (fabs(XL) < 1e-3)) // R
      {
        QString val_Res = misc::num2str(RL, 3, "Ohm");
        componentstr +=
            QString(
                "<R R1 1 %1 -30 15 -26 0 -1 \"%2\" 1 \"26.85\" 0 \"US\" 0>\n")
                .arg(x_pos)
                .arg(val_Res);
        wirestr += QString("<%1 -60 %1 -120>\n").arg(x_pos); // Vertical wire
        paintingstr += QString("<Text %1 50 12 #000000 0 \"%4 %2 @ %3\">\n")
                           .arg(x_pos)
                           .arg(QChar(0x2126))
                           .arg(misc::num2str(Freq, 3, "Hz"))
                           .arg(RL);
      } else if ((RL < 1e-3) && (XL > 1e-3)) // L
      {
        QString val_Ind = misc::num2str(XL / (2 * pi * Freq), 3, "H");
        componentstr +=
            QString(
                "<L L1 1 %1 -30 15 -26 0 -1 \"%2\" 1 \"26.85\" 0 \"US\" 0>\n")
                .arg(x_pos)
                .arg(val_Ind);
        wirestr += QString("<%1 -60 %1 -120>\n").arg(x_pos); // Vertical wire
        paintingstr +=
            QString("<Text %1 50 12 #000000 0 \"j%4 %2 @ %3\">\n")
                .arg(x_pos)
                .arg(QChar(0x2126))
                .arg(misc::num2str(Freq, 3, "Hz"))
                .arg(XL);
      } else if ((RL < 1e-3) && (XL < -1e-3)) // C
      {
        // Need to use abs() because XL < 0
        QString val_Cap = misc::num2str(1 / (fabs(XL) * 2 * pi * Freq), 3, "F");
        componentstr +=
            QString(
                "<C C1 1 %1 -30 15 -26 0 -1 \"%2\" 1 \"26.85\" 0 \"US\" 0>\n")
                .arg(x_pos)
                .arg(val_Cap);
        wirestr += QString("<%1 -60 %1 -120>\n").arg(x_pos); // Vertical wire
        paintingstr +=
            QString("<Text %1 50 12 #000000 0 \"-j%4 %2 @ %3\">\n")
                .arg(x_pos)
                .arg(QChar(0x2126))
                .arg(misc::num2str(Freq, 3, "Hz"))
                .arg(fabs(XL));
      }
      wirestr += QString("<%1 -120 %2 -120>\n")
                     .arg(x_pos - 100)
                     .arg(x_pos); // Horizontal wire
      componentstr += QString("<GND * 1 %1 0 0 -1 0 0>\n").arg(x_pos);

      // Box surrounding the load
      paintingstr +=
          QString("<Rectangle %1 -150 200 200 #000000 0 1 #c0c0c0 1 0>\n")
              .arg(x_pos - 30);
    }
  }

  // Substrate
  if (microsyn)
    componentstr +=
        QString("<SUBST Sub1 1 400 200 -30 24 0 0 \"%1\" 1 \"%2mm\" 1 \"%3um\" "
                "1 \"%4\" 1 \"%5\" 1 \"%6\" 1>\n")
            .arg(Substrate.er)
            .arg(Substrate.height * 1e3)
            .arg(Substrate.thickness * 1e6)
            .arg(Substrate.tand)
            .arg(Substrate.resistivity)
            .arg(Substrate.roughness);

  // Schematic header
  QString Schematic = "<Qucs Schematic " PACKAGE_VERSION ">\n";

  // Add components
  Schematic += "<Components>\n";
  Schematic += componentstr;
  Schematic += "</Components>\n";

  // Add wires
  Schematic += "<Wires>\n";
  Schematic += wirestr;
  Schematic += "</Wires>\n";

  // Add paintings
  Schematic += "<Paintings>\n";
  Schematic += paintingstr;
  Schematic += "</Paintings>\n";

  //Copy the schematic into clipboard
  QApplication::clipboard()->setText(Schematic, QClipboard::Clipboard);
}

//This function pops up a window for setting the parameters of the input matching network
void MatchDialog::slot_InputMatchingSettings()
{
    MatchSettingsDialog *M = new MatchSettingsDialog(input_network, this, TopoCombo_Input->currentIndex());
    if(M->exec())
    {
        struct ImplementationParams N = M->GetOptions();
        if (N.order != -1)
           input_network = N;
    }
    delete M;
}

//This function pops up a window for setting the parameters of the output matching network
void MatchDialog::slot_OutputMatchingSettings()
{
    MatchSettingsDialog *M = new MatchSettingsDialog(output_network, this, TopoCombo_Output->currentIndex());
    if(M->exec())
    {
        struct ImplementationParams N = M->GetOptions();
        if (N.order != -1)
            output_network = N;
    }
    delete M;
}

//This function pops up a window for setting the parameters of the microstrip substrate
void MatchDialog::slot_SubtrateSettings()
{
    MatchSubstrateDialog *M = new MatchSubstrateDialog(this);
    if(M->exec())
    {
        struct tSubstrate N = M->GetOptions();
        if (N.er != -1)
            params.Substrate = N;
    }
    delete M;
}

//This function is triggered by the input topology combo and its purpose is to enable/disable the settings button and the microstrip implementation checkbox
void MatchDialog::slot_InputTopologyChanged(int currentIndex)
{
    //Enable/Disable the microstrip substrate checkbox
    if ((Transmission_Line_Topologies.contains(TopoCombo_Input->currentIndex())) || Transmission_Line_Topologies.contains(TopoCombo_Output->currentIndex())) {
        MicrostripCheck->setEnabled(true);
     }
     else  {
         MicrostripCheck->setEnabled(false);
         MicrostripCheck->setChecked(false);
         Substrate_Button->setEnabled(false);
     }
}

//This function is triggered by the output topology combo and its purpose is to enable/disable the settings button and the microstrip implementation checkbox
void MatchDialog::slot_OutputTopologyChanged(int currentIndex)
{
    //Enable/Disable the microstrip substrate checkbox
    if ((Transmission_Line_Topologies.contains(TopoCombo_Input->currentIndex())) || Transmission_Line_Topologies.contains(TopoCombo_Output->currentIndex())) {
       MicrostripCheck->setEnabled(true);
    }
    else  {
        MicrostripCheck->setEnabled(false);
        MicrostripCheck->setChecked(false);
        Substrate_Button->setEnabled(false);
    }
}

// This function is called when the microstrip checkbox is clicked. Its purpose is to enable and disable the substrate settings window accordingly
void MatchDialog::slot_MicrostripCheckChanged()
{
    bool microstrip_implementation = false;
    if (MicrostripCheck->isChecked())
        microstrip_implementation = true;
    Substrate_Button->setEnabled(microstrip_implementation);
}

//This function calculates the lumped element equivalent of an arbitrary-length transmission line
QString MatchDialog::CalcTransmissionLineLumpedEquivalent(double f0, double Zm, int mode, double L)
{
    double w0 = 2*pi*f0;
    double Xs, Xp;
    double beta = 2*pi*f0/SPEED_OF_LIGHT;
    Xs = Zm*sin(beta*L);
    Xp = -Xs/(1-cos(beta*L));
    if (mode == 1){//Pi type equivalent
        if (Xs >= 0)// L <= lambda/2
           return QString("CP:%1;LS:%2;CP:%1;").arg(-1/(Xp*w0)).arg(Xs/w0);
        else//L > lambda
           return QString("LP:%1;CS:%2;LP:%1;").arg(Xp/w0).arg(-1/(w0*Xs));
    }else{//Tee type equivalent
        if (Xs >= 0)// L <= lambda/2
           return QString("LS:%1;CP:%2;LS:%1;").arg((Xp*Xs)/(w0*(2*Xp+Xs))).arg(-(2*Xp+Xs)/(Xp*Xp*w0));
        else//L > lambda
           return QString("CS:%1;LP:%2;CS:%1;").arg(-(2*Xp+Xs)/(Xp*Xs*w0)).arg(Xp*Xp/((2*Xp+Xs)*w0));
    }
}

//The purpose of this function is to simplify series parallel combinations of capacitors/inductors
QString MatchDialog::SimplifySeriesParallelConnections(QString laddercode)
{
    QStringList strlist = laddercode.split(";");//Slipt the string code to get the components
    QString component, component_aux, tag, tag_aux;
    double value, value_aux;
    QString output;

    for (int i = 0; i < strlist.count(); i++) {
        // Each token of the string descriptor has the following format:
        // 'tag:<value>;''tag:<value1>#<value2>;'
        // First, extract the tag
        component = strlist.at(i);
        int index_colon = component.indexOf(":");
        tag = component.mid(0, index_colon);
        value = component.mid(index_colon+1).toDouble();

      double new_val;
      bool simplify = false;
      if (!tag.compare(tag_aux))
          //Check if the components are LS, LP, CS or CP and simplify
          if (!tag.compare("LS") || !tag.compare("LP") || !tag.compare("CS") || !tag.compare("CP"))
          {
            simplify = true;
           if (!tag.compare("LS"))
              new_val = value+value_aux;
          else if (!tag.compare("LP"))
                    new_val = (value*value_aux)/(value+value_aux);
                 else if (!tag.compare("CS"))
                        new_val = (value*value_aux)/(value+value_aux);
                        else if (!tag.compare("CP"))
                                new_val= value+value_aux;
            }
      if (simplify){
          //Remove the value of the last component
          int index = output.lastIndexOf(":");
          output = output.mid(0, index);
          output += QString(":%1;").arg(new_val);
      }else
          output += component + QString(";");

      component_aux = component;
      tag_aux = tag;
      value_aux = value;
    }
    return output;
}
