/***************************************************************************
                          marker.cpp  -  description
                             -------------------
    begin                : Sat Apr 10 2004
    copyright            : (C) 2003 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
****************************************************************************
    Update               : Added delta markers feature. March 2017
    Author		 : Andres Martinez-Mera
    email                : andresmartinezmera@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*!
  \class Marker
  \brief The Marker class implements the marker object used for all the
         diagram
*/

#include "marker.h"
#include "diagram.h"
#include "graph.h"
#include "main.h"

#include <QString>
#include <QPainter>
#include <QDebug>
#include <QFile>
#include <limits.h>
#include <cmath>
#include <stdlib.h>

#include "misc.h"

static double default_Z0=50;

/*!
 * create a marker based on click position and
 * the branch number.
 *
 * the click position is used to compute the marker position. currently, the
 * marker position is the sampling point closest to the click.
 */

Marker::Marker(Graph *pg_, int branchNo, int cx_, int cy_, QString markerID) :
  Element(),
  pGraph(pg_),
  Precision(3),
  numMode(0),
  Z0(default_Z0) // BUG: see declaration.
{
  Type = isMarker;
  isSelected = transparent = false;
  cx =  cx_;
  cy = -cy_;
  fCX = float(cx);
  fCY = float(cy);
  
  MarkerID=markerID;
  MarkerColor = Qt::darkMagenta;//Default marker color

  ReferenceMarkerData = {0,0,0,0,0,0};
  MarkerLineWidth = 1;
  MarkerMode=0;// MarkerMode = 0 => Conventional marker
               // MarkerMode = 1 => Delta marker
  ReferenceMarkerID = "#NO_REF#";//No reference

  if(!pGraph){
    makeInvalid();
  }else{
    initText(branchNo);   // finally create marker
    fix();
    createText();
  }

  x1 =  cx + 60;
  y1 = -cy - 60;
}

Marker::~Marker()
{
}

QMap<QString, std::vector<double>> Marker::getMarkersMap()
{
  return ActiveMarkers;
}

QString Marker::getID(void)
{
return MarkerID;
}

void Marker::setID(QString ID)
{
MarkerID = ID;
createText();
}



int Marker::getLineWidth()
{
  return MarkerLineWidth;
}

void Marker::setLineWidth(int width)
{
  MarkerLineWidth = width;
}

void Marker::setReferenceMarkerID(QString ref)
{
   ReferenceMarkerID=ref;
}


// This function returns the marker values. Its main use is to provide data to the Diagram object so
// as to build a map with the data of all the active markers.
std::vector<double> Marker::getData()
{
  std::vector<double> data(6);
  data[0] = VarDep[0];//Real part
  data[1] = VarDep[1];//Imaginary part
  data[2] = VarPos[0];//Independent variable
  data[3] = fCX;//Marker cordinates
  data[4] = fCY;
  data[5] = MarkerMode;//This allows to save the marker mode in a map structure. This map is used by markerdialog class to separate those conventional from delta markers
  return data;
}

void Marker::setMarkersMap(QMap<QString, std::vector<double>> markersMap)
{
   ActiveMarkers = markersMap;
}

// ---------------------------------------------------------------------
/*!
 * compute VarPos from branch number n and click position (cx, cy)
 * this is done by recreating branch samples and comparing against click
 *
 * FIXME: should use ScrPoints instead. do not call calcCoordinate from here!
 */
