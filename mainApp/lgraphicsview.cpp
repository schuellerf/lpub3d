/****************************************************************************
**
** Copyright (C) 2016 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the
** GNU General Public Liceense (GPL) version 3.0
** which accompanies this distribution, and is
** available at http://www.gnu.org/licenses/gpl.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "lgraphicsview.h"

LGraphicsView::LGraphicsView(QGraphicsScene *scene) {
  setScene(scene);
  pageBackgroundItem = NULL;
  fitMode = FitVisible;
  pageRect = QRectF(0,0,0,0);

  /* Ruler
  setViewportMargins(RULER_BREADTH,RULER_BREADTH,0,0);
  QGridLayout* gridLayout = new QGridLayout();
  gridLayout->setSpacing(0);
  gridLayout->setMargin(0);

  QWidget* fake = new QWidget();
  fake->setBackgroundRole(QPalette::Window);
  fake->setFixedSize(RULER_BREADTH,RULER_BREADTH);

  QDRuler * mHorzRuler = new QDRuler(QDRuler::Horizontal,fake);
  QDRuler * mVertRuler = new QDRuler(QDRuler::Vertical,fake);

  gridLayout->addWidget(fake,0,0);
  gridLayout->addWidget(mHorzRuler,0,1);
  gridLayout->addWidget(mVertRuler,1,0);
  gridLayout->addWidget(this->viewport(),1,1);

  this->setLayout(gridLayout);
   */
}

void LGraphicsView::resizeEvent(QResizeEvent * /* unused */)
{
  if (pageBackgroundItem) {
      if (fitMode == FitVisible) {
          fitVisible(pageRect);
        } else if (fitMode == FitWidth) {
          fitWidth(pageRect);
        }
    }
}

void LGraphicsView::fitVisible(const QRectF rect)
{
  scale(1.0,1.0);
  pageRect = rect;

  QRectF unity = matrix().mapRect(QRectF(0,0,1,1));
  scale(1/unity.width(), 1 / unity.height());

  int margin = 2;
  QRectF viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
  QRectF sceneRect = matrix().mapRect(pageRect);
  qreal xratio = viewRect.width() / sceneRect.width();
  qreal yratio = viewRect.height() / sceneRect.height();

  xratio = yratio = qMin(xratio,yratio);
  scale(xratio,yratio);
  centerOn(pageRect.center());
  fitMode = FitVisible;
}

void LGraphicsView::fitWidth(const QRectF rect)
{
  scale(1.0,1.0);
  pageRect = rect;

  QRectF unity = matrix().mapRect(QRectF(0,0,1,1));
  scale(1/unity.width(), 1 / unity.height());

  int margin = 2;
  QRectF viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
  QRectF sceneRect = matrix().mapRect(pageRect);
  qreal xratio = viewRect.width() / sceneRect.width();

  scale(xratio,xratio);
  centerOn(pageRect.center());
  fitMode = FitWidth;
}

void LGraphicsView::actualSize(){
  resetMatrix();
  fitMode = FitNone;
}

void LGraphicsView::zoomIn(){
  scale(1.1,1.1);
  fitMode = FitNone;
}

void LGraphicsView::zoomOut(){
  scale(1.0/1.1,1.0/1.1);
  fitMode = FitNone;
}