void Marker::initText(int n)
{
  if(pGraph->isEmpty()) {
      makeInvalid();
      return;
  }

  Axis const *pa;
  assert(diag());
  if(pGraph->yAxisNo == 0)  pa = &(diag()->yAxis);
  else  pa = &(diag()->zAxis);
  double Dummy = 0.0;   // needed for 2D graph in 3D diagram
  double *px, *py=&Dummy, *pz;
  Text = "";

  bool isCross = false;
  int nn, nnn, m, x, y, d, dmin = INT_MAX;
  DataX const *pD = pGraph->axis(0);
  px  = pD->Points;
  nnn = pD->count;
  DataX const *pDy = pGraph->axis(1);
  if(pDy) {   // only for 3D diagram
    nn = pGraph->countY * pD->count;
    py  = pDy->Points;
    if(n >= nn) {    // is on cross grid ?
      isCross = true;
      n -= nn;
      n /= nnn;
      px += (n % nnn);
      if(pGraph->axis(2))   // more than 2 indep variables ?
        n  = (n % nnn) + (n / nnn) * nnn * pDy->count;
      nnn = pDy->count;
    }
    else py += (n/pD->count) % pDy->count;
  }

  // find exact marker position
  m  = nnn - 1;
  pz = pGraph->cPointsY + 2*n;
  if(diag()->Name=="Phasor") m = phasormk(pz,px,nnn);
  else
  {
    for(nn=0; nn<nnn; nn++) {
      diag()->calcCoordinate(px, pz, py, &fCX, &fCY, pa);
      ++px;
      pz += 2;
      if(isCross) {
	px--;
	py++;
	pz += 2*(pD->count-1);
      }
      
      x = int(fCX+0.5) - cx;
      y = int(fCY+0.5) - cy;
      d = x*x + y*y;
      if(d < dmin) {
	dmin = d;
	m = nn;
      }
    }
  }
  //inittext for waveac is only to find values for VarPos except for VarPos[0]
  if(diag()->Name=="Waveac") m=0;
  if(isCross) m *= pD->count;
  n += m;

  // why check over and over again?! do in the right place and just assert otherwise.
  if(VarPos.size() != pGraph->numAxes()){
    qDebug() << "huh, wrong size" << VarPos.size() << pGraph->numAxes();
    VarPos.resize(pGraph->numAxes());
  }

  // gather text of all independent variables
  nn = n;
  for(unsigned i=0; (pD = pGraph->axis(i)); ++i) {
    px = pD->Points + (nn % pD->count);
    VarPos[i] = *px;
    Text += pD->Var + ": " + QString::number(VarPos[i],'g',Precision) + "\n";
    nn /= pD->count;
  }

  // createText();
}
// ---------------------------------------------------------------------
/*this function finds the VarPos[0] of waveac*/
void Marker::fix()
{
  if(!(pGraph->cPointsY)) {
    makeInvalid();
    return;
  }
  if(diag()->Name!="Waveac") return;

  int nn,x,d,dmin = INT_MAX;
  Axis const *pa;
  if(pGraph->yAxisNo == 0)  pa = &(diag()->yAxis);
  else  pa = &(diag()->zAxis);
  double Dummy = 0.0;   // needed for 2D graph in 3D diagram
  double px=0, *py=&Dummy, pz=0;
  int nnn = 50*diag()->sc;
  int m  = nnn - 1;
  
  for(nn=0; nn<nnn; nn++) {
      px = diag()->wavevalX(nn);
      diag()->calcCoordinate(&px, &pz, py, &fCX, &fCY, pa);
      x = int(fCX+0.5) - cx;
      d = x*x;
      if(d < dmin) {
	dmin = d;
	m = nn;
      }
  }
  VarPos[0] = diag()->wavevalX(m);
  
}
// ---------------------------------------------------------------------
/*!
 * (should)
 * create marker label Text the screen position cx and cy from VarPos.
 * does a lot of fancy stuff to be sorted out.
 */
void Marker::createText()
{
  if(!(pGraph->cPointsY)) {
    makeInvalid();
    return;
  }
  // independent variables
  MarkerMode ? Text = MarkerID + QString("\nDelta mode, Ref.: ") + ReferenceMarkerID + "\n" :  Text = MarkerID + "\n";
  unsigned nVarPos = VarPos.size();

  if(nVarPos > pGraph->numAxes()){
    qDebug() << "huh, VarPos too big?!";
  }
  if(nVarPos != pGraph->numAxes()){
    qDebug() << "padding" << VarPos.size() << pGraph->numAxes();
    VarPos.resize(pGraph->numAxes());
    while((unsigned int)nVarPos < pGraph->numAxes()){
      VarPos[nVarPos++] = 0.; // pad
    }
  }
  std::vector<double> VarRef(nVarPos+2);//nVarPos independent variables + 1 complex dependent variable 
  
  if (MarkerMode == 1)//Delta mode activated
  {  
    //Get reference marker data
    ReferenceMarkerData = ActiveMarkers[ReferenceMarkerID];
  }
  double *pp;
  nVarPos = pGraph->numAxes();
  DataX const *pD;
  if(diag()->Name!="Waveac")
  {
    auto p = pGraph->findSample(VarPos);
    VarDep[0] = p.first;
    VarDep[1] = p.second;
  }
  else
  {
    VarDep[0] = wavevalY(VarPos[0],VarPos);
    VarDep[1] = 0;
  }

  double v=0.;   // needed for 2D graph in 3D diagram
  double *py=&v;
  pD = pGraph->axis(0);
  if(pGraph->axis(1)) {
    *py = VarPos[1];
  }else{
    qDebug() << *py << "is not" << VarPos[1]; // does it really matter?!
  }

  double pz[2];
  pz[0] = VarDep[0];
  pz[1] = VarDep[1];

  // now actually create text.
  for(unsigned ii=0; (pD=pGraph->axis(ii)); ++ii) {
    if (MarkerMode == 0)//Default marker
    { 
     if(ii==0 && diag()->Name=="Waveac")
      Text += "Time: " + unit(VarPos[ii]) + "\n";
     else
      Text += pD->Var + ": " + QString::number(VarPos[ii],'g',Precision) + "\n";
    }
    else//Delta marker
    {
     Text += QChar(0x0394) + pD->Var + ": " + QString::number(VarPos[ii]-ReferenceMarkerData[2],'g',Precision) + "\n";
    }
  }
  
  if (MarkerMode == 1)//Delta marker
  {
     Text += QChar(0x0394) + pGraph->Var + ": ";
  }
  else//Default marker
  {
     Text += pGraph->Var + ": ";
  }

  switch(numMode) {
    case nM_Rect: 
        if (MarkerMode)//Delta marker
        {
            Text += misc::complexRect(*pz-ReferenceMarkerData[0], *(pz+1)-ReferenceMarkerData[1], Precision);
        }
        else//Default marker
        {
            Text += misc::complexRect(*pz, *(pz+1), Precision);
        }
      break;
    case nM_Deg: 
        if (MarkerMode)//Delta marker
        {
            Text += misc::complexDeg(*pz-ReferenceMarkerData[0], *(pz+1)-ReferenceMarkerData[1], Precision);
        }
        else//Default marker
        {
            Text += misc::complexDeg(*pz, *(pz+1), Precision);
        }
      break;
    case nM_Rad: 
       if (MarkerMode)
       {
         Text += misc::complexRad(*pz-ReferenceMarkerData[0], *(pz+1)-ReferenceMarkerData[1], Precision);
       }
       else
       {
          Text += misc::complexRad(*pz, *(pz+1), Precision);
       }
      break;
  }

  assert(diag());
  Text += diag()->extraMarkerText(this);

    Axis const *pa,*pt;
  if(diag()->Name=="Phasor")
  {
    int z;
    findaxismk();
    pt=xA;
    if(pGraph->yAxisNo == 0)  pa = yA;
    else  pa = zA;
    for(z=0;z<diag()->nfreqt;z++)
    {
      if(diag()->freq[z]<=VarPos[0]) v=diag()->freq[z];
      if(diag()->freq[z]==VarPos[0]) break;

    }
    VarPos[0]=v;
    pp = &(VarPos[0]);

    diag()->calcCoordinatePh(pz, &fCX, &fCY, pa, pt);
  }
  else
  {
    if(pGraph->yAxisNo == 0)  pa = &(diag()->yAxis);
    else  pa = &(diag()->zAxis);
    pp = &(VarPos[0]);

    diag()->calcCoordinate(pp, pz, py, &fCX, &fCY, pa);
  }
  diag()->finishMarkerCoordinates(fCX, fCY);

  cx = int(fCX+0.5);
  cy = int(fCY+0.5);
  getTextSize();
}

// ---------------------------------------------------------------------
void Marker::makeInvalid()
{
  fCX = fCY = -1e3; // invalid coordinates
  assert(diag());
  diag()->finishMarkerCoordinates(fCX, fCY); // leave to diagram
  cx = int(fCX+0.5);
  cy = int(fCY+0.5);

  Text = QObject::tr("invalid");
  getTextSize();
}

// ---------------------------------------------------------------------
void Marker::getTextSize()
{
  // get size of text using the screen-compatible metric
  QFontMetrics metrics(QucsSettings.font, 0);
  QSize r = metrics.size(0, Text);
  x2 = r.width()+5;
  y2 = r.height()+5;
}

// ---------------------------------------------------------------------
bool Marker::moveLeftRight(bool left)
{
  int n;
  double *px;
  double x;

  DataX const *pD = pGraph->axis(0);
  px = pD->Points;
  if(!px) return false;
  if(diag()->Name != "Waveac" &&diag()->Name != "Phasor")
  {
    for(n=0; n<pD->count; n++) {
      if(VarPos[0] <= *px) break;
      px++;
    }
    if(n == pD->count) px--;

    if(left) {
      if(px <= pD->Points) return false;
      px--;  // one position to the left
    }
    else {
      if(px >= (pD->Points + pD->count - 1)) return false;
      px++;  // one position to the right
    }
    VarPos[0] = *px;
  }
  if(diag()->Name == "Waveac")
  {
    for(n=0; n < 50*diag()->sc; n++) {
      x=diag()->wavevalX(n);
      if(VarPos[0] <= x) break;
    }
    if(n == 50*diag()->sc) n--;

    if(left) {
      if(n <= 0) return false;
      n--;  // one position to the left
    }
    else {
      if(n >= 50*diag()->sc) return false;
      n++;  // one position to the right
    }
    VarPos[0] = diag()->wavevalX(n);
  }
  if(diag()->Name == "Phasor")
  {
    for(n=0; n < diag()->nfreqt; n++) {
      x=diag()->freq[n];
      if(VarPos[0] == x) break;
    }
    if(n == diag()->nfreqt) n = diag()->nfreqt-1;

    if(left) {
      if(n == 0) return false;
      n--;  // one position to the left
    }
    else {
      if(n == diag()->nfreqt-1) return false;
      n++;  // one position to the right
    }
    VarPos[0] = diag()->freq[n];
  }
  createText();

  return true;
}

// ---------------------------------------------------------------------
bool Marker::moveUpDown(bool up)
{
  int n, i=0;
  double *px;

  DataX const *pD = pGraph->axis(0);
  if(!pD) return false;

  if(up) {  // move upwards ? **********************
    do {
      pD = pGraph->axis(++i);
      if(!pD) return false;
      px = pD->Points;
      if(!px) return false;
      for(n=1; n<pD->count; n++) {  // go through all data points
        if(fabs(VarPos[i]-(*px)) < fabs(VarPos[i]-(*(px+1)))) break;
        px++;
      }

    } while(px >= (pD->Points + pD->count - 1));  // go to next dimension ?

    px++;  // one position up
    VarPos[i] = *px;
    while(i > 1) {
      pD = pGraph->axis(--i);
      VarPos[i] = *(pD->Points);
    }
  }
  else {  // move downwards **********************
    do {
      pD = pGraph->axis(++i);
      if(!pD) return false;
      px = pD->Points;
      if(!px) return false;
      for(n=0; n<pD->count; n++) {
        if(fabs(VarPos[i]-(*px)) < fabs(VarPos[i]-(*(px+1)))) break;
        px++;
      }

    } while(px <= pD->Points);  // go to next dimension ?

    px--;  // one position down
    VarPos[i] = *px;
    while(i > 1) {
      pD = pGraph->axis(--i);
      VarPos[i] = *(pD->Points + pD->count - 1);
    }
  }
  createText();

  return true;
}

void Marker::setColor(QColor c)
{
  MarkerColor = c;
}

QColor Marker::getColor()
{
  return MarkerColor;
}

void Marker::setMarkerMode(int mode)
{
  MarkerMode = mode;
}

int Marker::getMarkerMode() const
{
  return MarkerMode;
}


// ---------------------------------------------------------------------
void Marker::paint(ViewPainter *p, int x0, int y0)
{
  createText();

// is a flag which indicates whether the marker was previously saved or not.

  // keep track of painter state
  p->Painter->save();

  // Workaround for bug in Qt: If WorldMatrix is turned off, \n in the
  // text creates a terrible mess.
  p->Painter->setWorldMatrixEnabled(true);
  QMatrix wm = p->Painter->worldMatrix();
  p->Painter->setWorldMatrix(QMatrix());

  int x2_, y2_;
  p->Painter->setPen(QPen(Qt::black,1));
  x2_ = p->drawText(Text, x0+x1+3, y0+y1+3, &y2_);
  x2_ += int(6.0*p->Scale);
  y2_ += int(6.0*p->Scale);
  if(!transparent) {
    p->eraseRect(x0+x1, y0+y1, x2_, y2_);
    p->drawText(Text, x0+x1+3, y0+y1+3);
  }

   p->Painter->setPen(QPen(MarkerColor,MarkerLineWidth));//The color of the conventional markers is customizable. However, delta markers are black
   double pad = MarkerLineWidth;//Extra pad. Otherwise, the box would overlap the text if the linewidth is big
   p->drawRectD(x0+x1-pad/1.4142, y0+y1-pad/1.4142, x2_+pad, y2_+pad);


  p->Painter->setWorldMatrix(wm);
  p->Painter->setWorldMatrixEnabled(false);

  // restore painter state
  p->Painter->restore();

  p->Painter->setPen(QPen(Qt::darkMagenta,1));

  x2 = int(float(x2_) / p->Scale);
  y2 = int(float(y2_) / p->Scale);

  int x1_, y1_;
  p->map(x0+x1, y0+y1, x1_, y1_);
  // which corner of rectangle should be connected to line ?
  if(cx < x1+(x2>>1)) {
    if(-cy >= y1+(y2>>1))
      y1_ += y2_ - 1;
  }
  else {
    x1_ += x2_ - 1;
    if(-cy >= y1+(y2>>1))
      y1_ += y2_ - 1;
  }
  float fx2, fy2;
  fx2 = (float(x0)+fCX)*p->Scale + p->DX;
  fy2 = (float(y0)-fCY)*p->Scale + p->DY;
  p->Painter->drawLine(x1_, y1_, TO_INT(fx2), TO_INT(fy2));

  if (MarkerMode == 1) //Delta Marker
  {//Draws a second line from the box to the reference marker
    fx2 = (float(x0)+ReferenceMarkerData[3])*p->Scale + p->DX;
    fy2 = (float(y0)-ReferenceMarkerData[4])*p->Scale + p->DY;
    p->Painter->setPen(QPen(Qt::darkGray, 2, Qt::DotLine));
    p->Painter->drawLine(x1_, y1_, TO_INT(fx2), TO_INT(fy2));
  }

  if(isSelected) {
    p->Painter->setPen(QPen(Qt::darkGray,3));
    p->drawRoundRect(x0+x1-3, y0+y1-3, x2+6, y2+6);
  }
}

// ---------------------------------------------------------------------
void Marker::paintScheme(QPainter *p)
{
  assert(diag());
  int x0 = diag()->cx;
  int y0 = diag()->cy;
  p->drawRect(x0+x1, y0+y1, x2, y2);

  // which corner of rectangle should be connected to line ?
  if(cx < x1+(x2>>1)) {
    if(-cy < y1+(y2>>1))
      p->drawLine(x0+cx, y0-cy, x0+x1, y0+y1);
    else
      p->drawLine(x0+cx, y0-cy, x0+x1, y0+y1+y2-1);
  }
  else {
    if(-cy < y1+(y2>>1))
      p->drawLine(x0+cx, y0-cy, x0+x1+x2-1, y0+y1);
    else
      p->drawLine(x0+cx, y0-cy, x0+x1+x2-1, y0+y1+y2-1);
  }
}

// ------------------------------------------------------------
void Marker::setCenter(int x, int y, bool relative)
{
  if(relative) {
    x1 += x;  y1 += y;
  }
  else {
    x1 = x;  y1 = y;
  }
}

// -------------------------------------------------------
void Marker::Bounding(int& _x1, int& _y1, int& _x2, int& _y2)
{
  if(diag()) {
    _x1 = diag()->cx + x1;
    _y1 = diag()->cy + y1;
    _x2 = diag()->cx + x1+x2;
    _y2 = diag()->cy + y1+y2;
  }
  else {
    _x1 = x1;
    _y1 = y1+y2;
    _x2 = x1+x2;
    _y2 = y1;
  }
}

// ---------------------------------------------------------------------
// This function returns a QString with the marker properties (position, name, color, linewidth, etc...)
// it is called by a Graph object so as to include markers in a .dpl file
QString Marker::save()
{
  QString s  = "<Mkr ";

  for(auto i : VarPos){
    s += QString::number(i)+"/";
  }

  s.replace(s.length()-1,1,' ');
  //s.at(s.length()-1) = (const QChar&)' ';

  s += QString::number(x1) +" "+ QString::number(y1) +" "
      +QString::number(Precision) +" "+ QString::number(numMode);

  if(transparent)  s += " 1 ";
  else  s += " 0 ";

  //New fields (they are added at the end of the line so as to be compatible with old versions of Qucs)
  s += QString("%1 ").arg(MarkerID);
  
  //Add color and linewidth
  s += QString("%1 %2 ").arg(MarkerColor.name()).arg(MarkerLineWidth);

  //Add marker mode (delta or normal) and reference marker (if exists)

  s += QString("%1 %2 ").arg(MarkerMode).arg(ReferenceMarkerID);

  //Add reference marker data
  s += QString("%1#%2#%3#%4#%5 >").arg(ReferenceMarkerData[0]).arg(ReferenceMarkerData[1]).arg(ReferenceMarkerData[2]).arg(ReferenceMarkerData[3]).arg(ReferenceMarkerData[4]);

  return s;
}


// ---------------------------------------------------------------------
// All graphs must have been loaded before this function !
bool Marker::load(const QString& _s)
{
  bool ok;
  QString s = _s;

  if(s.at(0) != '<') return false;
  if(s.at(s.length()-1) != '>') return false;
  s = s.mid(1, s.length()-2);   // cut off start and end character

  if(s.section(' ',0,0) != "Mkr") return false;

  int i=0, j;
  QString n = s.section(' ',1,1);    // VarPos

  unsigned nVarPos = 0;
  j = (n.count('/') + 3);
  VarPos.resize(j);

  do {
    j = n.indexOf('/', i);
    VarPos[nVarPos++] = n.mid(i,j-i).toDouble(&ok);
    if(!ok) return false;
    i = j+1;
  } while(j >= 0);

  n  = s.section(' ',2,2);    // x1
  x1 = n.toInt(&ok);
  if(!ok) return false;

  n  = s.section(' ',3,3);    // y1
  y1 = n.toInt(&ok);
  if(!ok) return false;

  n  = s.section(' ',4,4);      // Precision
  Precision = n.toInt(&ok);
  if(!ok) return false;

  n  = s.section(' ',5,5);      // numMode (polar or cartersian)
  numMode = n.toInt(&ok);
  if(!ok) return false;

  n  = s.section(' ',6,6);      // transparent
  if(n.isEmpty()) return true;  // is optional
  if(n == "0")  transparent = false;
  else  transparent = true;

  //New fields

  QString tempID = s.section(' ',7,7); //temp string to check the marker format
  qDebug() << "MarkerID " << MarkerID;
  if (tempID.isEmpty())//Loaded legacy .dpl doc
  {
    MarkerColor = Qt::darkMagenta;//Color
    MarkerLineWidth = 1;//Line width
    MarkerMode = 0;//Conventional marker
    ReferenceMarkerID = QString("#NO_REF#");//Reference marker
    return true;
  }
  MarkerID = tempID;//Marker ID

  //Color
  MarkerColor.setNamedColor(s.section(' ',8,8));
  
  //Line width
  MarkerLineWidth = s.section(' ',9,9).toInt();

  //Marker mode (delta or normal)
  MarkerMode = s.section(' ',10,10).toInt();

  //Reference marker
  ReferenceMarkerID = s.section(' ',11,11);

  //Reference marker data
  QString RefMrkData = s.section(' ',12,12);
  ReferenceMarkerData[0] = RefMrkData.section('#', 0 ,0).toDouble();
  ReferenceMarkerData[1] = RefMrkData.section('#', 1 ,1).toDouble();
  ReferenceMarkerData[2] = RefMrkData.section('#', 2 ,2).toDouble();
  ReferenceMarkerData[3] = RefMrkData.section('#', 3 ,3).toDouble();
  ReferenceMarkerData[4] = RefMrkData.section('#', 4 ,4).toDouble();
  return true;
}

// ------------------------------------------------------------------------
// Checks if the coordinates x/y point to the marker text. x/y are relative
// to diagram cx/cy.
bool Marker::getSelected(int x_, int y_)
{
  if(x_ >= x1) if(x_ <= x1+x2) if(y_ >= y1) if(y_ <= y1+y2)
    return true;

  return false;
}
// ------------------------------------------------------------------------
/*will find the y value of a point in time for waveac*/
double Marker::wavevalY(double xn,std::vector<double>& VarPos)  
{
  double n;
  double af=0.0; //angles
  double A = 0.0;
  double yp[2];

  n=VarPos[0];
  VarPos[0]= diag()->freq[0];
  auto p = pGraph->findSample(VarPos);
  yp[0]=p.first;
  yp[1]=p.second;

  af = atan2 (yp[1],yp[0]);
  A = sqrt(yp[1]*yp[1] +yp[0]*yp[0]);
  VarPos[0]=n;
  return A*sin(2*pi*(diag()->freq[0])*xn + af);
}
// ------------------------------------------------------------------------
/*
 * the diagram this belongs to
 */
const Diagram* Marker::diag() const
{
  if(!pGraph) return NULL;
  return pGraph->parentDiagram();
}

// ------------------------------------------------------------------------
Marker* Marker::sameNewOne(Graph *pGraph_)
{
  Marker *pm = new Marker(pGraph_, 0, cx ,cy);
  pm->MarkerID=MarkerID;
  pm->MarkerLineWidth=MarkerLineWidth;
  pm->MarkerMode=MarkerMode;
  pm->ReferenceMarkerID=ReferenceMarkerID;
  pm->MarkerColor=MarkerColor;

  pm->x1 = x1;  pm->y1 = y1;
  pm->x2 = x2;  pm->y2 = y2;

  pm->VarPos = VarPos;

  pm->Text        = Text;
  pm->transparent = transparent;
  pm->Precision   = Precision;
  pm->numMode     = numMode;

  return pm;
}
// ------------------------------------------------------------------------
QString Marker::unit(double n)
{
  QString value="";
  if(n < 1e-9)
  {
    n/= 1e-9;
    value.setNum(n);
    value+= " p";
  }
  else if(n < 1e-6)
  {
    n/= 1e-9;
    value.setNum(n);
    value+= " n";
  }
  else if(n < 1e-3)
  {
    n/= 1e-6;
    value.setNum(n);
    value+= " u";
  }
  else if(n < 1)
  {
    n/= 1e-3;
    value.setNum(n);
    value+= " m";
  }
  else
  {
    value.setNum(n);
  }
  return value;

}
int Marker::phasormk(double *pz,double *px,int max)
{
  int m,n,nn,x,y,d,dmin = INT_MAX;
  Axis const *pa,*pt;

  findaxismk();
  pt=xA;
  if(pGraph->yAxisNo == 0)  pa = yA;
  else  pa = zA;

  for(nn=0; nn<max; nn++) {
    for(n=0;n<diag()->nfreqt;n++)
    {
      if(diag()->freq[n]==*px) break;
    }
    if(n < diag()->nfreqt) diag()->calcCoordinatePh(pz, &fCX, &fCY, pa, pt);
    ++px;
    pz += 2;
      
    x = int(fCX+0.5) - cx;
    y = int(fCY+0.5) - cy;
    d = x*x + y*y;
    if(d < dmin) {
      dmin = d;
      m = nn;
    }
  }
  return m;
}

void Marker::findaxismk()
{
  QString var = pGraph->Var;
    
    xA = &(diag()->xAxis);
    yA = &(diag()->yAxis);
    zA = &(diag()->zAxis);

    if(var.indexOf(".v",0,Qt::CaseSensitive) != -1)
    {
      xA = &(diag()->xAxisV);
      yA = &(diag()->yAxisV);
      zA = &(diag()->zAxisV);
    }
    else if(var.indexOf(".i",0,Qt::CaseSensitive) != -1)
    {
      xA = &(diag()->xAxisI);
      yA = &(diag()->yAxisI);
      zA = &(diag()->zAxisI);
    }
    else if(var.indexOf(".S",0,Qt::CaseSensitive) != -1)
    {
      xA = &(diag()->xAxisP);
      yA = &(diag()->yAxisP);
      zA = &(diag()->zAxisP);
    }
    else if(var.indexOf(".Ohm",0,Qt::CaseSensitive) != -1)
    {
      xA = &(diag()->xAxisZ);
      yA = &(diag()->yAxisZ);
      zA = &(diag()->zAxisZ);
    }

}
// vim:ts=8:sw=2:noet
